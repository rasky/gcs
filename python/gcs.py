# This is free and unencumbered software released into the public domain.

# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.

# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

# For more information, please refer to <http://unlicense.org/>

from __future__ import division
import sys, math
from hashlib import md5
import struct
from array import array
import codecs
import sys

def bitwriter(f):
    """
    Coroutine to write bits into a file-like.
    Use .send((N, V)) to write the N least-significant
    bits of V into the file.
    Remember to call .close() to flush the internal
    state when done.
    """
    v = 0
    n = 0
    try:
        while 1:
            (n2,v2) = yield None
            v <<= n2
            v |= v2 & ((1<<n2)-1)
            n += n2
            while n >= 8:
                b = (v >> (n-8)) & 255
                f.write(chr(b))
                n -= 8
            v &= 255
    except GeneratorExit:
        if n != 0:
            v = (v << (8-n)) & 255
            f.write(chr(v))
        raise

def bitreader(f):
    """
    Coroutine to read bits from a file-like.
    Use .send(N) to read N bits from the file.
    """
    summer = 0
    n = 0
    v = None
    while 1:
        n2 = yield v
        while n <= n2:
            summer <<= 8
            try:
                summer |= ord(f.read(1))
            except TypeError:
                # ord(None) => eof
                return
            n += 8
        v = (summer >> (n-n2)) & ((1<<n2)-1)
        n -= n2
        summer &= (1<<n)-1


def golomb_enc(f, P):
    """
    Coroutine to encode Golomb/Rice-encoded values into
    a file-like f, with parameter P.
    Use .send(V) to encode the value V into the file.
    This is an optimal encoding if values are geometrically
    distributed with P.
    """
    logp = int(math.log(P,2))
    f = bitwriter(f); f.next();
    while 1:
        v = yield None
        q,r = v//P, v%P
        f.send((q+1, (1<<(q+1))-2))
        f.send((logp, r))   

def golomb_dec(f, P):
    """
    Coroutine to decode Golomb/Rice-encoded values
    from a file-like f, with parameter P.
    Use .next() (or iterate over it) to read the values.
    """
    logp = int(math.log(P,2))
    f = bitreader(f); f.next()
    while 1:
        try:
            v = 0
            while f.send(1):
                v += P
            v += f.send(logp)
        except StopIteration:
            return
        yield v

def gcs_hash(w, (N,P)):
    """
    Hash value for a GCS with N elements and 1/P probability
    of false positives.
    We just need a hash that generates uniformally-distributed
    values for best results, so any crypto hash is fine. We
    default to MD5.
    """
    h = md5(w).hexdigest()
    h = long(h[24:32],16)
    return h % (N*P)

class GCSBuilder:
    def __init__(self, N, P):
        self.N = N
        self.P = P
        self.values = array("L")
        self.values.append(0)
        
    def add(self, v):
        self.values.append(gcs_hash(v, (self.N,self.P)))

    def finalize(self, f):
        values = sorted(self.values)
        f.write(struct.pack("!LL", self.N, self.P))
        f = golomb_enc(f, self.P)
        f.next()
        for i in range(len(values)-1):
            d = values[i+1] - values[i]
            if d == 0:
                continue
            f.send(d)
        f.close()

class GCSQuery:
    def __init__(self, f):
        self.f = f
        self.N, self.P = struct.unpack("!LL", self.f.read(8))

    def _rewind(self):
        self.f.seek(8)

    def query(self, w):
        h = gcs_hash(w, (self.N,self.P))
        N = 0
        self._rewind()
        for d in golomb_dec(self.f, self.P):
            N += d
            if h == N:
                return True
            if h < N:
                # early-exit since values are sorted
                return False
        return False
            

if __name__ == "__main__":
    if sys.argv[1] == "build":
        prob = 2**10
        try:
            words = codecs.open(sys.argv[2], 'r', 
                                encoding=sys.stdin.encoding).readlines()
        except UnicodeDecodeError, e:
            print "Warning: {0} encoding doesn't match stdin encoding ({1})".format(
                sys.argv[2], sys.stdin.encoding)
            words = codecs.open(sys.argv[2], 'r').readlines()

        gcs = GCSBuilder(len(words), prob)
        for w in words:
            gcs.add(w.strip())
        with open("table.gcs", "wb") as f:
            gcs.finalize(f)
            fsize = f.tell()
        print "Number of words: %d" % len(words)
        print "False positives: %f" % (1/prob)
        print "Size: %d" % (fsize)
        print "Bits per word: %f" % (fsize*8 / len(words))

    elif sys.argv[1] == "query":
        gcs = GCSQuery(open("table.gcs","rb"))
        for w in sys.argv[2:]:
            found = gcs.query(w)
            print 'Querying for "%s": %s' % (w, "TRUE" if found else "FALSE")

