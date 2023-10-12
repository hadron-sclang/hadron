pub mod source_buffer;

pub use source_buffer::SourceBuffer;

#[macro_export]
macro_rules! sclang {
    ( $s:literal ) => {
        &source::SourceBuffer::new_from_string($s,
            const_format::formatcp!("{}:{}:{}", file!(), line!(), column!())).unwrap()
    };
}