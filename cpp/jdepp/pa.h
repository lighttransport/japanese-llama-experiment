// opal -- online learning with kernel slicing
//  $Id: pa.h 1934 2022-01-23 02:45:17Z ynaga $
// Copyright (c) 2009-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef OPAL_PA_H
#define OPAL_PA_H

#include <getopt.h>
#include <stdint.h>
#include <err.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <limits>
#include <valarray>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <iterator>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#if   defined (USE_MT19937)
#include <random>
#elif defined (USE_TR1_MT19937)
#include <tr1/random>
#endif

#if   defined (USE_HASH)
#include <unordered_map>
#elif defined (USE_TR1_HASH)
#include <tr1/unordered_map>
#endif

#if ! defined (USE_HASH_TRIE) && ! defined (USE_MAP_TRIE)
#define USE_DOUBLE_ARRAY_TRIE
#endif

#if defined (USE_DOUBLE_ARRAY_TRIE)
#define USE_REDUCED_TRIE 1
#include "cedar.h"
#endif

#include "timer.h"
#ifdef USE_SSE4_2_POPCNT
#include <smmintrin.h>
#endif

#define OPAL_COPYRIGHT  "opal - online learning with kernel slicing\n\
Copyright (c) 2009-2015 Naoki Yoshinaga, All rights reserved.\n\
\n\
Usage: %s [options] train model test\n\
\n\
train   training file       set '-' to skip training\n\
model   model file          set '-' to training/test w/o saving a model\n\
test    test file           set '-' to skip testing\n\
\n"

#define OPAL_OPT0 "Optional parameters in training and testing:\n\
  -t, --kernel=TYPE         select type of kernel function\n\
                            * 0 - linear     (w^T * x)\n\
                              1 - polynomial (s^T * x + 1)^d\n\
  -d, --kernel-degree=INT   parameter d in polynomial kernel (0)\n"

#ifdef USE_POLYK
#define OPAL_OPT1 ""
#else
#define OPAL_OPT1 "      --kernel-splitN=INT   # common features in kernel splitting (0)\n\
      --max-trie-size=INT   adjust # common features to fit the feature trie\n\
                            within RAM size (MiB) (32)\n\
  -p, --pruning-margin      terminate margin computation if unnecessarily\n\
  -k, --kernel-slicing      perform kernel slicing\n"
#endif

#ifdef USE_MULTICLASS
#define OPAL_OPT2 "  -O, --output=TYPE         select output type of testing\n\
                             * 0 - report accuracy\n\
                               1 - report accuracy per iteration\n\
                               2 - labels\n\
                               3 - labels and margins\n\
  -o, --output!=TYPE        output examples with labels/margins\n\n"
#else
#define OPAL_OPT2 "  -O, --output=TYPE         select output type of testing\n\
                             * 0 - report accuracy\n\
                               1 - report accuracy per iteration\n\
                               2 - labels\n\
                               3 - labels and margins\n\
                               4 - labels and probabilities\n\
  -o, --output!=TYPE        output examples with labels/margins/probabilities\n\n"
#endif

#define OPAL_OPT_TRAIN0 "Optional parameters in training:\n\
  -l, --learner=TYPE        select learning algorithm\n\
                              0 - Perceptron\n\
                              1 - Passive Aggressive    (PA)\n\
                            * 2 - Passive Aggressive I  (PA-I)\n\
                              3 - Passive Aggressive II (PA-II)\n\
  -c, --reg-cost=FLOAT      PA-I/-II aggressiveness parameter C (1.0)\n\
                            ([i * avg. k(x,x)]^-1 if C is set to 0)\n\
  -i, --iteration=INT       # iterations (10)\n\
  -a, --averaging           average parameters\n\
  -s, --shuffling           shuffle training examples on RAM\n\
  -b, --buffer=TYPE         select type of buffer to read examples\n\
                            * 0 - Use RAM to load examples\n\
                              1 - Use DISK to cache examples\n\
                              2 - Do not cache examples\n\
      --model0=FILE         re-train a model trained w/ opal ('-')\n\
      --feat-threshold=INT  thresholding features by frequency (1)\n\
  -M, --max-examples=INT    max. # examples used in training (0: all)\n"

#ifdef _OPENMP
#define OPAL_OPT_TRAIN1 "  -n, --num-threads=INT     number of threads for parallel training (0)\n"
#else
#define OPAL_OPT_TRAIN1 ""
#endif

#ifdef USE_MULTICLASS
#define OPAL_OPT_TRAIN2 "      --num-classes=INT     max. # classes (needed for [-b 2])\n\n"
#else
#define OPAL_OPT_TRAIN2 "  -P, --probability-output  perform sigmoid fitting to output probabilities\n\n"
#endif

#define OPAL_OPT_MISC "Misc.:\n\
  -h, --help               show this help and exit\n"

#ifdef USE_MULTICLASS
#ifdef _OPENMP
static const char* opal_opts = "t:d:kpO:o:l:c:i:asn:b:M:h";
#else
static const char* opal_opts = "t:d:kpO:o:l:c:i:asb:M:h";
#endif
#else
#ifdef _OPENMP
static const char* opal_opts = "t:d:kpO:o:l:c:i:asn:b:M:Ph";
#else
static const char* opal_opts = "t:d:kpO:o:l:c:i:asb:M:Ph";
#endif
#endif

static struct option opal_long_opts[] = {
  {"kernel",             required_argument, NULL, 't'},
  {"kernel-degree",      required_argument, NULL, 'd'},
  {"kernel-splitN",      required_argument, NULL,  0 },
  {"max-trie-size",      required_argument, NULL,  0 },
  {"kernel-slicing",     required_argument, NULL, 'k'},
  {"pruning-margin",     required_argument, NULL, 'p'},
  {"output",             required_argument, NULL, 'O'},
  {"output!",            required_argument, NULL, 'o'},
  {"learner",            required_argument, NULL, 'l'},
  {"reg-cost",           required_argument, NULL, 'c'},
  {"iteration",          required_argument, NULL, 'i'},
  {"averaging",          no_argument,       NULL, 'a'},
  {"shuffing",           no_argument,       NULL, 's'},
  {"buffer",             required_argument, NULL, 'b'},
  {"feat-threshold",     required_argument, NULL,  0 },
  {"model0",             required_argument, NULL,  0 },
  {"max-examples",       required_argument, NULL, 'M'},
#ifdef USE_MULTICLASS
  {"num-classes",        required_argument, NULL,  0 },
#else
  {"probability-output", required_argument, NULL, 'P'},
#endif
#ifdef _OPENMP
  {"num-threads",        no_argument,       NULL, 'n'},
#endif
  {"help",               no_argument,       NULL, 'h'},
  {NULL, 0, NULL, 0}
};

extern char* optarg;
extern int   optind;

