// J.DepP -- Japanese Dependency Parsers
//  $Id: pdep.cc 1944 2022-03-17 17:50:39Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#include "pdep.h"

namespace pdep {

  static const char* input0[] = { "raw", "chunk", "depnd" };
  
  bool chunk_t::setup (const dict_t* dict, const int next) {
    particle_feature_bits.resize (dict->particle_feature_bit_len (), 0);
    if ((_token_num = next - _mzero) == 0) return false;
    for (const token_t* m = mlast (); m >= mzero (); --m) {
      if (m->pos1 () == dict->special) {
        if      (m->pos2 () == dict->comma)  comma  |= true;
        else if (m->pos2 () == dict->period) period |= true;
        else if (m->pos2 () == dict->bracket_in)  ++bracket_in;
        else if (m->pos2 () == dict->bracket_out) ++bracket_out;
      } else {
        if (_mtail == -1) _mtail = static_cast <int> (m - mzero ());
        if (m->pos1 () == dict->particle) {
          // adding suffix may capture agreement
          if (dict->is_particle_feature (static_cast <ny::uint> (m->surf ())))
            _set_particle_feature_bits (static_cast <ny::uint> (m->surf ()));
        } else
          if (_mhead == -1) _mhead = static_cast <int> (m - mzero ());
      }
    }
    if (head_id < 0 || head_id > id) return true;
#ifndef NDEBUG
    warnx ("\tbroken dependency?: %u->%d", id, did);
#endif
    return false;
  }
  inline void parser::_add_particle_feature (const chunk_t* const bi, const chunk_t* const bj = 0) {
    // assuming _particle_feature_bits to be cleared
    if (bj)
      for (const chunk_t* bk = bi; bk != bj; ++bk) { // merge feature bits;
        while (bk->head_id != -1 && bk->head_id != bj->id)
          bk = _s->chunk (bk->head_id); // only competitors
        if (bk >= bj) break;
        for (ny::uint b = 0; b < _particle_feature_bits.size (); ++b)
          _particle_feature_bits[b] |= bk->particle_feature_bits[b];
      }
    else
      for (ny::uint b = 0; b < _particle_feature_bits.size (); ++b)
        _particle_feature_bits[b] |= bi->particle_feature_bits[b];
    // set context features by twiddling bits
    //   cf. http://www-cs-faculty.stanford.edu/~uno/fasc1a.ps.gz (p. 8)
    for (flag_t::iterator it = _particle_feature_bits.begin ();
         it != _particle_feature_bits.end (); _fi += FLAG_LEN, ++it)
      while (*it) {
        // _fv.push_back (_fi + __builtin_ctzl (*it)); // faster
        pecco::byte_4 b (static_cast <float> (*it & - *it)); // pull rightmost 1
        _fv.push_back (_fi + (b.u >> 23) - 127); // pick it via floating number
        *it &= (*it - 1); // unset rightmost 1
      }
  }
  inline void parser::_add_local_feature (const chunk_t* const b, const int h) {
    // mod: bos  / parenthesis / punctuation
    _add_boolean_feature (b->id == h);
    _add_boolean_feature (b->bracket_in > b->bracket_out,
                          b->bracket_in < b->bracket_out,
                          b->bracket_in == 0);
    _add_boolean_feature (b->period && b->comma, b->period, b->comma);
    // (optional) particles
    _add_particle_feature (b);
  }
  inline void parser::_add_global_feature (const chunk_t* const bi, const chunk_t* const bj) {
    // activate global features
    ny::uint bracket_in (0), bracket_out (0);
    for (const chunk_t* bk = bi + 1; bk < bj; ++bk)
      bracket_in  += bk->bracket_in,
      bracket_out += bk->bracket_out;
    // distance / parenthesis / particle
    _add_boolean_feature (bj->id - bi->id == 1, bj->id - bi->id < 6);
    _add_boolean_feature (bracket_in > bracket_out,
                          bracket_in < bracket_out,
                          bracket_in == 0);
    _add_particle_feature (bi + 1, bj);
#ifdef USE_STACKING
    _add_boolean_feature (bi->head_id_cand == -1);
    _add_boolean_feature (bi->head_id_cand == bj.id);
#endif
  }
  inline void parser::_add_string_feature (const int id) {
    if (id >= 0) _fv.push_back (_fi + static_cast <ny::uint> (id));
    _fi += _dict->num_lexical_features ();
  }
  //
  // void parser::_add_cluster_feature (const token_t* const m) {
  //   for (ny::uint i = 0; i < _opt.clen; ++i)
  //     _add_string_feature ((1 << i) & _opt.cbits ? m->cluster (i) : -1);
  // }
  // set features in a particlular token
  inline void parser::_add_token_feature (const token_t* const m) {
    _add_string_feature (m->surf ());
    _add_string_feature (m->pos1 ());
    _add_string_feature (m->pos2 ());
    _add_string_feature (m->infl ());
  }
  inline void parser::_add_boolean_feature (const bool flag)
  { _fv.push_back (flag ? _fi : _fi + 1); _fi += 2; }
  inline void parser::_add_boolean_feature (const bool flag, const bool flag_)
  { _fv.push_back (flag ? _fi : (flag_ ? _fi + 1 : _fi + 2)); _fi += 3; }
  inline void parser::_add_boolean_feature (const bool flag, const bool flag_, const bool flag__)
  { _fv.push_back (flag ? _fi : (flag_ ? _fi + 1 : (flag__ ? _fi + 2 : _fi + 3))); _fi += 4; }
  // workaround feature to detect local coordination
  void parser::_add_coord_feature (const chunk_t* const bi, const chunk_t* const bj) {
    const token_t* const mi (bi->mhead ()), * const mj (bj->mhead ());
    if (mi != _s->token0 && mj != _s->token0 && mi->surf () == mj->surf () &&
        mi->length_ () == mj->length_ () &&
        std::memcmp (mi->surface_ (), mj->surface_ (), mi->length_ ()) == 0)
      _fv.push_back (_fi);
    _fi += 1;
  }
  void parser::_event_gen_from_tuple (const int i) { // for chunking
    _fi = 1;
    _fv.clear ();
    _add_string_feature (_s->token (i - 2)->surf ());
    _add_string_feature (_s->token (i - 2)->pos2 ());
    _add_string_feature (_s->token (i - 2)->infl ());
    _add_token_feature  (_s->token (i - 1));
    _add_token_feature  (_s->token (i));
    _add_string_feature (_s->token (i + 1)->surf ());
    _add_string_feature (_s->token (i + 2)->surf ());
    // add new features (>= _fi) here
    // put this tail for assertion
    static ny::uint fmax = 0;
    if (fmax && _fi != fmax)
      errx (1, HERE "feature offset broken; revert the change in features.");
    fmax = _fi;
  }
  void parser::_event_gen_from_tuple (const int i, const int j) {
    _fi = 1;
    _fv.clear ();
    const chunk_t* bh (_s->chunk (i - 1)), *bi (_s->chunk (i)), *bj (_s->chunk (j)),
      *bk (_s->chunk (j + 1));
    // i - 1
    _add_string_feature   (bh->mlast ()->surf ());
    // i
    _add_string_feature (bi->mhead ()->surf ());
    _add_string_feature (bi->mhead ()->pos2 ());
    _add_string_feature (bi->mhead ()->infl ());
    _add_token_feature  (bi->mtail ());
    _add_string_feature (bi->mlast ()->surf ());
    // j
    _add_string_feature (bj->mzero ()->surf ());
    _add_string_feature (bj->mzero ()->pos2 ());
    _add_token_feature  (bj->mhead ());
    _add_string_feature (bj->mtail ()->surf ());
    _add_string_feature (bj->mtail ()->pos2 ());
    _add_string_feature (bj->mtail ()->infl ());
    _add_string_feature (bj->mlast ()->pos2 ());
    // j + 1
    _add_string_feature (bk->mzero ()->surf ());
    _add_string_feature (bk->mhead ()->surf ());
    _add_string_feature (bk->mlast ()->surf ());
    // coordination
    _add_coord_feature  (bh, bj);
    _add_coord_feature  (bi, bj);
    _add_coord_feature  (bi, bk);
    // _add_coord_feature  (bj, bk);
    //
    _add_local_feature  (bi, 0);
    _add_local_feature  (bj, _s->chunk_num - 1);
    _add_global_feature (bi, bj);
#ifdef USE_STACKING
    _add_lexical_feature (_s->chunk (_s->chunk (i)->head_id_cand));
#endif
    // add new features (>= _fi) here
    // put this tail for assertion
    static ny::uint fmax = 0;
    if (fmax && _fi != fmax)
      errx (1, HERE "feature offset broken; revert the change in features.");
    fmax = _fi;
  }
  void parser::_event_gen_from_tuple (const int i, const int j, const int k) {
    _fi = 1;
    _fv.clear ();
    const chunk_t* bh (_s->chunk (i - 1)), *bi (_s->chunk (i)), *bj (_s->chunk (j)),
      *bk (_s->chunk (k)), *bl (_s->chunk (j + 1)), *bm (_s->chunk (k + 1));
    // i - 1
    _add_string_feature (bh->mlast ()->surf ());
    // i
    _add_string_feature (bi->mhead ()->surf ());
    _add_string_feature (bi->mhead ()->pos2 ());
    _add_string_feature (bi->mhead ()->infl ());
    _add_token_feature  (bi->mtail ());
    _add_string_feature (bi->mlast ()->surf ());
    // j
    _add_string_feature (bj->mzero ()->surf ());
    _add_string_feature (bj->mzero ()->pos2 ());
    _add_token_feature  (bj->mhead ());
    _add_string_feature (bj->mtail ()->surf ());
    _add_string_feature (bj->mtail ()->pos2 ());
    _add_string_feature (bj->mtail ()->infl ());
    _add_string_feature (bj->mlast ()->pos2 ());
    // k
    _add_string_feature (bk->mzero ()->surf ());
    _add_string_feature (bk->mzero ()->pos2 ());
    _add_token_feature  (bk->mhead ());
    _add_string_feature (bk->mtail ()->surf ());
    _add_string_feature (bk->mtail ()->pos2 ());
    _add_string_feature (bk->mtail ()->infl ());
    _add_string_feature (bk->mlast ()->pos2 ());
    // j + 1
    _add_string_feature (bl->mzero ()->surf ());
    _add_string_feature (bl->mhead ()->surf ());
    _add_string_feature (bl->mlast ()->surf ());
    // k + 1
    _add_string_feature (bm->mzero ()->surf ());
    _add_string_feature (bm->mhead ()->surf ());
    _add_string_feature (bm->mlast ()->surf ());
    // coordination
    _add_coord_feature  (bh, bj);
    _add_coord_feature  (bh, bk);
    _add_coord_feature  (bi, bj);
    _add_coord_feature  (bi, bk);
    _add_coord_feature  (bi, bl);
    _add_coord_feature  (bi, bm);
    //
    _add_local_feature  (bi, 0);
    _add_local_feature  (bj, _s->chunk_num - 1);
    _add_local_feature  (bk, _s->chunk_num - 1);
    _add_global_feature (bi, bj);
    _add_global_feature (bi, bk);
#ifdef USE_STACKING
    _add_lexical_feature (_s->chunk (_s->chunk (i)->head_id_cand));
#endif
    // add new features (>= _fi) here
    // put this tail for assertion
    static ny::uint fmax = 0;
    if (fmax && _fi != fmax)
      errx (1, HERE "feature offset broken; revert the change in features.");
    fmax = _fi;
  }
#if defined (USE_OPAL) || defined (USE_MAXENT)
  inline void parser::_processSample (const bool flag) {
    if (_opt.learner == OPAL) {
#ifdef USE_OPAL
      opal::ex_t x;
      _opal->set_ex (x, flag ? +1 : -1, _fv, true, _opal_opt.kernel == opal::POLY);
      _ex_pool.put (x);
#endif
    } else if (_opt.learner == MAXENT) {
#ifdef USE_MAXENT
      static ME_Sample ms;
      ms.label = flag ? "+1" : "-1";
      ms.features.clear ();
      _project (ms);
      _libme->add_training_sample (ms);
#endif
    }
  }
#endif
  template <const process_t MODE>
  void parser::_parseLinear () {
    const int len = _s->chunk_num;
    for (int j = 1; j < len; ++j) {
      _stack.push (j - 1);
      while (! _stack.empty ()) {
        const int i = _stack.top ();
        chunk_t* b = _s->chunk (i);
        b->depnd_prob = 0.0;
        if (j != len - 1) {
          _event_gen_from_tuple (i, j);
          bool flag = (j == b->head_id_gold);
          // output example for training / fstrie construction
          if (MODE != PARSE) _print_ex (flag);
          if (MODE != LEARN)
            flag =
              _opt.verbose < 0 ? (b->depnd_prob = _pecco->getProbability (_fv)) > _pecco->threshold () :
              _pecco->binClassify (_fv);
#if defined (USE_OPAL) || defined (USE_MAXENT)
          else
            _processSample (flag); // learn
#endif
          if (! flag) break;
        }
        b->head_id = j;
        _stack.pop ();
      }
    }
  }
  template <const process_t MODE>
  void parser::_parseChunking () {
    const int len = _s->chunk_num;
    for (int i = 0; i < len - 1; ++i)
      _cinfo.push_back (chunk_info (i)); // non-deterministic
    // _cinfo.push_back (chunk_info (i, false)); // deterministic
    while (! _cinfo.empty ()) {
      typename std::list <chunk_info>::reverse_iterator it = _cinfo.rbegin ();
      bool next = true; // D
      int i = 0; // bug fix
      int j = it->id;
      _s->chunk (j)->depnd_prob = 0.0;
      _s->chunk (j)->head_id = len - 1;
      for (++it; it != _cinfo.rend (); ++it) {
        i = it->id;
        // if (! it->done) { // deterministic (assuming no dynamic features)
        _event_gen_from_tuple (i, j);
        _s->chunk (i)->depnd_prob = 0.0;
        bool flag = (j == _s->chunk (i)->head_id_gold);
        if (MODE != PARSE) _print_ex (flag);
        if (MODE != LEARN) {
          flag =
            _opt.verbose < 0 ? (_s->chunk (i)->depnd_prob = _pecco->getProbability (_fv)) > _pecco->threshold ()
            : _pecco->binClassify (_fv);
        }
#if defined (USE_OPAL) || defined (USE_MAXENT)
        else _processSample (flag);
#endif
        if (flag) _s->chunk (i)->head_id = j;
        // it->done = true; // deterministic
        // }
        if (_s->chunk (i)->head_id == -1 && next) { // O & D
          it = std::list <chunk_info>::reverse_iterator (_cinfo.erase (it.base ()));
          // it->done = false; // deterministic
        }
        next = _s->chunk (i)->head_id != -1;
        j = i;
      }
      if (_s->chunk (i)->head_id != -1) _cinfo.erase (it.base ());
    }
  }
  template <const process_t MODE>
  void parser::_parseBackward () {
    const int len = _s->chunk_num;
    if (len < 2) return;
    for (int i = len - 2; i >= 0; --i) {
      _s->chunk (i)->depnd_prob = 0.0; // non-deterministic
      for (int j = i + 1; j != -1; j = _s->chunk (j)->head_id) { // non-deterministic
        // for (int j = i + 1; j != len - 1; j = _s->chunk (j)->head_id) { // deterministic
        double prob = 0.0;
        _event_gen_from_tuple (i, j);
        bool flag = (j == _s->chunk (i)->head_id_gold);
        // output example for training / fstrie construction
        if (MODE != PARSE) _print_ex (flag);
        if (MODE == LEARN) { // learn
#if defined (USE_OPAL) || defined (USE_MAXENT)
          _processSample (flag);
#endif
          if (flag) { _s->chunk (i)->head_id = j; prob = 1.0; } // non-deterministic
          // if (flag) { _s->chunk (i)->head_id = j; prob = 1.0; break; } else continue; // deterministic
        } else {
          prob = _pecco->getProbability (_fv);
        }
        if (prob > _s->chunk (i)->depnd_prob) { // non-deterministic
          _s->chunk (i)->head_id = j;
          _s->chunk (i)->depnd_prob = prob;
        }
        // if (prob > _pecco->threshold ()) { // deterministic
        //   _s->chunk (i)->head_id = j;
        //   _s->chunk (i)->depnd_prob = prob;
        //   break;
        // }
      }
      // if (_s->chunk (i)->head_id == -1) // deterministic
      //   _s->chunk (i)->head_id = len - 1;
    }
  }
  template <const process_t MODE>
  void parser::_parseTournament () {
    const int len = _s->chunk_num;
    if (len < 2) return;
    if (MODE == LEARN) { // I don't merge the two outermost loops
      for (int i = 0; i < len - 2; ++i) {
        const int h = _s->chunk (i)->head_id_gold;
        for (int j = i + 1; j <= len - 1; ++j) {
          bool flag = true;
          if      (j < h) { flag = true;  _event_gen_from_tuple (i, j, h); }
          else if (j > h) { flag = false; _event_gen_from_tuple (i, h, j); }
          else continue;
#if defined (USE_OPAL) || defined (USE_MAXENT)
          _processSample (flag);
#endif
          _print_ex (flag);
        }
      }
    } else {
      for (int i = len - 2; i >= 0; --i) {
        int head_id = i + 1; // head
        int j = head_id;
        while (_s->chunk (j)->head_id != -1) { // head of head
          j = _s->chunk (j)->head_id;
          _event_gen_from_tuple (i, head_id, j);
          if (MODE == CACHE)
            _print_ex (head_id < _s->chunk (i)->head_id);
          bool flag =
            _opt.verbose < 0 ? (_s->chunk (i)->depnd_prob = _pecco->getProbability (_fv))  > _pecco->threshold ()
            : _pecco->binClassify (_fv);
          if (flag) head_id = j; // RIGHT
        }
        _s->chunk (i)->head_id = head_id;
      }
    }
  }
  template <const process_t MODE>
  void parser::_parse () {
    TIMER (if (MODE != LEARN) _depnd_t->startTimer ());
    if (MODE != LEARN) _switch_classifier (DEPND);
    switch (_opt.parser) {
      case LINEAR:     _parseLinear     <MODE> (); break;
      case CHUNKING:   _parseChunking   <MODE> (); break;
      case TOURNAMENT: _parseTournament <MODE> (); break;
      case BACKWARD:   _parseBackward   <MODE> (); break;
    }
    TIMER (if (MODE != LEARN) _depnd_t->stopTimer ());
    if (MODE == PARSE && _opt.input != RAW) _collectStat <DEPND> ();
  }
  template <const process_t MODE>
  inline void parser::_chunk () {
    TIMER (if (MODE != LEARN) _chunk_t->startTimer ());
    if (MODE != LEARN) _switch_classifier (CHUNK);
    _s->add_chunk (0);
    _s->token (0)->chunk_start = true;
    const int token_num = _s->token_num;
    for (int i = 1; i < token_num; ++i) {
      token_t* m = _s->token (i);
      m->chunk_start = m->chunk_start_gold;
      _event_gen_from_tuple (i);
      // output example for training / fstrie construction
      if (MODE != PARSE) _print_ex (m->chunk_start_gold);
      if (MODE != LEARN) {
        if (_opt.verbose < 0)
          m->chunk_start = (m->chunk_start_prob = _pecco->getProbability (_fv)) > _pecco->threshold ();
        else
          m->chunk_start = _pecco->binClassify (_fv);
      }
#if defined (USE_OPAL) || defined (USE_MAXENT)
      else
        _processSample (m->chunk_start_gold);
#endif
      if (m->chunk_start) _s->add_chunk (i);
    }
    TIMER (if (MODE != LEARN) _chunk_t->stopTimer ());
    if (MODE == PARSE && _opt.input != RAW) _collectStat <CHUNK> ();
  }
  template <>
  void parser::_collectStat <CHUNK> () {
    ++_chunk_stat.snum;
    bool flag (true), prev (true);
    for (int i = 1; i < _s->token_num; ++i) {
      const token_t* const m = _s->token (i);
      if (m->chunk_start && m->chunk_start_gold) { // 'B' & 'B'
        if (prev) ++_chunk_stat.pp; else ++_chunk_stat.np, ++_chunk_stat.pn;
        prev = true;
      } else if (m->chunk_start || m->chunk_start_gold) { // 'B' & 'I' or 'I' & 'B'
        if (m->chunk_start) ++_chunk_stat.np; else ++_chunk_stat.pn;
        flag = false;
        prev = false;
      }
    }
    if (prev) ++_chunk_stat.pp; else  ++_chunk_stat.np, ++_chunk_stat.pn;
    if (flag) ++_chunk_stat.scorr;
  }
  template <> void parser::_collectStat <DEPND> () {
    if (_s->chunk_num >= 1) { // set >= 2 to ignore sentene w/ a single bunsetsu
      ++_depnd_stat.snum;
      _depnd_stat.bnum += _s->chunk_num - 1;
      int bcorr = 0;
      const int len = _s->chunk_num;
      for (int i = 0; i < len - 1; ++i)
        if (_s->chunk (i)->head_id == _s->chunk (i)->head_id_gold) ++bcorr;
      // std::printf ("%d\n", b->head_id == b->head_id_gold ? 1 : 0); // for McNemar
      _depnd_stat.bcorr += bcorr;
      if (bcorr == len - 1) ++_depnd_stat.scorr;
      // std::printf ("%d\n", _flag & 0x2 ? 1 : 0); // for McNemar
    }
  }
#ifdef USE_AS_STANDALONE
  const sentence_t* parser::parse (const char* sent, size_t len) {
    _s->clear (true);
    if (! len) len = std::strlen (sent);
#ifdef USE_UNI_POS
    char ret[IOBUF_SIZE];
    _tagger->parse (sent, len, ret, IOBUF_SIZE);
    for (char* p = &ret[0]; std::strncmp (p, "EOS\n", 4) != 0; ++p) {
      char* line = p; while (*p != '\n') ++p;
      _s->add_token (line, p - line, _dict);
    }
#else
    for (MeCab::Node* m = _tagger->parseToNode (sent, len)->next;
         m->stat != MECAB_EOS_NODE; m = m->next) // set_feature
      _s->add_token (m->length, m->surface, m->feature, _dict);
#endif
    TIMER (_preproc_t->stopTimer ());
    _chunk <PARSE> ();
    _s->setup (_dict); // bug?
    _parse <PARSE> ();
    return _s;
  }
  const char* parser::parse_tostr (const char* sent, size_t len)
  { return parse (sent, len)->print_tostr (_opt.input, _opt.verbose < 0); }
#endif
  const sentence_t* parser::parse_from_postagged (char* postagged, size_t len) {
    _s->clear (true);
    if (! len) len = std::strlen (postagged);
    _s->set_topos (postagged, len);
    //
    char *r = _s->postagged () + len - 4;
    if (len < 4 || *r != 'E' || *(r+1) != 'O' || *(r+2) != 'S' || *(r+3) != '\n')
      errx (1, HERE "found a tagged sentence that is not EOS-terminated.");
    for (char* p (_s->postagged ()), *q (p); p != r; p = ++q) {
      while (q != r && *q != '\n') ++q;
      if (! _opt.ignore || std::strncmp (p, _opt.ignore, _opt.ignore_len) != 0)
        _s->add_token (p, q - p, _dict);
    }
    _chunk <PARSE> ();
    _s->setup (_dict);
    _parse <PARSE> ();
    return _s;
  }
  const char* parser::parse_from_postagged_tostr (char* postagged, size_t len)
  { return parse_from_postagged (postagged, len)->print_tostr (_opt.input, _opt.verbose < 0); }
  const sentence_t* parser::read_result (char* parsed, size_t len) {
    _s->clear (true);
    if (! len) len = std::strlen (parsed);
    _s->set_topos (parsed, len); // in fact, parsed result
    //
    char *r = _s->postagged () + len - 4;
    if (len < 4 || *r != 'E' || *(r+1) != 'O' || *(r+2) != 'S' || *(r+3) != '\n')
      errx (1, HERE "found a tagged sentence that is not EOS-terminated.");
    for (char* p (_s->postagged ()), *q (p); p != r; p = ++q) {
      while (q != r && *q != '\n') ++q;
      if (! _opt.ignore || std::strncmp (p, _opt.ignore, _opt.ignore_len) != 0) {
        switch (*p) {
          case '#': break;
          case '*':
            _s->add_chunk (p, q - p, _s->token_num, true); break;
          default:
            _s->add_token (p, q - p, _dict); break;
        }
      }
    }
    _s->setup (_dict);
    return _s;
  }
  template <const process_t MODE>
  void parser::_batch () {
    create_sentence ();
    unsigned long n = 0;
    if (MODE == PARSE)
      std::fprintf (stderr, "(input: STDIN [-I %d])\n", _opt.input);
#ifdef USE_AS_STANDALONE
    if (_opt.input == RAW) {
      if (! _tagger)
        errx (1, HERE "fail to invoke MeCab; you may want to set [-d].");
      char header[1024];
      char*  line = 0;
      size_t read = 0;
      char buf[IOBUF_SIZE]; std::setvbuf (stdin, &buf[0], _IOFBF, IOBUF_SIZE);
      while (1) {
        TIMER (_io_t->startTimer ());
        const bool input_ok = ny::getLine (stdin, line, read);
        TIMER (_io_t->stopTimer ());
        if (! input_ok) break;
        if (_opt.ignore && std::strncmp (line, _opt.ignore, _opt.ignore_len) == 0) {
          std::fwrite (line, sizeof (char), read - 1, stdout);
          std::fwrite ("\n", sizeof (char), 1, stdout);
          std::fflush (stdout);
          continue;
        }
        TIMER (_preproc_t->startTimer ());
        const size_t header_len
          = static_cast <size_t> (std::sprintf (header, "# S-ID: %ld; J.DepP\n", ++n));
        _s->setHeader (header, header_len);
        parse (line, read - 1);
        TIMER (_io_t->startTimer ());
        _s->print (_opt.input, _opt.verbose < 0);
        TIMER (_io_t->stopTimer  ());
      }
      _s->clear ();
      std::fclose (stdin);
    } else {
#endif
      static const char* mode[] = { "learn", "parse", "both", "cache" };
      int fd = 0; // stdin
      if (MODE != PARSE) {
        if ((fd = open (_opt.train, O_RDONLY)) == -1)
          errx (1, HERE "no such file: %s", _opt.train);
        std::string model (_opt.model_dir);
        model += "/"; model += input0[_opt.input];
        if (_opt.input == DEPND) {
          char sigparse[16]; std::sprintf (sigparse, ".p%d", _opt.parser);
          model += sigparse;
        }
        if (MODE == LEARN) {
          const std::string train (model + ".train");
          _writer = std::fopen (train.c_str (), "wb");
        } else {
          std::string event (model + ".event");
#ifdef USE_MODEL_SUFFIX
          if (_opt.learner == SVM)
            event += std::string (".s") + _pecco_opt.sigma;
#endif
          _writer = std::fopen (event.c_str (), "wb");
        }
      }
      const bool output = _opt.input == RAW ||
                          (MODE == PARSE && _opt.verbose < 0);
      char buf[IOBUF_SIZE];
      char* q (&buf[0]), *q_end (&buf[0] + IOBUF_SIZE);
      ssize_t avail = 0;
      std::vector <std::pair <char*, size_t> > pos;
      while (1) {
        TIMER (if (MODE != LEARN) _io_t->startTimer ());
        const bool input_ok
          = (avail = read (fd, q, static_cast <size_t> (q_end - q))) > 0;
        TIMER (if (MODE != LEARN) _io_t->stopTimer ());
        if (! input_ok) { // output comments if found
          char* p = &buf[0];
          while (p != q) {
            char* p_end = p; while (p_end != q && *p_end != '\n') ++p_end;
            if (p_end != q && *p_end == '\n') ++p_end;
            if (_opt.ignore && std::strncmp (p, _opt.ignore, _opt.ignore_len) == 0) {
              std::fwrite (p, sizeof (char), p_end - p, stdout);
              p = p_end;
            } else {
              warnx ("found a tagged sentence that is not EOS-terminated.");
              break;
            }
          }
          break;
        }
        q_end = q + avail;
        q = &buf[0];
        while (1) {
          TIMER (if (MODE != LEARN) _io_t->startTimer ());
          // read input
          pos.clear ();
          bool eos = false;
          for (char* r = q; r + 3 < q_end; ++r) {
            pos.push_back (std::pair <char*, size_t> (r, 0));
            // workaround fix for broken strcmp () in x86_64; thanks Dr. ohtake
            // if (*r == 'E' && std::strncmp (r, "EOS\n", 4) == 0)
            if (*r == 'E' && *(r+1) == 'O' && *(r+2) == 'S' && *(r+3) == '\n') // to avoid using strncmp
              { q = r + 4; eos = true; break; }  // find EOS\n
            while (r != q_end && *r != '\n') ++r; // next line
            if (*r == '\n') // set line length
              pos.back ().second = r - pos.back ().first + 1;
          }
          if (! eos) { // premature input
            std::memmove (&buf[0], q, static_cast <size_t> (q_end - q));
            q     = &buf[0] + (q_end - q);
            q_end = &buf[0] + IOBUF_SIZE;
            break;
          }
          TIMER (if (MODE != LEARN) _io_t->stopTimer ());
          // process
          TIMER (if (MODE != LEARN) _preproc_t->startTimer ());
          ++n;
          if (_opt.input == RAW) {
            char header[1024];
            const size_t header_len
              = static_cast <size_t> (std::sprintf (header, "# S-ID: %ld; J.DepP\n", n));
            _s->setHeader (header, header_len);
          }
          _s->clear ();
          bool flag = false; // reference chunk annotation
          for (size_t i = 0; i < pos.size () - 1; ++i) {
            char* line = pos[i].first;
            const size_t read = static_cast <size_t> (pos[i].second);
            if (_opt.ignore && std::strncmp (line, _opt.ignore, _opt.ignore_len) == 0) {
              std::fwrite (line, sizeof (char), read - 1, stdout);
              std::fwrite ("\n", sizeof (char), 1, stdout);
              std::fflush (stdout);
              continue;
            }
            if (_opt.input == RAW) _s->add_token (line, read - 1, _dict);
            else
              switch (*line) {
                case '#': if (output) _s->setHeader (line, read); break;
                case '*':
                  if (_opt.input == DEPND)
                    _s->add_chunk (line, read - 1, _s->token_num);
                  else
                    flag = true;
                  break;
                default:
                  _s->add_token (line, read - 1, _dict, flag);
                  flag = false;
              }
          }
          TIMER (if (MODE != LEARN) _preproc_t->stopTimer ());
          if (_opt.input != DEPND) _chunk <MODE> ();
          _s->setup (_dict); // bug?
          if (_opt.input != CHUNK) _parse <MODE> ();
          TIMER (if (MODE != LEARN) _io_t->startTimer ());
          if (output) _s->print (_opt.input, _opt.verbose < 0);
          TIMER (if (MODE != LEARN) _io_t->stopTimer ());
          if (MODE != PARSE && n % 1000 == 0)
            std::fprintf (stderr, "\r%s: %ld sent. processed", mode[MODE], n);
          if (MODE != PARSE && _opt.max_sent && n >= _opt.max_sent)
            { lseek (fd, 0, SEEK_END); break; } // go to the end
        }
      }
      close (fd);
      if (avail == 0 && q == q_end)
        errx (1, HERE "set a larger value to IOBUF_SIZE.");
      if (MODE != PARSE) {
        std::fprintf (stderr, "\r%s: %ld sent. processed.", mode[MODE], n);
        std::fclose (_writer);
      }
      std::fprintf (stderr, "\n");
#ifdef USE_AS_STANDALONE
    }
#endif
  }
  void parser::_learn () {
    std::string model (_opt.model_dir);
    model += "/"; model += input0[_opt.input];
    if (_opt.input == DEPND) {
      char sigparse[16]; std::sprintf (sigparse, ".p%d", _opt.parser);
      model += sigparse;
    }
    switch (_opt.learner) { // they should be newed earlier
      case OPAL:
#ifdef USE_OPAL
        _opal->train (_ex_pool, _opal_opt.iter);
        _opal->save (model.c_str ());
#endif
        break;
      case SVM:
#ifdef USE_SVM
        {
          const std::string train (model + ".train");
          TinySVM::Example* ex = new TinySVM::Example;
          ex->read (train.c_str ());
          _tinysvm = ex->learn (_tiny_param);
          _tinysvm->write (model.c_str ());
          delete ex;
        }
#endif
        break;
      case MAXENT:
#ifdef USE_MAXENT
        switch (_maxent_opt.algo) {
          case SGD:   _libme->use_SGD ();
          case OWLQN: _libme->use_l1_regularizer (_maxent_opt.reg_cost); break;
          case LBFGS: _libme->use_l2_regularizer (_maxent_opt.reg_cost); break;
          default: errx (1, HERE "MAXENT optimizer disabled.");
        }
        _libme->train ();
        _libme->save_to_file (model);
#endif
        break;
    }
  }
  void parser::_register_token (char* cs, const size_t& len, sbag_t& sbag,
                                std::set <ny::uint>& particle_feature_ids) {
    ny::uint surf (0), i (0);
    bool flag = false;
    const char* post_particle
      = _opt.utf8 ? UTF8_POST_PARTICLE : EUC_POST_PARTICLE;
    for (char* p (cs), * const p_end (p + len);
         cs < p_end && i < NUM_FIELD; cs = ++p, ++i) {
      if (i == SURF) while (p != p_end && *p != SURFACE_END) ++p;
#ifdef USE_UNI_POS
      else if (i == POS1)
        while (p != p_end && *p != FEATURE_SEP && *p != FEATURE_SEP_UNI) ++p;
#endif
      else
        while (p != p_end && *p != FEATURE_SEP && *p != '\0') ++p;
#ifdef USE_UNI_POS
      const bool empty_POS2 = i == POS1 && *p == FEATURE_SEP;
#endif
      *p = '\0';
#ifdef USE_UNI_POS
      if (cs != p)
#endif
#ifdef USE_KYOTO_PARTIAL
        if (i == YOMI || i == POS1 || i == POS2 || i == INFL) { //  || i > OTHER
#else
        if (i == SURF || i == POS1 || i == POS2 || i == INFL) { //  || i > OTHER
#endif
          sbag_t::iterator it = sbag.find (cs);
          if (it == sbag.end ()) { // new entry
            char* copy = new char[p - cs + 1];
            std::strcpy (copy, cs);
            it = sbag.insert (sbag_t::value_type (copy, static_cast <ny::uint> (sbag.size ()))).first;
          }
          const ny::uint id = it->second;
          switch (i) {
#ifdef USE_KYOTO_PARTIAL
            case YOMI: surf = id; break;
#else
            case SURF: surf = id; break;
#endif
            case POS1: flag = std::strcmp (cs, post_particle) == 0; break;
            default: break;
          }
        }
        if (flag) particle_feature_ids.insert (surf);
#ifdef USE_UNI_POS
        if (empty_POS2) --p;
#endif
      }
#ifdef USE_JUMAN_POS
    if (i < NUM_FIELD)
      errx (1, HERE "# fields is less than %d.", NUM_FIELD);
#endif
  }
  // morphological dictionary are extracted from the training data
  void parser::_set_token_dict () {
    if (_opt.verbose > 0) std::fprintf (stderr, "Loading dict..");
    std::string dict (_opt.model_dir);
    dict += _opt.utf8 ? "/dic.utf8" : "/dic.euc";
    struct stat st;
    if (stat (dict.c_str (), &st) != 0) {
      if (_opt.verbose > 0)
        std::fprintf (stderr, "not found; reading %s..", _opt.train);
      sbag_t              sbag;
      std::set <ny::uint> particle_feature_ids;
      // insert some mandatory particles
      FILE* reader = std::fopen (_opt.train, "r");
      if (! reader) errx (1, HERE "no such file: %s", _opt.train);
      char* line = 0;
      size_t read = 0;
      while (ny::getLine (reader, line, read))
        if (*line != '*' && *line != '#' && *line != 'E')
          _register_token (line, read - 1, sbag, particle_feature_ids);
      std::fclose (reader);
      const ny::uint should_not_free = static_cast <ny::uint> (sbag.size ());
      // add some mandatory features
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_COMMA : EUC_COMMA,
                                       static_cast <ny::uint> (sbag.size ())));
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_PERIOD : EUC_PERIOD,
                                       static_cast <ny::uint> (sbag.size ())));
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_POST_PARTICLE : EUC_POST_PARTICLE,
                                       static_cast <ny::uint> (sbag.size ())));
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_BRACKET_IN : EUC_BRACKET_IN,
                                       static_cast <ny::uint> (sbag.size ())));
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_BRACKET_OUT : EUC_BRACKET_OUT,
                                       static_cast <ny::uint> (sbag.size ())));
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_SPECIAL : EUC_SPECIAL,
                                       static_cast <ny::uint> (sbag.size ())));
