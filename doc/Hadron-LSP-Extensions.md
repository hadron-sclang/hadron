# Hadron LSP Extensions

Hadron extends the [Language Server Protocol](https://microsoft.github.io/language-server-protocol) for its own
automation and testing purposes.

## Compilation Diagnostics

method: `hadron/compilationDiagnostics`
params: `HadronCompilationDiagnosticParams` defined as follows:

```
export interface HadronCompilationDiagnosticParams {
	/**
	 * Path to the source code file to return compilation diagnostic information about.
	 */
	textDocument: TextDocumentIdentifier;
}
```

response:

```
export interface HadronCompilationDiagnosticsResponse {
	/**
	 * The request id.
	 */
	id: integer;

	/**
	 * The parse tree.
	 */
	parseTree: HadronParseTreeNode;

	/**
	 * The root frame of the control flow graph.
	 */
	rootFrame: HadronFrame;

	/**
	 * The Linear Block, taken through to SSA resolution.
	 */
	 linearBlock: HadronLinearBlock;

	 /**
	  * JIT-compiled generic machine code.
	  */
	machineCode: HadronGenericMachineCode;
}
```
