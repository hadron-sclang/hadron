#![no_main]

use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: &[u8]| {
    let s = std::str::from_utf_unchecked(data);
    let _ = hadron_sclang::toolchain::lexer::tokenize(&s).count();
});
