// pecco -- please enjoy classification with conjunctive features
//  $Id: classify.h 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef CLASSIFY_H
#define CLASSIFY_H

#include <sys/stat.h>
#include <getopt.h>
#include <err.h>
#include <cmath>
#include <cassert>
#include <climits>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include "typedef.h"
#include "timer.h"

#define PECCO_COPYRIGHT  "pecco - please enjoy classification w/ conjunctive features\n\
Copyright (c) 2008-2015 Naoki Yoshinaga, All rights reserved.\n\
\n\
Usage: %s [options] model [test]\n\
\n\
model   model file             model\n\
test    test file              test examples; read STDIN if omitted\n\
\n"

#define PECCO_OPT0 "Optional parameters:\n\
  -t, --classifier=TYPE        select classifier type\n\
                                 0 - PKI (kernel model only)\n"

#if   defined (USE_PKE_AS_DEFAULT)
#define PECCO_OPT1 "                               * 1 - PKE | SPLIT\n\
                                 2 - FST\n"
#elif defined (USE_FST_AS_DEFAULT)
#define PECCO_OPT1 "                                 1 - PKE | SPLIT\n\
                               * 2 - FST\n"
#else
#define PECCO_OPT1 "                                 1 - PKE | SPLIT\n\
                                 2 - FST\n"
#endif

#ifdef USE_CEDAR
#if defined (USE_PMT_AS_DEFAULT)
#define PECCO_OPT2 "                               * 3 - PMT\n\
  -p, --pmsize=INT             set upper limit of partial margins (1 << INT; default: 20)\n"
#else
#define PECCO_OPT2 "                                 3 - PMT\n\
  -p, --pmsize=INT             set upper limit of partial margins (1 << INT; default: 20)\n"
#endif
#else
#define PECCO_OPT2
#endif

#define PECCO_OPT3 "  -e, --event=FILE             examples to obtain feature count for reordering ("")\n\
  -f, --fst-event=FILE         examples to enumerate feature sequence in FST ("")\n\
  -i, --fst-prune-factor=INT   use FST with 2^-INT of feature sequences (0)\n\
  -b, --fst-build-verbose      build FSTs with 2^-i (i > 0) of feature sequences\n\
  -c, --force-compile          force to recompile model\n\
  -O, --output=TYPE            select output type of testing\n\
                               * 0 - report accuracy\n\
                                 1 - + labels\n\
                                 2 - + labels + classifier outputs\n\
                                 3 - + labels + probabilities\n\
  -o, --output!=TYPE           output examples with labels/margins\n\
\n"

#define PECCO_OPT_KERNEL "Optional parameters in kernel model:\n\
  -m, --pke-minsup=INT         threshold to feature frequency (1)\n\
  -s, --pke-sigma=FLOAT        threshold to feature weight (0)\n\
  -r, --split-ratio=FLOAT      threshold to feature frequency ratio (0)\n\
\n"

#define PECCO_OPT_MISC "Misc.:\n\
  -v, --verbose=INT            verbosity level (0)\n\
  -h, --help                   show this help and exit\n"

static const  char* pecco_short_options = "t:e:p:f:i:bcO:o:m:s:r:v:h";
static struct option pecco_long_options[] = {
  {"classifier type",   required_argument, NULL, 't'},
  {"event",             required_argument, NULL, 'e'},
  {"pmt-size",          required_argument, NULL, 'p'},
  {"fst-event",         required_argument, NULL, 'f'},
  {"fst-prune_ratio",   required_argument, NULL, 'i'},
  {"fst-build-verbose", no_argument,       NULL, 'b'},
  {"force-compile",     no_argument,       NULL, 'c'},
  {"output",            required_argument, NULL, 'O'},
  {"output!",           required_argument, NULL, 'o'},
  {"pke-minsup",        required_argument, NULL, 'm'},
  {"pke-sigma",         required_argument, NULL, 's'},
  {"split-ratio",       required_argument, NULL, 'r'},
  {"verbose",           required_argument, NULL, 'v'},
  {"help",              no_argument,       NULL, 'h'},
  {NULL, 0, NULL, 0}
};

extern char* optarg;
extern int    optind;

