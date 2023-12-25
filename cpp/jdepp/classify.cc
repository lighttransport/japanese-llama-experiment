// pecco -- please enjoy classification with conjunctive features
//  $Id: classify.cc 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#include "classify.h"
#ifdef USE_SSE4_2_POPCNT
#include <smmintrin.h>
#endif
#ifdef USE_KERNEL
#include "kernel.h"
#endif
#ifdef USE_LINEAR
#include "linear.h"
#endif

#define SHIFT_LEFT_UNSIGNED_USES_SHL (((unsigned int)0xffffffff >> 1) == 0x7fffffff)

namespace pecco {
  // (a slightly) secure conversion from string to numerical
  template <typename T> T strton (const char* s, char** error) {
    const int64_t  ret  = static_cast <int64_t>  (std::strtoll  (s, error, 10));
    const uint64_t retu = static_cast <uint64_t> (std::strtoull (s, error, 10));
    if (std::numeric_limits <T>::is_specialized &&
        (ret  < static_cast <int64_t>  (std::numeric_limits <T>::min ()) ||
         retu > static_cast <uint64_t> (std::numeric_limits <T>::max ())))
      errx (1, HERE "overflow: %s", s);
    return static_cast <T> (ret);
  }
  template <> double strton <double> (const char* s, char** error)
  { return std::strtod (s, error); }
#ifdef USE_FLOAT
  template <> float  strton <float>  (const char* s, char** error)
  { return static_cast <float> (std::strtod (s, error)); }
#endif
  template <> ny::uint strton <ny::uint> (const char* s, char** error) {
    int64_t ret = 0;
    char* p = const_cast <char*> (s);
    for (; *p >= '0' && *p <= '9'; ++p) {
      ret *= 10, ret += *p - '0';
      if (ret > std::numeric_limits <ny::uint>::max ())
        errx (1, HERE "overflow: %s", s);
    }
    if (error) *error = p;
    return static_cast <ny::uint> (ret);
  }
  template size_t    strton (const char*, char**);
  template uint8_t   strton (const char*, char**);
  template algo_t    strton (const char*, char**);
  template int       strton (const char*, char**);
  //
  template <typename T> T strton (const char* s) {
    char* err;
    const T n = strton <T> (s, &err);
    if (*err != '\0') errx (1, HERE "invalid conversion: %s", s);
    return n;
  }
  template int      strton (const char*);
  template ny::uint strton (const char*);
  template float    strton (const char*);
  template double   strton (const char*);
  // convert example to feature vector
  template <typename T>
  void ClassifierBase <T>::_convertFv2Fv (char* p, ny::fv_t& fv) const {
    // convert feature numbers (in string) into feature indices
    fv.clear ();
    while (*p) {
      const ny::uint fn = strton <ny::uint> (p, &p);
      if (fn < _fn2fi.size ())
        if (const ny::uint fi = _fn2fi[fn])
          fv.push_back (fi);
      ++p; while (*p && ! isspace (*p)) ++p; // skip value
      while (isspace (*p)) ++p; // skip (trailing) spaces
    }
  }
  template <typename T>
  void ClassifierBase <T>::_convertFv2Fv (ny::fv_t& fv) const {
    // convert feature numbers to feature indices
    ny::fv_t::iterator jt = fv.begin ();
    for (ny::fv_it it = jt; it != fv.end (); ++it)
      if (*it < _fn2fi.size ())
        if (const ny::uint fi = _fn2fi[*it])
          *jt = fi, ++jt;
    fv.erase (jt, fv.end ());
  }
  //
  template <typename T>
  void ClassifierBase <T>::_sortFv (ny::fv_t& fv) {
#ifdef SHIFT_LEFT_UNSIGNED_USES_SHL
    // for feature vectors following Zipf
    _sorter.bucket_sort (fv.begin (), fv.end (), _nfbit);
    // _sorter.radix_sort (fv.begin (), fv.end (), _nfbit);
    // _sorter.insertion_sort (fv.begin (), fv.end ());
    // std::sort (fv.begin (), fv.end ());
#else
    std::sort (fv.begin (), fv.end ());
#endif
  }
  // * a function that estimates the additional cost
  //   when eliminating nodes
  template <typename T>
  inline size_t ClassifierBase <T>::_cost_fun (size_t m, size_t n) {
    // by frequency * # of features
    switch (_d) {
      case 1: return 0;
      case 2: return ((n*n - n) - (m*m - m)) / 2;
      case 3: return ((n*n*n - n) - (m*m*m - m)) / 6;
      case 4: return ((n*n*n*n - 2*n*n*n + 11*n*n - 10*n)
                      -(m*m*m*m - 2*m*m*m + 11*m*m - 10*m)) / 24;
      default: errx (1, HERE "please add case statement."); return 0; // dummy;
    }
  }
  // assign younger feature indices to important feature numbers
  template <typename T>
  bool ClassifierBase <T>::_packingFeatures (std::vector <ny::fv_t>& fvv) {
    if (_opt.verbose > 0)
      std::fprintf (stderr, "packing feature id..");
    // reorder features according to their frequency in training data
    if (_opt.train) {
      FILE* reader =  std::fopen (_opt.train, "r");
      if (! reader) errx (1, HERE "no such file: %s", _opt.train);
      _nt         = 0;
      char*  line = 0;
      size_t read = 0;
      while (ny::getLine (reader, line, read)) {
        if (*line != '\0') {
          // assume primitive feature vectors
          char* p (line), *p_end (line + read - 1);
          while (p != p_end && ! isspace (*p)) ++p; ++p;
          while (p != p_end) {
            const ny::uint fn = strton <ny::uint> (p, &p);
            counter_t::iterator it = _fncnt.find (fn);
            if (it != _fncnt.end ()) ++it->second;
            ++p; while (p != p_end && ! isspace (*p)) ++p; // skip value
            while (isspace (*p)) ++p; // skip (trailing) spaces
          }
          ++_nt;
        }
      }
      std::fclose (reader);
    } else {
      // reorder features according to their frequency in support vectors / conjunctive features
      for (std::vector <ny::fv_t>::const_iterator it = fvv.begin ();
           it != fvv.end (); ++it) // for each unit
        for (ny::fv_it fit = it->begin (); fit != it->end (); ++fit) {
          counter_t::iterator jt = _fncnt.find (*fit); // for each feature
          if (jt != _fncnt.end ()) ++jt->second;
        }
    }
    // building feature mapping
    ny::counter <ny::uint>::type fn_counter;
    fn_counter.reserve (_fncnt.size ());
    ny::uint fn_max = 0;
    for (counter_t::const_iterator it = _fncnt.begin ();
         it != _fncnt.end (); ++it) {
      const ny::uint fn  = it->first;
      const ny::uint cnt = it->second;
      fn_max = std::max (fn, fn_max);
      fn_counter.push_back (ny::counter <ny::uint>::type::value_type (cnt, fn));
    }
    std::sort (fn_counter.begin (), fn_counter.end ());
    _fn2fi.resize (fn_max + 1, 0);
    _fi2fn.resize (_nf + 1, 0); // _nf
    ny::uint fi = 1;
    for (ny::counter <ny::uint>::type::reverse_iterator it = fn_counter.rbegin ();
         it != fn_counter.rend (); ++it) {
      const ny::uint fn = it->second;
      _fi2fn[fi] = fn;
      _fn2fi[fn] = fi;
      ++fi;
    }
    // rename features in conjunctive features / support vectors
    for (std::vector <ny::fv_t>::iterator it = fvv.begin ();
         it != fvv.end (); ++it) { // for each unit
      _convertFv2Fv (*it);
      _sortFv (*it); // for pkb; bug
    }
    if (_opt.verbose > 0)
      std::fprintf (stderr, "done.\n");
    return true;
  }
  // fstrie construction
  template <typename T>
  bool ClassifierBase <T>::_setFStrie () {
    const bool abuse = abuse_trie ();
    std::string fstrie_fn_ (std::string (_opt.event) + (abuse ? "." TRIE_SUFFIX : ".n" TRIE_SUFFIX));
    std::string fsw_fn_    (std::string (_opt.event) + ".weight");
    std::ostringstream ss;
#ifdef USE_MODEL_SUFFIX
    ss << ".-" << static_cast <uint16_t> (_opt.fst_factor);
#endif
    std::string fstrie_fn  (fstrie_fn_ + ss.str ());
    std::string fsw_fn     (fsw_fn_    + ss.str ());
    // check the update time of events
    if (_opt.verbose > 0)
      std::fprintf (stderr, "loading fstrie..");
    if (! _opt.force && newer (fstrie_fn.c_str (), _opt.event) &&
        _fstrie.open (fstrie_fn.c_str ()) == 0) {
      if (! abuse) { // load the pre-computed weights
        FILE* reader = std::fopen (fsw_fn.c_str (), "rb");
        if (! reader) errx (1, HERE "no such file: %s", fsw_fn.c_str ());
        if (std::fseek (reader, 0, SEEK_END) != 0) return -1;
        const size_t nfs = static_cast <size_t> (std::ftell (reader)) / (_nl * sizeof (ny::fl_t));
        if (std::fseek (reader, 0, SEEK_SET) != 0) return -1;
        _fsw = new ny::fl_t [_nl * nfs];
        std::fread (&_fsw[0], sizeof (ny::fl_t), _nl * nfs, reader);
        std::fclose (reader);
      }
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    } else {
      if (_opt.verbose > 0) std::fprintf (stderr, "not found.\n");
      FILE* reader = std::fopen (_opt.event, "r");
      if (! reader)
        errx (1, HERE "no such event file: %s", _opt.event);
      if (_opt.verbose > 0)
        std::fprintf (stderr, "building fstrie from %s..", _opt.event);
      unsigned long nt (0), nkey (0);
      std::set <FstKey*, FeatKeypLess> fst_key;
      FstKey q;
      std::vector <ny::uchar> s;
      std::vector <double> w (_nl, 0);
      char*  line = 0;
      size_t read = 0;
      for (ny::fv_t fv; ny::getLine (reader, line, read); fv.clear ()) {
        if (*line != '\0') {
          // skip label
          char* p (line), *p_end (line + read - 1);
          while (p != p_end && ! isspace (*p)) ++p; ++p; // skip label & space
          _convertFv2Fv (p, fv);
          _sortFv (fv);
          // precompute score
          if (KEY_SIZE * fv.size () >= s.size ())
            s.resize (KEY_SIZE * fv.size () + 1);
          ny::uint len (0), prev (0);
          byte_encoder encoder;
          for (ny::fv_t::iterator jt = fv.begin (); jt != fv.end (); ) {
            len += encoder.encode (*jt - prev, &s[len]);
            prev = *jt;
            s[len] = '\0'; q.key = &s[0]; q.len = len;
            std::set <FstKey *, FeatKeypLess>::iterator it
              = fst_key.lower_bound (&q);
            ++jt;
            if (it == fst_key.end () || FeatKeypLess () (&q, *it)) {
              ++nkey;
              std::fill (&w[0], &w[_nl], 0);
              if (is_binary_classification ())
                _baseClassify <false, BINARY> (&w[0], jt - 1, fv.begin (), jt);
              else
                _baseClassify <false, MULTI>  (&w[0], jt - 1, fv.begin (), jt);
              it = fst_key.insert (it, new FstKey (&s[0], 0, len, _nl));
              std::copy (&w[0], &w[_nl], (*it)->cont);
              const size_t j
                = static_cast <size_t> (std::distance (fv.begin (), jt));
              (*it)->weight = _cost_fun (j - 1, j);
              if (jt == fv.end ()) (*it)->leaf = true;  // leaf (tentative)
            } else {
              if (jt != fv.end ()) (*it)->leaf = false; // this is not leaf
            }
            (*it)->count += 1;
          }
        }
        if (++nt % 1000 == 0 && _opt.verbose > 0)
          std::fprintf (stderr, "\r processing %ld events => %ld feature sequences",
                        nt, nkey);
      }
      q.key = 0; q.len = 0;
      std::fclose (reader);
      if (_opt.verbose > 0)
        std::fprintf (stderr, "\r processing %ld events => %ld feature sequences\n",
                      nt, nkey);
      std::set <FstKey*, FstKeypLess> fst_leaf;
      for (std::set <FstKey*, FstKeypLess>::const_iterator it = fst_key.begin ();
           it != fst_key.end (); ++it)
        if ((*it)->leaf) fst_leaf.insert (*it);
      if (_opt.verbose > 0)
        std::fprintf (stderr, " # leaf: %ld\n", fst_leaf.size ());
      std::vector <const char *> str;
      std::vector <size_t>       len;
      std::vector <int>          val;
      str.reserve (nkey);
      len.reserve (nkey);
      val.reserve (nkey);
      if (! abuse) _fsw = new ny::fl_t [_nl * nkey];
      size_t nkey_small = nkey;
      for (size_t j = 0; nkey_small > 0; ++j) {
        // sort dictionary order of key strings
        while (fst_key.size () > nkey_small) {
          // should memory release
          FstKey* const p = * fst_leaf.begin ();
          std::set <FstKey *, FeatKeypLess>::iterator it = fst_key.find (p);
          // WARNING: a leaf node is sorted by its impact; look left/right
          // add its longest prefix if there are no sibling
          // 1 2 3     l
          // 1 2 3 4   p
          // 1 2 3 5   r
          // 1 2 3 5 6
          if (it != fst_key.begin ()) { // not a root node
            FstKey* const l = *(--it); ++it;
            if (l->is_prefix (p)) { // prefix (next leaf)
              ++it;
              if (it == fst_key.end () || ! l->is_prefix (*it)) // no sibling
                 fst_leaf.insert (l);
              --it;
            }
          }
          fst_leaf.erase (p);
          fst_key.erase (it);
          delete p;
        }
        ny::uint i = 0;
        std::set <FstKey*, FeatKeypLess>::const_iterator it = fst_key.begin ();
        typedef std::map <std::vector <double>, size_t> w2id_t;
        w2id_t w2id;
        size_t uniq = 0;
        for (; it != fst_key.end (); ++it) {
          FstKey* p = *it;
          str.push_back (reinterpret_cast <char*> (p->key));
          len.push_back (p->len);
          ny::fl_t* wv = p->cont;
          if (abuse) {
            union byte_4 b4 (static_cast <float> (*wv));
            b4.u >>= 1;
            val.push_back (b4.i);
          } else {
            w.assign (&wv[0], &wv[_nl]);
            std::pair <w2id_t::iterator, bool> itb
              = w2id.insert (w2id_t::value_type (w, uniq * _nl));
            if (itb.second) {
              for (ny::uint li = 0; li < _nl; ++li)
                _fsw[uniq * _nl + li] = wv[li]; // 0:(li=0, i=0) 1:(li=1, i=0)
              ++uniq;
            }
            val.push_back (static_cast <int> (itb.first->second)); // i*_nl
          }
          ++i;
        }
        if (j == _opt.fst_factor || _opt.fst_verbose) {
          ss.str ("");
#ifdef USE_MODEL_SUFFIX
          ss << ".-" << j;
#endif
          if (! abuse) {
            std::string fsw_fn_j  (fsw_fn_ + ss.str ());
            FILE* writer = std::fopen (fsw_fn_j.c_str (), "wb");
            std::fwrite (&_fsw[0], sizeof (ny::fl_t), uniq *_nl, writer);
            std::fclose (writer);
          }
          std::string fstrie_fn_j (fstrie_fn_ + ss.str ());
          ss.str ("");
          ss << "feature sequence trie (with 2^-" << j << " feature sequences)";
          build_trie (&_fstrie, ss.str (), fstrie_fn_j, str, len, val, _opt.verbose > 0);
          _fstrie.clear (); // if (j == _opt.fs_factor)
        }
        str.clear ();
        len.clear ();
        val.clear ();
        nkey_small >>= 1;
        if (_opt.fst_factor == j && ! _opt.fst_verbose) break;
      }
      // try reload
      if (_fstrie.open (fstrie_fn.c_str ()) != 0)
        errx (1, HERE "no such double array: %s", fstrie_fn.c_str ());
      if (! abuse) {
        delete [] _fsw;
        // load computed score
        reader = std::fopen (fsw_fn.c_str (), "rb");
        if (std::fseek (reader, 0, SEEK_END) != 0) return -1;
        const size_t nfs
          = static_cast <size_t> (std::ftell (reader)) / (_nl * sizeof (ny::fl_t));
        if (std::fseek (reader, 0, SEEK_SET) != 0) return -1;
        _fsw = new ny::fl_t [_nl * nfs];
        std::fread (&_fsw[0], sizeof (ny::fl_t), _nl * nfs, reader);
        std::fclose (reader);
      }
      for (std::set <FstKey*>::const_iterator it = fst_key.begin ();
           it != fst_key.end (); ++it)
        delete *it;
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    }
    return true;
  }
#ifdef USE_ARRAY_TRIE
  template <typename T>
  template <int D, bool PRUNE, binary_t FLAG>
  inline bool ClassifierBase <T>::_pkePseudoInnerLoop (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end, const ny::uint pos) {
    for (; it != end; ++it) {
      if (_prune <PRUNE, FLAG> (score, static_cast <size_t> (std::distance (beg, it))))
        return true;
      ny::uint pos_ = pos;
      const ny::uint j = *it - 1;
      switch (D) {
        case 4: pos_ += j * (j - 1) * (j - 2) * (j - 3) / 24;
        case 3: pos_ += j * (j - 1) * (j - 2) / 6;
        case 2: pos_ += j * (j - 1) / 2;
        case 1: pos_ += j;
      }
      PROFILE (++_traverse);
      PROFILE (for (ny::uint i (0), n (pos_ * _nl); i < _nl; ++i)
                 if (_fw[n + i]) { ++_lookup; break; });
      addScore <FLAG> (score, pos_);
      _pkePseudoInnerLoop <D - 1, false, FLAG> (score, beg, beg, it, pos_ + 1);
    }
    return false;
  }
  // explicit specializations
#ifdef USE_KERNEL
  template <> template <>
  inline bool ClassifierBase <kernel_model>::_pkePseudoInnerLoop <0, false, BINARY> (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const ny::uint) { return false; }
  template <> template <>
  inline bool ClassifierBase <kernel_model>::_pkePseudoInnerLoop <0, false, MULTI>  (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const ny::uint) { return false; }
#endif
#ifdef USE_LINEAR
  template <> template <>
  inline bool ClassifierBase <linear_model>::_pkePseudoInnerLoop <0, false, BINARY> (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const ny::uint) { return false; }
  template <> template <>
  inline bool ClassifierBase <linear_model>::_pkePseudoInnerLoop <0, false, MULTI>  (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const ny::uint) { return false; }
#endif
#endif
  