#ifdef USE_JUMAN_POS
      sbag.insert (sbag_t::value_type (_opt.utf8 ? UTF8_SUFFIX : EUC_SUFFIX,
                                       static_cast <ny::uint> (sbag.size ())));
#endif
      if (_opt.verbose > 0)
        std::fprintf (stderr, "done.\n");
      const ny::uint num_particle_features = static_cast <ny::uint> (particle_feature_ids.size ());
      const ny::uint num_lexical_features  = static_cast <ny::uint> (sbag.size ());
      if (num_particle_features == 0)
        errx (1, HERE "no particles found in %s\n"
              "\tthe charset / posset may mismatch.",
              _opt.train);
      FILE* writer = std::fopen (dict.c_str (), "wb");
      std::fwrite (&num_particle_features, sizeof (ny::uint), 1, writer);
      std::fwrite (&num_lexical_features,  sizeof (ny::uint), 1, writer);
      std::fclose (writer);
      //
      std::vector <const char *> str;
      std::vector <size_t>       len;
      std::vector <int>          val;
      for (sbag_t::const_iterator it = sbag.begin (); it != sbag.end (); ++it) {
        str.push_back (it->first);
        len.push_back (std::strlen (it->first));
        // move (gap) lexical featuress forward and assign smaller IDs
        const ny::uint id = it->second;
        std::set <ny::uint>::iterator jt = particle_feature_ids.lower_bound (id);
        const int index
          = static_cast <int> (std::distance (particle_feature_ids.begin (), jt));
        val.push_back (*jt == id ? index: static_cast <int> (id + num_particle_features) - index);
      }
      ny::trie t;
      pecco::build_trie (&t, "dict trie", dict, str, len, val, _opt.verbose > 0, "ab");
      for (sbag_t::iterator it = sbag.begin (); it != sbag.end (); ++it)
        if (it->second < should_not_free)
          delete [] it->first;
        else
          std::fprintf (stderr, "no %s observed in the training data\n", it->first);
    }
    _dict = new dict_t (dict.c_str (), _opt.utf8);
    if (_opt.verbose > 0)
      std::fprintf (stderr, "done. (# strings + 1 (unseen) = %d).\n",
                    _dict->num_lexical_features ());
    _particle_feature_bits.resize (_dict->particle_feature_bit_len (), 0);
  }
  void parser::_setup_learner () {
    switch (_opt.learner) { // they should be newed earlier
      case OPAL:
#ifdef USE_OPAL
        _opal_opt.set (_opt.learner_argc, _opt.learner_argv);
        _opal = new opal::Model (_opal_opt);
#endif
        break;
      case SVM:
#ifdef USE_SVM
        _tiny_param.set (_opt.learner_argc, _opt.learner_argv);
        if (_tiny_param.kernel_type != TinySVM::POLY || _tiny_param.degree > 4)
          errx (1, HERE "only polynomial kernel [-t 1] of [-d] <= 4 is supported in SVM.");
#endif
        break;
      case MAXENT:
#ifdef USE_MAXENT
        _maxent_opt.set (_opt.learner_argc, _opt.learner_argv);
        _libme = new ME_Model;
#endif
        break;
    }
  }
  void parser::_cleanup_learner () {
    switch (_opt.learner) {
      case OPAL:
#ifdef USE_OPAL
        // _opal->printStat ();
        delete _opal;
#endif
        break;
      case SVM:
#ifdef USE_SVM
        delete _tinysvm;
#endif
        break;
      case MAXENT:
#ifdef USE_MAXENT
        delete _libme;
#endif
        break;
    }
  }
  void parser::_switch_classifier (const input_t in) // exchange classifiers
  { _pecco = in == CHUNK ? _pecco_chunk : _pecco_depnd; }
  void parser::_setup_classifier (const input_t in, int argc, char ** argv) {
    std::string model (_opt.model_dir);
    model += "/"; model += input0[in];
    if (in == DEPND) {
      char sigparse[16]; std::sprintf (sigparse, ".p%d", _opt.parser);
      model += sigparse;
    }
    if (_opt.mode == BOTH) { // induce an appropriate classifier from input model
#ifdef USE_OPAL
      if (_opt.learner == OPAL && _opal_opt.kernel == opal::POLY)
        _opt.learner = SVM; // fake
#endif
    } else {
      FILE* fp = std::fopen (model.c_str (), "r");
      if (! fp || std::feof (fp))
        errx (1, HERE "no model found: %s; train a model first [-t 0]",
              model.c_str ());
      switch (std::fgetc (fp)) {
        case  0 :
        case '#': _opt.learner = OPAL;   break;
        case 'o': // delegate
        case 'T': _opt.learner = SVM;    break;
        case '-':
        case '+': _opt.learner = MAXENT; break;
        default:  errx (1, HERE "unknown model found");
      }
#ifndef USE_OPAL
      if (_opt.learner == OPAL)
        errx (1, HERE "unsupported model found; configure with --enable-opal in compiling J.DepP");
#endif
      std::fclose (fp);
    }
    if  (_opt.learner == OPAL) { // opal as a classifier
#ifdef USE_OPAL
      opal::option opal_opt (argc, argv);
      opal_opt.model = model.c_str ();
      _pecco = new pecco::pecco (reinterpret_cast <opal::Model *> (0), opal_opt);
      _pecco->load (opal_opt.model);
#endif
    } else {
      const std::string train (model + ".train");
      const std::string event (model + ".event");
      _pecco_opt.set (argc, argv);
      _pecco_opt.model = model.c_str ();
      _pecco_opt.train = train.c_str ();
      _pecco_opt.event = event.c_str ();
      if (_opt.learner == SVM) {
#if defined (USE_SVM) || defined (USE_OPAL)
#ifdef USE_MODEL_SUFFIX
        _pecco_opt.event += std::string (".s") + _pecco_opt.sigma; // approx
#endif
        _pecco = new pecco::pecco (static_cast <pecco::kernel_model *> (0),
                                   _pecco_opt);
#endif
      } else {
#ifdef USE_MAXENT
        _pecco = new pecco::pecco (static_cast <pecco::linear_model *> (0),
                                   _pecco_opt);
#endif
      }
      _pecco->load (_pecco_opt.model);
    }
    if (in == CHUNK)  _pecco_chunk = _pecco; else _pecco_depnd = _pecco;
  }
  void parser::_cleanup_classifier (const input_t in) {
    _switch_classifier (in);
    // _pecco->printStat ();
    delete _pecco;
  }
