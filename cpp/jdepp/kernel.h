// pecco -- please enjoy classification with conjunctive features
//  $Id: kernel.h 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef POLYK_CLASSIFY_H
#define POLYK_CLASSIFY_H

#include <sys/stat.h>
#include <err.h>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include "typedef.h"
#include "timer.h"
#include "classify.h"

namespace pecco {
  class kernel_model : public ClassifierBase <kernel_model>
  {
    // type alias
    typedef std::vector <ny::uint> ss_t;
    typedef std::vector <ny::uint>::const_iterator ss_it;
  private:
    // kernel parameters; (_s w * x + _r)^_d
    double                 _s;
    double                 _r;
    double*                _b;
    double*                _m0;
    // kernel cache
    double*                _polyk;
    // support vector
    std::vector <ny::fv_t> _sv;   // SV ID -> <alpha, feature vector>
    std::vector <uint64_t> _svbits;
    std::vector <ss_t>     _f2ss; // inverted indices from feature to SV IDs
    pn_t*                  _f2pn;
    std::vector <ny::fl_t> _alph; // weight of SVs (used in PKI)
    // temporary variables
    ny::uint*              _dot;  // store feature on/off
    std::vector <char>     _fbit;
    ny::uint               _nsv;  // # support vectors
    ny::uint               _f_r;  // min rare feature id (= # common feature + 1)
    ny::uint               _maf;  // max active features per vector;
    // polynomial kernel expansion
    ny::uint               _minsup;
    double                 _sigma;
    double                 _fratio;
    std::vector <double>   _sigma_pos;
    std::vector <double>   _sigma_neg;
    double                 _coeff[MAX_KERNEL_DEGREE + 1];
    double                 _max_coeff;
    // sigmoid params
    double                 _sigmoid_A;
    double                 _sigmoid_B;
    // internal functions
    void _convertSVstr2Fv (char* p, ny::fv_t& fv);
    void _precomputeKernel ();
    void _sigmoid_fitting ();
    double _sigmoid (const double score) const
    { return 1.0 / (1.0 + std::exp (_sigmoid_A * score + _sigmoid_B)); }
    bool _setFtrie ();
    void _setPKEcoeff ();
    void _pkePrefixSpan (ny::fv_t& fc, std::vector <ny::fl_t>& fw, const std::vector <std::pair <ny::uint, int> >& proj, std::vector <FeatKey*>& pke_key, ny::fl_t* w, double* mu_pos, double* mu_neg, ny::uint& processed);
    // classifier
    template <bool PRUNE, binary_t FLAG>
    void _splitClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end);
    template <bool PRUNE, binary_t FLAG>
    void _splitClassify (ny::fv_t& fv, double* score) {
      if (_d == 1) {
        if (_f_r - 1 < _nf) _sortFv (fv);
        _splitClassify <false, FLAG> (score, fv.begin (), fv.begin (), fv.end ());
      } else {
        _sortFv (fv);
        if (PRUNE) _estimate_bound <FLAG> (fv.begin (), fv.begin (), fv.end ());
        _splitClassify <PRUNE, FLAG> (score, fv.begin (), fv.begin (), fv.end ());
      }
    }
    template <binary_t FLAG>
    void _pkiClassify (const ny::fv_t& fv, double* score);
    void _setup_binary_labels ();
  public:
    kernel_model (const pecco::option& opt) : ClassifierBase <kernel_model> (opt), _s (0), _r (0), _b (0), _m0 (0), _polyk (0), _sv (), _svbits (), _f2ss (), _f2pn (0), _alph (), _dot (0), _fbit (), _nsv (0), _f_r (1), _maf (0), _minsup (strton <ny::uint> (_opt.minsup)), _sigma (strton <double> (_opt.sigma)), _fratio (strton <double> (_opt.fratio)), _sigma_pos (0), _sigma_neg (0), _max_coeff (0), _sigmoid_A (-1.0), _sigmoid_B (0) {} //  _nl = 1; _li2l.push_back ("+1"); _li2l.push_back ("-1");
    ~kernel_model () {
      delete [] _b;
      if (_opt.algo == PKI || _f_r - 1 < _nf) delete [] _polyk;
      if (_opt.algo == PKI) {
        delete [] _dot;
      } else if (_d > 1) {
        delete [] _m0;
#ifdef USE_PRUNING
        delete [] _f2dpn;
        delete [] _f2nf;
        if (_f_r - 1 < _nf) delete [] _f2pn;
#endif
      }
#ifndef ABUSE_TRIE
      if (_opt.algo == PKE || _opt.algo == FST) delete [] _fw;
      if (_opt.algo == FST) delete [] _fsw;
#ifdef USE_CEDAR
      if (_opt.algo == PMT) delete [] _pms;
#endif
#endif
      for (size_t li = 0; li < _li2l.size (); ++li)
        delete [] _li2l[li];
    }
    //
    template <binary_t FLAG>
    void resetScore (double* score) const;
    template <binary_t FLAG>
    void resetScore (double* score, const double* const score_) const;
    template <binary_t FLAG>
    void addScore (double* score, const ny::uint pos) const;
    template <binary_t FLAG>
    void addScore (double* score, const int n, const ny::fl_t* const w) const;
    template <binary_t FLAG>
    void addScore (double* score, const ny::uint pos, const double m) const;
    template <binary_t FLAG>
    bool prune (double* m, const size_t i);
    template <int D, binary_t FLAG>
    void estimate_bound (const ny::fv_it& first, const ny::fv_it& beg, ny::fv_it it);
    template <bool PRUNE, binary_t FLAG>
    void baseClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end)
    { _splitClassify <PRUNE, FLAG> (score, it, beg, end); }
    bool load (const char* model); // set up model
    void printParam () {
      std::fprintf (stderr, "kernel: (%g * s^T x + %g)^%u\n", _s, _r, _d);
      std::fprintf (stderr, "# support vectors: %u\n", _nsv);
      std::fprintf (stderr, "# active features: %u",   _nf_cut);
      if (_opt.algo == PKE || _opt.algo == FST)
        std::fprintf (stderr, " (%u)", _ncf);
      std::fprintf (stderr, "\n");
      if (_opt.verbose > 1)
        std::fprintf (stderr, "  # common features: %u\n", _f_r - 1);
      if (is_binary_classification ())
        std::fprintf (stderr, "sigmoid A=%g; B=%g\n", _sigmoid_A, _sigmoid_B);
    }
    // classification interface
    template <bool PRUNE, binary_t FLAG>
    void classify (ny::fv_t& fv, double* score) {
      if (FLAG)
        *score = - *_b;
      else
        for (ny::uint li = 0; li < _nl; ++li) score[li] = - _b[li];
      TIMER (if (_opt.verbose > 0) _enc_t->startTimer ());
      _convertFv2Fv (fv);
      TIMER (if (_opt.verbose > 0) _enc_t->stopTimer ());
      TIMER (_classify_t->startTimer ());
//      const uint64_t start = ny::Timer::rdtsc ();
      if (_opt.algo != PKI)
        for (ny::uint li = 0; li < _nl; ++li) score[li] += _m0[li];
      if (fv.empty ()) {
//        _elapsed = ny::Timer::rdtsc () - start;
        TIMER (_classify_t->stopTimer ()); return; }
      switch (_opt.algo) {
        case PKE: _splitClassify <PRUNE, FLAG> (fv, score); break;
        case FST: _fstClassify   <PRUNE, FLAG> (fv, score); break;
#ifdef USE_CEDAR
        case PMT: _pmtClassify   <PRUNE, FLAG> (fv, score); break;
#endif
        case PKI: _pkiClassify   <FLAG>        (fv, score); break;
      }
//      _elapsed = ny::Timer::rdtsc () - start;
      TIMER (_classify_t->stopTimer ());
    }
    bool is_binary_classification () const { return _nl == 1; }
