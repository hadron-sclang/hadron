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
	textDocument: TextDocumentIdentifier;
}
```

response:

```
export interface HadronParseTreeResponse {
	/**
	 * The request id.
	 */
	id: integer;

	/**
	 * The root of the parse tree.
	 */
	parseTree: HadronParseTreeNode;
}
```

Where `HadronParseTreeNode` is a recursive JSON object that contains a literal transcription of the contents of each
node in the parse tree, plus the addition of a unique serial number that identifies each node, for ease of use.
