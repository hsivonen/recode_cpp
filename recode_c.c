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

#define INPUT_EMPTY 0

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

#define INPUT_BUFFER_SIZE 2048
#define UTF8_INTERMEDIATE_BUFFER_SIZE 4096
#define UTF16_INTERMEDIATE_BUFFER_SIZE 2048
#define OUTPUT_BUFFER_SIZE 4096

void
convert_via_utf8(Decoder* decoder, Encoder* encoder, FILE* read, FILE* write,
                 bool last)
{
  uint8_t input_buffer[INPUT_BUFFER_SIZE];
  uint8_t intermediate_buffer[UTF8_INTERMEDIATE_BUFFER_SIZE];
  uint8_t output_buffer[OUTPUT_BUFFER_SIZE];

  bool current_input_ended = false;
  while (!current_input_ended) {
    size_t decoder_input_end = fread(input_buffer, 1, INPUT_BUFFER_SIZE, read);
    if (ferror(read)) {
      fprintf(stderr, "Error reading input.");
      exit(-5);
    }
    current_input_ended = (decoder_input_end == 0);
    bool input_ended = last && current_input_ended;
    size_t decoder_input_start = 0;
    for (;;) {
      bool had_replacements;
      size_t decoder_read = decoder_input_end - decoder_input_start;
      size_t decoder_written = UTF8_INTERMEDIATE_BUFFER_SIZE;
      uint32_t decoder_result = decoder_decode_to_utf8_with_replacement(
        decoder, input_buffer + decoder_input_start, &decoder_read,
        intermediate_buffer, &decoder_written, input_ended, &had_replacements);
      decoder_input_start += decoder_read;

      bool last_output = (input_ended && (decoder_result == INPUT_EMPTY));

      // Regardless of whether the intermediate buffer got full
      // or the input buffer was exhausted, let's process what's
      // in the intermediate buffer.

      const char* utf8_name = "UTF-8"; // TODO: use extern constant
      if (encoder_encoding(encoder) ==
          encoding_for_name((const uint8_t*)utf8_name, strlen(utf8_name))) {
        // If the target is UTF-8, optimize out the encoder.
        size_t file_written =
          fwrite(intermediate_buffer, 1, decoder_written, write);
        if (file_written != decoder_written) {
          fprintf(stderr, "Error writing output.");
          exit(-7);
        }
      } else {
        size_t encoder_input_start = 0;
        for (;;) {
          size_t encoder_read = decoder_written - encoder_input_start;
          size_t encoder_written = OUTPUT_BUFFER_SIZE;
          uint32_t encoder_result = encoder_encode_from_utf8_with_replacement(
            encoder, intermediate_buffer + encoder_input_start, &encoder_read,
            output_buffer, &encoder_written, last_output, &had_replacements);
          encoder_input_start += encoder_read;
          size_t file_written =
            fwrite(output_buffer, 1, encoder_written, write);
          if (file_written != encoder_written) {
            fprintf(stderr, "Error writing output.");
            exit(-6);
          }
          if (encoder_result == INPUT_EMPTY) {
            break;
          }
        }
      }

      // Now let's see if we should read again or process the
      // rest of the current input buffer.
      if (decoder_result == INPUT_EMPTY) {
        break;
      }
    }
  }
}

void
convert_via_utf16(Decoder* decoder, Encoder* encoder, FILE* read, FILE* write,
                  bool last)
{
  uint8_t input_buffer[INPUT_BUFFER_SIZE];
  uint16_t intermediate_buffer[UTF16_INTERMEDIATE_BUFFER_SIZE];
  uint8_t output_buffer[OUTPUT_BUFFER_SIZE];

  bool current_input_ended = false;
  while (!current_input_ended) {
    size_t decoder_input_end = fread(input_buffer, 1, INPUT_BUFFER_SIZE, read);
    if (ferror(read)) {
      fprintf(stderr, "Error reading input.");
      exit(-5);
    }
    current_input_ended = (decoder_input_end == 0);
    bool input_ended = last && current_input_ended;
    size_t decoder_input_start = 0;
    for (;;) {
      bool had_replacements;
      size_t decoder_read = decoder_input_end - decoder_input_start;
      size_t decoder_written = UTF16_INTERMEDIATE_BUFFER_SIZE;
      uint32_t decoder_result = decoder_decode_to_utf16_with_replacement(
        decoder, input_buffer + decoder_input_start, &decoder_read,
        intermediate_buffer, &decoder_written, input_ended, &had_replacements);
      decoder_input_start += decoder_read;

      bool last_output = (input_ended && (decoder_result == INPUT_EMPTY));

      // Regardless of whether the intermediate buffer got full
      // or the input buffer was exhausted, let's process what's
      // in the intermediate buffer.

      size_t encoder_input_start = 0;
      for (;;) {
        size_t encoder_read = decoder_written - encoder_input_start;
        size_t encoder_written = OUTPUT_BUFFER_SIZE;
        uint32_t encoder_result = encoder_encode_from_utf16_with_replacement(
          encoder, intermediate_buffer + encoder_input_start, &encoder_read,
          output_buffer, &encoder_written, last_output, &had_replacements);
        encoder_input_start += encoder_read;
        size_t file_written = fwrite(output_buffer, 1, encoder_written, write);
        if (file_written != encoder_written) {
          fprintf(stderr, "Error writing output.");
          exit(-6);
        }
        if (encoder_result == INPUT_EMPTY) {
          break;
        }
      }

      // Now let's see if we should read again or process the
      // rest of the current input buffer.
      if (decoder_result == INPUT_EMPTY) {
        break;
      }
    }
  }
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
  const char* utf8_name = "UTF-8";
  const Encoding* input_encoding = encoding_for_name(
    (const uint8_t*)utf8_name, strlen(utf8_name)); // TODO: Use extern constant
  const Encoding* output_encoding = encoding_for_name(
    (const uint8_t*)utf8_name, strlen(utf8_name)); // TODO: Use extern constant
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
