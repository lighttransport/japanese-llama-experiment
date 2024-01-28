/// J.DepP -- Japanese Dependency Parsers
//  $Id: pdep.h 1943 2022-03-17 17:48:45Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef PDEP_H
#define PDEP_H

#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <stack>
#include <list>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef JDEPP_DEFAULT_MODEL
#define JDEPP_DEFAULT_MODEL ""
#endif
#ifndef MECAB_DICT
#define MECAB_DICT ""
#endif

#include "typedef.h"
// trie type
#if defined (USE_DARTS) || defined (USE_DARTS_CLONE)
#include <darts.h>
#endif
#ifdef USE_CEDAR
#include "cedar.h"
#endif
#ifdef USE_TIMER
#include "timer.h"
#endif
#if defined (USE_OPAL) || defined (USE_SVM)
#include "kernel.h"
#endif
#ifdef USE_SVM
#include <tinysvm.h>
#endif
#ifdef USE_OPAL
#include "pa.h"
#endif
#ifdef USE_MAXENT
#include "maxent.h"
#include "linear.h"
#endif
#include "pecco.h"
#ifdef USE_AS_STANDALONE
#include <mecab.h>
#endif

#define JDEPP_COPYRIGHT  "J.DepP - Japanese Dependency Parser\n\
Copyright (c) 2008-2015 Naoki Yoshinaga\n\
\n\
Usage: %s [options] -- [learner options] -- [chunker classifier options] -- [parser classifier options] < test\n\
\n\
test    test file\n\
\n"

#define JDEPP_OPT0 "Optional parameters in training / testing:\n\
  -t, --type=TYPE             select running mode of J.DepP\n\
                                0 - learn\n\
                              * 1 - parse\n\
                                2 - both\n\
                                3 - cache\n\
  -e, --encoding=TYPE         select encoding of input\n\
                              * 0 - UTF-8\n\
                                1 - EUC-JP\n\
  -i, --ignore=STR            ignore input line starting with STR\n\
  -c, --corpus=FILE           training corpus in JDEPP format ('train.JDP')\n\
  -m, --model-dir=DIR         model directory ('" JDEPP_DEFAULT_MODEL "')\n\
  -p, --parser=TYPE           select parsing algorithm\n\
                              * 0 - shift reduce\n\
                                1 - cascaded chunking\n\
                                2 - backward\n\
                                3 - tournament\n"

#ifdef USE_AS_STANDALONE
#define JDEPP_OPT1 "  -I, --input-format=TYPE     select type of input format\n\
                              * 0 - RAW sentences\n\
                                1 - + POS / CHUNK annotation\n\
                                2 - + DEPENDENCY annotation\n\
\n"
#else
#define JDEPP_OPT1 "  -I, --input-format=TYPE     select type of input format\n\
                              * 0 - POS-tagged sentences\n\
                                1 - + CHUNK annotation\n\
                                2 - + DEPENDENCY annotation\n\
\n"
#endif

#if defined (USE_OPAL)
#define OPAL_STR "                              * 0 - OPAL\n"
#else
#define OPAL_STR "                                0 - OPAL   (disabled)\n"
#endif
#if defined (USE_SVM)
#if ! defined (USE_OPAL)
#define SVM_STR "                              * 1 - SVM\n"
#else
#define SVM_STR "                                1 - SVM\n"
#endif
#else
#define SVM_STR "                                1 - SVM    (disabled)\n"
#endif
#if defined (USE_MAXENT)
#if ! defined (USE_OPAL) && ! defined (USE_SVM)
#define MAXENT_STR "                              * 2 - MaxEnt\n"
#else
#define MAXENT_STR "                                2 - MaxEnt\n"
#endif
#else
#define MAXENT_STR "                                2 - MaxEnt (disabled)\n"
#endif
#define JDEPP_OPT_TRAIN "Optional parameters in training:\n\
  -l, --learner=TYPE          select type of learning library\n" OPAL_STR SVM_STR MAXENT_STR "\
  -n, --max-sent=INT          max. # processing sentences (0: all)\n\
\n"

#ifdef USE_AS_STANDALONE
#define JDEPP_OPT_TEST "Optional parameters in testing:\n\
  -d, --mecab-dic=DIR         use MeCab dictionary ('" MECAB_DICT "')\n\
\n"
#else
#define JDEPP_OPT_TEST
#endif

#define JDEPP_OPT_MISC "Misc.:\n\
  -v, --verbose=INT           verbosity level (0)\n\
  -h, --help                  show this help and exit\n"

#define MAXENT_OPT "\nOptions for MaxEnt learners are as follows:\n\
  -d, --degree=INT            maximum degree of feature combinations (<=3)\n\
  -l, --algorithm=INT         select type of optimization algorithm\n\
                              * 0 - SGD-L1\n\
                                1 - OWLQN-L1\n\
                                2 - LBFGS-L2\n\
  -c, --reg-cost=INT          cost of regularization\n\
\n"

#ifdef USE_AS_STANDALONE
static const  char* jdepp_short_options = "t:e:i:c:m:p:I:b:l:n:d:x:v:h";
#else
static const  char* jdepp_short_options = "t:e:i:c:m:p:I:b:l:n:x:v:h";
#endif
static struct option jdepp_long_options[] = {
  {"type",         required_argument, NULL, 't'},
  {"encoding",     required_argument, NULL, 'e'},
  {"ignore",       required_argument, NULL, 'i'},
  {"corpus",       required_argument, NULL, 'c'},
  {"model-dir",    required_argument, NULL, 'm'},
  {"parser",       required_argument, NULL, 'p'},
  {"input-format", required_argument, NULL, 'I'},
  {"cluster-bits", required_argument, NULL, 'b'},
  {"learner",      required_argument, NULL, 'l'},
  {"max-sent",     required_argument, NULL, 'n'},
#ifdef USE_AS_STANDALONE
  {"mecab-dic",    required_argument, NULL, 'd'},
#endif
  {"xcode",        required_argument, NULL, 'x'},
  {"verbose",      required_argument, NULL, 'v'},
  {"help",         no_argument,       NULL, 'h'},
  {NULL, 0, NULL, 0}
};

