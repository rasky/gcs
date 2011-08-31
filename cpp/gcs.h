#ifndef GCS_H
#define GCS_H

#include <vector>
#include <iostream>

class GCSBuilder
{
	int N, P;
	std::vector<int> values;

public:
	GCSBuilder(int N, int P);
	void add(const void *data, int size);
	void finalize(std::ostream &f);
};	


#endif /* GCS_H */