#ifdef ABUSE_TRIE
    bool abuse_trie () const { return true; }
#else
    bool abuse_trie () const { return false; }
#endif
    double threshold () { return _sigmoid (0); }
    bool binClassify (ny::fv_t& fv, char* target = 0) {
      if (is_binary_classification ()) {
        double score = 0;
#ifdef USE_PRUNING
        classify <true, BINARY>  (fv, &score);
#else
        classify <false, BINARY> (fv, &score);
#endif
        return score > 0;
      } else {
#ifdef USE_RPUNING
        classify <true, MULTI>  (fv, &_score[0]);
#else
        classify <false, MULTI> (fv, &_score[0]);
#endif
        const ny::uint li =
          static_cast <ny::uint> (std::max_element (&_score[0], &_score[_nl]) - &_score[0]);
        if (target)
          return std::strcmp (_li2l[li], target) == 0;
        else
          return li == _tli;
      }
    }
    double getProbability (ny::fv_t& fv) {
      if (! is_binary_classification ())
        errx (1, HERE "sorry, probability output is unsupported.");
      double score = 0;
      classify <false, BINARY> (fv, &score);
      return _sigmoid (score);
    }
    ny::uint getLabel (const double* score) const {
      return is_binary_classification () ?
        (*score >= 0 ? 0 : 1) :
        static_cast <ny::uint> (std::max_element (&score[0], &score[_nl]) - &score[0]);
    }
    void printScore (const ny::uint li, const double* score) const {
      std::fprintf (stdout, "%s %f", _li2l[li],
                    is_binary_classification () ? *score : score[li]);
    }
    void printProb (const ny::uint li, const double* score) const {
      if (! is_binary_classification ())
        errx (1, HERE "no probability output is supported for multi-class kernel model");
      const double prob = li == _tli ? _sigmoid (*score) : 1 - _sigmoid (*score);
      std::fprintf (stdout, "%s %f", _li2l[li], prob);
    }
  };
  template <binary_t FLAG>
  inline void kernel_model::resetScore (double* score) const { *score = 0; }
  template <binary_t FLAG>
  inline void kernel_model::resetScore (double* score, const double* const score_) const
  { for (ny::uint li = 0; li < _nl; ++li) score[li] = score_[li]; }
  template <>
  inline void kernel_model::addScore <BINARY> (double* score, const ny::uint pos) const
  { *score += _fw[pos]; }
  template <>
  inline void kernel_model::addScore <MULTI>  (double* score, const ny::uint pos) const {
    const ny::fl_t* const fw = &_fw[pos * _nl];
    for (ny::uint li (0); li < _nl; ++li) score[li] += fw[li];
  }
  template <>
