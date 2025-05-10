// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

use clap::{Parser, Subcommand};
use gcs::{GCSBuilder, GCSQuery};
use std::fs::File;
use std::io::{BufRead, BufReader};

#[derive(Parser)]
#[command(name = "gcs")]
#[command(about = "Golomb Compressed Set implementation in Rust")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Build a GCS from a word list file
    Build {
        /// Path to the wordlist file
        #[arg(required = true)]
        wordlist_path: String,

        /// Path to save the GCS file (default: table.gcs)
        #[arg(short, long, default_value = "table.gcs")]
        output: Option<String>,

        /// False positive probability as 1/P (default: 1024)
        #[arg(short, long, default_value_t = 1024)]
        prob: u32,
    },

    /// Query words in a GCS file
    Query {
        /// Words to query
        #[arg(required = true)]
        words: Vec<String>,

        /// Path to the GCS file (default: table.gcs)
        #[arg(short, long, default_value = "table.gcs")]
        gcs_file: Option<String>,
    },
}

fn main() -> std::io::Result<()> {
    let cli = Cli::parse();

    match cli.command {
        Commands::Build {
            wordlist_path,
            output,
            prob,
        } => {
            let output_path = output.unwrap_or_else(|| "table.gcs".to_string());
            build_gcs(&wordlist_path, &output_path, prob)?
        }
        Commands::Query { words, gcs_file } => {
            let gcs_file = gcs_file.unwrap_or_else(|| "table.gcs".to_string());
            query_gcs(&gcs_file, &words)?
        }
    }

    Ok(())
}

fn build_gcs(wordlist_path: &str, output_path: &str, prob: u32) -> std::io::Result<()> {
    println!("Building GCS from '{}'...", wordlist_path);

    // Read the words from file
    let file = File::open(wordlist_path)?;
    let reader = BufReader::new(file);
    let words: Vec<String> = reader.lines().filter_map(Result::ok).collect();

    // Create and populate the GCS
    let mut gcs = GCSBuilder::new(words.len() as u32, prob);
    for word in &words {
        gcs.add(word.as_bytes());
    }

    // Finalize and save the GCS
    gcs.finalize_to_file(output_path)?;

    // Get file size for statistics
    let file_size = std::fs::metadata(output_path)?.len();

    println!("Number of words: {}", words.len());
    println!("False positives: {}", 1.0 / prob as f64);
    println!("Size: {} bytes", file_size);
    println!(
        "Bits per word: {}",
        file_size as f64 * 8.0 / words.len() as f64
    );

    Ok(())
}

fn query_gcs(gcs_file: &str, words: &[String]) -> std::io::Result<()> {
    println!("Querying GCS from '{}'...", gcs_file);

    // Open the GCS file
    let gcs = GCSQuery::from_file(gcs_file)?;

    // Query each word
    for word in words {
        let found = gcs.query(word.as_bytes())?;
        println!(
            "Querying for \"{}\": {}",
            word,
            if found { "TRUE" } else { "FALSE" }
        );
    }

    Ok(())
}
