#include "gcs.h"
#include "md5.h"
#include "order32.h"
#include <stdint.h>
#include <string.h>
#include <algorithm>

#define BITMASK(n)    ((1 << (n)) - 1)

static unsigned int gcs_hash(const void *data, int size, int N, int P)
{
	unsigned char digest[16];
	MD5Context ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)data, size);
	MD5Final(digest, &ctx);

	uint32_t dignum = 
		((uint32_t)digest[12]<<24) |
		((uint32_t)digest[13]<<16) |
		((uint32_t)digest[14]<<8) |
		((uint32_t)digest[15]<<0);
	return dignum % (N*P);
}

static int floor_log2(int v)
{
	return 32 - __builtin_clz(v-1);
}

/**************************************************************/

class BitEncoder
{
private:
	std::ostream &f;
	long accum;
	int n;
	enum { ACCUM_BITS = sizeof(long)*8 };

public:
	BitEncoder(std::ostream &_f)
		: f(_f), accum(0), n(0) {}

	void encode(int nbits, int value)
	{
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
				unsigned char ch = (accum >> (n-8)) & BITMASK(8);
				f.write((char*)&ch, 1);
				n -= 8;
				accum &= BITMASK(n);
			}
		}
	}

	void flush(void)
	{
		if (n > 0)
		{
			unsigned char ch = accum;
			f.write((char*)&ch, 1);
			n = 0;
			accum = 0;
		}
	}
};


class GolombEncoder
{
private:
	BitEncoder f;
	int P, log2P;

public:
	GolombEncoder(std::ostream &_f, int _P)
		: f(_f), P(_P)
	{
		log2P = floor_log2(P);
	}

	void encode(unsigned int value)
	{
		int q = value / P;
		int r = value - q*P;

		f.encode(q+1, BITMASK(q)<<1);
		f.encode(log2P, r);
	}

	void flush(void)
	{
		f.flush();
	}
};


GCSBuilder::GCSBuilder(int _n, int _p)
	: N(_n), P(_p)
{
	values.reserve(N*P);
	values.push_back(0);
}

void GCSBuilder::add(const void *data, int size)
{
	unsigned h = gcs_hash(data, size, N, P);
	values.push_back(h);
}

void GCSBuilder::finalize(std::ostream& f)
{
	std::sort(values.begin(), values.end());

	int v = O32_HOST_TO_BE(N);
	f.write((char*)&v, 4);
	v = O32_HOST_TO_BE(P);
	f.write((char*)&v, 4);

	GolombEncoder ge(f, P);
	for (int i=0; i<(int)values.size()-1; ++i)
	{
		int diff = values[i+1] - values[i];
		if (diff != 0)
			ge.encode(diff);
	}
	ge.flush();
}


/**************************************************************/