  template <typename T>
  template <int D, bool PRUNE, binary_t FLAG>
  inline bool ClassifierBase <T>::_pkeInnerLoop (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end, const size_t pos) {
    for (; it != end; ++it) {
      if (_prune <PRUNE, FLAG> (score, static_cast <size_t> (std::distance (beg, it))))
        return true;
      size_t pos_ (pos), p (0);
      byte_encoder encoder (*it);
      const int n = _ftrie.traverse (encoder.key (), pos_, p, encoder.len ());
      PROFILE (++_traverse);
      PROFILE (if (n >= 0) ++_lookup);
      if (n == -2) continue;
      if (n >=  0) addScore <FLAG> (score, n, _fw);
      _pkeInnerLoop <D - 1, false, FLAG> (score, beg, beg, it, pos_);
    }
    return false;
  }
  // explicit specializations
#ifdef USE_KERNEL
  template <> template <>
  inline bool ClassifierBase <kernel_model>::_pkeInnerLoop <0, false, BINARY> (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const size_t) { return false; }
  template <> template <>
  inline bool ClassifierBase <kernel_model>::_pkeInnerLoop <0, false, MULTI>  (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const size_t) { return false; }
#endif
#ifdef USE_LINEAR
  template <> template <>
  inline bool ClassifierBase <linear_model>::_pkeInnerLoop <0, false, BINARY> (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const size_t) { return false; }
  template <> template <>
  inline bool ClassifierBase <linear_model>::_pkeInnerLoop <0, false, MULTI>  (double*, ny::fv_it, const ny::fv_it&, const ny::fv_it&, const size_t) { return false; }
#endif
#ifdef USE_ARRAY_TRIE
  template <typename T>
  template <bool PRUNE, binary_t FLAG>
  bool ClassifierBase <T>::_pkeClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end) {
    const ny::fv_it pend = std::lower_bound (it, end, 1u << PSEUDO_TRIE_N[_d]);
    PROFILE (_flen += end - beg);
    switch (_d) {
      case 1:
        return _pkePseudoInnerLoop <1, PRUNE, FLAG> (score, it, beg, pend, 0) ||
          _pkeInnerLoop <1, PRUNE, FLAG> (score, pend, beg, end, 0);
      case 2:
        return _pkePseudoInnerLoop <2, PRUNE, FLAG> (score, it, beg, pend, 0) ||
          _pkeInnerLoop <2, PRUNE, FLAG> (score, pend, beg, end, 0);
      case 3:
        return _pkePseudoInnerLoop <3, PRUNE, FLAG> (score, it, beg, pend, 0) ||
          _pkeInnerLoop <3, PRUNE, FLAG> (score, pend, beg, end, 0);
      case 4:
        return _pkePseudoInnerLoop <4, PRUNE, FLAG> (score, it, beg, pend, 0) ||
          _pkeInnerLoop <4, PRUNE, FLAG> (score, pend, beg, end, 0);
      default: errx (1, HERE "please add case statement.");
    }
  }
