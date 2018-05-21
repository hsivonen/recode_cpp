# recode_cpp

recode_cpp is a test/sample app that's written in C++17 and uses
[encoding_rs](https://github.com/hsivonen/encoding_rs).

It expects [GSL](https://github.com/Microsoft/GSL) to have been checked out
to an adjacent directory. Revision `d6a2242d97827449f3f1c1c2e54214ceb9e80d62`
was tested.

## Licensing

Please see the file named COPYRIGHT.

## Building

Git, GNU Make and a version of GCC recent enough to accept `-std=c++17` are
assumed to be already installed. (Ubuntu 18.04 is known to work.)

### 0. Install Rust (including Cargo) if you haven't already

See [rustup.rs](https://rustup.rs/). For
Linux and OS X, this means:
```
curl https://sh.rustup.rs -sSf | sh
```

### 1. Clone recode_cpp

```
git clone https://github.com/hsivonen/recode_cpp.git
```

### 2. Clone GSL

```
git clone https://github.com/Microsoft/GSL.git
```

### 3. Build recode_cpp

```
cd recode_cpp
make
```

### 4. Run it

```
./recode_cpp --help
```
