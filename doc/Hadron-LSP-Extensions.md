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
	 * Individual compilation units for the document.
	 */
	compilationUnits: array;
}
```

As an input file may be a class file it can contain multiple units of compilation, the `compilationUnits` member is an
array of `HadronCompilationDiagnosticUnit` members.

```
export interface HadronCompilationDiagnosticUnit {
	/**
	 * The name of the compilation unit. If the input was an interpreter script  will be 'INTERPRET',
	 * otherwise the name will be formatted as "ClassName:methodName"
	 */
	 name: string;

	/**
	 * The parse tree for the entire document.
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