#else
  template <typename T>
  template <bool PRUNE, binary_t FLAG>
  bool ClassifierBase <T>::_pkeClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end) {
    PROFILE (_flen += end - beg);
    switch (_d) {
      case 1: return _pkeInnerLoop <1, PRUNE, FLAG> (score, it, beg, end, 0);
      case 2: return _pkeInnerLoop <2, PRUNE, FLAG> (score, it, beg, end, 0);
      case 3: return _pkeInnerLoop <3, PRUNE, FLAG> (score, it, beg, end, 0);
      case 4: return _pkeInnerLoop <4, PRUNE, FLAG> (score, it, beg, end, 0);
      default: errx (1, HERE "please add case statement.");
    }
  }
#endif
  template <typename T>
  template <bool PRUNE, binary_t FLAG>
  void ClassifierBase <T>::_fstClassify (double* score, const ny::fv_it& beg, const ny::fv_it& end) {
    ny::fv_it cit = beg;
    size_t pos = 0;
    for (ny::uint prev = 0; cit != end; prev = *cit, ++cit) {
      PROFILE (++_traverse_sp);
      size_t p = 0;
      byte_encoder encoder (*cit - prev);
      const int n = _fstrie.traverse (encoder.key (), pos, p, encoder.len ());
      if (n < 0) break;
      addScore <FLAG> (score, n, _fsw);
    }
    PROFILE (_all +=
             _cost_fun (0, static_cast <size_t> (std::distance (beg, end))));
    PROFILE (_hit +=
             _cost_fun (static_cast <size_t> (std::distance (beg, cit)),
                        static_cast <size_t> (std::distance (beg, end))));
    PROFILE (++_lookup_sp);
    //
    if (cit == end) return;
    if (PRUNE) _estimate_bound <FLAG> (cit, beg, end);
    _baseClassify <PRUNE, FLAG> (score, cit, beg, end); // splitSVM + FST
    // return _pkeClassify (fv, score, cit, end); // used in EMNLP
  }