#ifdef ABUSE_TRIE
  inline void kernel_model::addScore <BINARY> (double* score, const int n, const ny::fl_t* const) const
  { union byte_4 b4 (n); b4.u <<= 1; *score += b4.f; }
#else
  inline void kernel_model::addScore <BINARY> (double* score, const int n, const ny::fl_t* const w) const
  { *score += w[n]; }
#endif
  template <>
  inline void kernel_model::addScore <MULTI>  (double* score, const int n, const ny::fl_t* const w) const {
    const ny::fl_t* const fw = &w[n];
    for (ny::uint li = 0; li < _nl; ++li) score[li] += fw[li];
  }
  template <>
  inline void kernel_model::addScore <BINARY> (double* score, const ny::uint pos, const double m) const
  { *score += _alph[pos] * m; }
  template <>
  inline void kernel_model::addScore <MULTI>  (double* score, const ny::uint pos, const double m) const {
     const ny::fl_t* const alph = &_alph[pos * _nl];
    for (ny::uint li (0); li < _nl; ++li)
      score[li] += alph[li] * m;
  }
  template <>
  inline bool kernel_model::prune <BINARY> (double* m, const size_t i) {
    const pn_t& b = _bound[i];
    const ny::uint y = *m >= 0 ? 0 : 1;
    if (y == 0)
      { if (*m + b.neg <= 0) return false; *m += b.neg; }
    else
      { if (*m + b.pos >= 0) return false; *m += b.pos; }
    return true;
  }
  template <>
  inline bool kernel_model::prune <MULTI> (double* m, const size_t i) {
    const pn_t* const b = &_bound[i * _nl];
    const ny::uint y = static_cast <ny::uint> (std::max_element (m, m + _nl) - m);
    for (ny::uint li = 0; li < _nl; ++li)
      if (li != y && (m[y] + b[y].neg) - (m[li] + b[li].pos) <= 0) return false;
    for (ny::uint li = 0; li < _nl; ++li)
      m[li] += li == y ? b[li].neg : b[li].pos;
    return true;
  }
  template <int D, binary_t FLAG>
  inline void kernel_model::estimate_bound (const ny::fv_it& first, const ny::fv_it& beg, ny::fv_it it) {
    const ny::uint NL = FLAG ? 1 : _nl;
    ny::uint len = static_cast <ny::uint> (std::distance (beg, it));
    if (_bound.size () < len * NL) _bound.resize (len * NL);
    pn_t* p = &_bound[len * NL]; p -= NL;
    std::fill (p, p + NL, pn_t ());
    while (1) {
      --it; --len;
      if (*it >= _f_r) { // use incremental polynomial kernel to bound weights
        const pn_t* const pn = &_f2pn[*it * NL];
        const ny::uint max_dot = std::min (_maf, len); // max inner product
        for (ny::uint li = 0; li < NL; ++li) {
          p[li].pos += pn[li].pos * _polyk[max_dot] + pn[li].neg * _polyk[0];
          p[li].neg += pn[li].neg * _polyk[max_dot] + pn[li].pos * _polyk[0];
        }
      } else { // use maximum number of conjunctive features to bound weights
        for (ny::uint li = 0; li < NL; ++li) {
          ny::fl_t pos (0), neg (0);
          for (ny::uint i = 0; i < D; ++i) {
            pos += _f2dpn[(*it * NL + li) * D + i].pos * _dpolyk[len * D + i];
            neg += _f2dpn[(*it * NL + li) * D + i].neg * _dpolyk[len * D + i];
          }
          p[li].pos += std::min (pos, _f2nf[*it * NL + li].pos);
          p[li].neg += std::max (neg, _f2nf[*it * NL + li].neg);
        }
      }
      if (it == first) break;
      p -= NL, std::copy (p + NL, p + NL * 2, p);
    }
  }
}
#endif /* POLYK_CLASSIFY_H */
