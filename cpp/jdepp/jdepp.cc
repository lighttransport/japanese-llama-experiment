// J.DepP -- Japanese Dependency Parsers
//  $Id: jdepp.cc 1876 2015-01-29 11:47:54Z ynaga $
// Copyright (c) 2008-2015 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>
#include "pdep.h"

int main (int argc, char* argv[]) {

  pdep::option opt (argc, argv);
  pdep::parser parser (opt);

  parser.run ();
  return 0;
}
