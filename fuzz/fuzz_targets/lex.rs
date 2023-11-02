#![no_main]

use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    let s = unsafe { std::str::from_utf8_unchecked(data) };
    let source = hadron_sclang::toolchain::source::source_buffer::SourceBuffer::new_from_string(s,
        "fuzz_targets/tokenized_buffer.rs").unwrap();
    let _ = hadron_sclang::toolchain::lexer::TokenizedBuffer::tokenize(&source);
});
