use hadron_sclang::toolchain;

// Used on fuzzer inputs, takes a path to a file that Hadron should be able to lex without
// crashing.
fn lex(path: std::path::PathBuf) {
    let source = toolchain::source::SourceBuffer::new_from_file(&path);
    assert!(!source.is_err());
    let source = source.unwrap();
    let mut diags = toolchain::diagnostics::NullDiagnosticConsumer {};
    let _ = toolchain::lexer::TokenizedBuffer::tokenize(&source, &mut diags);
}

// Found in [#142](https://github.com/hadron-sclang/hadron/issues/142)
// Fixed in [#145](https://github.com/hadron-sclang/hadron/pull/145)
#[test]
fn test_invalid_utf8() {
    lex("tests/fuzz/lex/crash-aebb5cb819bee464ae8562d6f15645fab35bb87b".into());
}