#ifdef USE_CEDAR
  template <typename T>
  template <bool PRUNE, binary_t FLAG>
  void ClassifierBase <T>::_pmtClassify (double* score, const ny::fv_it& beg, const ny::fv_it& end) {
    ny::fv_it cit = beg;
    size_t pos = 0;
    if (PRUNE) _estimate_bound <FLAG> (cit, beg, end);
    for (ny::uint prev = 0; cit != end; prev = *cit, ++cit) {
      if (_prune <PRUNE, FLAG> (score, static_cast <size_t> (std::distance (beg, cit))))
        break;
      size_t p = 0;
      byte_encoder encoder (*cit - prev);
      int& n = _pmtrie.update (encoder.key (), pos, p, encoder.len (), 0, _pmfunc);
      double* pscore = 0;
#ifdef USE_LFU_PMT
      int m = 0;
      if (! n) { // not monitored
        int n_ = 0;
        if (_pmi == (1 << _opt.pmsize)) {
          m  = _bucket.next (m);
          n_ = _bucket.link (m);  // the least significant item
          int& l = _pmfunc.leaf[n_];
          if (l > 0) _pmtrie.erase (l); // old item
          l = static_cast <int> (pos);
        } else {
          n_ = _pmi; ++_pmi;
          _pmfunc.leaf[n_] = static_cast <int> (pos);
        }
        n = n_ + 1;
        pscore = &_pms[n_ * _nl];
        resetScore <FLAG> (pscore);
        _baseClassify <false, FLAG> (pscore, cit, beg, cit + 1);
      } else {  // already monitored
        pscore = &_pms[(n - 1) * _nl];
        m = _item.link (n - 1);
      }
      const size_t new_count = _bucket.count (m) + 1;
      if (_bucket.count (_bucket.next (m)) == new_count) { // push to the sibling
        if (m != 0) {
          if (_item.is_singleton (n - 1)) { // pop singleton
            _bucket.pop (m);
            int m_ = _bucket.prev (m);
            std::swap (m, m_);
            _bucket_pool.recycle_bucket (_bucket, m_);
          } else {
            _bucket.pop_item (_item, m, n - 1);
          }
        }
        _bucket.push_item (_item, _bucket.next (m), n - 1); // move bucket to bucket->next
      } else if (_item.is_singleton (n - 1)) { // reuse singleton
        _bucket.count (m) = new_count;
      } else { // prepare a new bucket
        if (m != 0) _bucket.pop_item (_item, m, n - 1);
        const int m_ = _bucket_pool.create_bucket (_bucket);
        _bucket.count (m_) = new_count;
        _bucket.link  (m_) = n - 1;
        _bucket.set_child (_item, m_, n - 1);
        _bucket.push (m_, m);
      }
      addScore <FLAG> (score, 0, pscore);
#else
      if (! n) { // not monitored
        int n_ = _ring.get_element (); // least recently used
        int& l = _pmfunc.leaf[n_];
        if (l > 0) _pmtrie.erase (l); // old item
        n = n_ + 1;
        l = static_cast <int> (pos);
        pscore = &_pms[n_ * _nl];
        resetScore <FLAG> (pscore);
        _baseClassify <false, FLAG> (pscore, cit, beg, cit + 1);
      } else { // already monitored
        _ring.move_to_back (n - 1); // recently used
        pscore = &_pms[(n - 1) * _nl];
      }
      addScore <FLAG> (score, 0, pscore);
#endif
    }
  }
