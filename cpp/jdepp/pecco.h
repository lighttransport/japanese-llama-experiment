#include "typedef.h"
#include "classify.h"
#ifdef USE_KERNEL
#include "kernel.h"
#endif
#ifdef USE_LINEAR
#include "linear.h"
#endif

namespace pecco {
  class pecco : private ny::Uncopyable { // classifier wrapper
  private:
    class pecco_impl {
    public:
      virtual ~pecco_impl () {}
      virtual bool   load           (const char * model) = 0;
      virtual void   batch          ()                   = 0;
      virtual void   printStat      ()                   = 0;
      virtual double threshold      ()                   = 0;
      virtual bool   binClassify    (ny::fv_t &fv)       = 0;
      virtual double getProbability (ny::fv_t &fv)       = 0;
    };
    template <typename T>
    class body : public pecco_impl  {
    public:
      template <typename U>
      explicit body (const U &opt) : obj (opt) {}
      bool     load     (const char * model) { return obj.load (model); }
      void     batch          ()             { obj.batch (); }
      void     printStat      ()             { obj.printStat (); }
      double   threshold      ()             { return obj.threshold (); }
      void     classify       (ny::fv_t &fv) { return obj.classify (fv); }
      bool     binClassify    (ny::fv_t &fv) { return obj.binClassify (fv); }
      double   getProbability (ny::fv_t &fv) { return obj.getProbability (fv); }
    private:
      T obj;
    };
  public:
    template <typename T, typename U>
    pecco  (T *, const U &opt) : _cont (new body <T> (opt)) {}
    ~pecco () { delete _cont; }
    bool   load     (const char * model) { return _cont->load (model); }
    void   batch          ()             { return _cont->batch (); }
    void   printStat      ()             { return _cont->printStat (); }
    double threshold      ()             { return _cont->threshold (); }
    bool   binClassify    (ny::fv_t &fv) { return _cont->binClassify (fv); }
    double getProbability (ny::fv_t &fv) { return _cont->getProbability (fv); }
  private:
    pecco_impl * _cont;
  };
}
