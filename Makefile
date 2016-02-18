# Copyright 2016 Mozilla Foundation. See the COPYRIGHT
# file at the top-level directory of this distribution.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

CPPFLAGS = -Wall -Wextra -Werror -O3 -std=c++14 -I../encoding-rs/target/include/ -I../encoding-rs/include/ -I../GSL/include/
LDFLAGS = -Wl,--gc-sections -ldl -lpthread -lgcc_s -lrt -lc -lm -lstdc++

recode_cpp: recode_cpp.o ../encoding-rs/target/release/libencoding_rs.a
	$(CC) -o $@ $^ $(LDFLAGS)

../encoding-rs/target/release/libencoding_rs.a: cargo

../encoding-rs/target/include/encoding_rs.h: cargo

.PHONY: cargo
cargo:
	cd ../encoding-rs; cargo build --release

.PHONY: all
all: recode_cpp

.PHONY: fmt
fmt:
	clang-format-3.7 --style=mozilla -i *.cpp

.PHONY: clean
clean:
	rm recode_cpp