#endif
  template <typename T>
  template <bool PRUNE, binary_t FLAG>
  void ClassifierBase <T>::classify (char* p, double* score) {
    char* q = p;
    _fv.clear ();
    while (*q) {
      const ny::uint fi = strton <ny::uint> (q, &q);
      if (*q != ':') errx (1, HERE "illegal feature index: %s", p);
      _fv.push_back (fi);
      ++q; while (*q && ! isspace (*q)) ++q;
      while (isspace (*q)) ++q; // skip (trailing) spaces
    };
    classify <PRUNE, FLAG> (_fv, &score[0]);
  }
  template <typename T>
  void ClassifierBase <T>::batch () { // batch classification
    if (_opt.verbose > 0) std::fprintf (stderr, "processing examples..");
    const bool     output_example = _opt.output & 0x100;
    const output_t output         = static_cast <output_t> (_opt.output & 0xff);
    FILE* reader = _opt.test ? std::fopen (_opt.test, "r") : stdin;
    if (! reader) errx (1, HERE "no such file: %s", _opt.test);
    if (reader == stdin)
      std::fprintf (stderr, "(input: STDIN)\n");
    char*  line = 0;
    size_t read = 0;
    // double* score = new double[_nl] (); // will be initialized later
    // workaround for a bug in value initialization in clang <= 3.0 (svn r150682)
    double* score = new double[_nl];
    ny::uint pp (0), np (0), pn (0), nn (0);
    while (ny::getLine (reader, line, read)) {
      if (*line != '\0') {
        char* p (line), *p_end (line + read - 1), *label (p);
        while (p != p_end && ! isspace (*p)) ++p; *p = '\0'; ++p;
        char* ex = p;
#ifdef USE_PRUNING
        if (is_binary_classification ()) {
          if (output == SCORE || output == PROB)
            classify <false, BINARY> (p, &score[0]);
          else
            classify <true,  BINARY> (p, &score[0]);
        } else {
          if (output == SCORE || output == PROB)
            classify <false, MULTI>  (p, &score[0]);
          else
            classify <true,  MULTI>  (p, &score[0]);
        }
#else
        if (is_binary_classification ())
          classify <false, BINARY> (p, &score[0]);
        else
          classify <false, MULTI>  (p, &score[0]);
#endif
        const ny::uint li = getLabel (score);
        if (std::strcmp (label, _li2l[li]) == 0)
          if (li == _tli) ++pp; else ++nn;
        else
          if (li == _tli) ++np; else ++pn;
        switch (output) {
          case NONE:
//            std::fprintf (stdout, "%s %Lg\n", line, _elapsed  / (ny::Timer::clock () * 1000)); break;
             break;
          case LABEL:
            printLabel (li);
            if (output_example) std::fprintf (stdout, "\t%s", ex);
            std::fprintf (stdout, "\n");
            break;
          case SCORE: printScore (li, score);
            if (output_example) std::fprintf (stdout, "\t%s", ex);
            std::fprintf (stdout, "\n");
            break;
          case PROB:  printProb  (li, score);
            if (output_example) std::fprintf (stdout, "\t%s", ex);
            std::fprintf (stdout, "\n");
            break;
        }
      }
    }
    delete [] score;
    if (reader != stdin) std::fclose (reader);
    if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    std::fprintf (stderr, "acc. %.4f (pp %u) (pn %u) (np %u) (nn %u)\n",
                  (pp + nn) * 1.0 / (pp + pn + np + nn), pp, pn, np, nn);
    printStat ();
  }
  template <typename T>
  void ClassifierBase <T>::printStat () { // print classifier statistics
#ifdef USE_TIMER
      _timer_pool.print ();
#endif
#ifdef USE_PROFILING
      std::fprintf (stderr, "# active primitive features: %ld\n", _flen);
      if (_opt.algo == PKE || _opt.algo == FST) {
        std::fprintf (stderr, "weight lookup (succeeded): %ld\n",  _lookup);
        std::fprintf (stderr, "weight lookup: %ld\n", _traverse);
        if (_opt.algo == FST) {
          std::fprintf (stderr, "weight lookup (split): %ld\n", _lookup_sp);
          std::fprintf (stderr, "weight traverse (split): %ld\n", _traverse_sp);
          std::fprintf (stderr, "node reduction: %f (%ld / %ld)\n",
                        static_cast <ny::fl_t> (_hit) / static_cast <ny::fl_t> (_all), _hit, _all);
        }
      }
#endif
  }
  // explicit specialization
#ifdef USE_KERNEL
  template class ClassifierBase <kernel_model>;
  template void ClassifierBase <kernel_model>::_pkeClassify <true, BINARY> (ny::fv_t&, double*);
  template void ClassifierBase <kernel_model>::_pkeClassify <true, MULTI>  (ny::fv_t&, double*);
  template void ClassifierBase <kernel_model>::_fstClassify <true, BINARY> (double*, const ny::fv_it&, const ny::fv_it&);
  template void ClassifierBase <kernel_model>::_fstClassify <true, MULTI>  (double*, const ny::fv_it&, const ny::fv_it&);
#endif
#ifdef USE_LINEAR
  template class ClassifierBase <linear_model>;
  template void ClassifierBase <linear_model>::_fstClassify <true, BINARY> (double*, const ny::fv_it&, const ny::fv_it&);
  template void ClassifierBase <linear_model>::_fstClassify <true, MULTI>  (double*, const ny::fv_it&, const ny::fv_it&);
#endif
}
// use pseudo array
