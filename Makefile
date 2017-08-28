# Copyright 2016 Mozilla Foundation. See the COPYRIGHT
# file at the top-level directory of this distribution.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

CPPFLAGS = -Wall -Wextra -Werror -O3 -std=c++14 -I../GSL/include/
LDFLAGS = -Wl,--gc-sections -ldl -lpthread -lgcc_s -lrt -lc -lm -lstdc++

recode_cpp: recode_cpp.o rustglue/target/release/librustglue.a
	$(CC) -o $@ $^ $(LDFLAGS)

recode_cpp.o: recode_cpp.cpp encoding_rs.h encoding_rs_statics.h encoding_rs_cpp.h ../GSL/include/gsl/gsl ../GSL/include/gsl/span

rustglue/target/release/librustglue.a: cargo

.PHONY: cargo
cargo:
	cd rustglue/; cargo build --release

.PHONY: all
all: recode_cpp

.PHONY: fmt
fmt:
	clang-format-3.8 --style=mozilla -i *.cpp

.PHONY: clean
clean:
	rm recode_cpp
	cd rustglue/; cargo clean
