// Copyright 2016 Mozilla Foundation. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int
main(int argc, char** argv)
{
  static struct option long_options[] = {
    { "output-file", required_argument, NULL, 'o' },
    { "input-encoding", required_argument, NULL, 'e' },
    { "output-encoding", required_argument, NULL, 'g' },
    { "utf16-intermediate", no_argument, NULL, 'u' },
    { "help", no_argument, NULL, 'h' },
    { 0, 0, 0, 0 }
  };
  for (;;) {
    int option_index = 0;

    int c = getopt_long(argc, argv, "o:e:g:uh", long_options, &option_index);
    if (c == -1) {
      break;
    }
    if (!c) {
      // Got a long option
      c = long_options[option_index].val;
    }
    switch (c) {
    case 'o':
    case 'e':
    case 'g':
    case 'u':
    case 'h':
    case '?':
    default:
    }
  }
}