#ifdef USE_MAXENT
static const  char*  maxent_short_options = "d:l:c:h";
static struct option maxent_long_options[] = {
  {"degree",       required_argument, NULL, 'd'},
  {"algorithm",    required_argument, NULL, 'l'},
  {"reg-cost",     required_argument, NULL, 'c'},
  {"help",         no_argument,       NULL, 'h'},
  {NULL, 0, NULL, 0}
};
#endif

extern char* optarg;
extern int   optind;

// raw strings for utf8
#define UTF8_COMMA         "\xE8\xAA\xAD\xE7\x82\xB9" // 読点
#define UTF8_PERIOD        "\xE5\x8F\xA5\xE7\x82\xB9" // 句点
#define UTF8_POST_PARTICLE "\xE5\x8A\xA9\xE8\xA9\x9E" // 助詞
#if defined (USE_JUMAN_POS)
#define UTF8_BRACKET_IN    "\xE6\x8B\xAC\xE5\xBC\xA7\xE5\xA7\x8B" // 括弧始
#define UTF8_BRACKET_OUT   "\xE6\x8B\xAC\xE5\xBC\xA7\xE7\xB5\x82" // 括弧終
#define UTF8_SPECIAL       "\xE7\x89\xB9\xE6\xAE\x8A" // 特殊
#define UTF8_SUFFIX        "\xE6\x8E\xA5\xE5\xB0\xBE\xE8\xBE\x9E" // 接尾辞
#elif defined (USE_IPA_POS)
#define UTF8_BRACKET_IN    "\xE6\x8B\xAC\xE5\xBC\xA7\xE9\x96\x8B" // 括弧開
#define UTF8_BRACKET_OUT   "\xE6\x8B\xAC\xE5\xBC\xA7\xE9\x96\x89" // 括弧閉
#define UTF8_SPECIAL       "\xE8\xA8\x98\xE5\x8F\xB7" // 記号
#elif defined (USE_UNI_POS)
#define UTF8_BRACKET_IN    "\xE6\x8B\xAC\xE5\xBC\xA7\xE9\x96\x8B" // 括弧開
#define UTF8_BRACKET_OUT   "\xE6\x8B\xAC\xE5\xBC\xA7\xE9\x96\x89" // 括弧閉
#define UTF8_SPECIAL       "\xE8\xA3\x9C\xE5\x8A\xA9\xE8\xA8\x98\xE5\x8F\xB7" // 補助記号
#endif

// same for euc
#define EUC_COMMA         "\xC6\xC9\xC5\xC0"
#define EUC_PERIOD        "\xB6\xE7\xC5\xC0"
#define EUC_POST_PARTICLE "\xBD\xF5\xBB\xEC"

#if defined (USE_JUMAN_POS)
#define EUC_BRACKET_IN    "\xB3\xE7\xB8\xCC\xBB\xCF"
#define EUC_BRACKET_OUT   "\xB3\xE7\xB8\xCC\xBD\xAA"
#define EUC_SUFFIX        "\xC0\xDC\xC8\xF8\xBC\xAD"
#define EUC_SPECIAL       "\xC6\xC3\xBC\xEC"
#elif defined (USE_IPA_POS)
#define EUC_BRACKET_IN    "\xB3\xE7\xB8\xCC\xB3\xAB"
#define EUC_BRACKET_OUT   "\xB3\xE7\xB8\xCC\xCA\xC4"
#define EUC_SPECIAL       "\xB5\xAD\xB9\xE6"
#elif defined (USE_UNI_POS)
#define EUC_BRACKET_IN    "\xB3\xE7\xB8\xCC\xB3\xAB"
#define EUC_BRACKET_OUT   "\xB3\xE7\xB8\xCC\xCA\xC4"
#define EUC_SPECIAL       "\xCA\xE4\xBD\xF5\xB5\xAD\xB9\xE6"
#endif

