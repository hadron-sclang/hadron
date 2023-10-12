use mmap_rs;
use std::fs::File;

enum SourceBufferKind<'a> {
    File { buffer: mmap_rs::Mmap },
    Memory { string: &'a str },
}

// Keeps source and a file name in the same object, so they provide the same lifetimes.
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
        Ok(SourceBuffer { kind: SourceBufferKind::File{buffer}, file_name })
    }

    pub fn new_from_string(string: &'a str, name: &str) -> Result<SourceBuffer<'a>, mmap_rs::Error> {
        let file_name = String::from(name);
        Ok(SourceBuffer { kind: SourceBufferKind::Memory{string}, file_name })
    }

    pub fn code(&self) -> &'_ str {
        match &self.kind {
            SourceBufferKind::File { buffer } => {
                unsafe {
                    std::str::from_utf8_unchecked(buffer.as_slice())
                }
            },
            SourceBufferKind::Memory { string } => { string }
        }
    }

    pub fn file_name(&self) -> &str {
        self.file_name.as_str()
    }
}

