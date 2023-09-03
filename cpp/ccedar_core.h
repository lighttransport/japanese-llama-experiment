// ccedar -- C++ implementation of Character-wise, Efficiently-updatable Double ARray trie (minimal version for Jagger)
//  $Id: ccedar_core.h 2025 2022-12-16 06:18:29Z ynaga $
// Copyright (c) 2022 Naoki Yoshinaga <ynaga@iis.u-tokyo.ac.jp>
#ifndef CCEDAR_CORE_H
#define CCEDAR_CORE_H
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ccedar {
  // typedefs
  template <typename T> struct to_unsigned;
  template <> struct to_unsigned <char> { typedef unsigned char  type; };
  template <> struct to_unsigned <int>  { typedef unsigned int   type; };
  template <typename T> inline size_t key_len (const T* p);
  template <> inline size_t key_len (const char* p) { return std::strlen(p); }
  // dynamic double array
  template <typename key_type,
            typename value_type,
            const int MAX_KEY_BITS = sizeof (key_type) * 8,
            const int NO_VALUE  = -1,
            const int NO_PATH   = -2,
            const int MAX_TRIAL = 1>
  class da {
  public:
    enum { MAX_KEY_CODE = 1 << MAX_KEY_BITS, MAX_ALLOC_SIZE = MAX_KEY_CODE << 4 };
    enum error_code { CEDAR_NO_VALUE = NO_VALUE, CEDAR_NO_PATH = NO_PATH };
    typedef typename to_unsigned <key_type>::type ukey_type;
    typedef value_type result_type;
    struct node {
      union { int base; value_type value; }; // negative means prev empty index
      int  check;                            // negative means next empty index
      node (int base_ = 0, int check_ = 0) : base (base_), check (check_) {}
    };
    struct ninfo {  // x1.5 update speed; x2 memory (8n -> 16n); can be 12n
      ukey_type sibling;   // right sibling (= 0 if not exist)
      ukey_type child;     // first child
      ninfo () : sibling (0), child (0) {}
    };
    struct block { // a block w/ sizeof (key_type) << 8 elements
      int prev;   // prev block; 3 bytes
      int next;   // next block; 3 bytes
      int num;    // # empty elements; 0 - sizeof (key_type) << 8
      int ok;     // minimum # branching failed to locate - 1; soft limit
      int trial;  // # trial
      int ehead;  // first empty item
      block () : prev (0), next (0), num (MAX_KEY_CODE), ok (MAX_KEY_CODE), trial (0), ehead (0) {}
    };
    da () : _array (0), _ninfo (0), _block (0), _bheadF (0), _bheadC (0), _bheadO (0), _capacity (0), _size (0), _no_delete (false), _ok ()
    { _initialize (); }
    ~da () { clear (); }
    // interfance
    template <typename T>
    T exactMatchSearch (const key_type* key) const
    { return exactMatchSearch <T> (key, key_len <key_type> (key)); }
    template <typename T>
    T exactMatchSearch (const key_type* key, size_t len, size_t from = 0) const {
      union { int i; value_type x; } b;
      size_t pos = 0;
      b.i = _find (key, from, pos, len);
      if (b.i == CEDAR_NO_PATH) b.i = CEDAR_NO_VALUE;
      T result;
      _set_result (&result, b.x, len, from);
      return result;
    }
    template <typename T>
    size_t commonPrefixSearch (const key_type* key, T* result, size_t result_len) const
    { return commonPrefixSearch (key, result, result_len, key_len <key_type> (key)); }
    template <typename T>
    size_t commonPrefixSearch (const key_type* key, T* result, size_t result_len, size_t len, size_t from = 0) const {
      size_t num = 0;
      for (size_t pos = 0; pos < len; ) {
        union { int i; value_type x; } b;
        b.i = _find (key, from, pos, pos + 1);
        if (b.i == CEDAR_NO_VALUE) continue;
        if (b.i == CEDAR_NO_PATH)  return num;
        if (num < result_len) _set_result (&result[num], b.x, pos, from);
        ++num;
      }
      return num;
    }
    value_type traverse (const key_type* key, size_t& from, size_t& pos) const
    { return traverse (key, from, pos, key_len <key_type> (key)); }
    value_type traverse (const key_type* key, size_t& from, size_t& pos, size_t len) const {
      union { int i; value_type x; } b;
      b.i = _find (key, from, pos, len);
      return b.x;
    }
    value_type& update (const key_type* key)
    { return update (key, key_len <key_type> (key)); }
    value_type& update (const key_type* key, size_t len, value_type val = value_type (0))
    { size_t from (0), pos (0); return update (key, from, pos, len, val); }
    value_type& update (const key_type* key, size_t& from, size_t& pos, size_t len, value_type val) {
      if (! len && ! from)
        _err (__FILE__, __LINE__, "failed to insert zero-length key\n");
      for (const ukey_type* const key_ = reinterpret_cast <const ukey_type*> (key);
           pos < len; ++pos) {
        from = static_cast <size_t> (_follow (from, key_[pos]));
      }
      const int to = _follow (from, 0);
      return _array[to].value += val;
    }
    int save (const char* fn, const char* mode = "wb") const {
      FILE* fp = std::fopen (fn, mode);
      if (! fp) return -1;
      std::fwrite (_array, sizeof (node), static_cast <size_t> (_size), fp);
      std::fclose (fp);
      return 0;
    }
    int open (const char* fn, const char* mode = "rb") {
      FILE* fp = std::fopen (fn, mode);
      if (! fp) return -1;
      // get size
      if (std::fseek (fp, 0, SEEK_END) != 0) return -1;
      const size_t size_ = static_cast <size_t> (std::ftell (fp)) / sizeof (node);
      if (std::fseek (fp, 0, SEEK_SET) != 0) return -1;
      // set array
      _array = static_cast <node*>  (std::malloc (sizeof (node)  * size_));
      if (size_ != std::fread (_array, sizeof (node), size_, fp)) return -1;
      std::fclose (fp);
      _size = static_cast <int> (size_);
      return 0;
    }
    void set_array (void* p, size_t size_ = 0) { // ad-hoc
      clear (false);
      _array = static_cast <node*> (p);
      _size  = static_cast <int> (size_);
      _no_delete = true;
    }
    const void* array () const { return _array; }
    void clear (const bool reuse = true) {
      if (_array && ! _no_delete) std::free (_array);
      if (_ninfo) std::free (_ninfo);
      if (_block) std::free (_block);
      _array = 0; _ninfo = 0; _block = 0;
      _bheadF = _bheadC = _bheadO = _capacity = _size = 0;
      if (reuse) _initialize ();
      _no_delete = false;
    }
  private:
    // currently disabled; implement these if you need
    da (const da&);
    da& operator= (const da&);
    node*   _array;
    ninfo*  _ninfo;
    block*  _block;
    int     _bheadF;  // first block of Full;   0
    int     _bheadC;  // first block of Closed; 0 if no Closed
    int     _bheadO;  // first block of Open;   0 if no Open
    int     _capacity;
    int     _size;
    int     _no_delete;
    int     _ok[MAX_KEY_CODE + 1];
    //
    static void _err (const char* fn, const int ln, const char* msg)
    { std::fprintf (stderr, "cedar: %s [%d]: %s", fn, ln, msg); std::exit (1); }
    template <typename T>
    static void _realloc_array (T*& p, const int size_n, const int size_p = 0) {
      void* tmp = std::realloc (p, sizeof (T) * static_cast <size_t> (size_n));
      if (! tmp)
        std::free (p), _err (__FILE__, __LINE__, "memory reallocation failed\n");
      p = static_cast <T*> (tmp);
      static const T T0 = T ();
      for (T* q (p + size_p), * const r (p + size_n); q != r; ++q) *q = T0;
    }
    void _initialize () { // initialize the first special block
      _realloc_array (_array, MAX_KEY_CODE, MAX_KEY_CODE);
      _realloc_array (_ninfo, MAX_KEY_CODE);
      _realloc_array (_block, 1);
      _array[0] = node (0, -1);
      for (int i = 1; i < MAX_KEY_CODE; ++i)
        _array[i] = node (i == 1 ? -(MAX_KEY_CODE - 1) : - (i - 1), i == (MAX_KEY_CODE - 1) ? -1 : - (i + 1));
      _block[0].ehead = 1; // bug fix for erase
      _capacity = _size = MAX_KEY_CODE;
      for (size_t i = 0; i <= MAX_KEY_CODE; ++i) _ok[i] = static_cast <int> (i);
    }
    // follow/create edge
    int _follow (size_t& from, const ukey_type& label) {
      int to = 0;
      const int base = _array[from].base;
      if (base < 0 || _array[to = base ^ label].check < 0) {
        to = _pop_enode (base, label, static_cast <int> (from));
        _push_sibling (from, to ^ label, label, base >= 0);
      } else if (_array[to].check != static_cast <int> (from))
        to = _resolve (from, base, label);
      return to;
    }
    // find key from double array
    int _find (const key_type* key, size_t& from, size_t& pos, const size_t len) const {
      for (const ukey_type* const key_ = reinterpret_cast <const ukey_type*> (key);
           pos < len; ) { // follow link
        size_t to = static_cast <size_t> (_array[from].base); to ^= key_[pos];
        if (_array[to].check != static_cast <int> (from)) return CEDAR_NO_PATH;
        ++pos;
        from = to;
      }
      const node n = _array[_array[from].base ^ 0];
      if (n.check != static_cast <int> (from)) return CEDAR_NO_VALUE;
      return n.base;
    }
    void _set_result (result_type* x, value_type r, size_t = 0, size_t = 0) const
    { *x = r; }
    void _pop_block (const int bi, int& head_in, const bool last) {
      if (last) { // last one poped; Closed or Open
        head_in = 0;
      } else {
        const block& b = _block[bi];
        _block[b.prev].next = b.next;
        _block[b.next].prev = b.prev;
        if (bi == head_in) head_in = b.next;
      }
    }
    void _push_block (const int bi, int& head_out, const bool empty) {
      block& b = _block[bi];
      if (empty) { // the destination is empty
        head_out = b.prev = b.next = bi;
      } else { // use most recently pushed
        int& tail_out = _block[head_out].prev;
        b.prev = tail_out;
        b.next = head_out;
        head_out = tail_out = _block[tail_out].next = bi;
      }
    }
    int _add_block () {
      if (_size == _capacity) { // allocate memory if needed
        _capacity += _size >= MAX_ALLOC_SIZE ? MAX_ALLOC_SIZE : _size;
        _realloc_array (_array, _capacity, _capacity);
        _realloc_array (_ninfo, _capacity, _size);
        _realloc_array (_block, _capacity >> MAX_KEY_BITS, _size >> MAX_KEY_BITS);
      }
      _block[_size >> MAX_KEY_BITS].ehead = _size;
      _array[_size] = node (- (_size + (MAX_KEY_CODE - 1)),  - (_size + 1));
      for (int i = _size + 1; i < _size + (MAX_KEY_CODE - 1); ++i)
        _array[i] = node (-(i - 1), -(i + 1));
      _array[_size + (MAX_KEY_CODE - 1)] = node (- (_size + (MAX_KEY_CODE - 2)),  -_size);
      _push_block (_size >> MAX_KEY_BITS, _bheadO, ! _bheadO); // append to block Open
      _size += MAX_KEY_CODE;
      return (_size >> MAX_KEY_BITS) - 1;
    }
    // transfer block from one start w/ head_in to one start w/ head_out
    void _transfer_block (const int bi, int& head_in, int& head_out) {
      _pop_block  (bi, head_in, bi == _block[bi].next);
      _push_block (bi, head_out, ! head_out && _block[bi].num);
    }
    // pop empty node from block; never transfer the special block (bi = 0)
    int _pop_enode (const int base, const ukey_type label, const int from) {
      const int e  = base < 0 ? _find_place () : base ^ label;
      const int bi = e >> MAX_KEY_BITS;
      node&  n = _array[e];
      block& b = _block[bi];
      if (--b.num == 0) {
        if (bi) _transfer_block (bi, _bheadC, _bheadF); // Closed to Full
      } else { // release empty node from empty ring
        _array[-n.base].check = n.check;
        _array[-n.check].base = n.base;
        if (e == b.ehead) b.ehead = -n.check; // set ehead
        if (bi && b.num == 1 && b.trial != MAX_TRIAL) // Open to Closed
          _transfer_block (bi, _bheadO, _bheadC);
      }
      // initialize the released node
      if (label) n.base = -1; else n.value = value_type (0); n.check = from;
      if (base < 0) _array[from].base = e ^ label;
      return e;
    }
    // push empty node into empty ring
    void _push_enode (const int e) {
      const int bi = e >> MAX_KEY_BITS;
      block& b = _block[bi];
      if (++b.num == 1) { // Full to Closed
        b.ehead = e;
        _array[e] = node (-e, -e);
        if (bi) _transfer_block (bi, _bheadF, _bheadC); // Full to Closed
      } else {
        const int prev = b.ehead;
        const int next = -_array[prev].check;
        _array[e] = node (-prev, -next);
        _array[prev].check = _array[next].base = -e;
        if (b.num == 2 || b.trial == MAX_TRIAL) // Closed to Open
          if (bi) _transfer_block (bi, _bheadC, _bheadO);
        b.trial = 0;
      }
      if (b.ok < _ok[b.num]) b.ok = _ok[b.num];
      _ninfo[e] = ninfo (); // reset ninfo; no child, no sibling
    }
    // push label to from's child
    void _push_sibling (const size_t from, const int base, const ukey_type label, const bool flag = true) {
      ukey_type* c = &_ninfo[from].child;
      if (flag && ! *c)
        c = &_ninfo[base ^ *c].sibling;
      _ninfo[base ^ label].sibling = *c, *c = label;
    }
    // pop label from from's child
    void _pop_sibling (const size_t from, const int base, const ukey_type label) {
      ukey_type* c = &_ninfo[from].child;
      while (*c != label) c = &_ninfo[base ^ *c].sibling;
      *c = _ninfo[base ^ label].sibling;
    }
    // check whether to replace branching w/ the newly added node
    bool _consult (const int base_n, const int base_p, ukey_type c_n, ukey_type c_p) const {
      do if (! (c_p = _ninfo[base_p ^ c_p].sibling)) return false;
      while ((c_n = _ninfo[base_n ^ c_n].sibling));
      return true;
    }
    // enumerate (equal to or more than one) child nodes
    ukey_type* _set_child (ukey_type* p, const int base, ukey_type c, const int label = -1) {
      --p;
      if (! c)  { *++p = c; c = _ninfo[base ^ c].sibling; } // 0: terminal
      if (label != -1) *++p = static_cast <ukey_type> (label);
      while (c) { *++p = c; c = _ninfo[base ^ c].sibling; }
      return p;
    }
    // explore new block to settle down
    int _find_place () {
      if (_bheadC) return _block[_bheadC].ehead;
      if (_bheadO) return _block[_bheadO].ehead;
      return _add_block () << MAX_KEY_BITS;
    }
    int _find_place (const ukey_type* const first, const ukey_type* const last) {
      if (int bi = _bheadO) {
        const int bz = _block[_bheadO].prev;
        const int nc = static_cast <int> (last - first + 1);
        while (1) { // set candidate block
          block& b = _block[bi];
          if (b.num >= nc && nc <= b.ok) // explore configuration
            for (int e = b.ehead;;) {
              const int base = e ^ *first;
              for (const ukey_type* p = first; _array[base ^ *++p].check < 0; )
                if (p == last) return b.ehead = e; // no conflict
              if ((e = -_array[e].check) == b.ehead) break;
            }
          b.ok = nc - 1; // mod
          if (b.ok < _ok[b.num]) _ok[b.num] = b.ok;
          const int bi_ = b.next;
          if (++b.trial == MAX_TRIAL) _transfer_block (bi, _bheadO, _bheadC);
          if (bi == bz) break;
          bi = bi_;
        };
      }
      return _add_block () << MAX_KEY_BITS;
    }
    // resolve conflict on base_n ^ label_n = base_p ^ label_p
    int _resolve (size_t& from_n, const int base_n, const ukey_type label_n) {
      // examine siblings of conflicted nodes
      const int to_pn  = base_n ^ label_n;
      const int from_p = _array[to_pn].check;
      const int base_p = _array[from_p].base;
      const bool flag // whether to replace siblings of newly added
        = _consult (base_n, base_p, _ninfo[from_n].child, _ninfo[from_p].child);
      ukey_type child[MAX_KEY_CODE];
      ukey_type* const first = &child[0];
      ukey_type* const last  =
        flag ? _set_child (first, base_n, _ninfo[from_n].child, label_n)
        : _set_child (first, base_p, _ninfo[from_p].child);
      const int base =
        (first == last ? _find_place () : _find_place (first, last)) ^ *first;
      // replace & modify empty list
      const int from  = flag ? static_cast <int> (from_n) : from_p;
      const int base_ = flag ? base_n : base_p;
      if (flag && *first == label_n) _ninfo[from].child = label_n; // new child
      _array[from].base = base; // new base
      for (const ukey_type* p = first; p <= last; ++p) { // to_ => to
        const int to  = _pop_enode (base, *p, from);
        const int to_ = base_ ^ *p;
        _ninfo[to].sibling = (p == last ? 0 : *(p + 1));
        if (flag && to_ == to_pn) continue; // skip newcomer (no child)
        node& n  = _array[to];
        node& n_ = _array[to_];
        if ((n.base = n_.base) > 0 && *p) { // copy base; bug fix
          ukey_type c = _ninfo[to].child = _ninfo[to_].child;
          do _array[n.base ^ c].check = to; // adjust grand son's check
          while ((c = _ninfo[n.base ^ c].sibling));
        }
        if (! flag && to_ == static_cast <int> (from_n)) // parent node moved
          from_n = static_cast <size_t> (to); // bug fix
        if (! flag && to_ == to_pn) { // the address is immediately used
          _push_sibling (from_n, to_pn ^ label_n, label_n);
          _ninfo[to_].child = 0; // remember to reset child
          if (label_n) n_.base = -1; else n_.value = value_type (0);
          n_.check = static_cast <int> (from_n);
        } else
          _push_enode (to_);
      }
      return flag ? base ^ label_n : to_pn;
    }
  };
}
#endif
