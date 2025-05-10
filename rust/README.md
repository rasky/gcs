# Golomb Compressed Set (GCS) - Rust Implementation

This is a Rust implementation of the Golomb Compressed Set, a statistical compressed data structure similar to Bloom filters but more compact. 

## Overview

Given N elements and P probability of a false positive, an optimal Bloom filter requires at least N*log2(e)*log2(1/P) bits, whereas GCS gets closer to the theoretical minimum of N*log2(1/P). This implementation provides functionality to build and query GCS data structures in Rust.

## Features

- Create a GCS from a list of strings/words
- Query for the existence of elements in the GCS
- Command-line interface for building and querying
- Efficient bit-level operations for compactness
- Comprehensive test vectors and unit tests

## Requirements

- Rust (latest stable version)
- Dependencies defined in Cargo.toml (md-5, byteorder, clap)

## Installation

1. Make sure Rust and Cargo are installed (https://www.rust-lang.org/tools/install)
2. Clone this repository
3. Build the project:

```bash
cd rust
cargo build --release
```

## Usage

### From Command Line

To build a GCS from a word list file:

```bash
./target/release/gcs build path/to/wordlist.txt
```

To query a GCS for multiple words:

```bash
./target/release/gcs query word1 word2 word3
```

### As a Library

```rust
use gcs::{GCSBuilder, GCSQuery};

// Create a GCS with N elements and 1/P chance of false positives
let mut builder = GCSBuilder::new(num_elements, prob);

// Add elements
for word in words {
    builder.add(word.as_bytes());
}

// Finalize and save to a file
builder.finalize_to_file("output.gcs")?;

// Query the GCS
let gcs = GCSQuery::from_file("output.gcs")?;
let found = gcs.query("test".as_bytes())?;
println!("Found: {}", found);
```

## Testing

The implementation includes extensive test vectors and unit tests for all components:

```bash
cd rust
cargo test
```

Test coverage includes:

- Bit-level operations with various patterns and edge cases
- Golomb encoding/decoding with different parameters
- GCS hash function distribution and consistency
- GCS construction and querying with predefined test vectors
- Edge cases like empty sets, binary data, and UTF-8 characters
- Large datasets to verify scalability

## License

This is free and unencumbered software released into the public domain. See the UNLICENSE file for details.