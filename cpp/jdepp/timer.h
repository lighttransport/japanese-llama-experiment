// pecco -- please enjoy classification with conjunctive features
//  $Id: timer.h 1875 2015-01-29 11:47:47Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>

#ifdef USE_TIMER
#define TIMER(e) e
#else
#define TIMER(e) ((void)0)
#endif

namespace ny {

  class Timer {
  public:
    Timer (const char* p = "timer", const char* u = "trial") : _start (0), _elapsed (0), _trial (0), _label (p), _unit (u) {}
    void startTimer () { _start = rdtsc (); }
    void stopTimer  () { _elapsed += (rdtsc () - _start); ++_trial; }
    static uint64_t rdtsc () {
#if defined(__i386__)
      uint64_t tsc;
      __asm__ __volatile__(".byte 0x0f, 0x31" : "=A" (tsc));
      return (tsc);
#elif (defined(__amd64__) || defined(__x86_64__))
      uint64_t lo, hi;
      __asm__ __volatile__( "rdtsc" : "=a" (lo), "=d" (hi) );
      return (lo | (hi << 32));
#elif (defined(__powerpc__) || defined(__ppc__))
      uint64_t tbl, tbu0, tbu1;
      do {
        __asm__ __volatile__( "mftbu %0" : "=r" (tbu0) );
        __asm__ __volatile__( "mftb  %0" : "=r" (tbl ) );
        __asm__ __volatile__( "mftbu %0" : "=r" (tbu1) );
      } while (tbu0 != tbu1);
      return (tbl);
#elif defined(__sparc__)
      uint64_t tick;
      __asm__ __volatile__ (".byte 0x83, 0x41, 0x00, 0x00");
      __asm__ __volatile__ ("mov   %%g1, %0" : "=r" (tick));
      return (tick);
#elif defined(__alpha__)
      uint64_t cc;
      __asm__ __volatile__ ("rpcc %0" : "=r" (cc));
      return (cc & 0xFFFFFFFF);
#elif defined(__ia64__)
      uint64_t itc;
      __asm__ __volatile__ ("mov %0 = ar.itc" : "=r" (itc));
      return (itc);
#endif
    }
    static long double clock ();
    long double        getElapsed () const { return _elapsed / (clock () * 1000); }
    unsigned long      getTrial   () const { return _trial; }
    void printElapsed () const;
  private:
    uint64_t      _start;
    uint64_t      _elapsed;
    unsigned long _trial;
    std::string   _label;
    std::string   _unit;
  };

  class TimerPool {
  private:
    typedef std::vector <Timer*> pool_t;
    pool_t      _pool;
    std::string _title;
    TimerPool (const TimerPool&);
    void operator= (const TimerPool&);
  public:
    TimerPool (const char * title = "") : _pool (), _title (title) {}
    Timer * push (const char* p = "timer", const char * u = "trial") {
      Timer * timer_t = new Timer (p, u);
      _pool.push_back (timer_t);
      return timer_t;
    }
    void print () {
      if (! _pool.empty ()) {
        if (! _title.empty ())
          std::fprintf (stderr, "%s:\n", _title.c_str ());
        for (pool_t::iterator it = _pool.begin (); it != _pool.end (); ++it)
          (*it)->printElapsed ();
        std::fprintf (stderr, "\n");
      }
    }
    ~TimerPool () {
      for (pool_t::iterator it = _pool.begin (); it != _pool.end (); ++it)
        delete *it;
    }
  };
}

#endif /* TIMER_H */
