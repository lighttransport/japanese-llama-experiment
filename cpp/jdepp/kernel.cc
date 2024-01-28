// pecco -- please enjoy classification with conjunctive features
//  $Id: kernel.cc 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#include "kernel.h"
#ifdef USE_SSE4_2_POPCNT
#include <smmintrin.h>
#endif

namespace pecco {
  // static const
  static const ny::uint COMMON_FACTOR   = 2;
  static const ny::uint COMMON_BIT_UNIT = static_cast <ny::uint> (sizeof (uint64_t)) * 8;
  static const ny::uint COMMON_BITS     = COMMON_BIT_UNIT * COMMON_FACTOR;
#ifndef USE_SSE4_2_POPCNT
  static const char popTable8bit[] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5, //   0- 31
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, //  32- 63
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, //  64- 95
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, //  96-127
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, // 128-159
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, // 160-191
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, // 192-223
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8  // 224-255
  };
  static char popTable16bit[1 << 16];
#endif
  // read a vector of feature indices
  void kernel_model::_convertSVstr2Fv (char* p, ny::fv_t& fv) {
    // convert feature numbers (in string) into feature indices
    fv.clear ();
    while (*p) {
      const ny::uint fn = strton <ny::uint> (p, &p);
      if (_fncnt.find (fn) == _fncnt.end ())
        _fncnt.insert (counter_t::value_type (fn, 0));
      fv.push_back (fn);
      ++p; while (*p && ! isspace (*p)) ++p; // skip value
      while (isspace (*p)) ++p;   // skip (trailing) spaces
    }
  }
  void kernel_model::_precomputeKernel () {
    if (_opt.verbose > 0) std::fprintf (stderr, "precomputing kernel..");
    _polyk = new double[_maf + 1];
    for (ny::uint i = 0; i <= _maf; ++i) // bug; i -> dot product rare
      if (_opt.algo == PKI)
        _polyk[i] = std::pow (_s * static_cast <double> (i) + _r,
                              static_cast <double> (_d));
      else
        _polyk[i] = std::pow (_s * (static_cast <double> (i) + 1) + _r,
                              static_cast <double> (_d)) -
                    std::pow (_s * static_cast <ny::fl_t> (i) + _r,
                              static_cast <double> (_d));
    if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
  }
  // C++ implemenation of Lin+ (2003):
  //   A Note on Platt's Probabilistic Outputs for Support Vector Machines
  void kernel_model::_sigmoid_fitting () {
    if (_opt.verbose > 0)
      std::fprintf (stderr, "Perform sigmoid fitting using examples [-e]..\n");
    // parameters
    if (!_opt.train) {
      warnx ("WARNING: no ref examples [-e], no sigmoid fitting");
      return;
    }
    std::vector <double> f;
    std::vector <bool>   label;
    ny::uint prior1 (0), prior0 (0);
    { // training examples
      FILE*  reader =  std::fopen (_opt.train, "r");
      if (! reader) errx (1, HERE "no such file: %s", _opt.train);
      char*  line = 0;
      size_t read = 0;
      for (ny::fv_t fv; ny::getLine (reader, line, read); fv.clear ()) {
        if (*line != '\0') {
          char* p (line), *p_end (line + read - 1), *l (p);
          while (p != p_end && ! isspace (*p)) ++p; *p = '\0'; ++p;
          const bool target = std::strcmp (l, _li2l[_tli]) == 0;
          double m = - _b[0];
          _convertFv2Fv (p, fv);
#ifdef USE_CEDAR
          if (_opt.algo == PKE || _opt.algo == FST || _opt.algo == PMT)
#else
          if (_opt.algo == PKE || _opt.algo == FST)
#endif
            m += _m0[0];
          if (! fv.empty ())
            switch (_opt.algo) {
              case PKE: _splitClassify <false, BINARY> (fv, &m); break;
              case FST: _fstClassify   <false, BINARY> (fv, &m); break;
#ifdef USE_CEDAR
              case PMT: _pmtClassify   <false, BINARY> (fv, &m); break;
#endif
              case PKI: _pkiClassify   <BINARY>        (fv, &m); break;
            }
          f.push_back (m);
          label.push_back (target);
          if (target) ++prior1; else ++prior0;
        }
      }
      std::fclose (reader);
    }
    // Construct initial values: target support in array t,
    //                           initial function value in F
    const double
      hiTarget ((prior1 + 1.0) / (prior1 + 2.0)),
      loTarget (1 / (prior0 + 2.0));
    const size_t len = prior1 + prior0; // Total number of data
    std::vector <double> t (f.size ());
    for (size_t i = 0; i < len; ++i)
      t[i] = label[i] ? hiTarget : loTarget;
    double A (0), B (std::log ((prior0 + 1.0) / (prior1 + 1.0))), F (0);
    for (size_t i = 0; i < len; ++i) {
      const double fApB = A * f[i] + B;
      F += fApB >= 0 ?
           t[i] * fApB + log1p (std::exp (-fApB)) :
           (t[i] - 1) * fApB + log1p (std::exp (fApB));
    }
    // Parameter setting
    static const ny::uint maxiter = 100;  // Maximum number of iterations
    static const double minstep (1e-10), sigma (1e-12), epsilon (1e-5);
    ny::uint j = 0;
    while (1) {
      // Update Gradient and Hessian (use H' = H + sigma I)
      double p (0), q (0), g1 (0), g2 (0), h11 (sigma), h22 (sigma), h21 (0);
      for (size_t i = 0; i < len; ++i) {
        const double fApB = A * f[i] + B;
        if (fApB >= 0) {
          const double efApB_ = std::exp (-fApB); // avoid expensive std::exp ()
          p = efApB_ / (1.0 + efApB_);
          q = 1.0 / (1.0 + efApB_);
        } else {
          const double efApB = std::exp (fApB);
          p = 1.0 / (1.0 + efApB);
          q = efApB / (1.0 + efApB);
        }
        const double d1 (t[i] - p), d2 (p * q);
        g1 += f[i] * d1;
        g2 += d1;
        h11 += f[i] * f[i] * d2;
        h22 += d2;
        h21 += f[i] * d2;
      }
      if (std::fabs (g1) < epsilon && std::fabs (g2) < epsilon)
        break; // Stopping criteria
      // Compute modified Newton directions
      const double det = h11 * h22 - h21 * h21;
      const double dA = -( h22 * g1 - h21 * g2) / det;
      const double dB = -(-h21 * g1 + h11 * g2) / det;
      const double gd = g1 * dA + g2 * dB;
      double stepsize = 1;
      while (stepsize >= minstep) { // Line search
        const double newA = A + stepsize * dA;
        const double newB = B + stepsize * dB;
        double newF = 0;
        for (size_t i = 0; i < len; ++i) {
          const double fApB = newA * f[i] + newB;
          newF += fApB >= 0 ?
                  t[i] * fApB + log1p (std::exp (-fApB)) :
                  (t[i] - 1) * fApB + log1p (std::exp (fApB));
        }
        if (newF < F + 0.0001 * stepsize * gd) // Sufficient decrease satisfied
          { A = newA; B = newB; F = newF; break; }
        else
          stepsize /= 2.0;
      }
      if (stepsize < minstep) {
        if (_opt.verbose > 0) std::fprintf (stderr, "  Line search fails.\n");
        break;
      }
      if (_opt.verbose > 0)
        std::fprintf (stderr, "  iter=%u  A=%f; B=%f\n", ++j, A, B);
    }
    if (_opt.verbose > 0) {
      if (j >= maxiter)
        std::fprintf (stderr, "  Reaching maximum iterations.\n");
      std::fprintf (stderr, "done.\n");
    }
    _sigmoid_A = A; _sigmoid_B = B;
  }
  void kernel_model::_setup_binary_labels () {
    _nl = 1;
    char* label_pos = new char[3]; std::strcpy (label_pos, "+1");
    char* label_neg = new char[3]; std::strcpy (label_neg, "-1");
    _li2l.push_back (label_pos); _li2l.push_back (label_neg);
    _l2li.insert (lmap::value_type (label_pos, 0));
    _l2li.insert (lmap::value_type (label_neg, 1));
  }
  bool kernel_model::load (const char* model) {
    TIMER (_model_t->startTimer ());
    const std::string model_bin (std::string (model) + ".bin");
    const bool compiled = ! _opt.force && newer (model_bin.c_str (), model);
    FILE* fp = 0;
    if (compiled) {
      if (_opt.verbose > 0)
        std::fprintf (stderr, "loading compiled model parameters..");
      fp = std::fopen (model_bin.c_str (), "rb");
      // model parameters
      std::fread (&_d,      sizeof (ny::uint),   1, fp);
      std::fread (&_nl,     sizeof (ny::uint),   1, fp);
      std::fread (&_nf,     sizeof (ny::uint),   1, fp);
      std::fread (&_nfbit,  sizeof (ny::uint),   1, fp);
      std::fread (&_nsv,    sizeof (ny::uint),   1, fp);
      std::fread (&_maf,    sizeof (ny::uint),   1, fp);
      std::fread (&_r,      sizeof (double),     1, fp);
      std::fread (&_s,      sizeof (double),     1, fp);
      _b  = new double[_nl] ();
      std::fread (_b,       sizeof (double),   _nl, fp);
#ifndef USE_MODEL_SUFFIX
      std::fread (&_minsup, sizeof (ny::uint),   1, fp);
      std::fread (&_sigma,  sizeof (double),     1, fp);
      std::fread (&_fratio, sizeof (double),     1, fp);
#endif
      // label map
      if (_nl >= 2)
        for (ny::uint li = 0; li < _nl; ++li) {
          ny::uint len = 0;
          std::fread (&len, sizeof (ny::uint), 1, fp);
          char* p = new char[len + 1]; p[len] = '\0';
          std::fread (p, sizeof (char), len, fp);
          _li2l.push_back (p);
          _l2li.insert (lmap::value_type (p, li));
          if (strton <int> (p) == 1) _tli = li;
        }
      else
        _setup_binary_labels ();
      // feature map
      _fi2fn.reserve (_nf + 1);
      _fi2fn.push_back (0);
      double ratio = 0;
      for (ny::uint fn = 0;
           _fi2fn.size () <= _nf &&
           std::fread (&fn,    sizeof (ny::uint), 1, fp) &&
           std::fread (&ratio, sizeof (double),   1, fp); ) {
        if (fn >= _fn2fi.size ()) _fn2fi.resize (fn + 1, 0);
        _fn2fi[fn] = static_cast <ny::uint> (_fi2fn.size ());
        _fi2fn.push_back (fn);
        if (ratio >= _fratio) _f_r = static_cast <ny::uint> (_fi2fn.size ());
      }
      // support vectors
      if (_opt.algo == PKI || _f_r - 1 < _nf)
        _f2ss.resize (_nf + 1);
#ifdef USE_PRUNING
#ifdef USE_CEDAR
      if (_opt.algo == PKE || _opt.algo == FST || _opt.algo == PMT)
#else
      if (_opt.algo == PKE || _opt.algo == FST)
#endif
        if (_d > 1 && _f_r - 1 < _nf)
          _f2pn = new pn_t [(_nf + 1) * _nl] ();
#endif
      std::ostringstream ss;
#ifdef USE_MODEL_SUFFIX
      ss << model << ".m" << _opt.minsup << ".s" << _opt.sigma << ".r" << _opt.fratio << ".fstat";
#else
      ss << model << ".fstat";
#endif
      // we should read data when PKE with unknown sigma/fratio
      if (_opt.algo == PKI || _f_r - 1 < _nf || _opt.force || ! newer (ss.str ().c_str (), model)) {
        std::vector <ny::fl_t> alph_ (_nl, 0);
        ny::uint len = 0;
        std::vector <ny::uchar> s;
        ny::fv_t fv;
        // support vectors
        _alph.reserve (_nsv * _nl);
#ifdef USE_CEDAR
        if (_opt.algo == PKE || _opt.algo == FST || _opt.algo == PMT)
#else
        if (_opt.algo == PKE || _opt.algo == FST)
#endif
          _sv.reserve (_nsv);
        for (ny::uint i = 0; i < _nsv; ++i) {
          if (std::fread (&alph_[0], sizeof (ny::fl_t), _nl, fp) != _nl ||
              std::fread (&len,      sizeof (ny::uint),   1, fp) != 1)
            errx (1, HERE "missing SVs; try to recompile model [-c]");
          // read
          if (s.size () < len) s.resize (len);
          std::fread (&s[0], sizeof (ny::uchar), len, fp);
          fv.clear ();
          byte_encoder encoder;
          for (ny::uint j (0), len_ (0), prev (0); len_ < len; fv.push_back (prev))
            len_ += encoder.decode (j, &s[len_]), prev += j;
          for (ny::uint li = 0; li < _nl; ++li) _alph.push_back (alph_[li]);
          if (_opt.algo == PKI) {
            for (ny::fv_it it = fv.begin (); it != fv.end (); ++it)
              _f2ss[*it].push_back (i);
          } else {
            _sv.push_back (fv);
            for (ny::fv_it it = std::lower_bound (fv.begin (), fv.end (), _f_r);
                 it != fv.end (); ++it) {
              _f2ss[*it].push_back (i);
              if (_d > 1)
                for (ny::uint li = 0; li < _nl; ++li) {
                  const ny::fl_t alpha = _alph[i * _nl + li];
                  pn_t& pn = _f2pn[*it * _nl + li];
                  if (alpha > 0) pn.pos += alpha; else pn.neg += alpha;
                }
            }
          }
        }
      }
      std::fclose (fp);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    } else {
      if (_opt.verbose > 0)
        std::fprintf (stderr, "loading/compiling model parameters..");
      FILE* reader = std::fopen (model, "r");
      if (! reader)
        errx (1, HERE "no such file: %s", model);
      bool   header = true;
      bool   linear = false;
      char*  line   = 0;
      size_t read   = 0;
      ny::fv_t fv;
      std::vector <ny::fl_t> alph_;
      while (ny::getLine (reader, line, read)) { // parse
        if (*line != '\0') {
          if (! header) {
            char* p (line);
            for (ny::uint li = 0; li < _nl; ++li)
              alph_[li] = strton <ny::fl_t> (p, &p), ++p;
            if (static_cast <ny::uint> (std::count (alph_.begin (), alph_.end (), 0)) == _nl)
              continue;
            for (ny::uint li = 0; li < _nl; ++li)
              _alph.push_back (alph_[li]);
            _convertSVstr2Fv (p, fv);
            _maf = std::max (_maf, static_cast <ny::uint> (fv.size ()));
            _sv.push_back (fv);
            ++_nsv;
          } else if (std::strstr (line, "# sigmoid")) { // opal
            if (_sigma == 0.0) { // approximate model needs sigmoid fitting
              if (line[read - 2] == 'A')
                _sigmoid_A = std::strtod (line, NULL);
              else
                _sigmoid_B = std::strtod (line, NULL);
            }
          } else if (std::strstr (line, " # kernel type") != NULL) {
            if (*line == '0')
              linear = true, _s = 1, _r = 0, _d = 1;
            else if (*line != '1')
              errx (1, HERE "unsupported kernel used.");
          } else if (char* q = std::strstr (line, " # kernel parameter")) {
            if (linear) continue;
            switch (q[21]) {
              case 's': _s = strton <ny::fl_t> (line, NULL); break;
              case 'r': _r = strton <ny::fl_t> (line, NULL); break;
              case 'd': _d = strton <ny::uint> (line, NULL); break;
            }
          } else if (std::strstr (line, " # labels: ") != NULL) { // multiclass
            char* p (line), * const p_end (line + read - 1);
            _nl = strton <ny::uint> (p, &p); p += 10;
            while (++p) {
              char* ys = p; while (p != p_end && *p != ' ') ++p; *p = '\0';
              char* copy = new char[p - ys + 1];
              std::strcpy (copy, ys);
              const ny::uint li = static_cast <ny::uint> (_l2li.size ());
              if (strton <int> (copy) == 1) _tli = li;
              _l2li.insert (lmap::value_type (copy, li)); // _nl
              _li2l.push_back (copy);
              if (p == p_end) break;
            }
          } else if (std::strstr (line, " # threshold b") != NULL) {
            if (_l2li.empty ()) _setup_binary_labels ();
            _b  = new double[_nl] ();
            char* p = line;
            for (ny::uint li = 0; li < _nl; ++li)
              _b[li] = strton <double> (p, &p), ++p;
            alph_.resize (_nl, 0);
            header = false;
          }
        }
      }
      std::fclose (reader);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
      // Zipf-aware feature indexing
      _nf = static_cast <ny::uint> (_fncnt.size ());
      // set _nfbit for radix sort
      while (((_nf >> _nfbit) | (NBIN - 1)) != NBIN - 1) _nfbit += NBIT;
      _packingFeatures (_sv);
      // output map
      fp = std::fopen (model_bin.c_str (), "wb");
      std::fwrite (&_d,      sizeof (ny::uint),   1, fp);
      std::fwrite (&_nl,     sizeof (ny::uint),   1, fp);
      std::fwrite (&_nf,     sizeof (ny::uint),   1, fp);
      std::fwrite (&_nfbit,  sizeof (ny::uint),   1, fp);
      std::fwrite (&_nsv,    sizeof (ny::uint),   1, fp);
      std::fwrite (&_maf,    sizeof (ny::uint),   1, fp);
      std::fwrite (&_r,      sizeof (double),     1, fp);
      std::fwrite (&_s,      sizeof (double),     1, fp);
      std::fwrite (_b,       sizeof (double),   _nl, fp);
#ifndef USE_MODEL_SUFFIX
      std::fwrite (&_minsup, sizeof (ny::uint),   1, fp);
      std::fwrite (&_sigma,  sizeof (double),     1, fp);
      std::fwrite (&_fratio, sizeof (double),     1, fp);
#endif
      std::fflush (fp);
      if (_nl >= 2)
        for (ny::uint li = 0; li < _nl; ++li) {
          const char* p = _li2l[li];
          ny::uint len = static_cast <ny::uint> (std::strlen (p));
          std::fwrite (&len, sizeof (ny::uint), 1,    fp);
          std::fwrite (p,    sizeof (char),     len, fp);
        }
      // common-to-rare ranking of features
      for (ny::uint fi = 1; fi <= _nf; ++fi) { // fi = 0; fi < _nf
        ny::uint fn = _fi2fn[fi];
        double ratio = static_cast <double> (_fncnt[fn]);
        ratio /= static_cast <double> (_opt.train ? _nt : _nsv);
        ratio *= 100.0;
        if (ratio >= _fratio) _f_r = fi + 1;
        std::fwrite (&fn,    sizeof (ny::uint), 1, fp);
        std::fwrite (&ratio, sizeof (double),   1, fp);
        std::fflush (fp);
      }
      // support vectors
      if (_opt.algo == PKI || _f_r - 1 < _nf)
        _f2ss.resize (_nf + 1);
#ifdef USE_CEDAR
      if (_opt.algo == PKE || _opt.algo == FST || _opt.algo == PMT)
#else
      if (_opt.algo == PKE || _opt.algo == FST)
#endif
        if (_d > 1 && _f_r - 1 < _nf)
          _f2pn = new pn_t [(_nf + 1) * _nl] ();
      std::vector <ny::uchar> s_;
      for (ny::uint i = 0; i < _nsv; ++i) {
        const ny::fv_t& s = _sv[i];
        // compile
        if (s_.size () < s.size () * KEY_SIZE)
          s_.resize (s.size () * KEY_SIZE);
        ny::uint len (0), prev (0);
        byte_encoder encoder;
        for (ny::fv_it it = s.begin (); it != s.end (); prev = *it, ++it)
          len += encoder.encode (*it - prev, &s_[len]);
        std::fwrite (&_alph[i * _nl], sizeof (ny::fl_t),  _nl, fp);
        std::fwrite (&len,            sizeof (ny::uint),    1, fp);
        std::fwrite (&s_[0],          sizeof (ny::uchar), len, fp);
        if (_opt.algo == PKI) {
          for (ny::fv_it it = _sv[i].begin (); it != _sv[i].end (); ++it)
            _f2ss[*it].push_back (i);
        } else {
          for (ny::fv_it it = std::lower_bound (_sv[i].begin (), _sv[i].end (), _f_r);
               it != _sv[i].end (); ++it) {
            _f2ss[*it].push_back (i);
            if (_d > 1)
              for (ny::uint li = 0; li < _nl; ++li) {
                const ny::fl_t alpha = _alph[i * _nl + li];
                pn_t& pn = _f2pn[*it * _nl + li];
                if (alpha > 0) pn.pos += alpha; else pn.neg += alpha;
              }
          }
        }
      }
      std::fclose (fp);
      if (_opt.algo == PKI)
        std::vector <ny::fv_t> ().swap (_sv);
      // counter_t ().swap (_fncnt); // compilation error in gcc 4.0
    }
#ifdef ABUSE_TRIE
    if (! is_binary_classification ())
      errx (1, HERE "ABUSE_TRIE accepts only a binary label.");
#endif
    if (_d == 1) {
#ifdef USE_CEDAR
      if (_opt.algo == FST || _opt.algo == PMT)
        warnx ("NOTE: [-t 2 or 3] is useless in d = 1.");
#else
      if (_opt.algo == FST)
        warnx ("NOTE: [-t 2] is useless in d = 1.");
#endif
      if (_f_r - 1 < _nf)
        warnx ("NOTE: [-r > 0] is useless in d = 1.");
    }
    _fv.reserve (_maf);
    _score.resize (_nl);
    // calculate dot product
    if (_opt.algo == PKI || _f_r - 1 < _nf)
      _precomputeKernel ();
    // used to indicate active features
    if (_opt.algo == PKI) {
      _dot = new ny::uint[_nsv] ();
      // workaround for a bug in value initialization in gcc 4.0
      for (ny::uint i = 0; i < _nsv; ++i) _dot[i] = 0;
    }
    else if (_f_r - 1 < _nf)
      _fbit.resize (_nf + 1, false);
    // kernel expansion
    _nf_cut = _nf;
#ifdef USE_CEDAR
    if (_opt.algo == PKE || _opt.algo == FST || _opt.algo == PMT) {
#else
    if (_opt.algo == PKE || _opt.algo == FST) {
#endif
      _setPKEcoeff ();
      _setFtrie ();
      if (_f_r - 1 == _nf) {
        std::vector <ny::fv_t> ().swap (_sv);
        std::vector <ny::fl_t> ().swap (_alph); // clear
      } else { // fv -> std::min (_f_r, COMMON_BITS) -> svbits;
        _svbits.resize (_nsv * COMMON_FACTOR, 0);
        const ny::uint lim = std::min (_f_r, COMMON_BITS);
        for (ny::uint i = 0; i < _nsv; ++i) {
          ny::fv_t& s = _sv[i];
          ny::fv_t::iterator end = std::lower_bound (s.begin (), s.end (), lim);
          for (ny::fv_it jt = s.begin (); jt != end; ++jt)
            _svbits[i * COMMON_FACTOR + *jt / COMMON_BIT_UNIT]
              |= 1UL << (*jt & (COMMON_BIT_UNIT - 1));
          s.erase (s.begin (), end);
          ny::fv_t (s).swap (s); // shrink
        }
#ifndef USE_SSE4_2_POPCNT
        for (ny::uint i = 0; i < (1 << 16); ++i)
          popTable16bit[i]
            = static_cast <char> (popTable8bit[i >> 8] + popTable8bit[i & 0xff]);
#endif
      }
    }
    if (_opt.algo != PKI && (_sigma != 0.0 || _minsup != 1) && _opt.verbose > 0) {
      warnx ("NOTE: approximated computation;");
      std::fprintf (stderr, " %d/%d features used, sigma=%g, minsup=%u\n",
                    _nf_cut, _nf, _sigma, _minsup); // _f_r - 1
    }
    // fstrie construction
    if (_opt.algo == FST) _setFStrie ();
#ifdef USE_CEDAR
    if (_opt.algo == PMT)
      _pms = new double[(1 << _opt.pmsize) * _nl];
#endif
    TIMER (_model_t->stopTimer ());
    // sigmoid fitting
    if (is_binary_classification ()) {
      std::ostringstream ss;
#ifdef USE_MODEL_SUFFIX
      ss << model << ".m" << _opt.minsup << ".s" << _opt.sigma << ".sigmoid";
#else
      ss << model << ".sigmoid";
#endif
      fp = std::fopen (ss.str ().c_str (), "rb");
      if (!_opt.force && fp) {
        std::fread (&_sigmoid_A, sizeof (double), 1, fp);
        std::fread (&_sigmoid_B, sizeof (double), 1, fp);
      } else {
        if (std::fpclassify (_sigmoid_A + 1.0) == FP_ZERO &&
            std::fpclassify (_sigmoid_B)       == FP_ZERO)
          _sigmoid_fitting ();
        fp = std::fopen (ss.str ().c_str (), "wb");
        std::fwrite (&_sigmoid_A, sizeof (double), 1, fp);
        std::fwrite (&_sigmoid_B, sizeof (double), 1, fp);
      }
      std::fclose (fp);
      if (_opt.verbose > 2)
        std::fprintf (stderr, "A=%f; B=%f\n", _sigmoid_A, _sigmoid_B);
    }
    if (_opt.verbose > 0) printParam ();
    return true;
  }
  void kernel_model::_setPKEcoeff () {
    // calculate weight coefficients for conjunctive features
    switch (_d) {
      case 1:
        _coeff[0] = _r;
        _coeff[1] = _s; break;
      case 2:
        _coeff[0] = _r*_r;
        _coeff[1] = _s*_s+2*_r*_s;
        _coeff[2] =  2*_s*_s; break;
      case 3:
        _coeff[0] = _r*_r*_r;
        _coeff[1] = _s*_s*_s + 3*_r*_s*_s + 3*_r*_r*_s;
        _coeff[2] =  6*_s*_s*_s + 6*_r*_s*_s;
        _coeff[3] =  6*_s*_s*_s; break;
      case 4:
        _coeff[0] = _r*_r*_r*_r;
        _coeff[1] = _s*_s*_s*_s + 4*_r*_s*_s*_s + 6*_r*_r*_s*_s + 4*_r*_r*_r*_s;
        _coeff[2] = 14*_s*_s*_s*_s + 24*_r*_s*_s*_s  + 12*_r*_r*_s*_s;
        _coeff[3] = 36*_s*_s*_s*_s + 24*_r*_s*_s*_s;
        _coeff[4] = 24*_s*_s*_s*_s; break;
      default: errx (1, HERE "please add case statement.");
    }
    _max_coeff = *std::max_element (&_coeff[0], &_coeff[_d+1]);
  }
  // implementation of kernel expansion (Kudo & Matsumoto, 2003) for multi-class
  void kernel_model::_pkePrefixSpan (ny::fv_t& fc, std::vector <ny::fl_t>& fw,
                                     const std::vector <std::pair <ny::uint, int> >& proj,
                                     std::vector <FeatKey*>& pke_key,
                                     ny::fl_t* w, double* mu_pos, double* mu_neg,
                                     ny::uint& processed) {
    // pseudo projection to mine conjunctive features
    typedef std::vector <std::pair <ny::uint, int> > proj_t;
    typedef ny::map <ny::uint, proj_t>::type proj_map_t;
    proj_map_t proj_map;
    for (proj_t::const_iterator pit = proj.begin (); pit != proj.end (); ++pit) {
      const ny::uint i   = pit->first;
      const ny::fv_t& fv = _sv[i];
      for (int j = pit->second; j >= 0; --j)
        proj_map[fv[static_cast <size_t> (j)]].push_back (proj_t::value_type (i, j - 1));
    }
    ny::uchar s[KEY_SIZE * MAX_KERNEL_DEGREE]; // need to new if copied?
    for (proj_map_t::const_iterator pmit = proj_map.begin ();
         pmit != proj_map.end (); ++pmit) {
      // extend span
      const ny::uint fi         = pmit->first;
      const proj_t&  proj_child = pmit->second;
      if (proj_child.size () < _minsup)
        continue;
      std::fill (&w[0],      &w[_nl],      0);
      std::fill (&mu_pos[0], &mu_pos[_nl], 0);
      std::fill (&mu_neg[0], &mu_neg[_nl], 0);
      fc.push_back (fi);
      bool terminate = (fc.size () >= _d || proj_child.empty ());
      for (proj_t::const_iterator pit = proj_child.begin ();
           pit != proj_child.end (); ++pit) {
        const ny::fl_t* alpha = &_alph[pit->first * _nl];
        for (ny::uint li = 0; li < _nl; ++li)
          w[li] += alpha[li] * _coeff[fc.size ()];
        if (! terminate) {
          for (ny::uint li = 0; li < _nl; ++li)
            if (alpha[li] > 0)
              mu_pos[li] += alpha[li] * _max_coeff;
            else
              mu_neg[li] += alpha[li] * _max_coeff;
        }
      }
      bool terminate_ = true;
      bool exceed     = false;
      for (ny::uint li = 0; li < _nl; ++li) { // pruning
        terminate_ &= (mu_neg[li] > _sigma_neg[li] && mu_pos[li] < _sigma_pos[li]);
        exceed    |= w[li] <= _sigma_neg[li] || w[li] >= _sigma_pos[li];
      }
      terminate |= terminate_; // the extending features will be useless
      if (exceed) { // approximation
        if (static_cast <ny::uint> (std::count (&w[0], &w[_nl], 0)) != _nl) {
          ++_ncf;
          if (fc.front () >> PSEUDO_TRIE_N[_d]) {
            ny::uint len = 0;
            byte_encoder encoder;
            for (ny::fv_it it = fc.begin (); it != fc.end (); ++it)
              len += encoder.encode (*it, s + len);
            // we remain the weights when a weight for one label exceeds threshold
            // because it does not take extra memory in current implementation
            pke_key.push_back (new FeatKey (s, &w[0], len, _nl));
          } else {
            ny::uint pos = 0;
            for (ny::uint i = 0; i < fc.size (); ++i) {
              const ny::uint j = fc[i] - 1;
              switch (_d - i) {
                case 4: pos += j * (j - 1) * (j - 2) * (j - 3) / 24;
                case 3: pos += j * (j - 1) * (j - 2) / 6;
                case 2: pos += j * (j - 1) / 2;
                case 1: pos += j;
              }
              if (i > 0) ++pos;
            }
            for (ny::uint li = 0; li < _nl; ++li)
              fw[pos * _nl + li] = static_cast <ny::fl_t> (w[li]);
          }
          // for pruning
          for (ny::fv_it it = fc.begin (); it != fc.end (); ++it)
            for (ny::uint li = 0; li < _nl; ++li) {
              if (w[li] > 0)
                _f2nf[*it * _nl + li].pos += static_cast <ny::fl_t> (w[li]);
              else
                _f2nf[*it * _nl + li].neg += static_cast <ny::fl_t> (w[li]);
#ifdef USE_PRUNING
              if (_d > 1) {
                pn_t& dpn = _f2dpn[*it * _nl * _d + li * _d + fc.size () - 1];
                if (w[li] > 0)
                  dpn.pos = std::max (dpn.pos, static_cast <ny::fl_t> (w[li]));
                else
                  dpn.neg = std::min (dpn.neg, static_cast <ny::fl_t> (w[li]));
              }
#endif
            }
        }
      }
      if (! terminate) _pkePrefixSpan (fc, fw, proj_child, pke_key, w, mu_pos, mu_neg, processed);
      fc.pop_back ();
      if (_opt.verbose > 0 && fc.empty ())
        std::fprintf (stderr,
                      "\r processed %d features => %d conjunctive features",
                      ++processed, _ncf);
    }
    if (_opt.verbose > 0 && fc.empty ()) std::fprintf (stderr, "\n");
  }
  // polynomial kernel expanded
  bool kernel_model::_setFtrie () {
    std::ostringstream ss;
    ss << _opt.model;
#ifdef USE_MODEL_SUFFIX
    ss << ".m" << _opt.minsup << ".s" << _opt.sigma << ".r" << _opt.fratio;
#endif
#ifdef ABUSE_TRIE
    const std::string ftrie_fn (ss.str () + ".e"  TRIE_SUFFIX);
#else
    const std::string ftrie_fn (ss.str () + ".ne" TRIE_SUFFIX); // w/o alpha
#endif
    const std::string fw_fn (ss.str () + ".weight");
    const std::string fs_fn (ss.str () + ".fstat");
    _m0 = new double[_nl] ();
    // workaround for a bug in value initialization in gcc 4.0
    for (ny::uint i = 0; i < _nl; ++i) _m0[i] = 0;
#ifdef USE_PRUNING
    if (_d > 1)
      _f2dpn = new pn_t[_f_r * _nl * _d] ();
#endif
    _f2nf  = new pn_t[_f_r * _nl] ();
    // set PKE coeff
    if (_opt.verbose > 0) std::fprintf (stderr, "loading feature weight trie..");
    if (! _opt.force && newer (fs_fn.c_str (), _opt.model)) {
      // read pre-computed weights
      _ftrie.open (ftrie_fn.c_str ()); // may fail
      // fstat
      FILE* reader = std::fopen (fs_fn.c_str (), "rb");
      if (! reader) errx (1, HERE "no such file: %s", fs_fn.c_str ());
      std::fread (_m0,       sizeof (double),   _nl, reader);
      std::fread (&_ncf,     sizeof (ny::uint),   1, reader);
      std::fread (&_nf_cut,  sizeof (ny::uint),   1, reader);
      std::fread (&_f2nf[0], sizeof (pn_t),     _f_r * _nl, reader);
#ifdef USE_PRUNING
      if (_d > 1) {
        std::fread (&_f2dpn[0], sizeof (pn_t), _f_r * _nl * _d, reader);
        if (_f_r - 1 < _nf)
          std::fread (&_f2pn[0],  sizeof (pn_t), (_nf + 1) * _nl, reader);
      }
#endif
      std::fclose (reader);
      if (_f_r - 1 == _nf)
        for (ny::uint fi = 1; fi < _f_r; ++fi)
          if (static_cast <ny::uint> (std::count (&_f2nf[fi * _nl], &_f2nf[fi * _nl + _nl], pn_t ())) == _nl)
            _fn2fi[_fi2fn[fi]] = 0; // this feature will be never used
#ifdef USE_PRUNING
      if (_d == 1) delete [] _f2nf;
#else
      delete [] _f2nf;
#endif
      //
      reader = std::fopen (fw_fn.c_str (), "rb");
      if (! reader) errx (1, HERE "no such file: %s", fw_fn.c_str ());
      if (std::fseek (reader, 0, SEEK_END) != 0) return -1;
      const size_t uniq
        = static_cast <size_t> (std::ftell (reader)) / (_nl * sizeof (ny::fl_t));
      if (std::fseek (reader, 0, SEEK_SET) != 0) return -1;
      _fw = new ny::fl_t [uniq * _nl];
      std::fread (&_fw[0], sizeof (ny::fl_t), uniq * _nl, reader);
      std::fclose (reader);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    } else {
      if (_opt.verbose > 0) {
        std::fprintf (stderr, "not found.\n");
        std::fprintf (stderr, "calculating conjunctive feature weight..");
      }
      if (_f_r == 1) { // no common feature
        if (_opt.verbose > 0) std::fprintf (stderr, "skipped.\n");
        return false;
      }
      if (_opt.verbose > 0) std::fprintf (stderr, "\n");
      size_t uniq = 0;
      const size_t N = std::min ((1U << PSEUDO_TRIE_N[_d]) - 1, _f_r - 1);
      switch (_d) {
        case 4: uniq += N * (N - 1) * (N - 2) * (N - 3) / 24;
        case 3: uniq += N * (N - 1) * (N - 2) / 6;
        case 2: uniq += N * (N - 1) / 2;
        case 1: uniq += N;
      }
      std::vector <ny::fl_t> fw (uniq * _nl, 0); // initialized to 0;
      std::vector <FeatKey *> pke_key;
      {
        std::vector <size_t> pos_num (_nl, 0);
        std::vector <size_t> neg_num (_nl, 0);
        // pseudo projection (PrefixSpan)
        std::vector <std::pair <ny::uint, int> > proj;
        for (ny::uint i = 0; i < _nsv; ++i) {
          const int tail =
            static_cast <int> (std::lower_bound (_sv[i].begin (), _sv[i].end (), _f_r) - _sv[i].begin () - 1);
          proj.push_back (std::make_pair (i, tail));
          const ny::fl_t* alpha = &_alph[i * _nl];
          for (ny::uint li = 0; li < _nl; ++li) {
            _m0[li] += alpha[li];
            if (alpha[li] > 0) ++pos_num[li]; else ++neg_num[li];
          }
        }
        _sigma_pos.resize (_nl, 0);
        _sigma_neg.resize (_nl, 0);
        for (ny::uint li = 0; li < _nl; ++li)  {
          _sigma_pos[li] =  _sigma * static_cast <double> (pos_num[li]) / _nsv;
          _sigma_neg[li] = -_sigma * static_cast <double> (neg_num[li]) / _nsv;
        }
        if (_opt.verbose > 0) {
          std::fprintf (stderr, " %d / %d features,\n", _f_r - 1, _nf);
          std::fprintf (stderr, "  minsup=%u\n", _minsup);
          for (ny::uint li = 0; li < _nl; ++li)
            std::fprintf (stderr, "  sigma_pos%u=%g, sigma_neg%u=%g\n",
                          li, _sigma_pos[li], li, _sigma_neg[li]);
        }
        ny::fv_t fc;
        std::vector <ny::fl_t> w (_nl);
        std::vector <double> mu_pos (_nl), mu_neg (_nl);
        ny::uint processed = 0;
        _pkePrefixSpan (fc, fw, proj, pke_key, &w[0], &mu_pos[0], &mu_neg[0], processed);
      }
      _nf_cut = 0;
      for (ny::uint fi = 1; fi < _f_r; ++fi)
        if (static_cast <ny::uint> (std::count (&_f2nf[fi * _nl], &_f2nf[fi * _nl + _nl], pn_t ())) != _nl)
          ++_nf_cut;
        else if (_f_r - 1 == _nf)
          _fn2fi[_fi2fn[fi]] = 0; // this feature will be never used
      if (_opt.verbose > 0)
        std::fprintf (stderr, " # active (common) features after thresholding %d => %d\n",
                      _nf, _nf_cut);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
#ifndef USE_ABUSE
      fw.resize ((uniq + _ncf) * _nl, 0);
#endif
      typedef std::map <std::vector <ny::fl_t>, size_t> w2id_t;
      w2id_t w2id;
      std::vector <ny::fl_t> w (_nl);
      for (size_t i = 0; i < uniq; ++i) {
        w.assign (&fw[i * _nl], &fw[(i + 1) * _nl]);
        w2id.insert (w2id_t::value_type (w, i * _nl));
      }
      // build feature weight trie
      if (! pke_key.empty ()) {
        std::vector <const char*> str; str.reserve (pke_key.size ());
        std::vector <size_t>      len; len.reserve (pke_key.size ());
        std::vector <int>         val; val.reserve (pke_key.size ());
        FeatKeypLess  featkeypless;
        std::sort (pke_key.begin (), pke_key.end (), featkeypless);
        std::vector <FeatKey *>::const_iterator dit = pke_key.begin ();
        for (; dit != pke_key.end (); ++dit) {
          str.push_back (reinterpret_cast <char *> ((*dit)->key));
          len.push_back ((*dit)->len);
          ny::fl_t* wv = (*dit)->cont;
#ifdef ABUSE_TRIE
          union byte_4 b4 (static_cast <float> (*wv)); b4.u >>= 1;
          val.push_back (b4.i);
#else
          // uniq identical weights
          // (minimize the double array with a slight cache-loss increase)
          w.assign (&wv[0], &wv[_nl]);
          std::pair <w2id_t::iterator, bool> itb
            = w2id.insert (w2id_t::value_type (w, uniq * _nl));
          if (itb.second) {
            for (ny::uint li = 0; li < _nl; ++li)
              fw[uniq * _nl + li] = wv[li]; // put weights for all labels
            ++uniq;
          }
          val.push_back (static_cast <int> (itb.first->second));
#endif
        }
        build_trie (&_ftrie, "feature weight trie", ftrie_fn, str, len, val,
                    _opt.verbose > 0);
        for (dit = pke_key.begin (); dit != pke_key.end (); ++dit) delete *dit;
      }
      for (ny::uint li = 0; li < _nl;  ++li) _m0[li] *= _coeff[0]; // bug fix
      FILE* writer = std::fopen (fw_fn.c_str (), "wb");
      std::fwrite (&fw[0], sizeof (ny::fl_t), uniq * _nl, writer);
      std::fclose (writer);
      _fw = new ny::fl_t [uniq * _nl];
      std::copy (&fw[0], &fw[uniq * _nl], _fw);
      //
      for (ny::uint li = 0; li < _nl;  ++li) _m0[li] *= _coeff[0]; // bug fix
      writer = std::fopen (fs_fn.c_str (), "wb");
      std::fwrite (_m0,       sizeof (double),          _nl, writer);
      std::fwrite (&_ncf,     sizeof (ny::uint),          1, writer);
      std::fwrite (&_nf_cut,  sizeof (ny::uint),          1, writer);
      std::fwrite (&_f2nf[0], sizeof (pn_t),     _f_r * _nl, writer);
#ifdef USE_PRUNING
      if (_d > 1) {
        std::fwrite (&_f2dpn[0], sizeof (pn_t), _f_r * _nl * _d, writer);
        if (_f_r - 1 < _nf)
          std::fwrite (&_f2pn[0],  sizeof (pn_t), (_nf + 1) * _nl, writer);
      }
#endif
      std::fclose (writer);
#ifdef USE_PRUNING
      if (_d == 1) delete [] _f2nf;
#else
      delete [] _f2nf;
#endif
    }
    return true;
  }
  
  template <binary_t FLAG>
  void kernel_model::_pkiClassify (const ny::fv_t& fv, double* score) {
    for (ny::fv_it it = fv.begin (); it != fv.end (); ++it) {
      const ss_t& ss = _f2ss[*it];
      for (ss_it st = ss.begin (); st != ss.end (); ++st) ++_dot[*st];
    }
    for (ny::uint i = 0; i < _nsv; ++i) {// can be faster if alpha in sv
      addScore <FLAG> (score, i, _polyk[_dot[i]]);
      _dot[i] = 0;
    }
  }
  // implementation of splitSVM (Goldberg & Elhadad, 2008)
  template <bool PRUNE, binary_t FLAG>
  void kernel_model::_splitClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end) {
    if (_f_r - 1 == _nf) // no rare feature
      { _pkeClassify <PRUNE, FLAG> (score, it, beg, end); return; }
    ny::fv_it rit = std::lower_bound (it, end, _f_r);
    if (it != rit) // remaining common features from it to end
      if (_pkeClassify <PRUNE, FLAG> (score, it, beg, rit) || rit == end)
        return;
    PROFILE (_hit += rit - fv.begin ());
    ny::fv_it cit = beg;
    // inner product using popcnt
    uint64_t fbits[COMMON_FACTOR];
    std::fill (&fbits[0], &fbits[COMMON_FACTOR], 0);
    for (; cit != rit && *cit < COMMON_BITS; ++cit)
      fbits[*cit / COMMON_BIT_UNIT] |= 1UL << (*cit & (COMMON_BIT_UNIT - 1));
    //
    for (; cit != rit; ++cit) _fbit[*cit] = true;
    for (; rit != end; ++rit) {
      if (_prune <PRUNE, FLAG> (score, static_cast <size_t> (std::distance (beg, rit)))) break;
      const ss_t& ss = _f2ss[*rit];
      for (ss_it st = ss.begin (); st != ss.end (); ++st) {
        const ny::fv_t& s = _sv[*st];
        const uint64_t* const sbits = &_svbits[*st * COMMON_FACTOR];
        int dot_c = 0;
        for (ny::uint i = 0; i < COMMON_FACTOR; ++i) {
#ifdef USE_SSE4_2_POPCNT
          dot_c += static_cast <int> (_mm_popcnt_u64 (fbits[i] & sbits[i]));
#else
          const uint64_t r = fbits[i] & sbits[i];
          dot_c += popTable16bit[(r >>  0) & 0xffff];
          dot_c += popTable16bit[(r >> 16) & 0xffff];
          dot_c += popTable16bit[(r >> 32) & 0xffff];
          dot_c += popTable16bit[(r >> 48) & 0xffff];
#endif
        }
        for (ny::fv_it sit = s.begin (); sit != s.end (); ++sit)
          dot_c += _fbit[*sit];
        addScore <FLAG> (score, *st, _polyk[dot_c]);
      }
      _fbit[*rit] = true;
    }
    for (it = beg; it != end; ++it) _fbit[*it] = false;
  }
  // explicit specialization
  template void kernel_model::_pkiClassify <BINARY> (const ny::fv_t&, double*);
  template void kernel_model::_pkiClassify <MULTI>  (const ny::fv_t&, double*);
  template void kernel_model::_splitClassify <true,  BINARY>  (ny::fv_t&, double*);
  template void kernel_model::_splitClassify <true,  MULTI>   (ny::fv_t&, double*);
  template void kernel_model::_splitClassify <false, BINARY>  (ny::fv_t&, double*);
  template void kernel_model::_splitClassify <false, MULTI>   (ny::fv_t&, double*);
}
// score to weight
