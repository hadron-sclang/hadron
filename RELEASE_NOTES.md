* **v0.1.0-alpha.1**
  * Adds a first implementation of `runtime::Slot`, the runtime dynamic type data container.
  * Adds a first implementation of `toolchain::lexer`.
* **v0.1.0-alpha.2**
  * Adds a first implementation of `toolchain::source`, a representation of input source either by
    memory-mapped files or from string literals. This is also useful for error reporting, as
    `SourceBuffer` objects include the file name or source location using the `sclang!()` macro.
  * Adds a first implementatin of a `toolchain::diagnostics` system, for flexible error reporting.
  * Adds just enough of a parser implementation to establish some of the patterns for how the
    rest could be written.

Next Release:

* **v0.1.0-alpha.3**