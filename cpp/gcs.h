#ifndef GCS_H
#define GCS_H

#include <vector>
#include <iostream>

typedef uint32_t hash_t;

class GCSBuilder
{
	int N, P;
	std::vector<hash_t> values;

public:
	GCSBuilder(int N, int P);
	void add(const void *data, int size);
	void finalize(std::ostream &f);
};	

class GCSQuery
{
	int N, P;
	std::istream &f;
	uint8_t *gcs;
	int gcs_len;

public:
	GCSQuery(std::istream &f);
	~GCSQuery();
	bool query(const void *data, int size);
};


#endif /* GCS_H */

