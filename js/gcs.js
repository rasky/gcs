function bitreader(arr) {
    var offset = 0,
        accum = 0,
        n = 0,
        v;

    return function c (n2) {
      /* Sorry, max 31 bits supported */
      while (n <= n2) {
        if (offset >= arr.length)
          throw "End of array";
        accum <<= 8;
        accum |= arr[offset++];
        n += 8;
      }
      v = (accum >>> (n-n2)) & ((1 << n2)-1);
      n -= n2;
      accum &= (1 << n)-1;
      return v;
    };
}

function bitwriter(arr) {
  var v = 0,
      n = 0,
      b = 0;

  function c (n2, v2) {
    while (1) {
      /* Sorry, max 31 bits supported */
      v <<= n2;
      v |= v2 & ((1 << n2) - 1);

      n += n2;
      while (n >= 8) {
        b = (v >>> (n-8)) & 255;
        arr.push(b);
        n -= 8;
      }
      v &= 255;
      return;
    }
  }

  c.close = function () {
    if (n !== 0) {
      v = (v << (8-n)) & 255;
      arr.push(v);
    }
  };

  return c;
}

function gcs_hash(w, N, P) {
    h = md5(w);
    h = parseInt(h.substring(24,32), 16) % (N*P);
    return h;
}

function golomb_enc(arr, P) {
  var logp = Math.round(Math.log(P) * Math.LOG2E);
  var f = bitwriter(arr);
  function c (v) {
    var q = ~~(v / P),
        r = v % P;
    f(q+1, (1 << (q+1)) - 2);
    f(logp, r);
    return;
  }

  c.close = function () {
    f.close();
  };

  c.write = f;

  return c;
}

function golomb_dec(arr, P) {
  var logp = Math.round(Math.log(P) * Math.LOG2E);
  var f = bitreader(arr);
  var v;
  return function () {
    while(1) {
      v = 0;
      while (f(1)) {
        v += P;
      }
      var tmp = f(logp);
      v += tmp;
      return v;
    }
  };
}

function GCSBuilder(_N, _P) {
  var N = _N, P = _P, values = [0], words = [];

  this.add = function (v) {
    words.push(v);
    values.push(gcs_hash(v, N, P));
  };

  this.finalize = function () {
    var i,
        d,
        ab,
        res = [],
        header = new Array(8),
        f = golomb_enc(res, P);
    values.sort(function (a, b) { return a - b; });
    for (i = 0; i < values.length - 1; i += 1) {
      d = values[i+1] - values[i];
      if (d === 0) {
        continue;
      }
      f(d);
    }
    f.close();
    res = header.concat(res);
    res = new Uint8Array(res);
    dw = new DataView(res.buffer);
    dw.setUint32(0, N);
    dw.setUint32(4, P);
    return res.buffer;
  };
}

function GCSQuery(_arrBuff) {
  var dw = new DataView(_arrBuff),
    N = dw.getUint32(0),
    P = dw.getUint32(4),
    u8arr = new Uint8Array(_arrBuff, 8);
  this.query = function (w) {
    var h = gcs_hash(w, N, P),
      n = 0,
      d,
      f = golomb_dec(u8arr, P);

    while (1) {
      try {
        d = f();
        n += d;
        if (h === n) {
          return true;
        }
        if (h < n) {
          return false;
        }
      } catch (err) {
        break;
      }
    }
    return false;
  };
}