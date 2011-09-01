#ifndef MD5_H
#define MD5_H

#include "order32.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t uint32;
    
struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
};


void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, const void *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* !MD5_H */
