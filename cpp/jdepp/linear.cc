// pecco -- please enjoy classification with conjunctive features
//  $Id: linear.cc 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#include "linear.h"

namespace pecco {
  void linear_model::_convertCfstr2Cf (char* p, ny::fv_t& cf) {
    // convert conjunctive feature string to set of primitive features
    cf.clear ();
    while (1) {
      const ny::uint fn = strton <ny::uint> (p, &p);
      if (_fncnt.find (fn) == _fncnt.end ())
        _fncnt.insert (counter_t::value_type (fn, 0));
      cf.push_back (fn);
      if (*p == ':') ++p; else break;
    }
  }
  bool linear_model::_setFtrie () {
    const std::string ftrie_fn (std::string (_opt.model) + ".ne" TRIE_SUFFIX);
    const std::string fw_fn    (std::string (_opt.model) + ".weight");
    const std::string fs_fn    (std::string (_opt.model) + ".fstat");
#ifdef USE_PRUNING
    if (_d > 1) {
      _f2dpn = new pn_t[(_nf + 1) * _nl * _d] ();
      _f2nf  = new pn_t[(_nf + 1) * _nl] ();
    }
#endif
    if (_opt.verbose > 0)
      std::fprintf (stderr, "loading feature weight trie..");
    if (! _opt.force && newer (fw_fn.c_str (), _opt.model)) {
      // read feature weight trie
      _ftrie.open (ftrie_fn.c_str ()); // may fail
      FILE* reader = std::fopen (fs_fn.c_str (), "rb");
      if (! reader) errx (1, HERE "no such file: %s", fs_fn.c_str ());
      std::fread (&_ncf, sizeof (ny::uint), 1, reader);
#ifdef USE_PRUNING
      if (_d > 1) {
        std::fread (&_f2nf[0],  sizeof (pn_t), (_nf + 1) * _nl,      reader);
        std::fread (&_f2dpn[0], sizeof (pn_t), (_nf + 1) * _nl * _d, reader);
      }
#endif
      std::fclose (reader);
      reader = std::fopen (fw_fn.c_str (), "rb");
      if (! reader) errx (1, HERE "no such file: %s", fw_fn.c_str ());
      if (std::fseek (reader, 0, SEEK_END) != 0) return -1;
      const size_t uniq
        = static_cast <size_t> (std::ftell (reader)) / (_nl * sizeof (ny::fl_t));
      if (std::fseek (reader, 0, SEEK_SET) != 0) return -1;
      _fw = new ny::fl_t [_nl * uniq]; // one-dimentional array is faster
      std::fread (&_fw[0], sizeof (ny::fl_t), uniq * _nl, reader);
      std::fclose (reader);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    } else {
      if (_opt.verbose > 0) std::fprintf (stderr, "not found.\n");
      std::vector <FeatKey *> pke_key;
      _ncf = static_cast <ny::uint> (_cfv.size ());
      size_t uniq = 0;
      const size_t N = std::min ((1U << PSEUDO_TRIE_N[_d]) - 1, _nf);
      switch (_d) {
        case 4: uniq += N * (N - 1) * (N - 2) * (N - 3) / 24;
        case 3: uniq += N * (N - 1) * (N - 2) / 6;
        case 2: uniq += N * (N - 1) / 2;
        case 1: uniq += N;
      }
      std::vector <ny::fl_t> fw ((uniq + _ncf) * _nl, 0);
      ny::uchar s[KEY_SIZE * MAX_KERNEL_DEGREE];
      // should remember feature combination including rare feature
      for (ny::uint cfi = 0; cfi < _ncf; ++cfi) {
        ny::fv_t& fc = _cfv[cfi];
        std::sort (fc.rbegin (), fc.rend ()); // don't comment out
        if (fc.front () >> PSEUDO_TRIE_N[_d]) {
          ny::uint len = 0;
          byte_encoder encoder;
          for (ny::fv_it it = fc.begin (); it != fc.end (); ++it)
            len += encoder.encode (*it, s + len);
          pke_key.push_back (new FeatKey (s, &_fw_tmp[cfi][0], len, _nl));
        } else {
          ny::uint pos = 0;
          for (ny::uint i = 0; i < fc.size (); ++i) {
            const ny::uint j = fc[i] - 1;
            switch (_d - i) {
              case 3: pos += j * (j - 1) * (j - 2) / 6;
              case 2: pos += j * (j - 1) / 2;
              case 1: pos += j;
            }
            if (i > 0) ++pos;
          }
          for (ny::uint li = 0; li < _nl; ++li)
            fw[pos * _nl + li] = _fw_tmp[cfi][li];
        }
        // for pruning
#ifdef USE_PRUNING
        if (_d > 1)
          for (ny::fv_it it = fc.begin (); it != fc.end (); ++it) {
            const ny::uint fi = *it;
            for (ny::uint li = 0; li < _nl; ++li)
              if (_fw_tmp[cfi][li] > 0)
                _f2nf[fi * _nl + li].pos += _fw_tmp[cfi][li];
              else
                _f2nf[fi * _nl + li].neg += _fw_tmp[cfi][li];
            for (ny::uint li = 0; li < _nl; ++li) {
              const size_t idx = fi * _nl * _d + li * _d + fc.size () - 1;
              if (_fw_tmp[cfi][li] > 0)
                _f2dpn[idx].pos = std::max (_f2dpn[idx].pos, _fw_tmp[cfi][li]);
              else
                _f2dpn[idx].neg = std::min (_f2dpn[idx].neg, _fw_tmp[cfi][li]);
            }
          }
#endif
      }
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
          // uniq identical weights
          // (minimize the double array with a slight cahce-miss increase)
          w.assign (&wv[0], &wv[_nl]);
          std::pair <w2id_t::iterator, bool> itb
            = w2id.insert (w2id_t::value_type (w, uniq * _nl));
          if (itb.second) {
            for (ny::uint li = 0; li < _nl; ++li)
              fw[uniq * _nl + li] = wv[li]; // put weights for all labels
            ++uniq;
          }
          val.push_back (static_cast <int> (itb.first->second));
        }
        build_trie (&_ftrie, "feature weight trie", ftrie_fn, str, len, val,
                    _opt.verbose > 0);
        for (dit = pke_key.begin (); dit != pke_key.end (); ++dit) delete *dit;
      }
      FILE* writer = std::fopen (fw_fn.c_str (), "wb");
      std::fwrite (&fw[0], sizeof (ny::fl_t), uniq * _nl, writer);
      std::fclose (writer);
      _fw = new ny::fl_t [uniq * _nl];
      std::copy (&fw[0], &fw[uniq * _nl], _fw);
      //
      writer = std::fopen (fs_fn.c_str (), "wb");
      std::fwrite (&_ncf, sizeof (ny::uint), 1, writer);
#ifdef USE_PRUNING
      if (_d > 1) {
        std::fwrite (&_f2nf[0],  sizeof (pn_t), (_nf + 1) * _nl, writer);
        std::fwrite (&_f2dpn[0], sizeof (pn_t), (_nf + 1) * _nl * _d, writer);
      }
#endif
      std::fclose (writer);
    }
    return true;
  }
  bool linear_model::load (const char* model) {
    TIMER (_model_t->startTimer ());
    const std::string model_bin (std::string (model) + ".bin"); // compiled model
    if (! _opt.force && newer (model_bin.c_str (), model)) {
      if (_opt.verbose > 0)
        std::fprintf (stderr, "loading compiled model parameters..");
      FILE* reader = std::fopen (model_bin.c_str (), "rb");
      std::fread (&_d,     sizeof (ny::uint), 1, reader);
      std::fread (&_nl,    sizeof (ny::uint), 1, reader);
      std::fread (&_nf,    sizeof (ny::uint), 1, reader);
      std::fread (&_nfbit, sizeof (ny::uint), 1, reader);
      // label map
      for (ny::uint li = 0; li < _nl; ++li) {
        ny::uint len = 0;
        std::fread (&len, sizeof (ny::uint), 1, reader);
        char* p = new char[len + 1];
        std::fread (p, sizeof (char), len, reader);
        p[len] = '\0';
        _li2l.push_back (p);
        _l2li.insert (lmap::value_type (p, li));
        if (std::strcmp (p, "+1") == 0) _tli = li;
      }
      // feature map
      _fi2fn.reserve (_nf + 1);
      _fi2fn.push_back (0);
      for (ny::uint fn (0); std::fread (&fn, sizeof (ny::uint), 1, reader);) {
        if (fn >= _fn2fi.size ()) _fn2fi.resize (fn + 1, 0);
        _fn2fi[fn] = static_cast <ny::uint> (_fi2fn.size ());
        _fi2fn.push_back (fn);
      }
      std::fclose (reader);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    } else {
      FILE *reader = std::fopen (model, "r");
      if (_opt.verbose > 0)
        std::fprintf (stderr, "loading/compiling model parameters..");
      if (! reader)
        errx (1, HERE "no such file: %s", model);
      std::map <ny::fv_t, ny::uint> fc2fci; // conjunctive features
      ny::fv_t cf;
      char*  line = 0;
      size_t read = 0;
      while (ny::getLine (reader, line, read)) {
        if (*line != '\0') {
          char* p (line), * const p_end (p + read - 1);
          while (p != p_end && *p != '\t') ++p; *p = '\0';
          lmap::iterator it = _l2li.find (line);
          if (it == _l2li.end ()) { // identify label
            char* label = new char[p - line + 1];
            std::strcpy (label, line);
            if (std::strcmp (label, "+1") == 0) _tli = _nl;
            it = _l2li.insert (std::make_pair (label, _nl++)).first;
            _li2l.push_back (label);
          }
          // read conjunctive feature
          const ny::uint li = it->second; // bug if all alpha null
          char* q = ++p; while (p != p_end && *p != '\t') ++p; ++p;
          const ny::fl_t w = strton <ny::fl_t> (p);
          if (std::fpclassify (w) == FP_ZERO) continue;
          _convertCfstr2Cf (q, cf);
          _sortFv (cf);
          _d = std::max (_d, static_cast <ny::uint> (cf.size ())); // degree
          std::map <ny::fv_t, ny::uint>::iterator jt = fc2fci.lower_bound (cf);
          if (jt == fc2fci.end () || jt->first > cf) {
            jt = fc2fci.insert (jt, std::make_pair (cf, _ncf));
            _fw_tmp.push_back (std::vector <ny::fl_t> ());
            _cfv.push_back (cf);
            ++_ncf;
          }
          const ny::uint uid = jt->second;
          _fw_tmp[uid].resize (_nl, 0);
          _fw_tmp[uid][li] = w;
        }
      }
      std::fclose (reader);
      for (ny::uint i = 0; i < _ncf; ++i)
        _fw_tmp[i].resize (_nl, 0);
      if (_opt.verbose > 0) std::fprintf (stderr, "done.\n");
    
      // construct dense feature;
      _nf = static_cast <ny::uint> (_fncnt.size ());
      // set _nfbit for radix sort
      while (((_nf >> _nfbit) | (NBIN - 1)) != NBIN - 1) _nfbit += NBIT;
      _packingFeatures (_cfv); // need afn
      // output map
      FILE* writer = std::fopen (model_bin.c_str (), "wb");
      std::fwrite (&_d,     sizeof (ny::uint), 1, writer);
      std::fwrite (&_nl,    sizeof (ny::uint), 1, writer);
      std::fwrite (&_nf,    sizeof (ny::uint), 1, writer);
      std::fwrite (&_nfbit, sizeof (ny::uint), 1, writer);
      for (ny::uint li = 0; li < _nl; ++li) {
        const char* p = _li2l[li];
        ny::uint len = static_cast <ny::uint> (std::strlen (p));
        std::fwrite (&len, sizeof (ny::uint), 1,    writer);
        std::fwrite (p,    sizeof (char),     len, writer);
      }
      for (ny::uint fi = 1; fi <= _nf; ++fi)
        std::fwrite (&_fi2fn[fi], sizeof (ny::uint), 1, writer);
      std::fclose (writer);
      _fncnt.clear ();
    }
#ifdef USE_CEDAR
    if (_d == 1 && (_opt.algo == FST || _opt.algo == PMT))
        warnx ("NOTE: [-t 2 or 3] is useless in d = 1.");
#else
    if (_d == 1 && _opt.algo == FST)
        warnx ("NOTE: [-t 2] is useless in d = 1.");
#endif
    _score.resize (_nl);
    _setFtrie ();
    // fstrie construction
    if (_opt.algo == FST) _setFStrie ();
#ifdef USE_CEDAR
    if (_opt.algo == PMT)
      _pms = new double[(1 << _opt.pmsize) * _nl];
#endif
    TIMER (_model_t->stopTimer ());
    if (_opt.verbose > 0) printParam ();
    return true;
  }
}