namespace pecco {
  enum model_t  { KERNEL, LINEAR };
#ifdef USE_CEDAR
  enum algo_t   { PKI, PKE, FST, PMT };
#else
  enum algo_t   { PKI, PKE, FST };
#endif
  enum binary_t { BINARY = true, MULTI = false };
  enum output_t { NONE = 0, LABEL = 1, SCORE = 2, PROB = 3 };
  // str to numerical wrapper
  template <typename T> T strton (const char* s, char** error);
  template <typename T> T strton (const char* s);
  // options
  struct option { // option handler
    const char* com, *train, *test, *model, *event;
    //
    const char*  minsup;
    const char*  sigma;
    const char*  fratio;
    model_t      type;    // model-type
    algo_t       algo;
    int          output;
    uint8_t      fst_factor;
    bool         fst_verbose;
    bool         force;
    uint8_t      verbose;
    size_t       pmsize;
    option () : com ("--"), train (0), test (0), model (0), event (""), minsup ("1"), sigma ("0"), fratio ("0"), type (KERNEL), algo (DEFAULT_CLASSIFIER),
                output (0), fst_factor (0), fst_verbose (false), force (false), verbose (0), pmsize (20)  {}
    option (int argc, char ** argv) : com (argc ? argv[0] : "--"), train (0), test (0), model (""), event (""), minsup ("1"), sigma ("0"), fratio ("0"), type (KERNEL), algo (DEFAULT_CLASSIFIER), output (NONE),  fst_factor (0), fst_verbose (false), force (false), verbose (0), pmsize (20) {
      set (argc, argv);
    }
    void set (int argc, char ** argv) { // getOpt
      if (argc == 0) return;
      optind = 1;
      size_t minsup_ = 1;
      double sigma_ (0.0), fratio_ (0.0);
      while (1) {
        int opt = getopt_long (argc, argv,
                               pecco_short_options, pecco_long_options, NULL);
        if (opt == -1) break;
        char* err = NULL;
        switch (opt) {
          case 't': algo   = strton <algo_t>   (optarg, &err); break;
          case 'o': output = 0x100;
          case 'O': output |= strton <int> (optarg, &err); break;
          case 'e': train  = optarg; break;
          case 'f': event  = optarg; break;
          case 'm': minsup = optarg; minsup_ = strton <size_t> (optarg, &err); break;
#ifdef USE_FLOAT
          case 's': sigma  = optarg; sigma_  = std::strtod (optarg, &err); break;
          case 'r': fratio = optarg; fratio_ = std::strtod (optarg, &err); break;
#else
          case 's': sigma  = optarg; sigma_  = std::strtod (optarg, &err); break;
          case 'r': fratio = optarg; fratio_ = std::strtod (optarg, &err); break;
#endif
          case 'p': pmsize = strton <size_t>  (optarg, &err); break;
          case 'i': fst_factor  = strton <uint8_t> (optarg, &err); break;
          case 'b': fst_verbose = true; break;
          case 'c': force       = true; break;
          case 'v': verbose     = strton <uint8_t> (optarg, &err); break;
          case 'h': printCredit (); printHelp (); std::exit (0);
          default:  printCredit (); std::exit (0);
        }
        if (err && *err)
          errx (1, HERE "unrecognized option value: %s", optarg);
      }
      // errors & warnings
#ifdef USE_CEDAR
      if (algo != PKE && algo != FST && algo != PMT && algo != PKI)
#else
      if (algo != PKE && algo != FST && algo != PKI)
#endif
        errx (1, HERE "unknown classifier type [-t].");
      if (algo == PKI) {
        if (minsup_ != 1)
          errx (1, HERE "PKE minsup [-m] must be 0 in PKI [-t 0].");
        if (sigma_ != 0.0)
          errx (1, HERE "PKE simga [-s] must be 0 in PKI [-t 0].");
        if (fratio_ != 0.0)
          errx (1, HERE "SPLIT ratio [-r] must be 0 in PKI [-t 0].");
      }
      if (std::strcmp (argv[0], "--") == 0) return; // skip
      if (argc < optind + 1) {
        printCredit ();
        errx (1, HERE "Type `%s --help' for option details.", com);
      }
#ifndef USE_MODEL_SUFFIX
      if (fst_verbose)
        errx (1, HERE "[-b] building multiple FSTs are useless since model suffix disabled.");
#endif
      model = argv[optind];
      setType ();
      if (type == LINEAR) {
        if (algo == PKI)
          errx (1, HERE "PKI [-t 0] is not available for LLM.");
        if (minsup_ != 1)
          warnx ("WARNING: PKE minsup [-m] is ignored in LLM.");
        if (sigma_ != 0.0)
          warnx ("WARNING: PKE sigma [-s] is ignored in LLM.");
        if (fratio_ != 0.0)
          warnx ("WARNING: SPLIT ratio [-r] is ignored in LLM.");
      }
#ifdef USE_CEDAR
      if (algo == PMT) {
        if (pmsize <= 0)
          errx (1, HERE "PMT [-t 3] requires > 0 size [-p]");
      } else pmsize = 0;
#endif
      if (algo == FST && std::strcmp (event, "") == 0)
        errx (1, HERE "FST [-t 2] requires possible examples [-f];\n (you can use the training examples)");
      if (++optind != argc) test = argv[optind];
    }
    void setType () {
      FILE* fp = std::fopen (model, "r");
      if (! fp || std::feof (fp))
        errx (1, HERE "no model found: %s; train a model first", model);
      switch (std::fgetc (fp)) {
        case 'o': // opal;      delegate
        case 's': // LIBSVM;    delegate
        case 'S': // SVM-light; delegate
        case 'T': type = KERNEL;  break;
        default:  type = LINEAR;  break;
      }
      std::fclose (fp);
    }
    void printCredit ()
    { std::fprintf (stderr, PECCO_COPYRIGHT, com); }
    void printHelp ()
    { std::fprintf (stderr, PECCO_OPT0 PECCO_OPT1 PECCO_OPT2 PECCO_OPT3 PECCO_OPT_KERNEL PECCO_OPT_MISC); }
  };
#ifdef USE_LFU_PMT
  // memory-efficient implementation of stream-summary data structure
  struct elem_t {
    int prev;
    int next;
    int link; // link to bucket / item
    elem_t (const int prev_ = -1, const int next_ = -1, const int link_ = -1)
      : prev (prev_), next (next_), link (link_) {}
  };
  struct count_t : public elem_t {
    size_t count;
    count_t () : elem_t (), count (0) {}
  };
  template <typename T>
  class ring_t {
  protected:
    T* _data;
  public:
    int& prev (const int n) const { return _data[n].prev; }
    int& next (const int n) const { return _data[n].next; }
    int& link (const int n) const { return _data[n].link; }
    void pop  (const int n) const {
      next (prev (n)) = next (n);
      prev (next (n)) = prev (n);
    }
    void push (const int n, const int prev_) const
    { next (prev (n) = prev_) = prev (next (n) = next (prev_)) = n; }
    ring_t () : _data (0) {}
  };
  class item_t : public ring_t <elem_t> {
  public:
    bool is_singleton (const int n) const
    { return prev (n) == n && next (n) == n; }
    item_t (const size_t capacity) : ring_t () { _data  = new elem_t[capacity]; }
    ~item_t () { delete [] _data; }
  };
  class bucket_t : public ring_t <count_t> {
  public:
    size_t& count (const int n) const { return _data[n].count; }
    int add_elem  () {
      if (_size == _capacity) {
        _capacity += _capacity;
        void* tmp = std::realloc (_data, sizeof (count_t) * _capacity);
        if (! tmp)
          std::free (_data), errx (1, HERE "memory reallocation failed");
        _data = static_cast <count_t*> (tmp);
      }
      return _size++;
    }
    void pop_item  (item_t& _item, const int m, const int n) const {
      _item.pop (n);
      if (link (m) == n) link (m) = _item.next (n);
    }
    void push_item (item_t& _item, const int m, const int n) const  {
      _item.push (n, link (m));
      _item.link (n) = m;
    }
    void set_child (item_t& _item, const int m, const int n) const {
      _item.next (n) = _item.prev (n) = n; // singleton
      _item.link (n) = m;
    }
    bucket_t () : ring_t <count_t> (), _size (0), _capacity (1) {
      _data = static_cast <count_t*> (std::malloc (sizeof (count_t)));
      prev (0) = next (0) = 0;
    }
    ~bucket_t () { std::free (_data); }
  private:
    int _size;
    int _capacity;
  };
  // eco-friendly recycling plant for bucket objects
  class bucket_pool {
  public:
    void recycle_bucket (bucket_t& _bucket, const int m)
    { _bucket.next (m) = _M; _M = m; }
    int create_bucket (bucket_t& _bucket) {
      if (_M == -1) return _bucket.add_elem ();
      const int m = _M;
      _M = _bucket.next (_M);
      return m;
    }
    bucket_pool () : _M (-1) {}
    ~bucket_pool () {}
  private:
    int _M;
    bucket_pool (const bucket_pool&);
    bucket_pool& operator= (const bucket_pool&);
  };
#else
  struct elem_t {
    int prev;
    int next;
    elem_t (const int prev_ = -1, const int next_ = -1)
      : prev (prev_), next (next_) {}
  };
  class ring_t { // linked list
  private:
    elem_t* _data;
    int     _head;
    int     _nelm;
    int     _capacity;
  public:
    int& prev (const int n) const { return _data[n].prev; }
    int& next (const int n) const { return _data[n].next; }
    int  get_element () { // return least recently used
      int n = 0;
      if (_nelm == _capacity) { // full
        n = _head;
        _head = next (_head);
      } else {
        if (_nelm) {
          const int tail = prev (_head);
          prev (_nelm) = tail;
          next (_nelm) = _head;
          prev (_head) = next (tail) = _nelm;
        } else {
          _head = 0;
          prev (_head) = next (_head) = _head;
        }
        n = _nelm;
        ++_nelm;
      }
      return n;
    }
    void move_to_back (const int n) { // bad?
      if (n == _head) {
        _head = next (n);
      } else { // pop and move
        next (prev (n)) = next (n);
        prev (next (n)) = prev (n);
        int& tail = prev (_head);
        prev (n) = tail;
        next (n) = _head;
        tail = next (tail) = n;
      }
    }
    ~ring_t () { delete [] _data; }
    ring_t (int capacity) : _data (new elem_t[capacity]), _head (-1), _nelm (0), _capacity (capacity) {}
  };
#endif
  // \sum_{i=0}^{k} nCk * num_class is assigned to an array-based pseudo trie
#ifdef USE_ARRAY_TRIE
  static const size_t PSEUDO_TRIE_N[] = {0, 21, 11, 8, 6};
#else
  static const size_t PSEUDO_TRIE_N[] = {0, 0, 0, 0, 0};
#endif
  // type alias
  // uchar * -> fl_t  ; conj. feat. -> weight (float)
  typedef ny::TrieKeyBase  <ny::uchar, ny::fl_t> FeatKey;
  typedef ny::TrieKeypLess <ny::uchar, ny::fl_t> FeatKeypLess;
  // uchar * -> fl_t  ; feat. seq.  -> weight (float) or weight ID (int)
  struct FstKey : public FeatKey {
    size_t   weight; // used for node cutoff
    ny::uint count;  // used for node cutoff
    bool     leaf;
    FstKey () : FeatKey (), weight (0), count (0), leaf (false) {}
    FstKey (ny::uchar* k, ny::fl_t* c, size_t l, ny::uint nl)
      : FeatKey (k, c, l, nl), weight (0), count (0), leaf (false) {}
    bool is_prefix (FstKey *a) const { // whether key is a->key's prefix
      if (len > a->len) return false;
      for (size_t i = 0; i < len; ++i)
        if (key[i] != a->key[i]) return false;
      return true;
    }
  };
  // feature sequence ordering; see Yoshinaga and Kitsuregawa (EMNLP 2009)
  struct FstKeypLess {
    bool operator () (const FstKey* a, const FstKey* b) const {
      // keep frequent / long keys
      if (a->count * a->weight < b->count * b->weight)
        return true;
      else if (a->count * a->weight > b->count * b->weight)
        return false;
      // keep keys with frequent features
      else return FeatKeypLess () (b, a);
      // return (std::memcmp (a->key, b->key, a->len) > 0);
    }
  };
  // int <-> uint <-> float converter
  union byte_4 {
    int i;
    ny::uint u;
    float f;
    byte_4 (int n)   : i (n) {}
    byte_4 (float b) : f (b) {}
  };
  // build / save da at once
  static inline void build_trie (ny::trie* da,
                                 const std::string& name,
                                 const std::string& fn,
                                 std::vector <const char*>& str,
                                 const std::vector <size_t>& len,
                                 const std::vector <ny::trie::result_type>& val,
                                 bool flag, const char* mode = "wb") {
    if (flag) std::fprintf (stderr, " building %s..", name.c_str ());
    if (da->build (str.size (), &str[0], &len[0], &val[0]) != 0 ||
        da->save  (fn.c_str (), mode) != 0 ) {
      errx (1, HERE "failed to build %s trie.", name.c_str ());
    }
    if (flag) std::fprintf (stderr, "done.\n");
  }
  // check file refreshness
  static inline bool newer (const char* newer, const char* base) {
    struct stat newer_fs, base_fs;
    if (stat (newer, &newer_fs) == -1) return false;
    stat (base,  &base_fs); // need not exist
    return difftime (newer_fs.st_mtime, base_fs.st_mtime) >= 0;
  }
  // bytewise coding (cf. Williams & Zobel, 1999)
  static const size_t KEY_SIZE = 8; // >= log_{2^7} _nf + 1
  class byte_encoder {
  public:
    byte_encoder () : _len (0), _key () {}
    byte_encoder (ny::uint i) : _len (0), _key () { encode (i); }
    ny::uint encode (ny::uint i, ny::uchar* const key_) const {
      ny::uint len_ = 0;
      for (key_[len_] = (i & 0x7f); i >>= 7; key_[++len_] = (i & 0x7f))
        key_[len_] |= 0x80;
      return ++len_;
    }
    void encode (const ny::uint i) { _len = encode (i, _key); }
    ny::uint decode (ny::uint& i, const ny::uchar* const key_) const {
      ny::uint b (0), len_ (0);
      for (i = key_[0] & 0x7f; key_[len_] & 0x80; i += (key_[len_] & 0x7fu) << b)
        ++len_, b += 7;
      return ++len_;
    }
    ny::uint    len () { return _len; }
    const char* key () { return reinterpret_cast <const char *> (&_key[0]); }
  private:
    ny::uint  _len;
    ny::uchar _key[KEY_SIZE];
  };
  struct pn_t {
    ny::fl_t pos;
    ny::fl_t neg;
    pn_t () : pos (0), neg (0) {}
    bool operator== (const pn_t& pn) const {
      return
        std::fpclassify (pos - pn.pos) == FP_ZERO &&
        std::fpclassify (neg - pn.neg) == FP_ZERO;
    }
  };
  //
  static const ny::uint INSERT_THRESH = 10;
  static const ny::uint NBIT = 5;
  static const ny::uint NBIN = 1 << NBIT;
  template <typename T>
  class sorter_t {
  public:
    typedef typename T::iterator Iterator;
    typedef typename std::iterator_traits <Iterator>::value_type value_type;
    sorter_t  () : _temp () {}
    ~sorter_t () {}
    // for short feature vector < 50
    void insertion_sort (const Iterator& first, const Iterator& last) {
      std::less <value_type> cmp;
      if (std::distance (first, last) <= 1) return;
      Iterator sorted = first;
      for (++sorted; sorted != last; ++sorted) {
        value_type temp = *sorted;
        Iterator prev, curr;
        prev = curr = sorted;
        for (--prev; curr != first && cmp (temp, *prev); --prev, --curr)
          *curr = *prev;
        *curr = temp;
      }
    }
    // MSD radix sort
    void radix_sort (const Iterator& first, const Iterator& last, const ny::uint shift) {
      const size_t len = static_cast <size_t> (std::distance (first, last));
      if (len <= 1) return;
      if (len < INSERT_THRESH * ((shift / NBIT) + 1)) { // may depend on depth
        insertion_sort (first, last);
      } else {
        // do radix sort
        if (_temp.size () < len) _temp.resize (len);
        int count[NBIN + 1];
        std::fill (&count[0], &count[NBIN + 1], 0);
        for (Iterator it = first; it != last; ++it)
          ++count[(*it >> shift) & (NBIN - 1)];
        for (size_t i = 1; i <= NBIN; ++i) count[i] += count[i - 1];
        for (Iterator it = first; it != last; ++it)
          _temp[static_cast <size_t> (--count[(*it >> shift) & (NBIN - 1)])] = *it;
        std::copy (&_temp[0], &_temp[len], first);
        if (! shift) return;
        for (size_t i = 0; i < NBIN; ++i)
          if (count[i + 1] - count[i] >= 2)
            radix_sort (first + count[i], first + count[i + 1], shift - NBIT);
      }
    }
    // Zipf-aware sorting funciton
    void bucket_sort (const Iterator& first, const Iterator& last, const ny::uint shift) {
      if (std::distance (first, last) <= 1) return;
      uint64_t bucket = 0;
      // assuming most feature IDs are less than sizeof (U) * 8
      Iterator it (last);
      for (Iterator jt = it; it != first; ) {
        if (*--it < sizeof (uint64_t) * 8)
          bucket |= (static_cast <uint64_t> (1) << *it); // bug in 32bit
        else // leave unsorted (large) feature IDs on tail
          *--jt = *it;
      }
      // do bucket sort
      // pick input numbers by twiddling bits
      //   cf. http://www-cs-faculty.stanford.edu/~uno/fasc1a.ps.gz (p. 8)
      while (bucket) {
        // *it = __builtin_ctzl (bucket);
        pecco::byte_4 b (static_cast <float> (bucket & - bucket)); // pull rightmost 1
        *it = (b.u >> 23) - 127; ++it; // pick it via floating number
        bucket &= (bucket - 1); // unset rightmost 1
      }
      // do radix sort for the tail
      radix_sort (it, last, shift); // for O(n) guarantee
    }
  private:
    std::vector <value_type> _temp;
  };
#ifdef USE_CEDAR
  typedef cedar::da <int, -1, -2, false>  pmtrie_t;
#endif
  template <typename T>
  class ClassifierBase : private ny::Uncopyable {
  protected:
    static const ny::uint MAX_KERNEL_DEGREE = 4;
    // type alias
    typedef ny::map <ny::uint, ny::uint>::type counter_t;
    typedef std::map <char *, ny::uint, ny::pless <char> > lmap; // unordered_map
    // options and classifier parameters
    const option           _opt;
    std::vector <double>   _score;
    ny::fv_t               _fv;
    sorter_t <ny::fv_t>    _sorter;
    ny::uint               _d;        // degree of feature combination
    ny::uint               _nl;       // # of labels
    // various sizes
    ny::uint               _nf;       // # features
    ny::uint               _nfbit;    // # features
    ny::uint               _nf_cut;   // # features (pruned)
    ny::uint               _ncf;      // # conjunctive features
    ny::uint               _nt;       // # training sample
    // for mapping label to label id
    ny::uint               _tli;      // default target label id
    std::vector <char*>    _li2l;     // label id -> label
    lmap                   _l2li;     // label -> label id
    // for mapping feature numbers to feature indices
    std::vector <ny::uint> _fn2fi;    // feature number -> feature index
    std::vector <ny::uint> _fi2fn;    // feature index  -> feature number
    counter_t              _fncnt;    // feature number -> count
    // double arrays for conjunctive features and fstrie
    ny::trie               _ftrie;    // conjunctive feature -> weight / weight_id
    ny::trie               _fstrie;   // feature sequece     -> weight / weight_id
#ifdef USE_CEDAR
    pmtrie_t               _pmtrie;   // feature sequece     -> weight / weight_id
    class func {
    public:
      int* const leaf;
      const int  size;
      void operator () (const int to_, const int to) {
        size_t from (static_cast <size_t> (to_)),  p (0);
        const int n = _trie->traverse ("", from, p, 0);
        if (n != pmtrie_t::CEDAR_NO_VALUE) leaf[n - 1] = to;
      }
      func (const int size_, pmtrie_t* const trie) : leaf (new int[size_]), size (size_), _trie (trie)
      { for (int i = 0; i < size; ++i) leaf[i] = 0; }
      ~func () { delete [] leaf; }
    private:
      pmtrie_t* _trie;
    } _pmfunc;
    int                    _pmi;
    double*                _pms;
#ifdef USE_LFU_PMT
    bucket_pool            _bucket_pool;
    bucket_t               _bucket;
    item_t                 _item;
#else
    ring_t                 _ring;
#endif
#endif
    // pruning when exact margin is unnecessary
    pn_t*                  _f2dpn;
    pn_t*                  _f2nf;
    std::vector <size_t>   _dpolyk;
    std::vector <pn_t>     _bound;
    // temporary variables
    ny::fl_t*              _fw;       // conjunctive feature id -> weight
    ny::fl_t*              _fsw;      // feature sequence id -> weight
    // timer
#ifdef USE_TIMER
    ny::TimerPool          _timer_pool;
    ny::Timer*             _enc_t;      // feature mapping
    ny::Timer*             _model_t;    // compiling/loading  model
    ny::Timer*             _classify_t; // pke classify
#endif
    // profiling
#ifdef USE_PROFILING
    unsigned long          _all;         // all       (for fst)
    unsigned long          _hit;         // hit ratio (for fst)
    unsigned long          _lookup;      // lookup    (for pke)
    unsigned long          _flen;        // traverse  (for pke)
    unsigned long          _traverse;    // traverse  (for pke)
    unsigned long          _lookup_sp;   // lookup    (for pke)
    unsigned long          _traverse_sp; // traverse  (for pke)
#endif
    T*        _derived ()             { return static_cast <T*>       (this); }
    const T*  _derived_const () const { return static_cast <const T*> (this); }
    bool isspace (const char p) const { return p == ' ' || p == '\t'; }
    size_t _cost_fun (size_t m, size_t n);
    // bytewise coding for feature indexes (cf. Williams & Zobel, 1999)
    // various functions for sorting feature (index) vectors
    template <typename Iterator>
    static void _insertion_sort (const Iterator& first, const Iterator& last);
    template <typename Iterator>
    static void _radix_sort (const Iterator& first, const Iterator& last, const ny::uint shift);
    template <typename Iterator>
    static void _bucket_sort (const Iterator& first, const Iterator& last, const ny::uint shift);
    // feature mapping
    void _sortFv (ny::fv_t& fv);
    void _convertFv2Fv (char* p, ny::fv_t& fv) const;
    void _convertFv2Fv (ny::fv_t& fv) const;
    bool _packingFeatures (std::vector <ny::fv_t>& fvv);
    // feature sequence trie builder
    bool _setFStrie ();
    // classification functions
    template <binary_t FLAG>
    void resetScore (double* score) const
    { _derived_const ()->template resetScore <FLAG> (score); }
    template <binary_t FLAG>
    void resetScore (double* score, const double* const score_) const
    { _derived_const ()->template resetScore <FLAG> (score, score_); }
    template <binary_t FLAG>
    void addScore (double* score, const ny::uint pos) const
    { _derived_const ()->template addScore <FLAG> (score, pos); }
    template <binary_t FLAG>
    void addScore (double* score, const int n, const ny::fl_t* const w) const
    { _derived_const ()->template addScore <FLAG> (score, n, w); }
    template <int D, bool PRUNE, binary_t FLAG>
    bool _pkePseudoInnerLoop (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end, const ny::uint pos);
    template <int D, bool PRUNE, binary_t FLAG>
    bool _pkeInnerLoop (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end, const size_t pos);
    template <bool PRUNE, binary_t FLAG>
    bool _pkeClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end);
    template <bool PRUNE, binary_t FLAG>
    void _pkeClassify (ny::fv_t& fv, double* score) {
      if (_d == 1) {
        _pkeClassify <false, FLAG> (score, fv.begin (), fv.begin (), fv.end ());
      } else {
        _sortFv (fv);
        if (PRUNE) _estimate_bound <FLAG> (fv.begin (), fv.begin (), fv.end ());
        _pkeClassify <PRUNE, FLAG> (score, fv.begin (), fv.begin (), fv.end ());
      }
    }
    template <bool PRUNE, binary_t FLAG>
    bool _prune (double* m, const size_t i)
    { return PRUNE && _derived ()->template prune <FLAG> (m, i); }
    template <binary_t FLAG>
    void _estimate_bound (const ny::fv_it& first, const ny::fv_it& beg, ny::fv_it it) {
      // compute bound from tail (rare) to head (common) feature
      for (size_t n = _dpolyk.size () / _d;
           n < static_cast <size_t> (std::distance (beg, it)); ++n) {
        _dpolyk.push_back (1);
        if (_d >= 2) _dpolyk.push_back (n);
        if (_d >= 3) _dpolyk.push_back (n * (n - 1) / 2);
        if (_d >= 4) _dpolyk.push_back (n * (n - 1) * (n - 2) / 6);
      }
      switch (_d) {
        case 2: _derived ()->template estimate_bound <2, FLAG> (first, beg, it); break;
        case 3: _derived ()->template estimate_bound <3, FLAG> (first, beg, it); break;
        case 4: _derived ()->template estimate_bound <4, FLAG> (first, beg, it); break;
        default: errx (1, HERE "_d = %d does not supported in pruning", _d);
      }
    }
    template <bool PRUNE, binary_t FLAG>
    void _fstClassify (double* score, const ny::fv_it& beg, const ny::fv_it& end);
    template <bool PRUNE, binary_t FLAG>
    void _fstClassify (ny::fv_t& fv, double* score) {
      _sortFv (fv);
      if (_d == 1)
        _fstClassify <false, FLAG> (score, fv.begin (), fv.end ());
      else
        _fstClassify <PRUNE, FLAG> (score, fv.begin (), fv.end ());
    }
    template <bool PRUNE, binary_t FLAG>
    void _pmtClassify (double* score, const ny::fv_it& beg, const ny::fv_it& end);
    template <bool PRUNE, binary_t FLAG>
    void _pmtClassify (ny::fv_t& fv, double* score) {
      _sortFv (fv);
      if (_d == 1)
        _pmtClassify <false, FLAG> (score, fv.begin (), fv.end ());
      else
        _pmtClassify <PRUNE, FLAG> (score, fv.begin (), fv.end ());
    }
    template <bool PRUNE, binary_t FLAG>
    void _baseClassify (double* score, ny::fv_it it, const ny::fv_it& beg, const ny::fv_it& end)
    { _derived ()->template baseClassify <PRUNE, FLAG> (score, it, beg, end); }
    bool _setOpt (const char* opt_str);
  public:
    ClassifierBase (const pecco::option& opt) :
      _opt (opt), _score (), _fv (), _sorter (), _d (0), _nl (0), _nf (0), _nfbit (0), _nf_cut (0), _ncf (0),  _nt (0), _tli (0), _li2l (), _l2li (), _fn2fi (), _fi2fn (), _fncnt (), _ftrie (), _fstrie (),
#ifdef USE_CEDAR
      _pmtrie (), _pmfunc (1 << _opt.pmsize, &_pmtrie), _pmi (0), _pms (0),
#ifdef USE_LFU_PMT
      _bucket_pool (), _bucket (), _item (1 << _opt.pmsize),
#else
      _ring (1 << _opt.pmsize),
#endif
#endif
      _f2dpn (0), _f2nf (0), _dpolyk (), _bound (), _fw (0), _fsw (0)
#ifdef USE_TIMER
      , _timer_pool ((std::string ("pecco profiler (") + _opt.model + ")").c_str ()), _enc_t (_timer_pool.push ("enc")), _model_t (_timer_pool.push ("model")), _classify_t (_timer_pool.push ("classify", "classify"))
#endif
#ifdef USE_PROFILING
      , _all (0), _hit (0), _lookup (0), _flen (0), _traverse (0), _lookup_sp (0), _traverse_sp (0)
#endif
    {
#ifdef USE_LFU_PMT
      _bucket_pool.create_bucket (_bucket); // root
#endif
    }
    // interface
    bool load (const char* model) { return _derived ()->load (model); }
    template <bool PRUNE, binary_t FLAG>
    void classify (ny::fv_t& fv, double* score)
    { _derived ()->template classify <PRUNE, FLAG> (fv, score); }
    bool abuse_trie () const
    { return _derived_const ()->abuse_trie (); }
    bool is_binary_classification () const
    { return _derived_const ()->is_binary_classification (); }
    void printLabel (const ny::uint li) const
    { std::fprintf (stdout, "%s", _li2l[li]); }
    void printScore (const ny::uint li, const double* score) const
    { _derived_const ()->printScore (li, score); }
    void printProb (const ny::uint li, const double* score) const
    { _derived_const ()->printProb (li, score); }
    ny::uint getLabel (const double* score)
    { return _derived ()->getLabel (score); }
    template <bool PRUNE, binary_t FLAG>
    void classify  (char* p, double* score);
    void batch     ();
    void printStat ();
  protected:
    ~ClassifierBase () {}
  };
}
#endif /* CLASSIFY_H */
