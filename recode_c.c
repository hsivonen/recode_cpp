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
#include <stdbool.h>
#include <string.h>

typedef struct _Encoding Encoding;
typedef struct _Decoder Decoder;
typedef struct _Encoder Encoder;
#include "encoding_rs.h"

const Encoding*
get_encoding(const char* label)
{
  const Encoding* enc =
    encoding_for_label((const uint8_t*)label, strlen(label));
  if (!enc) {
    fprintf(stderr, "%s is not a known encoding label; exiting.", label);
    exit(-2);
  }
  return enc;
}

void
print_usage(const char* program)
{
  printf(
    "Usage: %s [-e INPUT_ENCODING] [-g OUTPUT_ENCODING] [-o OUTFILE INFILE] "
    "[...]\n\n"
    "Options:\n"
    "    -o, --output-file PATH\n"
    "                        set output file name (- for stdout; the default)\n"
    "    -e, --input-encoding LABEL\n"
    "                        set input encoding (defaults to UTF-8)\n"
    "    -g, --output-encoding LABEL\n"
    "                        set output encoding (defaults to UTF-8)\n"
    "    -u, --utf16-intermediate\n"
    "                        use UTF-16 instead of UTF-8 as the intermediate\n"
    "                        encoding\n"
    "    -h, --help          print usage help\n",
    program);
}

void
convert_via_utf8(Decoder* decoder, Encoder* encoder, FILE* read, FILE* write,
                 bool last)
{
  uint8_t input_buffer[2048];
  uint8_t intermediate_buffer[2048];
  uint8_t output_buffer[4096];

  bool current_input_ended = false;
  while (!current_input_ended) {
    size_t decoder_input_end = fread(input_buffer, 1, 2048, read);
    if (ferror(read)) {
      fprintf(stderr, "Error reading input.");
      exit(-5);
    }
    // TODO
  }
}

void
convert_via_utf16(Decoder* decoder, Encoder* encoder, FILE* read, FILE* write,
                  bool last)
{
}

void
convert(Decoder* decoder, Encoder* encoder, FILE* read, FILE* write, bool last,
        bool use_utf16)
{
  if (use_utf16) {
    convert_via_utf16(decoder, encoder, read, write, last);
  } else {
    convert_via_utf8(decoder, encoder, read, write, last);
  }
}

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

  bool use_utf16 = false;
  const Encoding* input_encoding = NULL;  // TODO: default to UTF-8
  const Encoding* output_encoding = NULL; // TODO: default to UTF-8
  FILE* output = stdout;

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
        output = fopen(optarg, "wb");
        if (!output) {
          fprintf(stderr, "Cannot open %s for writing; exiting.", optarg);
          exit(-3);
        }
        break;
      case 'e':
        input_encoding = get_encoding(optarg);
        break;
      case 'g':
        output_encoding = get_encoding(optarg);
        break;
      case 'u':
        use_utf16 = true;
        break;
      case 'h':
        print_usage(argv[0]);
        exit(0);
      case '?':
        print_usage(argv[0]);
        exit(-1);
      default:
        break;
    }
  }

  Decoder* decoder = encoding_new_decoder(input_encoding);
  Encoder* encoder = encoding_new_encoder(output_encoding);

  if (optind == argc) {
    convert(decoder, encoder, stdin, output, true, use_utf16);
  } else {
    while (optind < argc) {
      const char* path = argv[optind++];
      FILE* read = fopen(path, "rb");
      if (!read) {
        fprintf(stderr, "Cannot open %s for reading; exiting.", path);
        exit(-4);
      }
      convert(decoder, encoder, read, output, (optind == argc), use_utf16);
    }
  }

  // If we exit() early, we leak these, which isn't valgrind-clean, but doesn't
  // matter.
  decoder_free(decoder);
  encoder_free(encoder);
  exit(0);
}
