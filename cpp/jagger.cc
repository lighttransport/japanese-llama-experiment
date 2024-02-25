// Jagger -- deterministic pattern-based Japanese tagger
//  $Id: jagger.cc 2031 2023-02-17 21:47:05Z ynaga $
// Copyright (c) 2022 Naoki Yoshinaga <ynaga@iis.u-tokyo.ac.jp>
#include "jagger.h"

static const size_t MAX_KEY_BITS     = 14;
static const size_t MAX_FEATURE_BITS = 7;

namespace ccedar {
  class da_ : public ccedar::da <int, int, MAX_KEY_BITS> {
  public:
    struct utf8_feeder { // feed one UTF-8 character by one while mapping codes
      const char *p, * const end;
      utf8_feeder (const char *key_, const char *end_) : p (key_), end (end_) {}
      int read (int &b) const { return p == end ? 0 : unicode (p, b); }
      void advance (const int b) { p += b; }
    };
    int longestPrefixSearchWithPOS (const char* key, const char* const end, int fi_prev, const uint16_t* const c2i, size_t from = 0) const {
      size_t from_ = 0;
      int n (0), i (0), b (0);
      for (utf8_feeder f (key, end); (i = c2i[f.read (b)]); f.advance (b)) {
        size_t pos = 0;
        const int n_ = traverse (&i, from, pos, pos + 1);
        if (n_ == CEDAR_NO_VALUE) continue;
        if (n_ == CEDAR_NO_PATH)  break;
        from_ = from;
        n = n_;
      }
      // ad-hock matching at the moment; it prefers POS-ending patterns
      if (! fi_prev) return n;
      for (const node* const array_ = reinterpret_cast <const node*> (array ());
           ; from = array_[from].check) { // hopefully, in the cache
        const int n_ = exactMatchSearch <int> (&fi_prev, 1, from);
        if (n_ != CEDAR_NO_VALUE) return n_;
        if (from == from_)        return n;
      }
    }
  };
}

