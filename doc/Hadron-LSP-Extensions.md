# Hadron LSP Extensions

Hadron extends the [Language Server Protocol](https://microsoft.github.io/language-server-protocol) for its own
automation and testing purposes.

## Get Parse Tree

method: `hadron/parseTree`
params: `HadronParseTreeParams` defined as follows:

```
export interface HadronParseTreeParams {
	/**
	 * Path to the file to parse.
	 */
	uri: DocumentUri;
}
```
