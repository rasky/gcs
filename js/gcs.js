function bitreader(arr) {
  var offset = 0,
      accum = 0,
      n = 0;

  function c (n2, v) {
    if (typeof v === 'undefined') v = 0;
    if (n2 > 8) {
      v = v * 256 + c(8);
      return c(n2-8, v);
    } else {
      n -= n2;
      if (n < 0) {
        if (offset >= arr.length) throw "End of array";
        accum = (accum << 8) | arr[offset++];
        n += 8;
      }
      v = v * Math.pow(2, n2) + (accum >>> n);
      accum &= (1 << n) - 1;
      return v;
    }
  }
  return c;
}

function bitwriter(arr) {
  var accum = 0,
      n = 0,
      tmp = 0;
  function c (n2, v2) {
    if (n2 > 8) {
      n2 -= 8;
      tmp = v2 / Math.pow(2, n2) >>> 0;
      c(8, tmp);
      c(n2, v2 - tmp * Math.pow(2, n2));
    } else {
      accum = accum * Math.pow(2, n2) + v2;
      n += n2;
      if (n >= 8) {
        arr.push(accum / Math.pow(2, n-8));
        n -= 8;
        accum = accum & ((1 << n) - 1);
      }
    }
  }

  c.close = function () {
    if (n !== 0) {
      accum = (accum << (8-n)) & 255;
      arr.push(accum);
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
      if (d === 0 && i > 0) {
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