namespace pdep {
  // type alias
  typedef std::vector <uint64_t> flag_t;
  static const ny::uint FLAG_LEN = static_cast <ny::uint> (sizeof (flag_t::value_type)) * 8;
  // field (POS tagger outputs)
#if   defined (USE_JUMAN_POS)
#if   defined (USE_MECAB)
  enum field_t {SURF, POS1, POS2, TYPE, INFL, FIN, YOMI, NUM_FIELD}; // OTHER, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15, C16, C17, C18, NUM_FIELD}; // 
#elif defined (USE_JUMAN)
  enum field_t {SURF, YOMI, FIN, POS1, POS_ID1, POS2, POS_ID2, TYPE, TYPE_ID, INFL, INFL_ID, NUM_FIELD};
#elif defined (USE_CONDARA)
  enum field_t {SURF, POS1, POS2, TYPE, INFL, NUM_FIELD};
#endif
#elif defined (USE_IPA_POS)
  enum field_t {SURF, POS1, POS2, POS3, POS4, TYPE, INFL, FIN, YOMI, NUM_FIELD}; // leave other irrelevant fields untoched
#elif defined (USE_UNI_POS)
  enum field_t {SURF, PRON, FIN, LEM, POS1, POS2, TYPE, INFL, NUM_FIELD}; // leave other irrelevant fields untouched
#endif
  // static const variables
  class sentence_t;
  template <typename T>
  static void widen (T*& array, const int& avail, const int& filled = 0) {
    T* tmp = static_cast <T*> (::operator new (static_cast <size_t> (avail) * sizeof (T)));
    for (int i = 0;      i < filled; ++i) new (&tmp[i]) T (array[i]);
    for (int i = filled; i < avail;  ++i) new (&tmp[i]) T ();
    std::swap (tmp, array);
    for (int i = 0;      i < filled; ++i) tmp[i].~T ();
    if (tmp) ::operator delete (tmp); // don't destruct delegated resource
  }
  template <typename T>
  T strton (const char* s, char** error)
  { return static_cast <T> (std::strtol (s, error, 10)); }
  //
  enum process_t { LEARN, PARSE, BOTH, CACHE };
  enum parser_t  { LINEAR, CHUNKING, BACKWARD, TOURNAMENT };
  enum learner_t { OPAL, SVM, MAXENT };
  enum input_t   { RAW, CHUNK, DEPND };
  //
  // The command-line arguments will override the following default parameters
  class option { // option handler
  public:
    const char* com, *train, *model_dir;
    //
    mutable learner_t  learner;
    parser_t     parser;
    input_t      input;
    process_t    mode;
    ny::uint     cbits;
    ny::uint     clen;
    ny::uint     max_sent;
    ny::uint     xcode;
#ifdef USE_AS_STANDALONE
    const char*  mecab_dic;
#endif
    char**       learner_argv;
    char**       chunk_argv;
    char**       depnd_argv;
    int          learner_argc;
    int          chunk_argc;
    int          depnd_argc;
    int          verbose;
    char*        ignore;
    int          ignore_len;
    bool         utf8;
    //
    option (int argc, char** argv) :
      com (argv[0]), train ("train.JDP"),
#ifdef USE_STACKING
      model_dir (JDEPP_DEFAULT_MODEL "_stack"),
#else
      model_dir (JDEPP_DEFAULT_MODEL),
#endif
#if   defined (USE_OPAL)
      learner (OPAL),
#elif defined (USE_SVM)
      learner (SVM),
#elif defined (USE_MAXENT)
      learner (MAXENT),
#endif
      parser (LINEAR), input (RAW), mode (PARSE), cbits (0), clen (0), max_sent (0), xcode (0),
#ifdef USE_AS_STANDALONE
      mecab_dic (MECAB_DICT),
#endif
      learner_argv (0), chunk_argv (0), depnd_argv (0), learner_argc (0), chunk_argc (0), depnd_argc (0), verbose (0), ignore (0), ignore_len (0), utf8 (true) {
      // getOpt
      if (argc == 0) return;
      optind = 1;
      while (1) {
        int opt = getopt_long (argc, argv,
                               jdepp_short_options, jdepp_long_options, NULL);
        if (opt == -1) break;
        char* err = NULL;
        switch (opt) {
          case 't': mode      = strton <process_t> (optarg, &err);   break;
          case 'e': utf8      = std::strtol (optarg, &err, 10) == 0; break;
          case 'i': ignore    = optarg; ignore_len = std::strlen (ignore); break;
          case 'c': train     = optarg; break;
          case 'm': model_dir = optarg; break;
          case 'p': parser    = strton <parser_t>  (optarg, &err); break;
          case 'I': input     = strton <input_t>   (optarg, &err); break;
          case 'b':
            do {
              const ny::uint depth = strton <ny::uint> (optarg, &optarg);
              cbits |= 1 << (depth - 1);
              clen = std::max (clen, depth);
            } while (*optarg++ != '\0');
            break;
            // training parameters
          case 'l': learner   = strton <learner_t> (optarg, &err); break;
          case 'n': max_sent  = strton <ny::uint> (optarg, &err);  break;
            // misc
#ifdef USE_AS_STANDALONE
          case 'd': mecab_dic = optarg; break;
#endif
          case 'x': xcode     = strton <ny::uint> (optarg, &err); break;
          case 'v': verbose   = strton <int> (optarg, &err);      break;
          case 'h': printCredit (); printHelp (); std::exit (0);
          default:  printCredit (); std::exit (0);
        }
        if (err && *err)
          errx (1, HERE "unrecognized option value: %s", optarg);
      }
      // print xcode
      if (xcode) { // xcode
        std::fprintf (stderr, "xcode: %d; ", xcode);
        for (ny::uint i = 0; i < 8; ++i)
          std::fprintf (stderr, " %c", ((xcode >> i) & 0x1) ? '+' : '-');
        std::fprintf (stderr, "\n");
      }
      // errors & warnings
      if (learner != OPAL && learner != SVM && learner != MAXENT)
        errx (1, HERE "unknown learner [-l].");
      if (mode != LEARN && mode != PARSE && mode != BOTH && mode != CACHE)
        errx (1, HERE "unknown running mode [-t].");
      if (parser != LINEAR && parser != CHUNKING && parser != BACKWARD &&
          parser != TOURNAMENT)
        errx (1, HERE "unknown parsing algorithm [-p].");
      if (input != RAW && input != CHUNK && input != DEPND)
        errx (1, HERE "unknown input format [-I].");
      struct stat st;
      if (stat (model_dir, &st) != 0)
        errx (1, HERE "no such directory: %s [-m]", model_dir);
#ifdef USE_AS_STANDALONE
      if (input == RAW && mecab_dic && stat (mecab_dic, &st) != 0)
        errx (1, HERE "MeCab dict [-d]: no such file or directory: %s", mecab_dic);
#endif
      if (input == CHUNK && parser != LINEAR)
        warnx ("NOTE: parsing algorithm [-p] is ignored in training a chunker.");
      // learner options
      if (std::strcmp (argv[optind - 1], "--") == 0) --optind;
      _set_library_options (optind, argc, argv, learner_argc, learner_argv);
      // classifier options for bunsetsu chunker
      _set_library_options (optind, argc, argv, chunk_argc, chunk_argv);
      // classifier options for dependency parser
      _set_library_options (optind, argc, argv, depnd_argc, depnd_argv);
    }
    void printCredit () { std::fprintf (stderr, JDEPP_COPYRIGHT, com); }
    void printHelp   () { std::fprintf (stderr, JDEPP_OPT0 JDEPP_OPT1 JDEPP_OPT_TRAIN JDEPP_OPT_TEST JDEPP_OPT_MISC); }
  private:
    void _set_library_options (int& i, const int argc, char** argv,
                               int& largc, char**& largv) {
      if (i < argc) {
        if (std::strcmp (argv[optind], "--") == 0) { // library options
          largv = &argv[optind];
          largc = 1;
          while (optind + largc < argc && std::strcmp (largv[largc], "--") != 0)
            ++largc;
          i += largc;
        } else {
          printCredit ();
          errx (1, HERE "Type `%s --help' for option details.", com);
        }
      }
    }
  };
#ifdef USE_MAXENT
  enum maxent_algo_t { SGD, OWLQN, LBFGS }; // MaxEnt optimizer
  // wrapper option class for maxent
  struct maxent_option { // option handler
    maxent_algo_t algo;
    ny::uint      degree;
    double        reg_cost;
    //
    maxent_option () : algo (SGD), degree (2), reg_cost (1.0) {}
    maxent_option (int argc, char** argv) : algo (SGD), degree (2), reg_cost (1.0)
    { set (argc, argv); }
    void set (int argc, char** argv) { // getOpt
      if (argc == 0) return;
      optind = 1;
      while (1) {
        int opt = getopt_long (argc, argv,
                               maxent_short_options, maxent_long_options, NULL);
        if (opt == -1) break;
        char* err = NULL;
        switch (opt) {
          case 'd': degree   = strton <ny::uint> (optarg, &err);      break;
          case 'l': algo     = strton <maxent_algo_t> (optarg, &err); break;
          case 'c': reg_cost = std::strtod (optarg, &err); break;
          case 'h': printHelp (); std::exit (0); break;
          default:  std::exit (0);
        }
        if (err && *err)
          errx (1, HERE "unrecognized option value: %s", optarg);
      }
      // errors
      if (algo != SGD && algo != OWLQN && algo != LBFGS)
        errx (1, HERE "unknown optimization algorithm [-l]");
      if (degree == 0 || degree >= 4)
        errx (1, HERE "degree of conjunctive features [-d] must be <= 3");

    }
    void printHelp () { std::fprintf (stderr, MAXENT_OPT); }
  };
#endif
  // dictionary utility variables
  typedef std::map <const char*, ny::uint, ny::pless <char> > sbag_t;
  class dict_base_t : private ny::Uncopyable {
  private:
    char*     _data_ptr;
    ny::trie  _data;
    ny::uint  _num_particle_features;
    ny::uint  _num_lexical_features;
  protected:
    dict_base_t (const char* fn) : _data_ptr (0), _data (), _num_particle_features (0), _num_lexical_features (0) {
      FILE* fp = std::fopen (fn, "rb");
      if (std::fread (&_num_particle_features, sizeof (ny::uint), 1, fp) != 1 ||
          std::fread (&_num_lexical_features,  sizeof (ny::uint), 1, fp) != 1)
        errx (1, HERE "broken dic: delete %s", fn);
      ++_num_lexical_features; // reserve room for unseen feature
      long offset = std::ftell (fp);
      std::fseek (fp, 0, SEEK_END);
      const size_t size = static_cast <size_t> (std::ftell (fp) - offset);
      _data_ptr = new char[size];
      std::fseek (fp, offset, SEEK_SET);
      std::fread (_data_ptr, sizeof (char), size, fp);
      _data.set_array (_data_ptr, size / _data.unit_size ());
      std::fclose (fp);
    }
    ~dict_base_t () { delete [] _data_ptr; }
  public:
    int lookup (const char* key) const { return lookup (key, std::strlen (key)); }
    int lookup (const char* key, const size_t len) const {
      int n = _data.exactMatchSearch <ny::trie::result_type> (key, len);
      return n >= 0 ? n : static_cast <int> (_num_lexical_features) - 1;
    }
    ny::uint particle_feature_bit_len () const
    { return (_num_particle_features - 1) / FLAG_LEN + 1; }
    bool     is_particle_feature (const ny::uint id) const
    { return id < _num_particle_features; }
    ny::uint num_lexical_features () const { return _num_lexical_features; }
  };
  class dict_t : public dict_base_t {
  public: // surface id aliases
    const int  comma;
    const int  period;
    const int  particle;
    const int  bracket_in;
    const int  bracket_out;
    const int  special;
#ifdef USE_JUMAN_POS
    const int  suffix;
#endif
    dict_t (const char* fn, bool utf8 = true) :
      dict_base_t (fn), comma (lookup (utf8 ? UTF8_COMMA : EUC_COMMA)), period (lookup (utf8 ? UTF8_PERIOD : EUC_PERIOD)), particle (lookup (utf8 ? UTF8_POST_PARTICLE : EUC_POST_PARTICLE)), bracket_in (lookup (utf8 ? UTF8_BRACKET_IN : EUC_BRACKET_IN)), bracket_out (lookup (utf8 ? UTF8_BRACKET_OUT : EUC_BRACKET_OUT)), special (lookup (utf8 ? UTF8_SPECIAL : EUC_SPECIAL))
#ifdef USE_JUMAN_POS
      , suffix (lookup (utf8 ? UTF8_SUFFIX : EUC_SUFFIX))
#endif
    {}
  };
  class token_t { // morpheme
  private:
    int          _field[NUM_FIELD];
#ifdef USE_KYOTO_PARTIAL
    ny::uint     _length;
    const char*  _surface;
#endif
  public:
    ny::uint     length;
    const char*  surface;
    const char*  feature;
    //
    double       chunk_start_prob;
    bool         chunk_start;
    bool         chunk_start_gold;
    token_t () : _field (),
#ifdef USE_KYOTO_PARTIAL
                 _length (0), _surface (0),
#endif
                 length (0), surface (0), feature (0), chunk_start_prob (0.0), chunk_start (false), chunk_start_gold (false)
    { std::fill (&_field[0], &_field[0] + NUM_FIELD, -1); }
    token_t (const token_t& m) : _field (),
#ifdef USE_KYOTO_PARTIAL
                                 _length (m._length), _surface (m._surface),
#endif
                                 length (m.length), surface (m.surface), feature (m.feature), chunk_start_prob (m.chunk_start_prob), chunk_start (m.chunk_start), chunk_start_gold (m.chunk_start_gold)
    { std::copy (&m._field[0], &m._field[0] + NUM_FIELD, &_field[0]); }
    ~token_t () {}
    void set (char* p, const size_t len, const dict_t* const dict, bool flag) {
      surface = p;
      char* const p_end = p + len;
      *p_end = SURFACE_END; while (*p != SURFACE_END) ++p;
      length  = static_cast <ny::uint> (p - surface);
      feature = ++p;
      chunk_start_gold = flag;
      *p_end  = '\0';
      set (dict);
    }
    void set (const ny::uint length_,  const char*   surface_,
              const char*    feature_, const dict_t* dict) {
      length = length_; surface = surface_; feature = feature_;
      set (dict);
    }
    void set (const dict_t* const dict) {
      _field[SURF] = dict->lookup (surface, length); // read surface
      // read feature
      ny::uint i = 1;
      for (const char* p (feature), *f (p); i < NUM_FIELD; f = ++p, ++i) {
#ifdef USE_UNI_POS
        if (i == POS1)
          while (*p != '\0' && *p != FEATURE_SEP && *p != FEATURE_SEP_UNI) ++p;
        else
#endif
          while (*p != '\0' && *p != FEATURE_SEP) ++p;
#ifdef USE_KYOTO_PARTIAL
        if (i == YOMI) _surface = f, _length = static_cast <ny::uint> (p - f);
        if (i == POS1 || i == POS2 || i == INFL || i == YOMI) // || i > OTHER)
#else
        if (i == POS1 || i == POS2 || i == INFL) // || i > OTHER)
#endif
          _field[i] = dict->lookup (f, static_cast <size_t> (p - f));
#ifdef USE_UNI_POS
        if (i == POS1 && *p == FEATURE_SEP) --p;
#endif
      }
#ifdef USE_JUMAN_POS
      if (i < NUM_FIELD) {
        std::fwrite (surface, sizeof (char), length, stderr);
        errx (1, HERE "# fields, %d, is less than %d.", i, NUM_FIELD);
      }
#endif
    }
    std::string str () const { return std::string (surface, length); }
#ifdef USE_KYOTO_PARTIAL
    const char* surface_ () const { return _surface; }
    ny::uint    length_  () const { return _length;  }
    int surf () const { return _field[YOMI]; }
#else
    const char* surface_ () const { return surface; }
    ny::uint    length_  () const { return length;  }
    int surf () const { return _field[SURF]; }
#endif
    int pos1 () const { return _field[POS1]; }
    int pos2 () const { return _field[POS2]; }
    int infl () const { return _field[INFL]; }
    // int cluster (ny::uint i) const { return _field[NUM_FIELD + i]; }
  };
  class sentence_t;
  class chunk_t {
  private:
    const sentence_t* _s;
    int        _token_num;     // number of tokens
    int        _mzero;         // start position
    int        _mhead;         // head_token offset
    int        _mtail;         // tail
    void _set_particle_feature_bits (const ny::uint i) {
      particle_feature_bits[i / FLAG_LEN]
        |= (static_cast <flag_t::value_type> (1) << (i % FLAG_LEN));
    }
  public:
    int        id;             // chunk id
    int        head_id;        // dest id
    int        head_id_gold;   // dest id (gold)
    int        head_id_cand;   // dest id (from stack parser)
    double     depnd_prob;
    char       depnd_type_gold;
    char       depnd_type_cand;
    bool       comma;          // has comma
    bool       period;         // has period
    uint16_t   bracket_in;     // # bracket_in
    uint16_t   bracket_out;    // # bracket_out
    flag_t     particle_feature_bits;
    // char dtype;
    chunk_t () :
      _s (0), _token_num (0), _mzero (-1), _mhead (-1), _mtail (-1), id (0), head_id (-1), head_id_gold (-1), head_id_cand (-1), depnd_prob (0.0), depnd_type_gold ('D'), depnd_type_cand ('D'), comma (0), period (0), bracket_in (0), bracket_out (0), particle_feature_bits () {}
    chunk_t (const chunk_t& b) : _s (b._s), _token_num (b._token_num), _mzero (b._mzero), _mhead (b._mhead), _mtail (b._mtail), id (b.id), head_id (b.head_id), head_id_gold (b.head_id_gold), head_id_cand (b.head_id_cand), depnd_prob (b.depnd_prob), depnd_type_gold (b.depnd_type_gold), depnd_type_cand (b.depnd_type_cand), comma (b.comma), period (b.period), bracket_in (b.bracket_in), bracket_out (b.bracket_out), particle_feature_bits (b.particle_feature_bits) {}
    ~chunk_t () {}
    void clear () {
      _token_num =  0;
      _mzero = _mhead = _mtail = -1;
      bracket_in = bracket_out = 0;
      comma = period = false;
      std::fill (particle_feature_bits.begin (), particle_feature_bits.end (), 0);
      head_id = head_id_gold = head_id_cand = -1;
      depnd_prob = 0.0; depnd_type_gold = depnd_type_cand = 'D';
    }
    void set (const sentence_t* s, const int i, const int mzero)
    { _s = s; id = i; _mzero = mzero; }
#ifdef USE_STACKING
    void set (const sentence_t* s, const int i, const int mzero, char* p, const size_t len = 0) { // ex. '* 1 7D'
      set (s, i, mzero);
      const char* p_end (p + len);
      if (id != strton <int> (p + 2, &p))
        errx (1, HERE "wrong chunk id annotation.");
      ++p;
      head_id   = strton <int> (p, &p);
      depnd_type_gold = *p;
      if (++p != p_end) {
        depnd_cand = strton <int> (++p, &p); // ignored in testing
        depnd_type_cand = *p;
      }
    }
#else
    void set (const sentence_t* s, const int i, const int mzero, char* p, const size_t = 0, bool flag = false) { // ex. '* 1 7D'
      set (s, i, mzero);
      if (id != strton <int> (p + 2, &p))
        errx (1, HERE "wrong chunk id annotation.");
      ++p;
      if (flag)
        head_id      = strton <int> (p, &p);
      else
        head_id_gold = strton <int> (p, &p);
      depnd_type_gold = *p;
    }
#endif
    bool setup (const dict_t* dict, const int next);
    std::string str () const {
      std::string ret;
      for (const token_t* m = mzero (); m <= mlast (); ++m)
        ret += m->str ();
      return ret;
    }
    const std::vector <const token_t*> tokens () const {
      std::vector <const token_t*> ret;
      for (const token_t* m = mzero (); m <= mlast (); ++m)
        ret.push_back (m);
      return ret;
    }
    const std::vector <const chunk_t*> dependents () const;
    const chunk_t* head () const;
    const token_t* mzero () const;
    const token_t* mhead () const;
    const token_t* mtail () const;
    const token_t* mlast () const;
  };
  //
  class sentence_t : private ny::Uncopyable {
  private:
    int            _cavail;
    int            _tavail;
    chunk_t*       _chunks;
    token_t*       _tokens;
    mutable char   _pos[IOBUF_SIZE]; // save input (postagged/parsed)
    mutable char   _res[IOBUF_SIZE]; // save output
    mutable char*  _ptr;             // current position in result buffer
  public:
    chunk_t*       chunk0;
    token_t*       token0;
    int            chunk_num;
    int            token_num;
    sentence_t () :
      _cavail (1), _tavail (1), _chunks (static_cast <chunk_t*> (::operator new (static_cast <size_t> (_cavail) * sizeof (chunk_t)))), _tokens (static_cast <token_t*> (::operator new (static_cast <size_t> (_tavail) * sizeof (token_t)))), _pos (), _res (), _ptr (&_res[0]), chunk0 (new chunk_t ()), token0 (new token_t ()), chunk_num (0), token_num (0) {
      for (int i = 0; i < _cavail; ++i) new (&_chunks[i]) chunk_t ();
      for (int i = 0; i < _tavail; ++i) new (&_tokens[i]) token_t ();
      chunk0->set (this, 0, 0); // bug fix
    }
    ~sentence_t () {
      for (int i = 0; i < _cavail; ++i) _chunks[i].~chunk_t ();
      ::operator delete (_chunks);
      for (int i = 0; i < _tavail; ++i) _tokens[i].~token_t ();
      ::operator delete (_tokens);
      delete chunk0;
      delete token0;
    }
    void clear (bool reset = false) {
      if (chunk_num)
        do { _chunks[--chunk_num].clear (); } while (chunk_num);
      token_num = 0;
      if (reset) _ptr = &_res[0];
    }
    chunk_t* chunk (const int i) const { return i >= 0 && i < chunk_num ? &_chunks[i] : chunk0; }
    token_t* token (const int i) const { return i >= 0 && i < token_num ? &_tokens[i] : token0; }
    void setHeader (char* cs, const size_t len) {
      if (_ptr == &_res[0]) {
        if (len > IOBUF_SIZE)
          errx (1, HERE "set a larger value to IOBUF_SIZE.");
        std::memcpy (_ptr, cs, len), _ptr += len;
      }
    }
    void setup (const dict_t *dict) { // complete information
      for (int i = 0; i < chunk_num; ++i)
        _chunks[i].setup (dict, i == chunk_num - 1 ? token_num : (_chunks[i + 1].mzero () - _tokens));
    }
    void add_chunk (char* cs, const size_t len, const int mzero, bool flag = false) {
      if (chunk_num == _cavail) _cavail <<= 1, widen (_chunks, _cavail, chunk_num);
      _chunks[chunk_num].set (this, chunk_num, mzero, cs, len, flag);
      ++chunk_num;
    }
    void add_chunk (const int mzero) {
      if (chunk_num == _cavail) _cavail <<= 1, widen (_chunks, _cavail, chunk_num);
      _chunks[chunk_num].set (this, chunk_num, mzero);
      ++chunk_num;
    }
    void add_token (char* cs, const size_t len, const dict_t* dict, bool flag = false) {
      if (token_num == _tavail)  _tavail <<= 1, widen (_tokens, _tavail, token_num);
      _tokens[token_num].set (cs, len, dict, flag);
      ++token_num;
    }
    void add_token (const ny::uint length, const char* surface,
                    const char* feature, const dict_t* dict) {
      if (token_num == _tavail) _tavail <<= 1, widen (_tokens, _tavail, token_num);
      _tokens[token_num].set (length, surface, feature, dict);
      ++token_num;
    }
    char* postagged () const { return &_pos[0]; }
    std::string str () const {
      std::string ret;
      for (int i = 0; i < chunk_num; ++i)
        ret += _chunks[i].str ();
      return ret;
    }
    const std::vector <const token_t*> tokens () const {
      std::vector <const token_t*> ret;
      for (const token_t* m = token (0); m <= token (token_num - 1); ++m)
        ret.push_back (m);
      return ret;
    }
    const std::vector <const chunk_t*> chunks () const {
      std::vector <const chunk_t*> ret;
      for (const chunk_t* b = chunk (0); b <= chunk (chunk_num - 1); ++b)
        ret.push_back (b);
      return ret;
    }
    int _vsnprintf (char* ptr, size_t size, const char* format, ...) const {
      va_list  args;
      va_start (args, format);
      const int n = std::vsnprintf (ptr, size, format, args);
      va_end (args);
      if (n == -1 || n >= static_cast <int> (size))
        errx (1, HERE "set a larger value to IOBUF_SIZE.");
      return n;
    }
    void set_topos (const char* postagged, const size_t len) {
      if (len > IOBUF_SIZE)
        errx (1, HERE "set a larger value to IOBUF_SIZE.");
      std::memcpy (&_pos[0], postagged, len);
    };
    const char* print_tostr (const input_t in, bool prob) const {
      if (token_num)
        for (int i = 0; i < chunk_num; ++i) {
          const chunk_t& b = _chunks[i];
          switch (in) {
            case RAW:
#ifdef USE_STACKING
              if (prob)
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD@%f #%d%c\n", i, b.head_id, b.depnd_prob, b.head_id_cand, b.head_id_cand_type);
              else
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD #%d%c\n", i, b.head_id, b.head_id_cand, b.depnd_type_cand);
#else
              if (prob)
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD@%f\n", i, b.head_id, b.depnd_prob);
              else
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD\n", i, b.head_id);
#endif
              break;
            case DEPND:
#ifdef USE_STACKING
              if (prob)
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %d%c %dD@%f #%d%c\n", i, b.head_id_gold, b.depnd_type_gold, b.head_id, b.depnd_prob, b.head_id_cand, b.depnd_type_cand);
              else
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %d%c %dD #%d%c\n", i, b.head_id, b.depnd_type, b.head_id, b.head_id_cand, b.depnd_type_cand);
#else
              if (prob)
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %d%c %dD@%f\n", i, b.head_id_gold, b.depnd_type_gold, b.head_id, b.depnd_prob);
              else
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %d%c %dD\n", i, b.head_id_gold, b.depnd_type_gold, b.head_id);
#endif
              break;
            case CHUNK:
#ifdef USE_STACKING
              _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD #%d%c\n", i, b.head_id, b.head_id_cand, b.depnd_type_cand);
#else
              _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "* %u %dD\n", i, b.head_id);
#endif
          }
          for (const token_t* m = b.mzero (); m <= b.mlast (); ++m) {
            if (_ptr - &_res[0] + m->length > IOBUF_SIZE)
              errx (1, HERE "set a larger value to IOBUF_SIZE.");
            std::memcpy (_ptr, m->surface, m->length); _ptr += m->length;
            switch (in) {
              case RAW:
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "%c%s", SURFACE_END, m->feature);
                if (prob)
                  _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "\t%c@%f", m->chunk_start ? 'B' : 'I', m->chunk_start_prob);
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "\n");
                break;
              case DEPND:
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "%c%s\n", SURFACE_END, m->feature);
                break;
              case CHUNK:
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "%c%s\t%c %c",
                                      SURFACE_END, m->feature, m->chunk_start_gold ? 'B' : 'I', m->chunk_start ? 'B' : 'I');
                if (prob)
                  _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "@%f", m->chunk_start_prob);
                _ptr += _vsnprintf (_ptr, IOBUF_SIZE - (_ptr - &_res[0]), "\n");
            }
          }
        }
      if (_ptr - &_res[0] + 5 > IOBUF_SIZE) // EOS\n\0
        errx (1, HERE "set a larger value to IOBUF_SIZE.");
      std::memcpy (_ptr, "EOS\n", 4); _ptr += 4;
      *_ptr = '\0';
      return &_res[0];
    }
    void print (const input_t in, bool prob) {
      const char* check = this->print_tostr (in, prob);
      while (check < _ptr)
        check += write (1, check, static_cast <size_t> (_ptr - check));
      _ptr = &_res[0];
    }
  };
  inline const std::vector <const chunk_t*> chunk_t::dependents () const {
    std::vector <const chunk_t*> ret;
    for (int i = 0; i < id; ) {
      int head_id = _s->chunk (i)->head_id;
      if (head_id == id)
        ret.push_back (_s->chunk (i));
      i = head_id < id ? head_id : i + 1;
    }
    return ret;
  }
  inline const chunk_t* chunk_t::head  () const { return head_id > 0 ? _s->chunk (head_id) : 0; }
  inline const token_t* chunk_t::mzero () const { return _s->token (_mzero); }
  inline const token_t* chunk_t::mhead () const { return _s->token (_mhead >= 0 ? _mzero + _mhead : -1); }
  inline const token_t* chunk_t::mtail () const { return _s->token (_mtail >= 0 ? _mzero + _mtail : -1); }
  inline const token_t* chunk_t::mlast () const { return _s->token (_mzero + _token_num - 1); }
  
  //
  template <typename T>
  struct stat_base {
    int snum;  // # sentence
    int scorr; // # sentence correctly recognized
    stat_base () : snum (0), scorr (0) {}
    void print () {
      if (! snum) return;
      static_cast <T*> (this)->print_impl ();
      std::fprintf (stderr, "acc. (complete)\t%.4f (%5u/%5u)\n\n",
                    scorr * 1.0 / snum, scorr, snum);
    }
  protected:
    ~stat_base () {}
  };
  struct chunk_stat : public stat_base <chunk_stat> {
    int pp;  // # chunks correctly recognized
    int np;  // # chunks incorrectly recognized
    int pn;  // # chunks missed
    chunk_stat () : pp (0), np (0), pn (0) {}
    void print_impl () {
      std::fprintf (stderr, "J.DepP performance statistics (chunk):\n");
      const double prec (pp * 1.0 / (pp + np)), rec (pp * 1.0 / (pp + pn));
      std::fprintf (stderr, "precision\t%.4f (%5u/%5u)\n", prec, pp, pp + np);
      std::fprintf (stderr, "recall   \t%.4f (%5u/%5u)\n", rec,  pp, pp + pn);
      std::fprintf (stderr, "f1       \t%.4f\n", 2 * prec * rec / (prec + rec));
    }
  };
  struct depnd_stat : public stat_base <depnd_stat> {
    int bnum;   // # dependencies
    int bcorr;  // # dependencies correctly recognized
    depnd_stat () : bnum (0), bcorr (0) {}
    void print_impl () {
      std::fprintf (stderr, "J.DepP performance statistics (depnd):\n");
      std::fprintf (stderr, "acc. (partial)\t%.4f (%5u/%5u)\n",
                    bcorr * 1.0 / bnum, bcorr, bnum);
    }
  };
  struct chunk_info {
    int  id;
    chunk_info (int id_) : id (id_) {} // non-deterministic
    // bool done; // deterministic
    // chunk_info (ny::uint id_, bool done_) : id (id_), done (done_) {}; // deterministic
  };
  class parser : private ny::Uncopyable {
  private:
    const option    _opt;
    pecco::option   _pecco_opt;
    pecco::pecco*   _pecco;
    pecco::pecco*   _pecco_chunk;
    pecco::pecco*   _pecco_depnd;
#ifdef USE_OPAL
    opal::option     _opal_opt;
    opal::Model*     _opal;
    opal::mem_pool <opal::ex_t> _ex_pool;
#endif
#ifdef USE_SVM
    TinySVM::Param  _tiny_param;
    TinySVM::Model* _tinysvm;
#endif
#ifdef USE_MAXENT
    maxent_option   _maxent_opt;
    ME_Model*       _libme;
#endif
    // variables to handle events
    sentence_t*       _s;
    dict_t*         _dict;
    flag_t          _particle_feature_bits;
    ny::fv_t        _fv; // feature vector of the example
    ny::uint        _fi; // offset of feature index
    chunk_stat      _chunk_stat;
    depnd_stat      _depnd_stat;
    FILE*           _writer;
#ifdef USE_AS_STANDALONE
    MeCab::Tagger*  _tagger;
#endif
    // parser specific variablts
    std::stack <int>       _stack;
    std::list <chunk_info> _cinfo;
    // timer
#ifdef USE_TIMER
    ny::TimerPool   _timer_pool;
    ny::Timer*      _io_t;
    ny::Timer*      _dict_t;
    ny::Timer*      _preproc_t;
    ny::Timer*      _chunk_t;
    ny::Timer*      _depnd_t;
    ny::Timer*      _classify_t;
#endif
    void _print_ex (const bool flag) const {
      std::fprintf (_writer, "%c1", flag ? '+' : '-');
      for (ny::fv_it it = _fv.begin (); it != _fv.end (); ++it)
        std::fprintf (_writer, " %d:1", *it);
      std::fprintf (_writer, "\n");
    }
    // feature vector generators
    __attribute__((always_inline)) void _add_boolean_feature (const bool flag);
    __attribute__((always_inline)) void _add_boolean_feature (const bool flag, const bool flag_);
    __attribute__((always_inline)) void _add_string_feature  (const int id);
    __attribute__((always_inline)) void _add_local_feature   (const chunk_t* const bi, const int h);
    __attribute__((always_inline)) void _add_global_feature  (const chunk_t* const bi, const chunk_t* const bj);
    // void _add_cluster_feature  (const token_t* const m);
    __attribute__((always_inline)) void _add_token_feature (const token_t* const m);
    __attribute__((always_inline)) void _add_particle_feature (const chunk_t* const bi, const chunk_t* const bj);
    void _add_boolean_feature  (const bool flag, const bool flag_, const bool flag__)__attribute__((always_inline));
    void _add_coord_feature    (const chunk_t* const bi, const chunk_t* const bj);
    void _event_gen_from_tuple (const int i);
    void _event_gen_from_tuple (const int i, const int j);
    void _event_gen_from_tuple (const int i, const int j, const int k);
    void _set_token_dict ();
    void _register_token (char* cs, const size_t& len, sbag_t& sbag,
                          std::set <ny::uint>& particle_feature_ids);
    void _learn ();
#if defined (USE_OPAL) || defined (USE_MAXENT)
    void _processSample (const bool flag);
#endif
    template <const process_t MODE> void _batch   ();
    template <const process_t MODE> void _chunk   ();
    template <const process_t MODE> void _parse   ();
    template <const process_t MODE> void _parseLinear     ();
    template <const process_t MODE> void _parseChunking   ();
    template <const process_t MODE> void _parseBackward   ();
    template <const process_t MODE> void _parseTournament ();
    template <const input_t INPUT>  void _collectStat ();
    void _switch_classifier  (const input_t in);
    void _setup_learner      ();
    void _cleanup_learner    ();
    void _setup_classifier   (const input_t in, int argc, char ** argv);
    void _cleanup_classifier (const input_t in);
#ifdef USE_MAXENT
    void _project (ME_Sample& ms) const;
#endif
  public:
    parser (const option& opt) :
      _opt (opt), _pecco_opt (), _pecco (0), _pecco_chunk (0), _pecco_depnd (0),
#ifdef USE_OPAL
      _opal_opt (), _opal (0), _ex_pool (),
#endif
#ifdef USE_SVM
      _tiny_param (), _tinysvm (0),
#endif
#ifdef USE_MAXENT
      _maxent_opt (), _libme (0),
#endif
      _s (0), _dict (0), _particle_feature_bits (0), _fv (), _fi (1), _chunk_stat (), _depnd_stat (), _writer (0)
#ifdef USE_AS_STANDALONE
      , _tagger (0)
#endif
      , _stack (), _cinfo ()
#ifdef USE_TIMER
      , _timer_pool ("J.DepP profiler"), _io_t (_timer_pool.push ("io")), _dict_t (_timer_pool.push ("dict")), _preproc_t (_timer_pool.push ("preproc", "sent.")), _chunk_t (_timer_pool.push ("chunk", "sent.")), _depnd_t (_timer_pool.push ("depnd", "sent.")), _classify_t (_timer_pool.push ("classify"))
#endif
    {
#ifdef USE_AS_STANDALONE
      std::string mecab_opt ("$0");
      if (_opt.mecab_dic) mecab_opt += std::string (" -d ") + _opt.mecab_dic;
      _tagger = MeCab::createTagger (mecab_opt.c_str ());
#endif
    }
    ~parser () {
      delete _dict;
#ifdef USE_AS_STANDALONE
      delete _tagger;
#endif
      if (_s) { _s->clear (); delete _s; }
    }
    // interface
    void init ();
    void run  ();
    void create_sentence () { _s = new sentence_t (); }
#ifdef USE_AS_STANDALONE
    const sentence_t* parse       (const char* sent, size_t len = 0);
    const char*       parse_tostr (const char* sent, size_t len = 0);
#endif
    const sentence_t* parse_from_postagged       (char* result, size_t len = 0);
    const char*       parse_from_postagged_tostr (char* result, size_t len = 0);
    const sentence_t* read_result (char* result, size_t len = 0);
  };
}
#endif /* PDEP_H */
