//! An in-memory representation of input source code, with associated file name.
//!
//! Rust demands that all strings represented by a `&str` must be valid utf-8. When converting from
//! `&[u8]` to `&str` the usual process is to scan and validate the entire string. This adds an
//! additional pass through the string, however, adding unacceptable latency. Here we represent
//! the input string as a `bstr::BStr`, which allows validation of the string while scanning it
//! from codepoint to codepoint. Hadron essentially folds the utf-8 validation into the lexing
//! pass, with the expectation that any invalid utf-8 characters will halt lexing and invalidate
//! the input entirely. This allows the lexer to represent the input using `&str` for subsequent
//! passes, because it has validated each character while lexing.
//!
use bstr;
use mmap_rs;
use std::fs::File;


enum SourceBufferKind<'a> {
    File { buffer: mmap_rs::Mmap },
    Memory { string: &'a bstr::BStr },
}

// Keeps source and a file name in the same object, so they provide the same lifetime.
pub struct SourceBuffer<'a> {
    kind: SourceBufferKind<'a>,
    file_name: String,
}

impl<'a> SourceBuffer<'a> {
    // Does nothing to check if the input file is valid utf8
    pub fn new_from_file(file_path: &std::path::Path) -> Result<SourceBuffer<'_>, mmap_rs::Error> {
        let file = File::open(file_path)?;
        let len = File::metadata(&file)?.len();
        let buffer = unsafe {
            mmap_rs::MmapOptions::new(len.try_into().unwrap())?.with_file(&file, 0).map()?
        };
        let file_name = String::from(file_path.to_str().unwrap());
        Ok(SourceBuffer { kind: SourceBufferKind::File { buffer }, file_name })
    }

    pub fn new_from_string(
        string: &'a bstr::BStr,
        name: &str,
    ) -> Result<SourceBuffer<'a>, mmap_rs::Error> {
        let file_name = String::from(name);
        Ok(SourceBuffer { kind: SourceBufferKind::Memory { string }, file_name })
    }

    pub fn code(&self) -> &'_ bstr::BStr {
        match &self.kind {
            SourceBufferKind::File { buffer } => bstr::BStr::new(buffer.as_slice()),
            SourceBufferKind::Memory { string } => string,
        }
    }

    pub fn file_name(&self) -> &str {
        self.file_name.as_str()
    }
}
