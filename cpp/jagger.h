// Jagger -- deterministic pattern-based Japanese tagger
//  $Id: jagger.h 2028 2023-01-30 05:39:25Z ynaga $
// Copyright (c) 2022 Naoki Yoshinaga <ynaga@iis.u-tokyo.ac.jp>
#ifndef JAGGER_H
#define JAGGER_H
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <err.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iterator>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include "ccedar_core.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define NUM_POS_FIELD 4
#endif

static const size_t BUF_SIZE = 1 << 18;
static const size_t CP_MAX   = 0x10ffff;
static const size_t MAX_PLEN = 1 << 6;

static const char* FEAT_UNK = "\x09\xE5\x90\x8D\xE8\xA9\x9E\x2C\xE6\x99\xAE\xE9\x80\x9A\xE5\x90\x8D\xE8\xA9\x9E\x2C\x2A\x2C\x2A";

// compute length of UTF8 character *p
static inline int u8_len (const char *p) {
  static const uint8_t u8bytes[256] = { // must be static to tame compilers
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
  };
  return u8bytes[static_cast <uint8_t> (*p)];
}

// examine UTF8 sequence p consist of only num / alpha / kana characters
static inline int char_type (const char* p, const char* end, const ccedar::da <char, int>& chars) {
  int b (u8_len (p)), n (chars.exactMatchSearch <int> (p, b));
  if (n == -1) return 3;
  while ((p += b) != end)
    if (chars.exactMatchSearch <int> (p, b = u8_len (p)) != n) return 3;
  return n;
}

// convert UTF-8 char to code point
static inline int unicode (const char* p, int& b) {
  const unsigned char *p_ = reinterpret_cast <const unsigned char*> (p);
  const int p0 (p_[0]), p1 (p_[1]), p2 (p_[2]), p3 (p_[3]);
  switch (b = u8_len (p)) {
    case 1: return   p0 & 0x7f;
    case 2: return ((p0 & 0x1f) << 6)  |  (p1 & 0x3f);
    case 3: return ((p0 & 0xf)  << 12) | ((p1 & 0x3f) << 6)  |  (p2 & 0x3f);
    case 4: return ((p0 & 0x7)  << 18) | ((p1 & 0x3f) << 12) | ((p2 & 0x3f) << 6)  | (p3 & 0x3f);
    default: errx (1, "UTF-8 decode error: %s", p);
  }
  return 0;
}

static const char* skip_to (const char* p, const size_t n, const char c) {
  for (size_t i = 0; i < n; ++i, ++p)
    while (*p != c && *p != '\n') ++p;
  return p;
}

class sbag_t {
private:
  ccedar::da <char, int>    _str2id;
  std::vector <std::string> _id2str;
public:
  sbag_t () : _str2id (), _id2str () {}
  sbag_t (const char *f) : _str2id (), _id2str () { to_i (f, std::strlen (f)); }
  ~sbag_t () {}
  const std::string& to_s (const size_t fi) const { return _id2str[fi]; }
  size_t to_i (const std::string& f) { return to_i (f.c_str (), f.size ()); }
  size_t to_i (const char *f, const size_t len) {
    int &n = _str2id.update (f, len);
    if (n) return n - 1;
    _id2str.push_back (std::string (f, len));
    return static_cast <size_t> ((n = static_cast <int> (_id2str.size ())) - 1);
  }
  int find (const char* f, const size_t len) const
  { return _str2id.exactMatchSearch <int> (f, len); }
  size_t size () const { return _id2str.size (); }
  void serialize (std::vector <char>& ret, std::vector <size_t>& offsets) {
    for (std::vector <std::string>::const_iterator it = _id2str.begin ();
         it != _id2str.end (); ++it) {
      const uint16_t len = static_cast <uint16_t> (it->size ());
      size_t offset = ret.size ();
      offsets.push_back (offset);
#ifdef USE_COMPACT_DICT
      ret.resize (offset + sizeof (uint16_t) + len);
      std::memcpy (&ret[offset], &len, sizeof (uint16_t));
      offset += sizeof (uint16_t);
#else
      ret.resize (offset + len);
#endif
      std::memcpy (&ret[offset], it->c_str (), len);
    }
  }
};

class simple_reader {
private:
  const int _fd;
  char*  _buf;
  size_t _start, _end, _size, _capacity; // ..._start..._end..._size..._capacity
public:
  simple_reader (const char* fn = 0, size_t size = BUF_SIZE) : _fd (fn ? ::open (fn, O_RDONLY) : 0), _buf (static_cast <char*> (std::malloc (sizeof (char) * size))), _start (0), _end (0), _size (::read (_fd, _buf, size)), _capacity (size)
  { if (_fd == -1) std::free (_buf), errx (1, "no such file: %s", fn); }
  ~simple_reader () { std::free (_buf); }
  size_t gets (char** line) {
    if (! _size) return 0;
    do { // search '\n' in the buffer
      if (void *p = std::memchr (_buf + _end, '\n', _size - _end)) {
        *line = _buf + _start;
        _start = _end = static_cast <char*> (p) - _buf + 1;
        return _buf + _end - *line;
      }
      _end = _size - _start;
      if (_start) { // prepare space for loading more data
        std::memmove (_buf, _buf + _start, _size - _start);
        _size -= _start; _start = 0;
      } else // buffer is too short to read a single line
        _buf = static_cast <char*> (std::realloc (_buf, _capacity <<= 1));
      if (size_t size = ::read (_fd, _buf + _size, _capacity - _size)) {
        _size += size; // read some data
      } else { // EOF or premature INPUT
        *line = _buf + _start;
        _size = 0; // end loop
        return _end - _start;
      }
    } while (1);
  }
};
#endif