#ifdef USE_MAXENT
  void parser::_project (ME_Sample& ms) const {
    char key[64];
    for (ny::fv_it beg (_fv.begin ()), end (_fv.end ()), it (beg);
         it != end; ++it) {
      const int pos_i = std::sprintf (key, "%d", *it);
      ms.add_feature (key);
      if (_maxent_opt.degree != 1)
        for (ny::fv_it jt = beg; jt != it; ++jt) {
          const int pos_j = pos_i + std::sprintf (key + pos_i, ":%d", *jt);
          ms.add_feature (key);
          if (_maxent_opt.degree != 2)
            for (ny::fv_it kt = beg; kt != jt; ++kt) {
              std::sprintf (key + pos_j, ":%d", *kt);
              ms.add_feature (key);
            }
        }
    }
  }
#endif
  void parser::init () {
    _set_token_dict ();
    _setup_classifier (CHUNK, _opt.chunk_argc, _opt.chunk_argv);
    _setup_classifier (DEPND, _opt.depnd_argc, _opt.depnd_argv);
    create_sentence ();
  }
  void parser::run () {
#ifdef USE_TIMER
    if (_opt.verbose > 0)
      std::fprintf (stderr, "Processor Speed: %.3LfGHz.\n",
                    ny::Timer::clock () / 1000.0);
#endif
    if (_opt.input == RAW && _opt.mode != PARSE)
#ifdef USE_AS_STANDALONE
      errx (1, HERE "You can input RAW sentences [-I 0] only for parsing [-t 1].");
#else
    errx (1, HERE "You can input POS-tagged sentences [-I 0] only for parsing [-t 1].");
#endif

    TIMER (_dict_t->startTimer ());
    _set_token_dict ();
    TIMER (_dict_t->stopTimer ());
    // learn
    if  (_opt.mode == LEARN || _opt.mode == BOTH) { // learn or both
      _setup_learner ();
      _batch <LEARN> ();
      _learn ();
      _cleanup_learner ();
    }
    // parse, cache
    if (_opt.mode != LEARN) {
      if (_opt.input != DEPND)
        _setup_classifier (CHUNK, _opt.chunk_argc, _opt.chunk_argv);
      if (_opt.input != CHUNK)
        _setup_classifier (DEPND, _opt.depnd_argc, _opt.depnd_argv);
      if (_opt.mode == CACHE) {
        if (_opt.learner == OPAL)
          errx (1, HERE "needless to cache in opal classifier [-t 0].");
        _batch <CACHE> ();
      } else {
        _batch <PARSE> ();
        if (_opt.input == CHUNK) _chunk_stat.print ();
        if (_opt.input == DEPND) _depnd_stat.print ();
      }
      // cleanup
      TIMER (_timer_pool.print ());
      if (_opt.input != DEPND) _cleanup_classifier (CHUNK);
      if (_opt.input != CHUNK) _cleanup_classifier (DEPND);
    }
  }
}
// ToDo
// - output model info, or at least add signature
// - implement POS tagger / word segmenter faster than MeCab
// - class for generating string features; add & increment