namespace jagger {
  class tagger {
  private:
    ccedar::da_ da;
    uint16_t* c2i; // mapping from utf8, BOS, unk to character ID
    uint64_t* p2f; // mapping from pattern ID to feature strings
    char*     fs;  // feature strings
    std::vector <std::pair <void*, size_t> > mmaped;
    static inline void write_string (char* &p, const char* s, size_t len = 0) {
#ifdef USE_COMPACT_DICT
      if (! len) {
        len = *reinterpret_cast <const uint16_t*> (s);
        s += sizeof (uint16_t);
      }
#endif
      std::memcpy (p, s, len);
      p += len;
    }
    static inline void write_buffer (char* &p, char* buf, const size_t limit) {
      if (p - buf <= limit) return;
      ::write (1, buf, static_cast <size_t> (p - buf));
      p = buf;
    }
    template <typename T>
    static inline void write_array (T& data, const std::string& fn) {
      FILE *fp = std::fopen (fn.c_str (), "wb");
      if (! fp) errx (1, "no such file: %s", fn.c_str ());
      std::fwrite (&data[0], sizeof (typename T::value_type), data.size (), fp);
      std::fclose (fp);
    }
    void* read_array (const std::string& fn, size_t &nbytes) {
      int fd = ::open (fn.c_str (), O_RDONLY);
      if (fd == -1) errx (1, "no such file: %s", fn.c_str ());
      // get size and read;
      const size_t size = ::lseek (fd, 0, SEEK_END);
      ::lseek (fd, 0, SEEK_SET);
      void *data = ::mmap (0, size, PROT_READ, MAP_SHARED, fd, 0);
      ::close (fd);
      mmaped.push_back (std::make_pair (data, size));
      nbytes = size;
      return data;
    }
  public:
    tagger () : da (), c2i (0), p2f (0), fs (0), mmaped () {}
    ~tagger () {
      for (size_t i = 0; i < mmaped.size (); ++i)
        ::munmap (mmaped[i].first, mmaped[i].second);
    }
    void read_model (const std::string& m) { // read patterns to memory
      const std::string da_fn (m + ".da"), c2i_fn (m + ".c2i"), p2f_fn (m + ".p2f"), fs_fn (m + ".fs");
      struct stat st;
      if (::stat (da_fn.c_str (), &st) != 0) { // compile
        std::fprintf (stderr, "building DA trie from patterns..");
        std::vector <uint16_t> c2i_; // mapping from utf8, BOS, unk to char ID
        std::vector <uint64_t> p2f_; // mapping from pattern ID to feature str
        std::vector <char>      fs_; // feature strings
        sbag_t fbag ("\tBOS");
#ifdef USE_COMPACT_DICT
        fbag.to_i (FEAT_UNK);
        sbag_t fbag_ (",*,*,*\n");
#else
        sbag_t fbag_ ((std::string (FEAT_UNK) + ",*,*,*\n").c_str ());
#endif
        std::map <uint64_t, int> fs2pid;
        fs2pid.insert (std::make_pair ((1ul << 32) | 2, fs2pid.size ()));
        p2f_.push_back ((1ul << 32) | 2);
        // count each character to obtain dense mapping
        std::vector <std::pair <size_t, int> > counter (CP_MAX + 3);
        for (int u = 0; u < counter.size (); ++u) // allow 43 bits for counting
          counter[u] = std::make_pair (0, u);
        std::vector <std::pair <std::string, uint64_t > > keys;
        char *line = 0;
        simple_reader reader (m.c_str ());
        while (const size_t len = reader.gets (&line)) { // find pos offset
          // pattern format: COUNT PATTEN PREV_POS BYTES CHAR_TYPE FEATURES
          char *p (line), * const p_end (p + len);
          const size_t count = std::strtoul (p, &p, 10);
          const char *pat = ++p;
          for (int b = 0; *p != '\t'; p += b)
            counter[unicode (p, b)].first += count + 1;
          size_t fi_prev = 0;
          const char* f_prev = p; // starting with '\t'
          if (*++p != '\t') { // with pos context
            p = const_cast <char*> (skip_to (p, 1, '\t')) - 1;
            fi_prev = fbag.to_i (f_prev, p - f_prev) + 1;
            if (fi_prev + CP_MAX == counter.size ()) // new part-of-speech
              counter.push_back (std::make_pair (0, (fi_prev + CP_MAX)));
            counter[fi_prev + CP_MAX].first += count + 1;
          }
          const size_t bytes = std::strtoul (++p, &p, 10);
          const size_t ctype = std::strtoul (++p, &p, 10);
          const char* f = p; // starting with '\t'
          p = const_cast <char*> (skip_to (p, NUM_POS_FIELD, ',')) - 1;
          const size_t fi_  = fbag.to_i  (f, p - f) + 1;
#ifndef USE_COMPACT_DICT
          p = const_cast <char*> (f);
#endif
          const size_t fi = fbag_.to_i (p, p_end - p) + 1;
          if (fi_ + CP_MAX == counter.size ()) // new part-of-speech
            counter.push_back (std::make_pair (0, fi_ + CP_MAX));
          std::pair <std::map <uint64_t, int>::iterator, bool> itb
            = fs2pid.insert (std::make_pair ((fi << 32) | fi_, fs2pid.size ()));
          if (itb.second) p2f_.push_back ((fi << 32) | fi_);
          keys.push_back (std::make_pair (std::string (pat, f_prev - pat),
                                          (((bytes << 23) | ((ctype & 0x7) << 20) | (itb.first->second & 0xfffff)) << 12) | fi_prev));
        }
        // save c2i
        std::sort (counter.begin () + 1, counter.end (), std::greater <std::pair <size_t, int> > ());
        c2i_.resize (counter.size ());
        for (unsigned int i = 1; i < counter.size () && counter[i].first; ++i)
          c2i_[counter[i].second] = static_cast <uint16_t> (i);
        // save feature strings
        std::vector <size_t> offsets;
#ifdef USE_COMPACT_DICT
        fbag.serialize  (fs_, offsets); // required only for compact dict
#endif
        fbag_.serialize (fs_, offsets);
        write_array (fs_, fs_fn);
        // save mapping from morpheme ID to morpheme feature strings
        for (size_t i = 0; i < p2f_.size (); ++i) {
#ifdef USE_COMPACT_DICT
          p2f_[i] = (offsets[(p2f_[i] >> 32) - 1 + fbag.size ()] << 34) |
                    (offsets[(p2f_[i] & 0xffffffff) - 1] << MAX_KEY_BITS) |
#else
          const std::string& f = fbag_.to_s ((p2f_[i] >> 32) - 1);
          const char* q = skip_to (f.c_str (), NUM_POS_FIELD, ',') - 1;
          p2f_[i] = (offsets[(p2f_[i] >> 32) - 1] << 34) |
                    (fbag_.to_s ((p2f_[i] >> 32) - 1).size () << (MAX_KEY_BITS + MAX_FEATURE_BITS)) |
                    (q - f.c_str ()) << MAX_KEY_BITS |
#endif
                    c2i_[(p2f_[i] & 0xffffffff) + CP_MAX];
        }
        write_array (p2f_, p2f_fn);
        // save pattern trie
        for (std::vector <std::pair <std::string, uint64_t> >::const_iterator it = keys.begin (); it != keys.end (); ++it) {
          std::vector <int> key;
          for (int offset (0), b (0); offset < it->first.size (); offset += b)
            key.push_back (c2i_[unicode (&it->first[offset], b)]);
          if (it->second & 0xfff)
            key.push_back (c2i_[(it->second & 0xfff) + CP_MAX]);
          da.update (&key[0], key.size ()) = it->second >> 12;
        }
        c2i_.resize (CP_MAX + 2); // chop most of part-of-speech mapping
        write_array (c2i_, c2i_fn);
        da.save (da_fn.c_str ());
        std::fprintf (stderr, "done.\n");
      }
      size_t buf_nbytes;
      const void *buf_ptr = read_array(da_fn, buf_nbytes);

      da.set_array (buf_ptr, buf_nbytes);
      c2i = static_cast <uint16_t*> (read_array (c2i_fn, buf_nbytes));
      p2f = static_cast <uint64_t*> (read_array (p2f_fn, buf_nbytes));
      fs  = static_cast <char*> (read_array (fs_fn, buf_nbytes));
    }
    template <const int BUF_SIZE_, const bool POS_TAGGING>
    void run () const {
      if (BUF_SIZE_ == 0) std::fprintf (stderr, "(input: stdin)\n");
      char _res[BUF_SIZE], *_ptr (&_res[0]), *line (0);
      simple_reader reader;
      while (const size_t len = reader.gets (&line)) {
        int bytes (0), bytes_prev (0), id (0), ctype (0), ctype_prev (0);
        uint64_t offsets = c2i[CP_MAX + 1];
        bool bos (true), ret (line[len - 1] == '\n'), concat (false);
        for (const char *p (line), * const p_end (p + len - ret); p != p_end; bytes_prev = bytes, ctype_prev = ctype, offsets = p2f[static_cast <size_t> (id)], p += bytes) {
          const int r = da.longestPrefixSearchWithPOS (p, p_end, offsets & 0x3fff, &c2i[0]); // found word
          id    = r & 0xfffff;
          bytes = (r >> 23) ? (r >> 23) : u8_len (p);
          ctype = (r >> 20) & 0x7; // 0: num|unk / 1: alpha / 2: kana / 3: other
          if (! bos) { // word that may concat with the future context
            if (ctype_prev != ctype || // different character types
                ctype_prev == 3 ||     // seen words in non-num/alpha/kana
                (ctype_prev == 2 && bytes_prev + bytes >= 18)) {
              if (POS_TAGGING) {
#ifdef USE_COMPACT_DICT
                write_string (_ptr, &fs[((offsets >> MAX_KEY_BITS) & 0xfffff)]);
                if (concat)
                  write_string (_ptr, ",*,*,*\n", 7);
                else
                  write_string (_ptr, &fs[(offsets >> 34)]);
#else
                if (concat) {
                  write_string (_ptr, &fs[(offsets >> 34)], (offsets >> MAX_KEY_BITS) & 0x7f);
                  write_string (_ptr, ",*,*,*\n", 7);
                } else
                  write_string (_ptr, &fs[(offsets >> 34)], (offsets >> (MAX_KEY_BITS + MAX_FEATURE_BITS)) & 0x3ff);
#endif
                concat = false;
              } else
                write_string (_ptr, " ", 1);
            } else
              concat = true;
          } else
            bos = false;
          write_string (_ptr, p, static_cast <size_t> (bytes));
        }
        if (! bos) // output fs of last token
          if (POS_TAGGING) {
#ifdef USE_COMPACT_DICT
            write_string (_ptr, &fs[((offsets >> MAX_KEY_BITS) & 0xfffff)]);
            if (concat)
              write_string (_ptr, ",*,*,*\n", 7);
            else
              write_string (_ptr, &fs[(offsets >> 34)]);
#else
            if (concat) {
              write_string (_ptr, &fs[(offsets >> 34)], (offsets >> MAX_KEY_BITS) & 0x7f);
              write_string (_ptr, ",*,*,*\n", 7);
            } else
              write_string (_ptr, &fs[(offsets >> 34)], (offsets >> (MAX_KEY_BITS + MAX_FEATURE_BITS)) & 0x3ff);
#endif
          }
        write_string (_ptr, POS_TAGGING ? "EOS\n" : "\n", POS_TAGGING ? 4 : 1);
        write_buffer (_ptr, &_res[0], BUF_SIZE_);
      }
      write_buffer (_ptr, &_res[0], 0);
    }
  };
}

#if 0
int main (int argc, char** argv) {
  std::string model (JAGGER_DEFAULT_MODEL "/patterns");
  bool tag (true), fbf (false);
  { // options (minimal)
    extern char *optarg;
    for (int opt = 0; (opt = getopt (argc, argv, "m:wfh")) != -1;)
      switch (opt) {
        case 'm': model = optarg; model += "/patterns"; break;
        case 'w': tag = false; break;
        case 'f': fbf = true;  break;
        case 'h':
          errx (1, "Pattern-based Jappanese Morphological Analyzer\nUsage: %s -m dir [-wf] < input\n\nOptions:\n -m dir\tpattern directory (default: " JAGGER_DEFAULT_MODEL ")\n -w\tperform only segmentation\n -f\tfull buffering (fast but not interactive)", argv[0]);
      }
  }
  jagger::tagger jagger;
  jagger.read_model (model);
  switch ((fbf << 4) | tag) {
    case 0x00: jagger.run <0, false> (); break;
    case 0x01: jagger.run <0, true> (); break;
    case 0x10: jagger.run <(BUF_SIZE >> 1), false> (); break;
    case 0x11: jagger.run <(BUF_SIZE >> 1), true> (); break;
  }
  return 0;
}
#endif
