#include "gcs.h"
#include "md5.h"
#include "order32.h"
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <assert.h>

#define BITMASK(n)    ((1 << (n)) - 1)


static uint32_t gcs_hash(const void *data, int size, int N, int P)
{
	unsigned char digest[16];
	MD5Context ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)data, size);
	MD5Final(digest, &ctx);

	hash_t dignum = 0;
	for (int i = 16-sizeof(hash_t); i < 16; ++i)
	{
		dignum <<= 8;
		dignum += digest[i];
	}

	return dignum % (N*P);
}

static int floor_log2(int v)
{
	return sizeof(int)*8 - __builtin_clz(v-1);
}

/**************************************************************/

class BitWriter
{
private:
	std::ostream &f;
	unsigned long accum;
	int n;
	enum { ACCUM_BITS = sizeof(unsigned long)*8 };

public:
	BitWriter(std::ostream &_f)
		: f(_f), accum(0), n(0) {}

	void write(int nbits, unsigned value)
	{
		assert(nbits >= 0);
		while (nbits)
		{
			int nb = std::min(ACCUM_BITS-n, nbits);
			accum <<= nb;
			value &= BITMASK(nbits);
			accum |= value >> (nbits-nb);
			n += nb;
			nbits -= nb;

			while (n >= 8)
			{
				f.put((accum >> (n-8)) & BITMASK(8));
				n -= 8;
				accum &= BITMASK(n);
			}
		}
	}

	void flush(void)
	{
		if (n > 0)
		{
			assert(n < 8);
			f.put(accum & BITMASK(8));
			n = 0;
			accum = 0;
		}
	}
};


class GolombEncoder
{
private:
	BitWriter f;
	int P, log2P;

public:
	GolombEncoder(std::ostream &_f, int _P)
		: f(_f), P(_P)
	{
		log2P = floor_log2(P);
		assert(log2P > 0);
	}

	void encode(hash_t value)
	{
		hash_t q = value / P;
		hash_t r = value - q*P;

		f.write(q+1, BITMASK(q)<<1);
		f.write(log2P, r);
	}

	void flush(void)
	{
		f.flush();
	}
};


GCSBuilder::GCSBuilder(int _n, int _p)
	: N(_n), P(_p)
{
	assert(N*P <= ~(hash_t)0);
	values.reserve(N*P);
	values.push_back(0);
}

void GCSBuilder::add(const void *data, int size)
{
	hash_t h = gcs_hash(data, size, N, P);
	values.push_back(h);
}

void GCSBuilder::finalize(std::ostream& f)
{
	std::sort(values.begin(), values.end());

	int32_t v = O32_HOST_TO_BE(N);
	f.write((char*)&v, 4);
	v = O32_HOST_TO_BE(P);
	f.write((char*)&v, 4);

	GolombEncoder ge(f, P);
	for (int i=0; i<(int)values.size()-1; ++i)
	{
		hash_t diff = values[i+1] - values[i];
		if (diff != 0)
			ge.encode(diff);
	}
	ge.flush();
}


/**************************************************************/

class BitReader
{
private:
	std::istream &f;
	unsigned long accum;
	int n;

public:
	BitReader(std::istream &f_)
		: f(f_), accum(0), n(0)
	{}

	bool eof(void)
	{
		return f.eof();
	}

	unsigned int read(int nbits)
	{
		while (n < nbits)
		{
			accum <<= 8;
			accum |= f.get();
			n += 8;
		}

		unsigned int ret = (accum >> (n-nbits));
		n -= nbits;
		accum &= BITMASK(n);
		return ret;
	}
};


class GolombDecoder
{
	BitReader f;
	int P, log2P;

public:
	GolombDecoder(std::istream &f_, int P_)
		: f(f_), P(P_)
	{
		log2P = floor_log2(P);
	}

	bool eof(void)
	{
		return f.eof();
	}

	hash_t next(void)
	{
		hash_t v = 0;
		while (f.read(1))
		{
			v += P;
			if (f.eof())
				return 0;
		}
		v += f.read(log2P);
		return v;
	}
};

GCSQuery::GCSQuery(std::istream &f_)
	: f(f_)
{
	int32_t v;

	f.read((char*)&v, 4);
	N = O32_BE_TO_HOST(v);

	f.read((char*)&v, 4);
	P = O32_BE_TO_HOST(v);
}

bool GCSQuery::query(const void *data, int size)
{
	unsigned h = gcs_hash(data, size, N, P);
	unsigned int value = 0;
	
	f.seekg(8);
	f.clear();
	
	GolombDecoder gd(f, P);
	while (!gd.eof())
	{
		unsigned int diff = gd.next();
		value += diff;

		if (value == h)
			return true;
		else if (value > h)
			return false;
	}

	return false;
}

