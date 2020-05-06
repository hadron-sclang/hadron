Lexer Design Notes
==================

Source code is always assumed to fit in memory. mmap() could be an option for down the road.

Some tips from STB [here](https://nothings.org/computer/lexing.html), mostly about branch prediction.

lexer will ignore line and column, this can be re-parsed if needed. Output is vector of tokens and byte offsets within
string. Lexer diagnostics can later provide info if needed.

All in-memory, no copies.

