rustup install nightly

Maybe?
sudo apt-get install libssl-dev

cargo install cargo-fuzz

cargo +nightly fuzz list
cargo +nightly fuzz run tokenized_buffer