namespace opal {
  // global type alias
#ifdef USE_FLOAT
  typedef float  fl_t;
  static fl_t strtof (const char* p, char** q) { return std::strtof (p, q); }
#else
  typedef double  fl_t;
  static fl_t strtof (const char* p, char** q) { return std::strtod (p, q); }
#endif
  typedef unsigned int   uint;
  typedef unsigned char  uchar;
  typedef std::vector <uint>  fv_t;
  typedef std::vector <const char*>  sbag_t;
#ifdef USE_MULTICLASS
  typedef uint  label_t;
  typedef std::valarray <fl_t>  w_t;
  typedef std::vector <std::pair <fl_t, size_t> >  loss_t;
#else
  typedef int   label_t;
  typedef fl_t  w_t;
#endif
  typedef std::vector <w_t>  wv_t;
  // static variables and functions
  static const size_t KEY_SIZE = 8;
  static const size_t BUF_SIZE = 1 << 18;
  static const int    MAX_KERNEL_DEGREE = 4;
  static const int    MAX_TRIAL = 1; // 4 is good enough
  // \sum_{i=0}^{k} nCk (* num_class) features in an array-based pseudo trie
#ifdef USE_ARRAY_TRIE
  static const uint PSEUDO_TRIE_N[MAX_KERNEL_DEGREE] = {0, 21, 11, 8};
#else
  static const uint PSEUDO_TRIE_N[MAX_KERNEL_DEGREE] = {0, 0, 0, 0};
#endif
  //
  static const uint COMMON_FACTOR   = 2;
  static const uint COMMON_BIT_UNIT = static_cast <uint> (sizeof (uint64_t)) * 8;
#ifndef USE_SSE4_2_POPCNT
  static char popTable16bit[65536]; // swig lua cannot handle 1 << 16
#endif
  // coefficient for kernel expansion; support s=1 and r=1 case
  // refer a general case to pecco/kernel.cc
  static const fl_t COEFF[MAX_KERNEL_DEGREE][MAX_KERNEL_DEGREE] =
    {{0,  0,  0,  0},  // 0
     {1,  1,  0,  0},  // 1
     {1,  3,  2,  0},  // 2
     {1,  7, 12,  6}}; // 3
  static inline bool getLine (FILE*& fp, char*& line, size_t& read) {
#ifdef __APPLE__
    if ((line = fgetln (fp, &read)) == NULL) return false;
#else
    static ssize_t read_ = 0; static size_t size = 0; // static helps inlining
    if ((read_ = getline (&line, &size, fp)) == -1) return false;
    read = read_;
#endif
    *(line + read - 1) = '\0';
    return true;
  }
  template <typename T> T strton (const char* p, char** q) {
    const int64_t  ret  = static_cast <int64_t>  (std::strtoll  (p, q, 10));
    const uint64_t retu = static_cast <uint64_t> (std::strtoull (p, q, 10));
    if (std::numeric_limits <T>::is_specialized &&
        (ret  < static_cast <int64_t>  (std::numeric_limits <T>::min ()) ||
         retu > static_cast <uint64_t> (std::numeric_limits <T>::max ())))
      errx (1, "overflow: %s", p);
    return static_cast <T> (ret);
  }
  class byte_encoder {
  public:
    byte_encoder () : _len (0), _key () {}
    byte_encoder (uint i) : _len (0), _key () { encode (i); }
    uint encode  (uint i, uchar* const key_) const {
      uint len_ = 0;
      for (key_[len_] = (i & 0x7f); i >>= 7; key_[++len_] = (i & 0x7f))
        key_[len_] |= 0x80;
      return ++len_;
    }
    void encode (const uint i) { _len = encode (i, _key); }
    uint decode (uint& i, const uchar* const key_) const {
      uint b (0), len_ (0);
      for (i = key_[0] & 0x7f; key_[len_] & 0x80; i += (key_[len_] & 0x7fu) << b)
        ++len_, b += 7;
      return ++len_;
    }
    uint        len () { return _len; }
    const char* key () { return reinterpret_cast <const char*> (&_key[0]); }
  private:
    uint  _len;
    uchar _key[KEY_SIZE];
  };
#if defined (USE_HASH) || defined (USE_TR1_HASH)
  // incremental Folwer / Noll / Vo (FNV) Hash (FNV-1)
  // http://isthe.com/chongo/tech/comp/fnv/#FNV-param
  static const uint FNV_PRIME = 0x01000193;
  static const uint FNV_BASIS = 0x811c9dc5;
  struct inc_fnv {
    size_t operator () (const uint ret, const uint fi) const
    { return (ret * FNV_PRIME) ^ fi; }
  };
#endif
  // map- or unordered_map-based trie implementation
#ifdef USE_DOUBLE_ARRAY_TRIE
  typedef cedar::da <int, -1, -2, false, MAX_TRIAL, MAX_KERNEL_DEGREE> trie_base_t;
#else
  typedef uint64_t edge; // 32bit parent id + 32bit integer label
  struct node {
    uint  id;
    int   value;
    node (const uint id_ = 0, const int value_ = 0) : id (id_), value (value_) {}
  };
#ifdef USE_HASH_TRIE
#ifdef USE_HASH
  typedef std::unordered_map <edge, node> trie_base_t;
#else
  typedef std::tr1::unordered_map <edge, node> trie_base_t;
#endif
#else
  typedef std::map <edge, node> trie_base_t;
#endif
#endif
  //
  class trie_t : public trie_base_t {
  public:
    trie_t () : trie_base_t () {}
#ifdef USE_DOUBLE_ARRAY_TRIE
    using trie_base_t::update;
    using trie_base_t::traverse;
    int& update (size_t& pid, const uint label, const int n = 0) {
      byte_encoder encoder (label); size_t p (0);
      return update (encoder.key (), pid, p, encoder.len (), n);
    }
    int traverse (size_t& pid, const uint label) const {
      size_t pid_ (pid), p (0);
      byte_encoder encoder (label);
      const int n = traverse (encoder.key (), pid_, p, encoder.len ());
      if (n == trie_t::CEDAR_NO_PATH) return 0;
      pid = pid_;
      return n == trie_t::CEDAR_NO_VALUE ? 0 : n;
    }
#else
    int& update (size_t& pid, const uint label, const int n = 0) {
      const edge e ((static_cast <uint64_t> (pid) << 32) | label);
      pid = size () + 1;
      trie_t::iterator tit
        = insert (trie_base_t::value_type (e, node (static_cast <uint> (pid), n))).first;
      pid = tit->second.id;
      return tit->second.value;
    }
    int traverse (size_t& pid, const uint label) const {
      const edge e ((static_cast <uint64_t> (pid) << 32) | label);
      trie_base_t::const_iterator tit = find (e);
      if (tit == end ()) return 0;
      pid = tit->second.id;
      return tit->second.value;
    }
#endif
  };
  // options
  enum kernel_t { LINEAR, POLY };
  enum algo_t   { P, PA, PA1, PA2 };
  enum buffer_t { RAM, DISK, null };
  static const char* algo[] = { "P", "PA", "PA1", "PA2" };
  struct option { // option handler
    enum mode_t { BOTH, TRAIN, TEST, DUMP };
    const char* com, *train, *model0, *model, *test;
    //
    mutable kernel_t  kernel;   // kernel-type
    mutable uint      d;        // kernel-degree
    mutable uint      splitN;   // kernel-splitN
    uint              trieT;    // max-trie-size
    mutable bool      pruning;
    mutable bool      slicing;
    uint16_t          output;
    algo_t            algo;     // algorithm
    mutable double    C;        // reg-cost
    uint              iter;     // # iteration
    bool              average;
    bool              shuffle;
    bool              prob;
    bool              shrink;
#ifdef USE_MULTICLASS
    mutable uint      nclass;   // num-classes
#else
    uint              nclass;   // num-classes
#endif
    uint              featT;    // threshold to feature frequency
    long              nthr;     // max threads
    size_t            M;        // max-examples
    buffer_t          buffer;   // buffer type
    mode_t            mode;
    option () : com ("--"), train ("-"), model0 ("-"), model ("-"), test ("-"), kernel (LINEAR), d (0), splitN (0), trieT (32 << 20), pruning (false), slicing (false), output (0), algo (PA1), C (1.0), iter (10), average (false), shuffle (false), prob (false), shrink (false), nclass (1), featT (1), nthr (0), M (0), buffer (RAM), mode (BOTH) {}
    option (int argc, char** argv) : com (argc ? argv[0] : "--"), train ("-"), model0 ("-"), model ("-"), test ("-"), kernel (LINEAR), d (0), splitN (0), trieT (32 << 20), pruning (false), slicing (false), output (0), algo (PA1), C (1.0), iter (10), average (false), shuffle (false), prob (false), shrink (false), nclass (1), featT (1), nthr (0), M (0), buffer (RAM), mode (BOTH)
    { set (argc, argv); }
    void set (int argc, char** argv) { // getOpt
      if (argc == 0) return;
      optind = 1;
      int id = 0;
      while (1) {
        int opt = getopt_long (argc, argv, opal_opts, opal_long_opts, &id);
        if (opt == -1) break;
        char* err = NULL;
        switch (opt) {
          case 't': kernel  = strton <kernel_t> (optarg, &err); break;
          case 'd': d       = strton <uint>     (optarg, &err); break;
#ifndef USE_POLYK
          case 'p': pruning = true; break;
          case 'k': slicing = true; break;
#endif
          case 'o': output  = 0x100;
          case 'O': output  |= strton <uint16_t> (optarg, &err); break;
            // training params
          case 'l': algo    = strton <algo_t>   (optarg, &err); break;
          case 'c': C       = strtof (optarg, &err); break;
          case 'i': iter    = strton <uint>     (optarg, &err); break;
          case 'a': average = true; break;
          case 's': shuffle = true; break;
#ifndef USE_MULTICLASS
          case 'P': prob    = true; break;
#endif
          case 'b': buffer  = strton <buffer_t> (optarg, &err); break;
          case 'n': nthr    = strton <uint>   (optarg, &err);   break;
          case 'M': M       = strton <size_t> (optarg, &err);   break;
            // misc
          case 'h': printCredit (); printHelp (); std::exit (0);
          case  0 :
            if      (std::strcmp (opal_long_opts[id].name, "kernel-splitN") == 0)
              splitN = strton <uint> (optarg, &err);
            else if (std::strcmp (opal_long_opts[id].name, "max-trie-size") == 0)
              trieT = strton <uint> (optarg, &err) << 20;
            else if (std::strcmp (opal_long_opts[id].name, "feat-threshold") == 0)
              featT = strton <uint> (optarg, &err);
            else if (std::strcmp (opal_long_opts[id].name, "model0") == 0)
              model0  = optarg;
#ifdef USE_MULTICLASS
            else if (std::strcmp (opal_long_opts[id].name, "num-classes") == 0)
              nclass  = strton <uint> (optarg, &err);
#endif
            break;
          default:  printCredit (); std::exit (0);
        }
        if (err && *err)
          errx (1, "unrecognized option value: %s", optarg);
      }
      // errors & warnings
      if (! trieT) trieT = std::numeric_limits <uint>::max ();
      if (kernel != POLY && kernel != LINEAR)
        errx (1, "unknown kernel fucntion [-t].");
      if (algo != P && algo != PA && algo != PA1 && algo != PA2)
        errx (1, "unknown learning algorithm [-l].");
      if (buffer != RAM && buffer != DISK && buffer != null)
        errx (1, "unknown buffering method [-b].");
      if (iter == 0) errx (1, "# iterations [-i] must be >= 1.");
      if (C != 1.0 && (algo == P || algo == PA))
        warnx ("NOTE: reg-cost C [-c] is ignored in P [-l 0] and PA [-l 1].");
      if (kernel == LINEAR) {
        if (d != 0) warnx ("NOTE: kernel-degree [-d] is ignored in linear kernel [-t 0].");
        d = 0;
      } else {
        if (d == 0 || d >= 4) errx (1, "set kernel_degree [-d] to 1-3.");
        if (d == 1 && (slicing || pruning)) {
          warnx ("NOTE: kernel slicing [-k] (or [-p]) is disabled since it is useless for d=1.");
          slicing = pruning = false;
        }
      }
      if (! splitN) // enable shrinkage
        shrink = true, splitN = std::numeric_limits <uint>::max ();
      if (std::strcmp (com, "--") == 0) return;
      if (argc < optind + 3) {
        printCredit ();
        errx (1, "Type `%s --help' for option details.", com);
      }
      train = argv[optind];
      model = argv[++optind];
      test  = argv[++optind];
      setMode (); // induce appropriate mode
    }
    void setMode () {
      if (std::strcmp (train, "-") == 0 && std::strcmp (test, "-") == 0) {
#ifdef USE_DOUBLE_ARRAY_TRIE
        if (std::strcmp (model, "-") != 0) mode = DUMP;
        else
#endif
          errx (1, "specify at least training or test file.");
      }
      else if (std::strcmp (test,  "-") == 0) mode = TRAIN;
      else if (std::strcmp (train, "-") == 0) mode = TEST;
      else                                    mode = BOTH;
      if (std::strcmp (model, "-") == 0 && mode != BOTH)
        errx (1, "instant mode needs both train/test files.");
      if (mode == TRAIN && output == 1)
        errx (1, "per-iteration testing requires test file.");
      const char* mode0 [] = {"BOTH", "TRAIN", "TEST", "DUMP" };
      std::fprintf (stderr, "mode: %s\n", mode0[mode]);
    }
    void printCredit () { std::fprintf (stderr, OPAL_COPYRIGHT, com); }
    void printHelp   () {
      std::fprintf (stderr, OPAL_OPT0 OPAL_OPT1 OPAL_OPT2 OPAL_OPT_TRAIN0
                    OPAL_OPT_TRAIN1 OPAL_OPT_TRAIN2 OPAL_OPT_MISC);
    }
  };
  // label bag: assuming small # classes; or you can use unordered_map
  struct less_charp {
    bool operator () (const char* a, const char* b) const
    { return std::strcmp (a, b) < 0; }
  };
  class lmap {
  public:
    typedef std::map <const char*, label_t, less_charp>  l2i_t;
    lmap  () : _l2i (), _sbag () {}
    ~lmap () {
      for (sbag_t::iterator it = _sbag.begin (); it != _sbag.end (); ++it)
        delete [] *it;
    }
    label_t set_id (const char* ys, size_t len = 0) {
      l2i_t::iterator it = _l2i.lower_bound (ys);
      if (it == _l2i.end () || std::strcmp (it->first, ys) != 0) {
        if (! len) len = std::strlen (ys);
        it = _l2i.insert (it, l2i_t::value_type (std::strcpy (new char[len + 1], ys),
                                                 static_cast <label_t> (_sbag.size ())));
        _sbag.push_back (it->first);
      }
      return it->second;
    }
    label_t get_id (const char* ys) const {
      l2i_t::const_iterator it = _l2i.find (ys);
      if (it == _l2i.end ()) errx (1, "unknown label: %s", ys);
      return it->second;
    }
    const char* get_label (const size_t i) const { return _sbag[i]; }
    void read (char* p, char* const p_end) { // # labels:
      if (std::strncmp (p, "# labels: ", 10) != 0)
        errx (1, "premature label definition.");
      p += 9; // "# labels:"
      while (++p) {
        char* ys = p; while (p != p_end && ! isspace (*p)) ++p; *p = '\0';
        set_id (ys, static_cast <size_t> (p - ys));
        if (p == p_end) break;
      }
    }
    void write (FILE* fp) {
      std::fprintf (fp, "%u # labels:", nclass ());
      std::vector <std::pair <label_t, const char*> > sorter;
      for (uint i = 0; i < _sbag.size (); ++i)
        std::fprintf (fp, " %s", _sbag[i]);
      std::fprintf (fp, "\n");
    }
    uint nclass () const { return static_cast <uint> (_sbag.size ()); }
  private:
    l2i_t  _l2i;
    sbag_t _sbag;
  };
  // feature bag; pack sparse feature indices into ordered dense ones
  //              to minimize memory consumption / trie retrieval cost
  class fmap {
  public:
    fmap (const size_t thresh) :
#ifdef USE_STRING_FEATURE
      _fs2fn (), _fn2fs (1, std::strcpy (new char[2], " ")),
#endif
      _fn2fi (), _fi2fn (1, 0), _counter (), _thresh (thresh) {}
#ifdef USE_STRING_FEATURE
    ~fmap () {
      for (sbag_t::iterator it = _fn2fs.begin (); it != _fn2fs.end (); ++it)
        delete [] *it;
    }
    int fs2fn (const char* fs) {
#ifdef USE_DOUBLE_ARRAY_TRIE
      const int n = _fs2fn.exactMatchSearch <trie_t::result_type> (fs);
      return n == trie_t::CEDAR_NO_VALUE ? -1 : n;
#else
      fbag_t::const_iterator it = _fs2fn.find (fs);
      return it == _fs2fn.end () ? -1 : it->second;
#endif
    }
    int fs2fn (char* fs, const size_t len) {
#ifdef USE_DOUBLE_ARRAY_TRIE
      int& fn = _fs2fn.update (fs, len);
      if (! fn) {
        fn = static_cast <int> (_fn2fs.size ());
        char* copy = std::strncpy (new char[len + 1], fs, len);
        copy[len] = '\0'; // bug fix
        _fn2fs.push_back (copy);
      }
      return fn;
#else
      char c = '\0'; std::swap (fs[len], c);
      fbag_t::const_iterator it = _fs2fn.find (fs);
      if (it == _fs2fn.end ()) {
        const char* copy = std::strcpy (new char[len + 1], fs);
        const int fn = static_cast <int> (_fn2fs.size ());
        it = _fs2fn.insert (fbag_t::value_type (copy, fn)).first;
        _fn2fs.push_back (copy);
      }
      std::swap (fs[len], c);
      return it->second;
#endif
    }
    const char* fn2fs (const uint fn) const { return _fn2fs[fn]; }
    void load (const char* ffn) {
      _fn2fs.clear ();
      FILE* fp = std::fopen (ffn, "rb");
      if (! fp) errx (1, "cannot read features: %s", ffn);
      char*  line = 0;
      size_t read = 0;
      while (getLine (fp, line, read)) {
        const char* copy = std::strcpy (new char[read], line);
        const int fn = static_cast <int> (_fn2fs.size ());
#ifdef USE_DOUBLE_ARRAY_TRIE
        _fs2fn.update (line, read - 1) = fn;
#else
        _fs2fn.insert (fbag_t::value_type (copy, fn));
#endif
        _fn2fs.push_back (copy);
      }
      std::fclose (fp);
    }
    void save (const char* ffn) const {
      FILE* fp = std::fopen (ffn, "wb");
      if (! fp)
        errx (1, "cannot write the features: %s", ffn);
      for (sbag_t::const_iterator it = _fn2fs.begin ();
           it != _fn2fs.end (); ++it)
        std::fprintf (fp, "%s\n", *it);
      std::fclose (fp);
    }
#endif
    uint fi2fn (const uint fi) const { return _fi2fn[fi]; }
    void revertFv2Fv  (uint* const begin, uint* const end) const {
      for (uint* it = begin; it != end; ++it)
        *it = fi2fn (*it);
      std::sort (begin, end);
    }
    void convertFv2Fv (uint* const begin, uint* const end, uint& len) const {
      uint* jt = begin;
      for (uint* it = jt; it != end; ++it)
        if (uint fi = _fn2fi[*it < _fn2fi.size () ? *it : 0])
          *jt = fi, ++jt;
      len = static_cast <uint> (jt - begin);
      std::sort (begin, jt);
    }
    void convertFv2Fv (uint* const begin, uint* const end, const uint* const maxF) {
      if (*maxF >= _fn2fi.size ()) _fn2fi.resize (*maxF + 1, 0); // widen
      uint* jt = begin;
      for (uint* it = jt; it != end; ++it, ++jt)
        if (uint& fi = _fn2fi[*it]) // register
          *jt = fi;
        else
          *jt = fi = static_cast <uint> (_fi2fn.size ()), _fi2fn.push_back (*it);
      std::sort (begin, end);
    }
    void thresholding (uint* const begin, uint* const end, uint& len) {
      uint* jt = begin;
      for (uint* it = jt; it != end; ++it)
        if (*it < _counter.size () && _counter[*it].first >= _thresh)
          *jt = *it, ++jt;
      len = static_cast <uint> (jt - begin);
    }
    void build () { // build feature mapping; empty element should be removed
      if (_fn2fi.size () < _counter.size ()) // widen if needed
        _fn2fi.resize (_counter.size (), 0);
      std::sort (_counter.rbegin (), _counter.rend ());
      for (counter_t::const_iterator it = _counter.begin ();
           it != _counter.end () && it->first >= _thresh; ++it) {
        uint& fi = _fn2fi[it->second];
        if (! fi)
          fi = static_cast <uint> (_fi2fn.size ()), _fi2fn.push_back (it->second);
      }
      counter_t ().swap (_counter);
    }
    void inc_count (const uint* const begin, const uint* const end, const uint maxF) {
      for (uint i = static_cast <uint> (_counter.size ()); i <= maxF; ++i)
        _counter.push_back (counter_t::value_type (0, i));
      for (const uint* it = begin; it != end; ++it)
        ++_counter[*it].first;
    }
  private:
    typedef std::vector <std::pair <uint, uint> > counter_t;
#ifdef USE_STRING_FEATURE
#if   defined (USE_HASH_TRIE)
    struct hash_charp : std::unary_function <const char*, size_t> {
      result_type operator () (argument_type f) const
      { return std::accumulate (f, f + std::strlen (f), FNV_BASIS, inc_fnv ()); }
    };
    struct eq_charp {
      bool operator () (const char* a, const char* b) const
      { return std::strcmp (a, b) == 0; }
    };
#ifdef USE_HASH
    typedef std::unordered_map <const char*, int, hash_charp, eq_charp> fbag_t;
#else
    typedef std::tr1::unordered_map <const char*, int, hash_charp, eq_charp> fbag_t;
#endif
#elif defined (USE_MAP_TRIE)
    typedef std::map <const char*, int, less_charp> fbag_t;
#else
    typedef trie_t  fbag_t;
#endif
    fbag_t  _fs2fn;
    sbag_t  _fn2fs;
#endif
    std::vector <uint>  _fn2fi;
    std::vector <uint>  _fi2fn;
    counter_t           _counter;
    size_t              _thresh;
  };
  template <typename T, typename U>
  class ex_base {
  protected:
    uint*  _x;   // (binary) feature vector
    U      _y;   // label or weight
    uint   _len;
    static bool isspace (const char p) { return p == ' ' || p == '\t'; }
    ~ex_base () {}
  public:
    ex_base ()                  : _x (0), _y (U (0)), _len (0) {}
    ex_base (const ex_base& ex) : _x (ex._x), _y (ex._y), _len (ex._len) {}
    ex_base (const uint* fv, const U& y_, const uint len)
      : _x (new uint[len]), _y (y_), _len (len)
    { std::copy (&fv[0], &fv[len], begin ()); }
    ex_base& operator= (const ex_base& ex)
    { _x = ex._x; _y = ex._y; _len = ex._len; return *this; }
    void set (fv_t& fv, char* ex, char* const ex_end, const bool store, lmap* lm, fmap* fm, const bool count = false) {
      // set label (weight) of feature vectors (support vectors)
      char* p = ex;
      read (p, lm); // _y
      // set features
      fv.clear ();
      ++p;
      while (p != ex_end) {
#ifdef USE_STRING_FEATURE
        char* q = p; while (*q && *q != ':' && ! isspace (*q)) ++q;
        const int fn = fm->fs2fn (p, static_cast <size_t> (q - p));
        p = q;
#else
        int64_t fn = 0;
        for (; *p >= '0' && *p <= '9'; ++p) {
          fn *= 10, fn += *p, fn -= '0';
          if (fn > std::numeric_limits <uint>::max ())
            errx (1, "overflow: %s", ex);
        }
        if (*p != ':') errx (1, "illegal feature index: %s", ex);
#endif
        fv.push_back (static_cast <uint> (fn));
        while (*p && ! isspace (*p)) ++p;
        while (isspace (*p)) ++p;
      }
#ifdef USE_STRING_FEATURE
      std::sort (fv.begin (), fv.end ());  // heavy
#endif
      set_x (&fv[0], fv.size (), store, count ? fm : 0);
    }
    void set_x (const uint* fv, const size_t len, const bool store = false, fmap* fm = 0) {
      if (store)
        _x = new uint[len], std::copy (&fv[0], &fv[len], begin ());
      else
        _x = const_cast <uint*> (&fv[0]);
      _len = static_cast <uint> (len);
      if (fm) fm->inc_count (begin (), end (), maxF ());
    }
    void read  (char*& p, lmap* lm) { _derived ()->read_impl (p, lm); }
#ifdef USE_STRING_FEATURE
    void print (FILE* fp, const fmap* fm) {
      _derived ()->print_weight (fp);
      for (const uint* it = begin (); it != end (); ++it)
        std::fprintf (fp, " %s:1", fm->fn2fs (*it));
      std::fprintf (fp, "\n");
    }
#else
    void print (FILE* fp, const fmap* = 0) {
      _derived ()->print_weight (fp);
      for (const uint* it = begin (); it != end (); ++it)
        std::fprintf (fp, " %u:1", *it);
      std::fprintf (fp, "\n");
    }
#endif
    // interface
    uint*        begin   ()       { return &_x[0]; }
    uint*        end     ()       { return begin () + _len; }
    const uint*  begin   () const { return &_x[0]; }
    const uint*  end     () const { return begin () + _len; }
    const uint*  rbegin  () const { return end   () - 1; }
    const uint*  rend    () const { return begin () - 1; }
    uint         maxF    () const { return _len == 0 ? 0 : _x[_len - 1]; }
    uint&        size    ()       { return _len; }
    uint         getSize () const { return _len; }
    const uint*  getBody () const { return _x; }
  private:
    T*           _derived () { return static_cast <T*> (this); }
  };
  struct ex_t : public ex_base <ex_t, label_t> {
    typedef label_t y_t;
#ifdef USE_MULTICLASS
    void read_impl (char*& p, lmap* lm) {
      char* ys = p; while (*p && ! isspace (*p)) ++p;
      char c = '\0'; std::swap (*p, c);
      _y = lm->set_id (ys, static_cast <size_t> (p - ys));
      std::swap (*p, c);
    }
#else
    void read_impl (char*& p, lmap*)
    { _y = static_cast <int> (std::strtol (p, &p, 10)); }
#endif
    void print_weight (FILE* fp) const { std::fprintf (fp, "%d", _y); }
    label_t&       label    ()       { return _y; }
    const label_t& getLabel () const { return _y; }
    double         getNorm  () const { return static_cast <fl_t> (getSize ()); }
    ex_t (const uint* x, const label_t& y, const uint len)
      : ex_base <ex_t, label_t> (x, y, len) {}
    ex_t () : ex_base <ex_t, label_t> () {}
  };
  class sv_t : public ex_base <sv_t, w_t> {
  public:
    ~sv_t () {}
#ifdef USE_POLYK
    sv_t  () : ex_base <sv_t, w_t> () {}
    sv_t (const uint* fv, const w_t& y, const uint len)
      : ex_base <sv_t, w_t> (fv, y, len) {}
#else
    sv_t  () : ex_base <sv_t, w_t> (), _pos (0), _sbits () {}
    sv_t (const uint* fv, const w_t& y, const uint len, const uint pos)
      : ex_base <sv_t, w_t> (fv, y, len), _pos (pos), _sbits () {
      for (const uint* it = this->begin (); it != begin_r (); ++it)
        _sbits[*it / COMMON_BIT_UNIT] |= 1UL << (*it & (COMMON_BIT_UNIT - 1));
    }
    uint64_t    sbit    (const uint i) const { return _sbits[i]; }
    const uint* begin_r ()             const { return this->begin () + _pos; }
#endif
    w_t&        weight    ()        { return this->_y; }
    const w_t&  getWeight ()  const { return this->_y; }
#ifdef USE_MULTICLASS
    void read_impl (char*& p, lmap* lm) {
      _y.resize (lm->nclass (), fl_t (0));
      for (size_t i = 0; i < _y.size (); ++p, ++i) _y[i] = strtof (p, &p);
      --p;
    }
    void print_weight (FILE* fp) const {
      std::fprintf (fp, "%.16g", _y[0]);
      for (size_t i = 1; i < _y.size (); ++i)
        std::fprintf (fp, " %.16g", _y[i]);
    }
#else
    void read_impl    (char*& p, lmap*) { _y = strtof (p, &p); }
    void print_weight (FILE* fp) const  { std::fprintf (fp, "%.16g", _y); }
#endif
#ifndef USE_POLYK
  private:
    uint      _pos;
    uint64_t  _sbits[COMMON_FACTOR];
#endif
  };
  // type traits
  template <template <class> class T, typename U>
  struct is_memory { enum { value = 0 }; };
  template <typename elem_t> class mem_pool;
  template <typename U>
  struct is_memory <mem_pool, U> { enum { value = 1 }; };
  //
  template <template <class> class T, typename U>
  class basic_pool {
  public:
    typedef U elem_t;
    typedef typename std::vector <elem_t>  data_t;
    elem_t* init  () { return _derived ()->init_impl (); }
    elem_t* get   () { return _derived ()->get_impl ();  }
    void    put   (const elem_t& x) { _derived ()->put_impl (x); }
    void    setup (fmap& fm, const bool flag = true)
    { _derived ()->setup_impl (fm, flag); }
    void read (FILE* fp, lmap* lm, fmap* fm,
               const bool count = false, const size_t M = 0) {
      char* line = 0;
      for (size_t read_ (0), i (0); getLine (fp, line, read_); ) {
        if (line[0] == '#') continue;
        if (M && ++i > M) break;
        _x.set (_fv, line, line + read_ - 1, is_memory <T, U>::value, lm, fm, count);
        _derived ()->put_impl (_x);
      }
    }
    virtual void read (const char* lfn, lmap* lm, fmap* fm = 0,
                       const bool count = false, const size_t M = 0) {
      FILE* fp = std::fopen (lfn, "r");
      if (! fp) errx (1, "no such file: %s", lfn);
      char buf[BUF_SIZE]; std::setvbuf (fp, &buf[0], _IOFBF, BUF_SIZE);
      read (fp, lm, fm, count, M);
      std::fclose (fp);
    }
    virtual void assign  (const elem_t*, const size_t) {}
    virtual void shuffle () { warnx ("WARNING: use rand_shuf for shuffling."); }
    virtual const data_t* body () const { return 0; }
    virtual const char*   curr () const { return 0; }
  protected:
    basic_pool  () : _x (), _fv () {}
    ~basic_pool () {}
    elem_t  _x;
    fv_t    _fv;
  private:
    T <U>* _derived () { return static_cast <T <U>*> (this); }
    basic_pool (const basic_pool&);
    basic_pool& operator= (const basic_pool&);
  };
  template <typename elem_t>
  class mem_pool : public basic_pool <mem_pool, elem_t> {
  public:
    typedef typename std::vector <elem_t>  data_t;
    struct rng {
#if   defined (USE_MT19937)
      std::uniform_int_distribution <size_t> dist;
      std::mt19937 mt;
      rng () : dist (), mt () {}
      size_t operator () (size_t max) { return dist (mt, std::uniform_int_distribution <size_t>::param_type (0, max)); }
#elif defined (USE_TR1_MT19937)
      std::tr1::variate_generator <std::tr1::mt19937,
                                   std::tr1::uniform_int <size_t> > gen;
      rng () : gen (std::tr1::mt19937 (), std::tr1::uniform_int <size_t> ()) {}
      size_t operator () (size_t max) { return gen (max); }
#else
      size_t gen () { // Xorshift RNG; http://www.jstatsoft.org/v08/i14/paper
        static size_t x (123456789), y (362436069), z (521288629), w (88675123);
        size_t t = (x ^ (x << 11)); x = y; y = z; z = w;
        return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
      }
      size_t operator () (const size_t max) { return gen () % max; }
#endif
    };
    mem_pool () : _ex (), _data (0), _p (0), _end (0) {}
    void assign (const elem_t* data, const size_t M)
    { _data = data; _p = _data; _end = _data + M; }
    virtual ~mem_pool () {
      for (typename data_t::iterator it = _ex.begin (); it != _ex.end (); ++it)
        delete it->getBody ();
    }
    elem_t* init_impl () { // for parallel training
      if (! _data) { _data = &_ex[0]; _end = _data + _ex.size (); }
      _p = _data;
      return get_impl ();
    }
    elem_t* get_impl () { return _p == _end ? 0 : const_cast <elem_t*> (_p++); }
    void put_impl (const elem_t& x) { _ex.push_back (x); }
    void setup_impl (fmap& fm, const bool flag) {
      for (typename data_t::iterator it = _ex.begin (); it != _ex.end (); ++it)
        if (flag)
          fm.convertFv2Fv (it->begin (), it->end (), it->size ());
        else
          fm.thresholding (it->begin (), it->end (), it->size ());
    }
    void shuffle () { rng g; std::random_shuffle (_ex.begin (), _ex.end (), g); }
    const data_t* body () const { return &_ex; }
  private:
    data_t  _ex;
    const elem_t*  _data;
    const elem_t*  _p;
    const elem_t*  _end;
  };
  template <typename elem_t>
  class disk_pool : public basic_pool <disk_pool, elem_t> {
  public:
    FILE*  fp;
    disk_pool () : fp (std::tmpfile ()), _s () {}
    virtual ~disk_pool () { std::fclose (fp); }
    elem_t* init_impl () { std::fseek (fp, 0, SEEK_SET); return get_impl (); }
    elem_t* get_impl  () {
      typename elem_t::y_t y = 0;
      if (std::fread (&y, sizeof (typename elem_t::y_t), 1, fp) == 0) return 0;
      uint len = 0; std::fread (&len, sizeof (uint), 1, fp);
      if (_s.size () < len) _s.resize (len);
      std::fread (&_s[0], sizeof (uchar), len, fp);
      this->_fv.clear ();
      byte_encoder encoder;
      for (uint i (0), len_ (0), prev (0); len_ < len;
           this->_fv.push_back (prev))
        len_ += encoder.decode (i, &_s[len_]), prev += i;
      this->_x.label () = y;
      this->_x.set_x (&this->_fv[0], this->_fv.size ());
      return &this->_x;
    }
    void put_impl (const elem_t& x) {
      if (_s.size () < x.getSize () * KEY_SIZE)
        _s.resize (x.getSize () * KEY_SIZE);
      const label_t y = x.getLabel (); // prohibit elem_t = sv_t
      uint len (0), prev (0);
      byte_encoder encoder;
      for (const uint* it = x.begin (); it != x.end (); prev = *it, ++it)
        len += encoder.encode (*it - prev, &_s[len]);
      std::fwrite (&y,     sizeof (label_t), 1,   fp);
      std::fwrite (&len,   sizeof (uint),    1,   fp);
      std::fwrite (&_s[0], sizeof (uchar),   len, fp);
    }
    void setup_impl (fmap& fm, const bool flag) {
      disk_pool <elem_t> tmp;
      for (elem_t* x = this->init (); x; x = this->get ()) {
        if (flag)
          fm.convertFv2Fv (x->begin (), x->end (), x->size ());
        else
          fm.thresholding (x->begin (), x->end (), x->size ());
        tmp.put (*x);
      }
      std::swap (fp, tmp.fp);
    }
  private:
    std::vector <uchar>  _s;
  };
  template <typename elem_t>
  class null_pool : public basic_pool <null_pool, elem_t> {
    using basic_pool <opal::null_pool, elem_t>::read;
  public:
    null_pool  (const bool print = false) :
      _fp (0), _line (0), _read (0), _lm (0), _fm (0), _i (0), _M (0), _flag (false), _thresh (false), _print (print), _buf () {}
    virtual ~null_pool () {}
    elem_t* init_impl () {
      _line = 0; _read = 0; _i = 0;
      if (_fp != stdin) std::fseek (_fp, 0, SEEK_SET);
      return get_impl ();
    }
    void read (const char* lfn, lmap* lm, fmap* fm, const bool flag = false, const size_t M = 0) {
      _fp = lfn ? std::fopen (lfn, "r") : stdin;  // initialize
      if (! _fp) errx (1, "no such file: %s", lfn);
      std::setvbuf (_fp, &_buf[0], _IOFBF, BUF_SIZE);
      _lm = lm; _fm = fm; _flag = flag; _M = M; 
      if (_flag) // read data for feature packing / thresholding
        read (_fp, _lm, _fm, _flag, _M), _flag = false;
    }
    elem_t* get_impl () {
      do {
        if (! getLine (_fp, _line, _read)) return 0;
        if (_line[0] != '#') break;
        if (this->_print) std::fprintf (stdout, "%s\n", _line);
      } while (1);
      if (_M && ++_i > _M) return 0;
      this->_x.set (this->_fv, _line, _line + _read - 1, false, _lm, _fm);
      if (_flag) // needs remapping
        _fm->convertFv2Fv (this->_x.begin (), this->_x.end (), this->_x.size ());
      else if (_thresh)
        _fm->thresholding (this->_x.begin (), this->_x.end (), this->_x.size ());
      return &this->_x;
    }
    void put_impl (const elem_t&) {} // ignore
    void setup_impl (fmap& fm, const bool flag)
    { _fm = &fm; _flag = flag; _thresh = ! flag; } // ignore
    const char* curr () const { return _line; } // previous example
  private:
    FILE*   _fp;
    char*   _line;
    size_t  _read;
    lmap*   _lm;
    fmap*   _fm;
    size_t  _i;
    size_t  _M;
    bool    _flag;
    bool    _thresh;
    const bool _print;
    char    _buf[BUF_SIZE];
  };
  //
  class Model {
  public:
    Model (const opal::option& opt) :
      _opt (opt), _MIN_N (_opt.shrink ? 1U << PSEUDO_TRIE_N[_opt.d] : _opt.splitN + 1), _COMMON_BITS (std::min (_MIN_N, COMMON_BIT_UNIT * COMMON_FACTOR)), _lm (), _fm (_opt.featT), _nex (0), _nsv (0), _nf (0), _s (), _m0 (),  _bias (), _f2ss (), _fv2s (), _fbit (), _fbits (), _alpha (), _sv (), _w (), _wa (),
#ifdef USE_POLYK
      _dot (),
#endif
      _polyk (), _nbin (static_cast <uint> (std::floor (std::log (opt.splitN) / std::log (2)) + 1)), _ftrie (new trie_t*[_nbin]), _fw (new wv_t[_nbin] ()), _f2pn (), _bound (), _f2ss_up (), _pmtrie (), _tpm (),
#ifdef USE_TEMPORAL_PRUNING
                                              _tpn (),
#endif
                                              _limk (), _pfv (), _thresh (_opt.algo == P ? 0 : 1.0),
#ifdef USE_MULTICLASS
      _m (), _pm (), _pm1 (), _pm2 (), _pm3 (), _pn (), _ls (),
#endif
      _sigmoid_A (-1.0), _sigmoid_B (0)
#ifdef USE_TIMER
      , _timer_pool (), _timer (0)
#endif
    {
      for (uint i = 0; i < _nbin; ++i) _ftrie[i] = new trie_t;
#ifndef USE_SSE4_2_POPCNT
      const char popTable8bit[] = {
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5, //   0-
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, //  32-
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, //  64-
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, //  96-
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, // 128-
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, // 160-
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, // 192-
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8  // 224-
      };
      for (uint i = 0; i < (1 << 16); ++i)
        popTable16bit[i]
          = static_cast <char> (popTable8bit[i >> 8] + popTable8bit[i & 0xff]);
#endif
    }
    ~Model () { // clean up
      printStat ();
      for (fv2s_t::iterator it = _fv2s.begin (); it != _fv2s.end (); ++it) {
        delete [] it->first->getBody ();
        delete it->first;
      }
      for (uint i = 0; i < _nbin; ++i) delete _ftrie[i];
      delete [] _ftrie;
      delete [] _fw;
    }
    // helper functions
    bool _is_zero (const w_t& w) const {
#ifdef USE_MULTICLASS
      return std::equal (&w[0], &w[_opt.nclass], &_m0[0]);
#else
      return std::fpclassify (w) == FP_ZERO;
#endif
    }
    //
#ifdef USE_ARRAY_TRIE
    void init_weight_trie () {
      const uint N = std::min ((1U << PSEUDO_TRIE_N[_opt.d]) - 1, _opt.splitN);
      switch (_opt.d) {
        case 1: _w.resize (N, _m0);                   break;
        case 2: _w.resize (N * (N + 1) / 2, _m0);     break;
        case 3: _w.resize (N * (N * N + 5) / 6, _m0); break;
      }
    }
#endif
#ifdef USE_MULTICLASS
    void init_weight (const uint nclass) {
      if (nclass >= MAX_NUM_CLASSES)
        errx (1, "set MAX_NUM_CLASSES > %d; configure --with-num-classes=NUM)", nclass);
      _m0.resize  (nclass, fl_t (0));
      if (_opt.kernel == POLY) _bias.resize (nclass, fl_t (0));
      _opt.nclass = nclass;
    }
#endif
    // clasification interface
    void printStat () { TIMER (_timer_pool.print ()); }
#ifdef USE_STRING_FEATURE
    void classify (std::vector <const char*>& fv_, w_t* m) {
      fv_t fv; // do not expect fast classification
      fv.reserve (fv.size ());
      for (std::vector <const char*>::iterator it = fv_.begin ();
           it != fv_.end (); ++it) {
        int fn = _fm.fs2fn (*it);
        if (fn != -1) fv.push_back (static_cast <uint> (fn));
      }
#else
    void classify (std::vector <uint>& fv, w_t* m) {
#endif
#ifdef USE_MULTICLASS
      if (m->size () < _opt.nclass) m->resize (_opt.nclass);
#endif
      uint len = static_cast <uint> (fv.size ());
      if (_opt.kernel == LINEAR)
        _getMargin (*m, &fv[0], &fv[len]);
      else {
        _fm.convertFv2Fv (&fv[0], &fv[len], len);
        _getMarginPoly (*m, &fv[0], &fv[len]);
      }
    }
#ifdef USE_MULTICLASS
#ifdef USE_STRING_FEATURE
    const char* getLabel (std::vector <const char*>& fv) {
#else
    const char* getLabel (fv_t& fv) {
#endif
      classify (fv, &_m);
      const label_t y_
        = static_cast <label_t> (std::max_element (&_m[0], &_m[_opt.nclass]) - &_m[0]);
      return _lm.get_label (y_);
    }
#ifdef USE_STRING_FEATURE
    bool binClassify (std::vector <const char*>& fv, const char* label) {
#else
    bool binClassify (fv_t& fv, const char* label) {
#endif
      const label_t y = _lm.get_id (label);
      classify (fv, &_m);
      const label_t y_
        = static_cast <label_t> (std::max_element (&_m[0], &_m[_opt.nclass]) - &_m[0]);
      return y_ == y;
    }
#else
    // sigmoid w/o fitting (a stupid variant of J. Plat 1999)
    double threshold () { return _sigmoid (0); }
#ifdef USE_STRING_FEATURE
    bool binClassify (std::vector <const char*>& fv)
#else
    bool binClassify (fv_t& fv)
#endif
    { w_t m = 0; classify (fv, &m); return m > 0; }
#ifdef USE_STRING_FEATURE
    double getProbability (std::vector <const char*>& fv)
#else
    double getProbability (fv_t& fv)
#endif
    { w_t m = 0; classify (fv, &m); return _sigmoid (m); }
#endif
    // training interface
#ifdef USE_MULTICLASS
    void set_ex (ex_t& x, const char* ys, fv_t& fv , bool store, bool flag)
    { set_ex (x, _lm.set_id (ys), fv, store, flag); }
#endif
    void set_ex (ex_t& x, const label_t y, fv_t& fv, bool store, bool flag) {
      x.label () = y;
      x.set_x (&fv[0], fv.size (), store, flag ? &_fm : 0);
    }
    void process_example (ex_t& x, bool online = true) {
      if (online && _opt.kernel == POLY && x.getSize () > 0)
        _fm.convertFv2Fv (x.begin (), x.end (), x.rbegin ());
      _process_example (x);
    }
    void train_from_file (const char* lfn, const uint iter, const char* tfn = "", const bool instant = false) {
      switch (_opt.buffer) {
        case RAM:  train <mem_pool  <ex_t> > (lfn, iter, tfn, instant); break;
        case DISK: train <disk_pool <ex_t> > (lfn, iter, tfn, instant); break;
        case null: train <null_pool <ex_t> > (lfn, iter, tfn, instant); break;
      }
    }
    template <typename Pool>
    void train (const char* lfn, const uint iter, const char* tfn = "", const bool instant = false) {
      // read examples into pool prior to training
      Pool pool;
      const char* pool_action[] = { "loading", "caching", "preparing" };
      std::fprintf (stderr, "%s examples..", pool_action[_opt.buffer]);
      pool.read (lfn, &_lm, &_fm, _opt.kernel != LINEAR || _opt.featT != 1, _opt.M);
      std::fprintf (stderr, "done.\n");
      train (pool, iter, tfn, instant); // do
    }
    template <typename Pool>
    void train (Pool& pool, const uint iter, const char* tfn = "", const bool instant = false) {
#ifdef _OPENMP
      if (_opt.nthr) {
        if (_opt.kernel == POLY)
          warnx ("WARNING: parallel training is avelable for linear kernel [-t 0].");
        else if (_opt.buffer != RAM)
          warnx ("WARNING: parallel training needs buffeirng examples [-b 0].");
      }
#endif
#ifdef USE_MULTICLASS
      if (_opt.buffer == null && _opt.nclass <= 1)
        errx (1, "tell # classes [--num-classes] to opal when you don't buffer the data.");
      // # classes has been changed
      init_weight (_opt.buffer == null ? _opt.nclass : _lm.nclass ());
#endif
      if (_opt.kernel == POLY)
#ifdef USE_ARRAY_TRIE
        init_weight_trie (),
#endif
          _fm.build (), pool.setup (_fm); // feature packing
      else if (_opt.featT != 1)
        pool.setup (_fm, false);
      if (_opt.shuffle) pool.shuffle (); // shuffling data
      Pool test_pool;
      if (std::strcmp (tfn, "") != 0) {
        test_pool.read (tfn, &_lm, &_fm);
        if (_opt.kernel == POLY) test_pool.setup (_fm);
      }
      // automatically adjust _opt.C value
      if (std::fpclassify (_opt.C) == FP_ZERO) _adjust_C (pool, iter);
#ifdef _OPENMP
      Model* model = 0;
      Pool*  pool_ = 0;
      // std::vector <pthread_t> thr (_opt.nthr);
      // std::vector <thr_args_t <Pool> > thr_args (_opt.nthr);
      if (_opt.nthr) {
        model = static_cast <Model*> (::operator new (static_cast <size_t> (_opt.nthr) * sizeof (Model)));
        pool_ = new Pool[_opt.nthr];
        const typename Pool::data_t& data = *pool.body ();
        const size_t nex
          = data.size () / _opt.nthr + (data.size () % _opt.nthr == 0 ? 0 : 1);
        for (long i (0), offset (0); i < _opt.nthr; ++i, offset += nex) {
          new (&model[i]) Model (_opt);
#ifdef USE_MULTICLASS
          model[i].init_weight (_opt.nclass);
#endif
          pool_[i].assign (&data[offset], std::min (nex, data.size () - offset));
        }
      }
#endif
      for (uint i = 1; i <= iter; ++i) {
#ifdef _OPENMP
        if (_opt.nthr) { // distribute
#pragma omp parallel for num_threads (_opt.nthr)
          for (int j = 0; j < _opt.nthr; ++j) { // parallel training
            if (i != 1) model[j].reinit_weight (_w);
            model[j].train1 (pool_[j]);
          }
          std::fill (_w.begin (), _w.end (), _m0); // reset
          for (int j = 0; j < _opt.nthr; ++j) model[j].join (_w);
          _nf = static_cast <uint> (_w.size ()) - 1;
          const fl_t factor = static_cast <fl_t> (1.0 / _opt.nthr);
          for (uint fi = 0; fi <= _nf; ++fi) _w[fi] *= factor;
          if (_opt.average) {
#pragma omp parallel for num_threads (_opt.nthr)
            for (int j = 0; j < _opt.nthr; ++j)
              model[j].average1 (_w);
            if (i == iter || std::strcmp (tfn, "") != 0) {
              std::fill (_wa.begin (), _wa.end (), _m0); //
              for (int j = 0; j < _opt.nthr; ++j) model[j].join (_wa, true);
            }
          }
          _nex += pool.body ()->size ();
        } else
#endif
          train1 (pool);
        std::fprintf (stderr, "\r%s%s iter=%-3u #ex=%-8ld ",
                      _opt.average ? "Avg " : "", algo[_opt.algo], i, _nex);
        if (_opt.kernel == POLY) std::fprintf (stderr, "#SV=%u; ", _nsv);
        if (std::strcmp (tfn, "") != 0) {
          Model m (_opt);
#ifdef USE_MULTICLASS
          m.init_weight (_opt.nclass);
#endif
          const fl_t n = static_cast <fl_t> (_opt.nthr == 0 ? _nex + 1 : _nex + (1 + i) * static_cast <size_t> (_opt.nthr));
          if (_opt.kernel == LINEAR)
            m.test (test_pool, _w, _wa, n);
          else {
#ifdef USE_ARRAY_TRIE
            m.init_weight_trie ();
#endif
            m.test (test_pool, _ftrie, _fw, n, _sv, _alpha);
          }
        }
#ifdef USE_MULTICLASS
        if (_opt.buffer == null && _opt.nclass != _lm.nclass ())
          errx (1, "\n# classes mismatch; [--num-clsses] says %d, actual %d in training data.", _opt.nclass, _lm.nclass ());
#endif
      }
#ifdef _OPENMP
      if (_opt.nthr) {
        for (int i = 0; i < _opt.nthr; ++i) model[i].~Model ();
        ::operator delete (model);
        delete [] pool_;
      }
#endif
      // averaging here
      if (_opt.average) {
        const fl_t n = static_cast <fl_t> (_opt.nthr == 0 ? _nex + 1 : _nex + (1 + iter) * static_cast <size_t> (_opt.nthr));
        _average (n, instant || _opt.prob);
      }
#ifndef USE_MULTICLASS
      if (_opt.prob) _sigmoid_fitting (pool); // sigmoid fitting
#endif
    }
#ifdef _OPENMP
    void join (wv_t& w, const bool average = false) const {
      if (w.size () < _nf) w.resize (_nf + 1, _m0);
      const wv_t& w_ = average ? _wa : _w;
      for (uint fi = 0; fi <= _nf; ++fi) w[fi] += w_[fi];
    }
    void reinit_weight (const wv_t& w) {
      ++_nex;
      if (_w.size () < w.size ()) {
        _nf = static_cast <uint> (w.size ()) - 1;
        _w.resize  (_nf + 1, _m0);
        if (_opt.average) _wa.resize (_nf + 1, _m0);
      }
      std::copy (w.begin (), w.end (), _w.begin ());
    }
    void average1 (const wv_t& w) {
      ++_nex;
      for (uint fi = 0; fi <= _nf; ++fi)
        _wa[fi] += (w[fi] - _w[fi]) * static_cast <fl_t> (_nex);
    }
#endif
    template <typename Pool>
    void train1 (Pool& pool) {
      for (typename Pool::elem_t* x = pool.init (); x; x = pool.get ()) {
#ifdef USE_MULTICLASS
        if (_opt.nclass < _lm.nclass ())
          errx (1, "set # classes [--num-classes] to an appropriate value.");
#endif
        _process_example (*x);
      }
    }
    void test_on_file (const char* tfn = 0, const uint16_t output = 0) {
      null_pool <ex_t> pool (output >= 2);
      pool.read (tfn, &_lm, &_fm);
      if (_opt.kernel == POLY) pool.setup (_fm);
      test (pool, output);
    }
    // inherit parameters prior to test
    template <typename Pool> // linear
    void test (Pool& pool, const wv_t& w, const wv_t& wa, const fl_t n) {
      wv_t (w).swap (_w); _nf = static_cast <uint> (_w.size () - 1); // dup?
      if (_opt.average)
        for (uint fi = 0; fi <= _nf; ++fi) _w[fi] -= wa[fi] / n;
      test (pool);
    }
    template <typename Pool> // polynomial
    void test (Pool& pool, trie_t** ftrie, wv_t* fw, const fl_t n, const std::vector <sv_t*>& sv, const wv_t& alpha) {
      std::swap (_ftrie, ftrie); // it's a deal
      for (uint i = PSEUDO_TRIE_N[_opt.d]; i < _nbin; ++i)
        _fw[i].resize (fw[i].size (), _m0);
      for (uint i = 0; i < sv.size (); ++i)
        _pushTo (*sv[i], sv[i]->getWeight () - (_opt.average ? alpha[i] / n : _m0));
      test (pool);
      std::swap (_ftrie, ftrie);
    }
    template <typename Pool>
    void test (Pool& pool, uint16_t output = 0) {
      const bool prune = _opt.pruning;
      const bool output_example = output & 0x100;
      output &= 0xff;
      if (output >= 3) _opt.pruning = false; // turn off pruning if margin needed
      TIMER (_timer = _timer_pool.push ("classify"));
#ifdef USE_MULTICLASS
      uint corr (0), incorr (0);
      if (_m.size () < _opt.nclass) _m.resize (_opt.nclass);
#else
      uint pp (0), pn (0), np (0), nn (0);
      w_t _m = 0;
      if (output >= 4 &&
          std::fpclassify (_sigmoid_A + 1.0) == FP_ZERO &&
          std::fpclassify (_sigmoid_B) == FP_ZERO)
        warnx ("WARNING: you may have forgotten to set [-P] in training.");
#endif
      for (const typename Pool::elem_t* x = pool.init (); x; x = pool.get ()) {
#ifdef USE_MULTICLASS
        if (_opt.nclass < _lm.nclass ())
          warnx ("WARNING: found a label unseen in training.");
#endif
        TIMER (_timer->startTimer ());
        if (_opt.kernel == LINEAR) _getMargin     (_m, x->begin (), x->end ());
        else                       _getMarginPoly (_m, x->begin (), x->end ());
        TIMER (_timer->stopTimer ());
#ifdef USE_MULTICLASS
        const label_t y_
          = static_cast <label_t> (std::max_element (&_m[0], &_m[_opt.nclass]) - &_m[0]);
        if (y_ == x->getLabel ()) ++corr; else ++incorr;
        if (output >= 2) {
          std::fprintf (stdout, "%s", _lm.get_label (static_cast <size_t> (y_)));
          if (output >= 3)
            for (uint i = 0; i < _opt.nclass; ++i)
              std::fprintf (stdout, " %s:%f", _lm.get_label (i), _m[i]);
          if (output_example) std::fprintf (stdout, "\t%s", pool.curr ());
          std::fprintf (stdout, "\n");
        }
#else
        if (_m >= 0) if (x->getLabel () > 0) ++pp; else ++np;
        else         if (x->getLabel () > 0) ++pn; else ++nn;
        if (output >= 2) {
          std::fprintf (stdout, "%s", _m >= 0 ? "+1" : "-1");
          if (output >= 3)
            std::fprintf (stdout, " %f",
                          output == 3 ? _m : (_m > 0 ? _sigmoid (_m) : 1 - _sigmoid (_m)));
          if (output_example) std::fprintf (stdout, "\t%s", pool.curr ());
          std::fprintf (stdout, "\n");
        }
#endif
      }
#ifdef USE_MULTICLASS
      std::fprintf (stderr, "acc. %.4f (corr %u) (incorr %u)\n",
                    corr * 1.0 / (corr + incorr), corr, incorr);
#else
      std::fprintf (stderr, "acc. %.4f (pp %u) (pn %u) (np %u) (nn %u)\n",
                    static_cast <fl_t> (pp + nn) * 1.0 / (pp + pn + np + nn),
                    pp, pn, np, nn);
#endif
      _opt.pruning = prune;
    }
    void batch () { test_on_file (); }
    bool load (const char* mfn) {
      std::fprintf (stderr, "loading..");
#ifdef USE_STRING_FEATURE
      char* ffn
        = std::strcat (std::strcpy (new char[std::strlen (mfn) + 10], mfn), 
                       ".features");
      _fm.load (ffn);
      delete [] ffn;
#endif
      FILE* reader = std::fopen (mfn, "r");
      // examine model type
      if (! reader || std::feof (reader)) 
        errx (1, "cannot read a model: %s", mfn);
      char buf[BUF_SIZE]; std::setvbuf (reader, &buf[0], _IOFBF, BUF_SIZE);
      const char flag = static_cast <char> (std::fgetc (reader));
      if (std::fseek (reader, 0, SEEK_SET) != 0) return false;
      char*  line = 0;
      size_t read = 0;
      if (flag != 'o') {
        _opt.kernel = LINEAR;
#ifdef USE_MULTICLASS
        if (flag == 0) // w[0] == 0
          errx (1, "found a model for binary-class; use opal instead of opal-multi");
        if (! getLine (reader, line, read)) return false;
        if (char* p = std::strstr (line, "# labels: "))
          _lm.read (p, line + read - 1);
        init_weight (_lm.nclass ());
        //
        const long offset = std::ftell (reader);
        if (std::fseek (reader, 0, SEEK_END) != 0) return false; // move to end
        const long eow = std::ftell (reader);
#else
        if (flag != 0) // w[0] != 0
          errx (1, "found a model for multi-class; use opal-multi instead of opal");
        const long offset = std::ftell (reader);
        if (std::fseek (reader, 0, SEEK_END) != 0) return false; // move to end
        const long eow = std::ftell (reader)
                         - static_cast <long> (sizeof (double) * 2); // sigmoid
#endif
        _nf = static_cast <uint> (eow - offset) / static_cast <uint> (sizeof (fl_t) * _opt.nclass) - 1;
        // read a model
        if (std::fseek (reader, offset, SEEK_SET) != 0) return false;
        _w.resize (_nf + 1, _m0);
        if (_opt.average) _wa.resize (_nf + 1, _m0);
        long nf (0);
#ifdef USE_MULTICLASS
        for (size_t i = 0; i <= _nf; ++i) {
          std::fread (&_w[i][0], sizeof (fl_t), _opt.nclass, reader);
          if (! std::equal (&_w[i][0], &_w[i][_opt.nclass], &_m0[0])) ++nf;
        }
#else
        std::fread (&_w[0], sizeof (fl_t), _nf + 1, reader);
        std::fread (&_sigmoid_A, sizeof (double), 1, reader);
        std::fread (&_sigmoid_B, sizeof (double), 1, reader);
        nf = _nf - static_cast <uint> (std::count (&_w[1], &_w[_nf + 1], fl_t (0)));
#endif
        std::fprintf (stderr, "done (# features = %ld).\n", nf);
        std::fclose (reader);
      } else {
        _opt.kernel = POLY;
        if (! getLine (reader, line, read) || std::strncmp (line, "opal", 4))
          return false;
        while (getLine (reader, line, read))
          if (std::strstr (line, "# threshold b") != NULL) break;
#ifdef USE_MULTICLASS
          else if (char* const p = std::strstr (line, "# labels: ")) {
            _lm.read (p, line + read - 1);
            init_weight (_lm.nclass ());
          }
#else
          else if (std::strstr (line, "# sigmoid")) {
            if (line[read - 2] == 'A')
              _sigmoid_A = std::strtod (line, NULL);
            else
              _sigmoid_B = std::strtod (line, NULL);
          }
#endif
          else if (std::strstr (line, "# kernel parameter -d") != NULL) {
            const uint d = strton <uint> (line, NULL);
            if (_opt.d) {
              if (_opt.d != d)
                errx (1, "input kernel_degree [-d] conflicts with %s", mfn);
            } else {
              _opt.d = d;
              _MIN_N = _opt.shrink ? 1U << PSEUDO_TRIE_N[_opt.d] : _opt.splitN + 1;
              _COMMON_BITS = std::min (_MIN_N, COMMON_BIT_UNIT * COMMON_FACTOR);

            }
#ifdef USE_ARRAY_TRIE
            init_weight_trie ();
#endif
          }
#ifdef USE_MULTICLASS
        if (_opt.nclass == 1)
          errx (1, "found a model for binary-class; try opal");
#endif
        mem_pool <sv_t> pool;
        pool.read (reader, &_lm, &_fm, true);
        std::fclose (reader);
        _fm.build ();
        pool.setup (_fm);
        for (const mem_pool <sv_t>::elem_t* x = pool.init (); x; x = pool.get ())
          _pushTo (*x, x->getWeight ());
        long ncf (0);
        for (size_t i = 0; i < _w.size (); ++i)
          if (! _is_zero (_w[i])) ++ncf;
        for (uint i = PSEUDO_TRIE_N[_opt.d]; i < _nbin; ++i)
          for (size_t j = 0; j < _fw[i].size (); ++j)
            if (! _is_zero (_fw[i][j])) ++ncf;
        std::fprintf (stderr, "done (# explicit features = %ld).\n", ncf);
      }
      return true;
    }
    void save (const char* mfn) { // const
      std::fprintf (stderr, "saving..");
      // write weight vectors
      FILE* writer = std::fopen (mfn, "w");
      if (! writer)
        errx (1, "cannot write the model: %s", mfn);
      char buf[BUF_SIZE]; std::setvbuf (writer, &buf[0], _IOFBF, BUF_SIZE);
      if (_opt.kernel == LINEAR) { // linear training
#ifdef USE_MULTICLASS
        _lm.write (writer);
        for (size_t i = 0; i <= _nf; ++i)
          std::fwrite (&_w[i][0], sizeof (fl_t), _opt.nclass, writer);
#else
        std::fwrite (&_w[0],      sizeof (fl_t), _nf + 1, writer);
        std::fwrite (&_sigmoid_A, sizeof (double),     1, writer);
        std::fwrite (&_sigmoid_B, sizeof (double),     1, writer);
#endif
      } else { // kernel; output model in SVM-Light-like format
        std::fprintf (writer, "opal # $Id: pa.h 1934 2022-01-23 02:45:17Z ynaga $\n");
#ifdef USE_MULTICLASS
        _lm.write (writer);
#endif
        std::fprintf (writer, "1 # kernel type\n");
        std::fprintf (writer, "%u # kernel parameter -d\n", _opt.d);
        std::fprintf (writer, "1 # kernel parameter -s\n");
        std::fprintf (writer, "1 # kernel parameter -r\n");
#ifndef USE_MULTICLASS
        std::fprintf (writer, "%.16g # sigmoid -A\n", _sigmoid_A);
        std::fprintf (writer, "%.16g # sigmoid -B\n", _sigmoid_B);
#endif
        for (uint i = 0; i < _opt.nclass; ++i) std::fprintf (writer, "0 ");
        std::fprintf (writer, "# threshold b\n"); // for pecco
        // output support vectors w/ alpha
        for (uint i = 0; i < _nsv; ++i) {
          _fm.revertFv2Fv (_sv[i]->begin (), _sv[i]->end ());
          _sv[i]->print (writer, &_fm);
        }
      }
      std::fclose (writer);
#ifdef USE_STRING_FEATURE
      char* ffn
        = std::strcat (std::strcpy (new char[std::strlen (mfn) + 10], mfn),
                       ".features");
      _fm.save (ffn);
#endif
      std::fprintf (stderr, "done.\n");
    }
#ifdef USE_DOUBLE_ARRAY_TRIE
    void dump_feature (const w_t& w, uint* f = 0, size_t d = 0) const {
      if (f == 0) // bias
        std::fprintf (stdout, "polynomial_bias");
      else { // printing features
#ifdef USE_STRING_FEATURE
        std::fprintf (stdout, "f_%s", _fm.fn2fs (f[0]));
        for (uint j = 1; j < d; ++j)
          std::fprintf (stdout, ":%s", _fm.fn2fs (f[j]));
#else
        std::fprintf (stdout, "f_%d", f[0]);
        for (uint j = 1; j < d; ++j)
          std::fprintf (stdout, ":%d", f[j]);
#endif
      }
      // printing weights
#ifdef USE_MULTICLASS
      for (size_t i = 0; i < _opt.nclass; ++i)
        std::fprintf (stdout, "\t%s:%.16g", _lm.get_label (i),
                      _opt.kernel == LINEAR ? w[i] : w[i] * COEFF[_opt.d][d]);
      std::fprintf (stdout, "\n");
#else
      std::fprintf (stdout, "\t%.16g\n",
                    _opt.kernel == LINEAR ? w : w * COEFF[_opt.d][d]);
#endif
    }
    void dump () const {
      if (_opt.kernel == LINEAR) {
        for (uint i = 1; i <= _nf; ++i)
          if (! _is_zero (_w[i]))
            dump_feature (_w[i], &i);
      } else {
        dump_feature (_bias);
        const uint N = std::min ((1U << PSEUDO_TRIE_N[_opt.d]) - 1, _opt.splitN);
        uint f[MAX_KERNEL_DEGREE];
        for (uint i = 1; i <= std::min (N, _nf); ++i) {
          size_t p[MAX_KERNEL_DEGREE];
          f[0] = _fm.fi2fn (i);
          p[0] = _opt.d == 1 ? i - 1 :
                 (_opt.d == 2 ? i * (i - 1) / 2 : (i - 1) * (i * i - 2 * i + 6) / 6);
          if (! _is_zero (_w[p[0]]))
            dump_feature (_w[p[0]], &f[0], 1);
          if (_opt.d > 1)
            for (uint j = 1; j < i; ++j) {
              f[1] = _fm.fi2fn (j);
              p[1] = p[0] + 1 + (_opt.d == 2 ? j - 1 : j * (j - 1) / 2);
              if (! _is_zero (_w[p[1]])) dump_feature (_w[p[1]], &f[0], 2);
              if (_opt.d > 2)
                for (uint k = 1; k < j; ++k) {
                  f[2] = _fm.fi2fn (k);
                  p[2] = p[1] + 1 + k - 1;
                  if (! _is_zero (_w[p[2]])) dump_feature (_w[p[2]], &f[0], 3);
                }
            }
        }
        if (N < _opt.splitN)
          for (uint i = PSEUDO_TRIE_N[_opt.d]; i < _nbin; ++i) {
            trie_t* const trie = _ftrie[i];
            const wv_t& fw = _fw[i];
            const size_t nkeys = trie->num_keys ();
            trie_t::result_triple_type* fs = new trie_t::result_triple_type[nkeys];
            trie->dump (fs, nkeys);
            for (uint j = 0; j < nkeys; ++j) {
              const uint n = static_cast <uint> (fs[j].value) - 1; // bug here
              if (_is_zero (fw[n])) continue;
              uchar s[16];
              trie->suffix (reinterpret_cast <char*> (&s[0]), fs[j].length, fs[j].id);
              byte_encoder encoder;
              uint k = 0;
              for (uint len (0); len < fs[j].length; ++k)
                len += encoder.decode (f[k], &s[len]), f[k] = _fm.fi2fn (f[k]);
              dump_feature (fw[n], &f[0], k);
            }
            delete [] fs;
          }
      }
    }
#endif
  private:
    // type alias
#ifdef USE_POLYK
    typedef std::vector <uint>   ss_t;
#else
    typedef std::vector <sv_t*>  ss_t;
#endif
    typedef ss_t::const_iterator  ss_it;
    typedef std::vector <ss_t>    f2ss_t;
#if defined (USE_HASH) || defined (USE_TR1_HASH)
    struct hash_sv : std::unary_function <const sv_t*, size_t> {
      result_type operator () (argument_type f) const
      { return std::accumulate (f->begin (), f->end (), FNV_BASIS, inc_fnv ()); }
    };
    struct eq_sv : std::binary_function <const sv_t*, const sv_t*, bool> {
      result_type operator () (first_argument_type a, second_argument_type b) const {
        return a->getSize () == b->getSize () &&
          std::equal (a->begin (), a->end (), b->begin ());
      }
    };
#ifdef USE_HASH
    typedef std::unordered_map <const sv_t*, uint, hash_sv, eq_sv> fv2s_t;
#else
    typedef std::tr1::unordered_map <const sv_t*, uint, hash_sv, eq_sv> fv2s_t;
#endif
#else
    struct less_sv : std::binary_function <const sv_t*, const sv_t*, bool> {
      result_type operator () (first_argument_type a, second_argument_type b) const {
        for (const uint* p (a->begin ()), *q (b->begin ());; ++p, ++q) {
          if (p == a->end ()) return q != b->end ();
          if (q == b->end ()) return false;
          else if (*p < *q)   return true;
          else if (*p > *q)   return false;
        }
      }
    };
    typedef std::map <const sv_t*, uint, less_sv> fv2s_t;
#endif
    // variables
    const option         _opt;
    uint                 _MIN_N;
    uint                 _COMMON_BITS;
    lmap                 _lm;     // label map
    fmap                 _fm;     // feature map
    size_t               _nex;    // # processed examples
    uint                 _nsv;    // # support vectors
    uint                 _nf;     // # features (= max feature id)
    sv_t                 _s;
    w_t                  _m0;
    w_t                  _bias;   // just keep constant for kernel expansion
    f2ss_t               _f2ss;   // feature id        -> support vector id
    fv2s_t               _fv2s;   // feature vector    -> support vector id
    std::vector <char>   _fbit;
    uint64_t             _fbits[COMMON_FACTOR];  //
    wv_t                 _alpha;  // support vector id -> (averaged) alpha
    std::vector <sv_t*>  _sv;     // support vector id -> support vector
    wv_t                 _w;      // feature id        -> weight
    wv_t                 _wa;     // feature id        -> (average) weight
#ifdef USE_POLYK
    std::vector <uint>   _dot;
#endif
    std::vector <fl_t>   _polyk;   // poly kernel (sliced); int -> fl_t
    size_t    _nbin;   // # feature weight tries
    trie_t**  _ftrie;  // feature weight tries
    wv_t*     _fw;
    struct pn_t { // upper- and lower-bounds of weights
      w_t  pos;  // or max
      w_t  neg;  // or min
      pn_t (const w_t& m0 = w_t ()) : pos (m0), neg (m0) {}
      pn_t& operator= (const pn_t& b) { pos = b.pos; neg = b.neg; return *this; }
    };
    std::vector <pn_t>  _f2pn;  // feature id -> weight summation of pos/neg svs
    std::vector <pn_t>  _bound;
#pragma pack(1)
    struct tpm_t { // TODO: we can mark and sweep older nodes
      w_t   tpm; // temporal partial margin
      uint  t;   // time signature
      tpm_t (const w_t& m0 = w_t ()) : tpm (m0),     t (0)   {}
      tpm_t (const tpm_t& l_)        : tpm (l_.tpm), t (l_.t) {}
      tpm_t& operator= (const tpm_t& l) { tpm = l.tpm; t = l.t; return *this; }
    };
#pragma pack()
    class ring {
      // circular buffer is implemented using vector instead of deque
      // since the overhead of deque is substantial
    public:
      typedef std::pair <const sv_t*, w_t>  data_t;
      uint  t;  // # updates
      ring () : t (0), _max (0), _M () {}
      size_t max  () const { return _max; }
      size_t size () const { return std::min (max (), _M.size ()); }
      const data_t& operator[] (const size_t i) const { return _M[i]; }
      std::vector <data_t>::const_reverse_iterator rbegin () const
      { return _M.rbegin (); }
      void set_max (const uint n) { _max = n; }
      void push (const data_t& s, bool overt) {
        if      (! overt || _max == 0) ++_max;
        else if (_M.size () == (_max << 1))
          std::copy (&_M[_max], &_M[_max << 1], &_M[0]), _M.resize (_max);
        _M.push_back (s);
        ++t;
      }
    private:
      uint  _max;  // # max elements allowed
      std::vector <data_t>  _M;
    };
    std::vector <ring>    _f2ss_up;
    trie_t                _pmtrie;
    std::vector <tpm_t>   _tpm;
#ifdef USE_TEMPORAL_PRUNING
    std::vector <pn_t>    _tpn;
#endif
    std::vector <size_t>  _limk;  // condition in reusing margin
    std::vector <int>     _pfv;
    const double          _thresh;
#ifdef USE_MULTICLASS
    w_t                   _m, _pm, _pm1, _pm2, _pm3;
    pn_t                  _pn;
    loss_t                _ls;
#endif
    double                _sigmoid_A; // sigmoid parameter A
    double                _sigmoid_B; // sigmoid parameter B
#ifdef USE_TIMER
    ny::TimerPool         _timer_pool;
    ny::Timer*            _timer;
#endif
    // uncopyable
    Model (const Model&);
    Model& operator= (const Model&);
    // internal functions
    void _precompute_kernel (const size_t nf) {
      for (size_t i = _polyk.size (); i <= nf; ++i)
#ifdef USE_POLYK
        _polyk.push_back  (std::pow (static_cast <double> (i) + 1,
                                     static_cast <double> (_opt.d)));
#else
        _polyk.push_back (std::pow ((static_cast <double> (i) + 1) + 1,
                                    static_cast <double> (_opt.d)) -
                          std::pow (static_cast <double> (i) + 1,
                                    static_cast <double> (_opt.d)));
#endif
    }
    void _getMargin (w_t& m, const uint* first, const uint* last) const {
      m = fl_t (0);
      for (const uint* it = first; it != last && *it <= _nf; ++it)
        m += _w[*it];
    }
#ifdef USE_POLYK
    void _getMarginPoly (w_t& m, const uint* first, const uint* last, const label_t = MAX_NUM_CLASSES) {
      m = fl_t (0);
      // polynomial kernel inverted
      if (_dot.size () < _nsv) _dot.resize (_nsv, 0);
      for (const uint* it = first; it != last && *it <= _nf; ++it) {
        const ss_t& ss = _f2ss[*it];
        for (ss_it st = ss.begin (); st != ss.end (); ++st) ++_dot[*st];
      }
      for (uint i = 0; i < _nsv; ++i)
        m += _sv[i]->getWeight () * _polyk[_dot[i]], _dot[i] = 0;
    }
#else
    void _getMarginPoly (w_t& m, const uint* first, const uint* last, const label_t y = MAX_NUM_CLASSES) {
      m = fl_t (0);
      if (_opt.slicing)
        while (_limk.size () <= static_cast <size_t> (last - first))
          switch (_opt.d) {
            case 1: _limk.push_back (0); break;
            case 2: _limk.push_back (1); break;
            case 3: _limk.push_back (static_cast <uint> ((_limk.size () >> 1) + (_limk.size () & 0x1))); break;
          }
      _project_ro (m, first, last, y);
    }
#endif
    // update
    void _addTo (const ex_t& x, const w_t& t) {
      if (x.maxF () > _nf) { // widen
        _nf = x.maxF ();
        _w.resize (_nf + 1, _m0);
        if (_opt.average) _wa.resize (_nf + 1, _m0);
      }
      for (const uint* it = x.begin (); it != x.end (); ++it) {
        _w[*it] += t;
        if (_opt.average) _wa[*it] += t * static_cast <fl_t> (_nex);
      }
    }
    template <typename elem_t>
    void _pushTo (const elem_t& x, const w_t& t) {
      // check support vector overlap; a little overhead
      _s.set_x (x.getBody (), x.getSize ());
      fv2s_t::const_iterator fit =_fv2s.find (&_s);
      if (fit == _fv2s.end ()) { // add support vector
        if (x.maxF () > _nf) {
          _nf = x.maxF ();
          _f2ss.resize (_nf + 1);
          _fbit.resize (_nf + 1, false);
          if (_opt.pruning) _f2pn.resize (_nf + 1, pn_t (_m0));
          if (_opt.slicing) _f2ss_up.resize (_nf + 1); // processed SV
        }
        _precompute_kernel (x.getSize ());
#ifdef USE_POLYK
        sv_t* p = new sv_t (x.getBody (), t, x.getSize ());
#else
        const uint* end = std::lower_bound (x.begin (), x.end (), _COMMON_BITS);
        sv_t* p = new sv_t (x.getBody (), t, x.getSize (),
                            static_cast <uint> (end - x.begin ()));
#endif
        fit = _fv2s.insert (fv2s_t::value_type (p, _nsv)).first;
#ifdef USE_POLYK
        for (const uint* it = x.rbegin (); it != x.rend (); --it)
          _f2ss[*it].push_back (_nsv);
#else
        for (const uint* it = x.rbegin (); it != x.rend () && *it >= _MIN_N; --it)
          _f2ss[*it].push_back (p);
#endif
        _sv.push_back (p);
        ++_nsv;
        if (_opt.average) _alpha.push_back (t * static_cast <fl_t> (_nex));
      } else { // just update
        _sv[fit->second]->weight () += t;
        if (_opt.average) _alpha[fit->second] += t * static_cast <fl_t> (_nex);
      }
#ifndef USE_POLYK
      if (_opt.slicing)
        for (const uint* it = x.begin (); it != x.end (); ++it)
          _f2ss_up[*it].push (ring::data_t (_sv[fit->second], t),
                              _f2ss_up[*it].size () >= _f2ss[*it].size () ||
                              *it < _MIN_N);
      if (_opt.pruning)
        for (const uint* it = x.begin (); it != x.end (); ++it) {
          pn_t& fb = _f2pn[*it];
#ifdef USE_MULTICLASS
          for (uint i = 0; i < _opt.nclass; ++i)
            if (t[i] > 0) fb.pos[i] += t[i]; else fb.neg[i] += t[i];
#else
          if (t > 0) fb.pos += t; else fb.neg += t;
#endif
        }
      _project_rw (t, x.begin (), x.end ()); // explicit conjunctive features
      if (_opt.shrink) _shrink_trie ();
#endif
    }
#ifndef USE_POLYK
    size_t _inner_product (const sv_t* s, const uint lim, const bool flag = true) const {
      int dot = 0;
      for (uint j = 0; j < COMMON_FACTOR; ++j) {
#ifdef USE_SSE4_2_POPCNT
        dot += static_cast <int> (_mm_popcnt_u64 (_fbits[j] & s->sbit (j)));
#else
        const uint64_t r = _fbits[j] & s->sbit (j);
        dot += popTable16bit[(r >>  0) & 0xffff];
        dot += popTable16bit[(r >> 16) & 0xffff];
        dot += popTable16bit[(r >> 32) & 0xffff];
        dot += popTable16bit[(r >> 48) & 0xffff];
#endif
      }
      if (flag)
        for (const uint* sit = s->begin_r (); sit != s->end () && *sit <= lim; ++sit)
          dot += _fbit[*sit];
      return static_cast <size_t> (dot);
    }
    bool _reuse_pm (w_t& m_, size_t& pid, const size_t i, const uint fi, const uint prev, int& n, const bool projected = true) {
      // reuse partial margin; TODO: avoid expanding trie when unnecessary
      ring& ss = _f2ss_up[fi];
      if (n < 0) { // new feature sequence
        n = static_cast <int> (_tpm.size ());
        _pmtrie.update (pid, fi - prev, n + 1);
        _tpm.push_back (tpm_t (_m0));
#ifdef USE_TEMPORAL_PRUNING
        if (_opt.pruning) _tpn.push_back (pn_t (_m0));
#endif
        if (projected && ss.max () < _limk[i])
          ss.set_max (static_cast <uint> (_limk[i]));
      }
      tpm_t* const l = &_tpm[static_cast <uint> (n)];
      size_t nusv = ss.t - l->t; // # newly updated support vectors
      l->t = ss.t;
      if (nusv > ss.size ()) return false;
      if (nusv > (projected ? _limk[i] : _f2ss[fi].size ())) return false;
      m_ += l->tpm; // TODO: sweep older weights
      const bool flag = prev >= _COMMON_BITS;
      for (std::vector <ring::data_t>::const_reverse_iterator it = ss.rbegin ();
           nusv > 0; --nusv, ++it)
        m_ += it->second * _polyk[_inner_product (it->first, prev, flag)];
      return true;
    }
    void _update_weight (trie_t* const trie, size_t& pid, const uint fi, wv_t& fw, const w_t& t) {
      if (int& n = trie->update (pid, fi))
        fw[static_cast <uint> (n) - 1] += t; // too much
      else
        fw.push_back (t), n = static_cast <int> (fw.size ());
    }
    bool _retrieve_weight (const trie_t* const trie, size_t& pid, const uint fi, const wv_t& fw, w_t& t) {
      if (const int n = trie->traverse (pid, fi))
        t += fw[static_cast <uint> (n) - 1];
      else
        return false;
      return true;
    }
    void _retrieve_pm (const uint* it, const uint* tail, size_t& pid) {
      _pfv.clear ();
      for (uint prev = 0; it != tail; prev = *it, ++it) {
        if (const int n = _pmtrie.traverse (pid, *it - prev))
          _pfv.push_back (n - 1); // too much
        else
          break;
      }
    }
    void _estimate_bound (const uint* const first, const uint* it) {
      // compute bound from tail (rare) to head (common) feature
      const size_t len = static_cast <size_t> (it - first);
      if (_bound.size () < len) _bound.resize (len, pn_t (_m0));
      pn_t* p = &_bound[len - 1];
#ifdef USE_MULTICLASS
      if (_pn.pos.size () < _opt.nclass)
        _pn.pos.resize (_opt.nclass), _pn.neg.resize (_opt.nclass);
#else
      pn_t _pn;
#endif
      for (p->pos = p->neg = 0;; *(p - 1) = *p, --p) {
        _pn = _f2pn[*--it];
        const size_t max_dot // max inner product between common features
          = std::min (_polyk.size () - 1, static_cast <size_t> (it - first));
#ifdef USE_TEMPORAL_PRUNING
        const size_t index = static_cast <size_t> (p - &_bound[0]);
        if (index < _pfv.size ()) { // tighter bounds
          const uint n = static_cast <uint> (_pfv[index]);
          _pn.pos -= _tpn[n].pos, _pn.neg -= _tpn[n].neg;
          p->pos += _tpm[n].tpm, p->neg += _tpm[n].tpm;
        }
#endif
        p->pos += _pn.pos * _polyk[max_dot] + _pn.neg * _polyk[0];
        p->neg += _pn.neg * _polyk[max_dot] + _pn.pos * _polyk[0];
        if (it == first) break;
      }
    }
    bool _prune (w_t& m, const size_t i, label_t y) {
      const pn_t& b = _bound[i];
      const double thresh = y == MAX_NUM_CLASSES ? 0 : _thresh;
#ifdef USE_MULTICLASS
      if (y == MAX_NUM_CLASSES)
        y = static_cast <label_t> (std::max_element (&m[0], &m[_opt.nclass]) - &m[0]);
      for (uint j = 0; j < _opt.nclass; ++j)
        if (j != y && (m[y] + b.neg[y]) - (m[j] + b.pos[j]) <= thresh)
          return false;
      for (uint j = 0; j < _opt.nclass; ++j)
        m[j] += j == y ? b.neg[j] : b.pos[j];
#else
      if (y == MAX_NUM_CLASSES)
        y = m >= 0 ? 1 : -1;
      if ((y > 0 ? m + b.neg : - (m + b.pos)) <= thresh)
        return false;
      m += y > 0 ? b.neg : b.pos;
#endif
      return true;
    }
    void _project_rw (const w_t& t, const uint* const first, const uint* const last) {
      _bias += t;
      const uint* it  = first;
      const uint* rit = std::upper_bound (first, last, _opt.splitN);
#ifdef USE_ARRAY_TRIE
      const uint  N   = std::min ((1U << PSEUDO_TRIE_N[_opt.d]) - 1, _opt.splitN);
      for (; it != rit && *it <= N; ++it) {
        size_t p[MAX_KERNEL_DEGREE];
        switch (_opt.d) {
          case 1: _w[*it - 1] += t; break;
          case 2: _w[p[0] = *it * (*it - 1) / 2] += t;
              for (const uint* jt = first; jt != it; ++jt)
                _w[p[0] + 1 + *jt - 1] += t;
              break;
          case 3: _w[p[0] = (*it - 1) * (*it * *it - 2 * *it + 6) / 6] += t;
            for (const uint* jt = first; jt != it; ++jt) {
              _w[p[1] = p[0] + 1 + *jt * (*jt - 1) / 2] += t;
              for (const uint* kt = first; kt != jt; ++kt)
                _w[p[1] + 1 + *kt - 1] += t;
            }
        }
      }
#endif
      for (uint bin = PSEUDO_TRIE_N[_opt.d]; it != rit; ++it) {
        while (*it >> (bin + 1)) ++bin;
        trie_t* const trie = _ftrie[bin];
        wv_t& fw = _fw[bin];
#ifdef USE_DOUBLE_ARRAY_TRIE
        size_t* const p (&(trie->tracking_node[0])); // track traversed nodes
#else
        size_t p[MAX_KERNEL_DEGREE];
#endif
        _update_weight (trie, p[0] = 0, *it, fw, t);
        if (_opt.d > 1)
          for (const uint* jt = first; jt != it; ++jt) {
            _update_weight (trie, p[1] = p[0], *jt, fw, t);
            if (_opt.d > 2)
              for (const uint* kt = first; kt != jt; ++kt)
                _update_weight (trie, p[2] = p[1], *kt, fw, t);
          }
      }
    }
    void _project_ro (w_t& m, const uint* const first, const uint* const last, const label_t y) {
      // kernel slicing
      const fl_t* const coeff = COEFF[_opt.d];
      m += _bias * coeff[0];
      const uint* const end = std::upper_bound (first, last, _nf);
      if (first == end) return; // bug fix
      size_t pid = 0;
      if (_opt.slicing) _retrieve_pm (first, end, pid);
      if (_opt.pruning && ! _polyk.empty ()) _estimate_bound (first, end);
#ifdef USE_MULTICLASS
      if (_pm.size () < _opt.nclass) {
        _pm.resize  (_opt.nclass, 0);
        _pm1.resize (_opt.nclass, 0);
        _pm2.resize (_opt.nclass, 0);
        _pm3.resize (_opt.nclass, 0);
      }
#else
      w_t _pm (0), _pm1 (0), _pm2 (0), _pm3 (0);
#endif
#ifdef USE_ARRAY_TRIE
      const uint N = std::min ((1U << PSEUDO_TRIE_N[_opt.d]) - 1, _opt.splitN);
#endif
      std::fill (&_fbits[0], &_fbits[COMMON_FACTOR], 0);
      const uint* it = first;
      for (uint prev (0), bin (PSEUDO_TRIE_N[_opt.d]); it != end; ++it) {
        const size_t index = static_cast <size_t> (it - first);
        if (_opt.pruning && _prune (m, index, y)) break;
        _pm = fl_t (0);
        int n = index < _pfv.size () ? _pfv[index] : -1;
        if (! _opt.slicing ||
            ! _reuse_pm (_pm, pid, index, *it, prev, n, *it <= _opt.splitN)) {
          if (*it <= _opt.splitN) {
            size_t p[MAX_KERNEL_DEGREE];
            _pm1 = _pm2 = _pm3 = fl_t (0);
#ifdef USE_ARRAY_TRIE
            if (*it <= N) { // pseudo trie
              switch (_opt.d) {
                case 1: _pm1 += _w[*it - 1]; break;
                case 2: _pm1 += _w[p[0] = *it * (*it - 1) / 2];
                  for (const uint* jt = first; jt != it; ++jt)
                    _pm2 += _w[p[0] + 1 + *jt - 1];
                  break;
                case 3: _pm1 += _w[p[0] = (*it - 1) * (*it * *it - 2 * *it + 6) / 6];
                  for (const uint* jt = first; jt != it; ++jt) {
                    _pm2 += _w[p[1] = p[0] + 1 + *jt * (*jt - 1) / 2];
                    for (const uint* kt = first; kt != jt; ++kt)
                      _pm3 += _w[p[1] + 1 + *kt - 1];
                  }
              }
            } else
#endif
              {
                while (*it >> (bin + 1)) ++bin;
                const trie_t* const trie = _ftrie[bin];
                const wv_t& fw = _fw[bin];
                if (_retrieve_weight (trie, p[0] = 0, *it, fw, _pm1) &&
                    _opt.d > 1)
                  for (const uint* jt = first; jt != it; ++jt)
                    if (_retrieve_weight (trie, p[1] = p[0], *jt, fw, _pm2) &&
                        _opt.d > 2)
                      for (const uint* kt = first; kt != jt; ++kt)
                        _retrieve_weight (trie, p[2] = p[1], *kt, fw, _pm3);
              }
            _pm1 *= coeff[1], _pm2 *= coeff[2], _pm3 *= coeff[3];
            _pm += _pm1, _pm += _pm2, _pm += _pm3;
          } else {
            const ss_t& ss = _f2ss[*it];
            for (ss_it st = ss.begin (); st != ss.end (); ++st)
              _pm += (*st)->getWeight () * _polyk[_inner_product (*st, prev)];
          }
        }
        if (_opt.slicing) {
          _tpm[static_cast <uint> (n)].tpm = _pm;
#ifdef USE_TEMPORAL_PRUNING
          if (_opt.pruning) _tpn[static_cast <uint> (n)] = _f2pn[*it];
#endif
        }
        m += _pm; prev = *it;
        if (*it < _COMMON_BITS)
          _fbits[*it / COMMON_BIT_UNIT] |= 1UL << (*it & (COMMON_BIT_UNIT - 1));
        else
          _fbit[*it] = true;
      }
      for (const uint* jt = first; jt != it; ++jt) _fbit[*jt] = false; // reset
    }
    void _shrink_trie () {
      size_t rss = 0;
      for (uint i = PSEUDO_TRIE_N[_opt.d]; i < _nbin; ++i) {
#if   defined (USE_DOUBLE_ARRAY_TRIE)
        rss += sizeof (trie_t::node)  * _ftrie[i]->capacity ();
        rss += sizeof (trie_t::ninfo) * _ftrie[i]->capacity ();
        rss += sizeof (trie_t::block) * _ftrie[i]->capacity () >> 8;
#elif defined (USE_HASH_TRIE)
#ifdef USE_HASH
        typedef std::__detail::_Hash_node <trie_t::value_type, false> _Node;
#else
        typedef std::tr1::__detail::_Hash_node <trie_t::value_type, false> _Node;
#endif
        rss += sizeof (_Node*) * _ftrie[i]->bucket_count ();
        rss += sizeof (_Node)  * _ftrie[i]->size (); // ignore alignment
#else
        typedef std::_Rb_tree_node <trie_t::value_type> _Node;
        rss += sizeof (_Node)  * _ftrie[i]->size (); // ignore alignment
#endif
        if (rss > _opt.trieT) {
          std::fprintf (stderr,
                        "shrink splitN: 2^%lu-1 (= %u) => 2^%u-1 (= %u)\n",
                        _nbin, _opt.splitN, i, (1 << i) - 1);
          while (_nbin > i) {
            --_nbin;
            delete _ftrie[_nbin];
            wv_t ().swap (_fw[_nbin]);
          }
          _opt.splitN = (1U << _nbin) - 1;
          break;
        }
      }
    }
#endif /* USE_POLYK */
    void _process_example (const ex_t& x) {
#ifdef USE_MULTICLASS
      if (_m.size () < _opt.nclass) _m.resize (_opt.nclass);
#else
      w_t _m = 0;
#endif
      const label_t y = x.getLabel ();
      if (_opt.kernel == LINEAR) _getMargin     (_m, x.begin (), x.end ());
      else                       _getMarginPoly (_m, x.begin (), x.end (), y);
      ++_nex;
#ifdef USE_MULTICLASS
      label_t y_ = y ? 0 : 1; // most probable label other than y
      for (uint i = 0; i < _opt.nclass; ++i)
        if (i != y && _m[i] > _m[y_]) y_ = i;
      if (_m[y] - _m[y_] <= _thresh) {
        w_t& t = _m;
        if (_opt.algo == P) {
          t = fl_t (0), t[y] += 1.0, t[y_] -= 1.0;  // what's a simple
        } else {
          _ls.clear ();
          // compute margin
          for (uint j = 0; j < _opt.nclass; ++j)
            if (j != y) {
              const fl_t lj = std::max (fl_t (0), fl_t (1) - (_m[y] - _m[j]));
              if (lj > 0) _ls.push_back (loss_t::value_type (lj, j));
            }
          // examine support class
          std::sort (_ls.rbegin (), _ls.rend ());
          size_t k = 0;
          fl_t   l = 0; // summation of loss
          for (bool is_sc = true; k < _ls.size (); l += _ls[k].first, ++k) {
            switch (_opt.algo) {
              case P:   break;
              case PA:  is_sc &= l < (k + 1) * _ls[k].first; break;
              case PA1: is_sc &= l < std::min (k * _ls[k].first + _opt.C * x.getNorm (), static_cast <double> ((k + 1) * _ls[k].first)); break;
              case PA2: is_sc &= l < ((k + 1) * x.getNorm () + k / (2 * _opt.C)) / (x.getNorm () + 1 / (2 * _opt.C)) * _ls[k].first; break;
            }
            if (! is_sc) { _ls.resize (k); break; }
          } // k = |S|; support class 0 -> k - 1
            // update weights
          double penalty = 0;
          switch (_opt.algo) {
            case P:   break;
            case PA:  penalty = l / (k + 1); break;
            case PA1: penalty = std::max (l / k - _opt.C * x.getNorm () / k, static_cast <double> (l / (k + 1))); break;
            case PA2: penalty = (x.getNorm () + 1 / (2 * _opt.C)) / ((k + 1) * x.getNorm () + (k / (2 * _opt.C))) * l; break;
          }
          t = fl_t (0);
          for (loss_t::iterator it = _ls.begin (); it != _ls.end (); ++it) {
            const fl_t t_ =
              static_cast <fl_t> (std::max (0.0, it->first - penalty) / x.getNorm ());
            t[y] += t_, t[it->second] -= t_;
          }
        }
        if  (_opt.kernel == LINEAR) _addTo (x, t); else _pushTo (x, t);
      }
#else
      _m *= static_cast <fl_t> (y);
      if (_m <= _thresh) {
        fl_t t = static_cast <fl_t> (y > 0 ? 1 : -1); // tau
        switch (_opt.algo) {
          case P:   break;
          case PA:  t *= (1 - _m) / x.getNorm (); break;
          case PA1: t *= std::min (_opt.C, (1 - _m) / x.getNorm ()); break;
          case PA2: t *= (1 - _m) / (x.getNorm () + 1 / (2 * _opt.C)); break;
        }
        if (_opt.kernel == LINEAR) _addTo (x, t); else _pushTo (x, t);
      }
#endif
    }
    template <typename Pool>
    void _adjust_C (Pool& pool, const uint iter) {
      fl_t   sum = 0;
      size_t nex = 0;
      for (const typename Pool::elem_t* x = pool.init (); x; x = pool.get ()) {
        if (_opt.d > 0)
          _precompute_kernel (x->getSize ()), sum += _polyk[x->getSize ()];
        else
          sum += x->getNorm ();
        ++nex;
      }
      _opt.C = static_cast <fl_t> (1) / ((sum / nex) * iter);
      std::fprintf (stderr, "C is adjusted to %.16g\n", _opt.C);
    }
    void _average (const fl_t n, bool reuse = true) {
      if (_opt.kernel == LINEAR)
        for (uint fi = 0; fi <= _nf; ++fi) _w[fi] -= _wa[fi] / n;
      else
        for (uint i = 0; i < _nsv; ++i)
          if (reuse) _pushTo (*_sv[i], - _alpha[i] / n);
          else       _sv[i]->weight () -= _alpha[i] / n; // do not project
    }
#ifndef USE_MULTICLASS
    // C++ implemenation of Lin+ (2003):
    //   A Note on Platt's Probabilistic Outputs for Support Vector Machines
    template <typename Pool>
    void _sigmoid_fitting (Pool& pool) {
      std::fprintf (stderr, "\nPerform sigmoid fitting using examples..\n");
      std::vector <double> f;
      std::vector <bool>   label;
      size_t prior1 (0), prior0 (0);
      // training examples
      const bool prune   = _opt.pruning;
      const bool slicing = _opt.slicing;
      _opt.pruning = false;
      _opt.slicing = false;
      for (const typename Pool::elem_t* x = pool.init (); x; x = pool.get ()) {
        w_t m = 0;
        const bool target = x->getLabel () > 0;
        if (_opt.kernel == LINEAR) _getMargin     (m, x->begin (), x->end ());
        else                       _getMarginPoly (m, x->begin (), x->end ());
        f.push_back (m);
        label.push_back (target);
        if (target) ++prior1; else ++prior0;
      }
      _opt.pruning = prune;
      _opt.slicing = slicing;
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
      static const uint maxiter = 100;  // Maximum number of iterations
      static const double minstep (1e-10), sigma (1e-12), epsilon (1e-5);
      uint j = 0;
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
        if (stepsize < minstep)
          { std::fprintf (stderr, "  Line search fails."); break; }
        std::fprintf (stderr, "  iter=%u  A=%f; B=%f\n", ++j, A, B);
      }
      if (j >= maxiter)
        std::fprintf (stderr, "  Reaching maximum iterations.");
      std::fprintf (stderr, "done.\n");
      _sigmoid_A = A; _sigmoid_B = B;
    }
    double _sigmoid (const fl_t m) const
    { return 1.0 / (1.0 + std::exp (_sigmoid_A * m + _sigmoid_B)); }
#endif
  };
}
#endif /* OPAL_PA_H */
// averaging for retraining; keep averaged vector + nex (reluctant)
// having weights for multi-class in a sparse vector to reduce weights
// - speed up testing
// - pruning pmtrie as patricia
// - parallel mini-batch
