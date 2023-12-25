// pecco -- please enjoy classification with conjunctive features
//  $Id: linear.h 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef LLM_CLASSIFY_H
#define LLM_CLASSIFY_H

#include <sys/stat.h>
#include <err.h>
#include <cmath>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include "typedef.h"
#include "timer.h"
#include "classify.h"

// switch off
#undef ABUSE_TRIE

namespace pecco {
  class linear_model : public ClassifierBase <linear_model> {
  private:
    // temporary variables
    std::vector <ny::fv_t>                _cfv;
    std::vector <std::vector <ny::fl_t> > _fw_tmp;
    // internal functions
    void _convertCfstr2Cf (char* p, ny::fv_t& cf);
    bool _setFtrie ();
    // classification
    double _calcProb (const double* const score, ny::uint target_id) const {
      double sum (0), prob_pos (0);
      for (size_t li = 0; li < _nl; ++li) {
        double prob = std::exp (score[li]);
        sum += prob;
        if (li == target_id) prob_pos = prob;
      }
      return (prob_pos / sum);
    }
public:
    linear_model (const pecco::option& opt) : ClassifierBase <linear_model> (opt), _cfv (), _fw_tmp () {}
    ~linear_model () {
      for (size_t i = 0; i < _nl; ++i)  delete [] _li2l[i];
#ifdef USE_PRUNING
      if (_d > 1) {
        delete [] _f2dpn;
        delete [] _f2nf;
      }
#endif
      delete [] _fw;
      if (_opt.algo == FST) delete [] _fsw;
#ifdef USE_CEDAR
      if (_opt.algo == PMT) delete [] _pms;
#endif
    }
    template <binary_t FLAG>
    void resetScore (double* score) const;
    template <binary_t FLAG>
    void resetScore (double* score, const double* const score_) const;
    template <binary_t FLAG>
    void addScore (double* score, const ny::uint pos) const;
    template <binary_t FLAG>
    void addScore (double* score, const int n, const ny::fl_t* const w) const;
    template <binary_t FLAG>
    bool prune (double* m, const size_t i);
    template <int D, binary_t FLAG>
    void estimate_bound (const ny::fv_it& first, const ny::fv_it& beg, ny::fv_it it);
    template <bool PRUNE, binary_t FLAG>
    void baseClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end)
    { _pkeClassify <PRUNE, FLAG> (score, it, beg, end); }
    bool load  (const char* model); // set up model
    void printParam () {
      std::fprintf (stderr, "maximum order of feature combination: %u\n", _d);
      std::fprintf (stderr, "# of active features: %u (%u)\n", _nf, _ncf);
    }
    // classification interface
    template <bool PRUNE, binary_t FLAG>
    void classify (ny::fv_t& fv, double* score) {
      if (FLAG) score[0] = score[1] = 0; else std::fill (score, score + _nl, 0);
      TIMER (if (_opt.verbose > 0) _enc_t->startTimer ());
      _convertFv2Fv (fv);
      TIMER (if (_opt.verbose > 0) _enc_t->stopTimer ());
      TIMER (_classify_t->startTimer ());
//      const uint64_t start = ny::Timer::rdtsc ();
      if (fv.empty ()) {
//        _elapsed = ny::Timer::rdtsc () - start;
        TIMER (_classify_t->stopTimer ()); return; }
      switch (_opt.algo) {
        case PKE: _pkeClassify <PRUNE, FLAG> (fv, score); break;
        case FST: _fstClassify <PRUNE, FLAG> (fv, score); break;
#ifdef USE_CEDAR
        case PMT: _pmtClassify <PRUNE, FLAG> (fv, score); break;
#endif
        case PKI: errx (1, HERE "PKI [-t 0] is not supported.");
      }
//      _elapsed = ny::Timer::rdtsc () - start;
      TIMER (_classify_t->stopTimer ());
    }
    bool is_binary_classification () const { return _nl == 2; }
    bool abuse_trie               () const { return false; }
    double threshold () { return 0.5; }
    bool binClassify (ny::fv_t& fv, char* target = 0) {
#ifdef USE_PRUNING
      if (is_binary_classification ())
        classify <true, BINARY> (fv, &_score[0]);
      else
        classify <true, MULTI>  (fv, &_score[0]);
#else
      if (is_binary_classification ())
        classify <false, BINARY> (fv, &_score[0]);
      else
        classify <false, MULTI>  (fv, &_score[0]);
#endif
      const ny::uint li
        = static_cast <ny::uint> (std::max_element (&_score[0], &_score[_nl]) - &_score[0]);
      if (target)
        return std::strcmp (_li2l[li], target) == 0;
      else
        return li == _tli;
    }
    double getProbability (ny::fv_t& fv, char* target = 0) {
      ny::uint target_id = _tli;
      if (target) {
        lmap::const_iterator it = _l2li.find (target);
        if (it == _l2li.end ())
          errx (1, HERE "unknown label: %s", target);
        target_id = it->second;
      }
      if (is_binary_classification ())
        classify <false, BINARY> (fv, &_score[0]);
      else
        classify <false, MULTI>  (fv, &_score[0]);
      return _calcProb (&_score[0], target_id);
    }
    ny::uint getLabel (const double* score) const {
      return static_cast <ny::uint> (std::max_element (&score[0], &score[_nl]) - &score[0]);
    }
    void printScore (const ny::uint li, const double* score) const
    { std::fprintf (stdout, "%s %f", _li2l[li], score[li]); }
    void printProb (const ny::uint li, const double* score) const
    { std::fprintf (stdout, "%s %f", _li2l[li], _calcProb (score, li)); }
  };
  //
  template <binary_t FLAG>
  inline void linear_model::resetScore (double* score) const {
    const ny::uint NL = FLAG  ? 2 : _nl;
    for (ny::uint li = 0; li < NL; ++li) score[li] = 0;
  }
  template <binary_t FLAG>
  inline void linear_model::resetScore (double* score, const double* const score_) const {
    const ny::uint NL = FLAG  ? 2 : _nl;
    for (ny::uint li = 0; li < NL; ++li) score[li] = score_[li];
  }
  template <binary_t FLAG>
  inline void linear_model::addScore (double* score, const ny::uint pos) const {
    const ny::uint NL = FLAG  ? 2 : _nl;
    const ny::fl_t* const fw = &_fw[pos * NL];
    for (ny::uint li = 0; li < NL; ++li) score[li] += fw[li];
  }
  template <binary_t FLAG>
  inline void linear_model::addScore (double* score, const int n, const ny::fl_t* const w) const {
    const ny::uint NL = FLAG  ? 2 : _nl;
    const ny::fl_t* const fw = &w[n];
    for (ny::uint li = 0; li < NL; ++li) score[li] += fw[li];
  }
  //
  template <>
  inline bool linear_model::prune <BINARY> (double* m, const size_t i) {
    const pn_t& b0 (_bound[i * 2]), &b1 (_bound[i * 2 + 1]);
    const ny::uint y = m[0] >= m[1] ? 0 : 1; // current label
    if (y == 0) {
      if ((m[0] + b0.neg) - (m[1] + b1.pos) <= 0) return false;
      m[0] += b0.neg; m[1] += b1.pos;
    } else {
      if ((m[1] + b1.neg) - (m[0] + b0.pos) <= 0) return false;
      m[0] += b0.pos; m[1] += b1.neg;
    }
    return true;
  }
  template <>
  inline bool linear_model::prune <MULTI> (double* m, const size_t i) {
    const pn_t* const b = &_bound[i * _nl];
    const ny::uint y = static_cast <ny::uint> (std::max_element (m, m + _nl) - m);
    for (ny::uint li = 0; li < _nl; ++li)
      if (li != y && (m[y] + b[y].neg) - (m[li] + b[li].pos) <= 0)
        return false;
    for (ny::uint li = 0; li < _nl; ++li)
      m[li] += li == y ? b[li].neg : b[li].pos;
    return true;
  }
  template <int D, binary_t FLAG>
  inline void linear_model::estimate_bound (const ny::fv_it& first, const ny::fv_it& beg, ny::fv_it it) {
    const ny::uint NL = FLAG  ? 2 : _nl;
    ny::uint len = static_cast <ny::uint> (std::distance (beg, it));
    if (_bound.size () < len * NL) _bound.resize (len * NL);
    pn_t* p = &_bound[len * NL]; p -= NL;
    std::fill (p, p + NL, pn_t ());
    while (1) {
      --it; --len;
      // use maximum number of conjunctive features to bound weights
      for (ny::uint li = 0; li < NL; ++li) {
        ny::fl_t pos (0), neg (0);
        for (ny::uint i = 0; i < D; ++i) {
          pos += _f2dpn[(*it * NL + li) * D + i].pos * _dpolyk[len * D + i];
          neg += _f2dpn[(*it * NL + li) * D + i].neg * _dpolyk[len * D + i];
        }
        p[li].pos += std::min (pos, _f2nf[*it * NL + li].pos);
        p[li].neg += std::max (neg, _f2nf[*it * NL + li].neg);
      }
      if (it == first) break;
      p -= NL, std::copy (p + NL, p + NL * 2, p);
    }
  }
}

#endif /* LLM_CLASSIFY_H */
