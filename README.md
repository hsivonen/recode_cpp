# recode-cpp

recode-cpp is a test/sample app that's written in C++ and uses
[encoding_rs](https://github.com/hsivonen/encoding_rs).

It expects encoding_rs and [GSL](https://github.com/Microsoft/GSL) to have been checked out to adjacent directories.

## Licensing

Please see the file named COPYRIGHT.

## Building

Git, GNU Make and a version of GCC recent enough to accept `-std=c++14` are
assumed to be already installed.

###0. Install Rust (including Cargo) if you haven't already

See [rustup.rs](https://rustup.rs/). For
Linux and OS X, this means:
```
curl https://sh.rustup.rs -sSf | sh
```

###1. Clone encoding_rs

```
git clone https://github.com/hsivonen/encoding_rs.git
```

###2. Enable staticlib support for encoding_rs

Edit `encoding_rs/Cargo.toml` and uncomment the line
```
# crate-type = ["rlib", "staticlib"]
```

(The line is commented out due to a
[rustfmt bug](https://github.com/rust-lang-nursery/rustfmt/issues/828).)

###3. Clone GSL

```
git clone https://github.com/Microsoft/GSL.git
```

###4. Clone recode-cpp

```
git clone https://github.com/hsivonen/recode-cpp.git
```

###5. Build recode-cpp

```
cd recode-cpp
make
```

###6. Run it

```
./recode_cpp --help
```
