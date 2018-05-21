// Copyright 2016 Mozilla Foundation. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <getopt.h>
#include <iterator>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding_rs_cpp.h"

using namespace encoding_rs;

const Encoding*
get_encoding(const char* label)
{
  const Encoding* enc =
    Encoding::for_label(gsl::cstring_span<>(label, strlen(label)));
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
    "Usage: %s [-f INPUT_ENCODING] [-t OUTPUT_ENCODING] [-o OUTFILE] [INFILE] "
    "[...]\n\n"
    "Options:\n"
    "    -o, --output PATH\n"
    "                        set output file name (- for stdout; the default)\n"
    "    -f, --from-code LABEL\n"
    "                        set input encoding (defaults to UTF-8)\n"
    "    -t, --to-code LABEL\n"
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
convert_via_utf8(Decoder& decoder,
                 Encoder& encoder,
                 FILE* read,
                 FILE* write,
                 bool last)
{
  std::array<uint8_t, INPUT_BUFFER_SIZE> input_buffer;
  std::array<uint8_t, UTF8_INTERMEDIATE_BUFFER_SIZE> intermediate_buffer;
  std::array<uint8_t, OUTPUT_BUFFER_SIZE> output_buffer;

  bool current_input_ended = false;
  while (!current_input_ended) {
    size_t decoder_input_end =
      fread(input_buffer.data(), 1, input_buffer.size(), read);
    if (ferror(read)) {
      fprintf(stderr, "Error reading input.");
      exit(-5);
    }
    current_input_ended = (decoder_input_end == 0);
    bool input_ended = last && current_input_ended;
    size_t decoder_input_start = 0;
    for (;;) {
      size_t decoder_read;
      size_t decoder_written;
      uint32_t decoder_result;

      std::tie(decoder_result, decoder_read, decoder_written, std::ignore) =
        decoder.decode_to_utf8(
          gsl::span<const uint8_t>(input_buffer)
            .subspan(decoder_input_start,
                     decoder_input_end - decoder_input_start),
          intermediate_buffer,
          input_ended);
      decoder_input_start += decoder_read;

      bool last_output = (input_ended && (decoder_result == INPUT_EMPTY));

      // Regardless of whether the intermediate buffer got full
      // or the input buffer was exhausted, let's process what's
      // in the intermediate buffer.

      if (encoder.encoding() == UTF_8_ENCODING) {
        // If the target is UTF-8, optimize out the encoder.
        size_t file_written =
          fwrite(intermediate_buffer.data(), 1, decoder_written, write);
        if (file_written != decoder_written) {
          fprintf(stderr, "Error writing output.");
          exit(-7);
        }
      } else {
        size_t encoder_input_start = 0;
        for (;;) {
          size_t encoder_read;
          size_t encoder_written;
          uint32_t encoder_result;

          std::tie(encoder_result, encoder_read, encoder_written, std::ignore) =
            encoder.encode_from_utf8(
              std::string_view(
                reinterpret_cast<char*>(intermediate_buffer.data()),
                intermediate_buffer.size())
                .substr(encoder_input_start,
                        decoder_written - encoder_input_start),
              output_buffer,
              last_output);
          encoder_input_start += encoder_read;
          size_t file_written =
            fwrite(output_buffer.data(), 1, encoder_written, write);
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
convert_via_utf16(Decoder& decoder,
                  Encoder& encoder,
                  FILE* read,
                  FILE* write,
                  bool last)
{
  std::array<uint8_t, INPUT_BUFFER_SIZE> input_buffer;
  std::array<char16_t, UTF16_INTERMEDIATE_BUFFER_SIZE> intermediate_buffer;
  std::array<uint8_t, OUTPUT_BUFFER_SIZE> output_buffer;

  bool current_input_ended = false;
  while (!current_input_ended) {
    size_t decoder_input_end =
      fread(input_buffer.data(), 1, input_buffer.size(), read);
    if (ferror(read)) {
      fprintf(stderr, "Error reading input.");
      exit(-5);
    }
    current_input_ended = (decoder_input_end == 0);
    bool input_ended = last && current_input_ended;
    size_t decoder_input_start = 0;
    for (;;) {
      size_t decoder_read;
      size_t decoder_written;
      uint32_t decoder_result;

      std::tie(decoder_result, decoder_read, decoder_written, std::ignore) =
        decoder.decode_to_utf16(
          gsl::span<const uint8_t>(input_buffer)
            .subspan(decoder_input_start,
                     decoder_input_end - decoder_input_start),
          intermediate_buffer,
          input_ended);
      decoder_input_start += decoder_read;

      bool last_output = (input_ended && (decoder_result == INPUT_EMPTY));

      // Regardless of whether the intermediate buffer got full
      // or the input buffer was exhausted, let's process what's
      // in the intermediate buffer.

      size_t encoder_input_start = 0;
      for (;;) {
        size_t encoder_read;
        size_t encoder_written;
        uint32_t encoder_result;

        std::tie(encoder_result, encoder_read, encoder_written, std::ignore) =
          encoder.encode_from_utf16(
            std::u16string_view(intermediate_buffer.data(),
                                intermediate_buffer.size())
              .substr(encoder_input_start,
                      decoder_written - encoder_input_start),
            output_buffer,
            last_output);
        encoder_input_start += encoder_read;
        size_t file_written =
          fwrite(output_buffer.data(), 1, encoder_written, write);
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
convert(Decoder& decoder,
        Encoder& encoder,
        FILE* read,
        FILE* write,
        bool last,
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
    { "output", required_argument, NULL, 'o' },
    { "from-code", required_argument, NULL, 'f' },
    { "to-code", required_argument, NULL, 't' },
    { "utf16-intermediate", no_argument, NULL, 'u' },
    { "help", no_argument, NULL, 'h' },
    { 0, 0, 0, 0 }
  };

  bool use_utf16 = false;
  const Encoding* input_encoding = UTF_8_ENCODING;
  const Encoding* output_encoding = UTF_8_ENCODING;
  FILE* output = stdout;

  for (;;) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "o:f:t:uh", long_options, &option_index);
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
      case 'f':
        input_encoding = get_encoding(optarg);
        break;
      case 't':
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

  std::unique_ptr<Decoder> decoder = input_encoding->new_decoder();
  std::unique_ptr<Encoder> encoder = output_encoding->new_encoder();

  if (optind == argc) {
    convert(*decoder, *encoder, stdin, output, true, use_utf16);
  } else {
    while (optind < argc) {
      const char* path = argv[optind++];
      FILE* read = fopen(path, "rb");
      if (!read) {
        fprintf(stderr, "Cannot open %s for reading; exiting.", path);
        exit(-4);
      }
      convert(*decoder, *encoder, read, output, (optind == argc), use_utf16);
    }
  }

  exit(0);
}
