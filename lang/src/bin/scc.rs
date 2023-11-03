use hadron_sclang::toolchain;

const HELP: &str = "\
    scc - the Hadron SuperCollider compiler

    USAGE:
        scc [--phase PHASE] [--quiet] FILENAME

    OPTIONS:
        --phase PHASE       Which compilation phase to take the compilation through. PHASE is one
                            of 'lex', or 'parse'.
        --quiet             If present, all non-error output is suppressed.

    ARGS:
        FILENAME            The path to the sclang file to compile, or supply '-' to take input
                            from stdin.
";

#[derive(Debug, Eq, PartialEq)]
enum CompilationPhase {
    Lex,
    Parse,
}

#[derive(Debug, Eq, PartialEq)]
struct SCCArgs {
    phase: Option<CompilationPhase>,
    quiet: bool,
    source_file: std::path::PathBuf,
}

fn main() {
    let args = match parse_args() {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Error: {}.", e);
            std::process::exit(1);
        }
    };

    let source = toolchain::source::SourceBuffer::new_from_file(&args.source_file);
    if source.is_err() {
        eprintln!(
            "Error opening source file {}: {}",
            args.source_file.display(),
            source.err().unwrap()
        );
        std::process::exit(1);
    }
    let source = source.unwrap();

    let mut diags = toolchain::diagnostics::diagnostic_emitter::StreamDiagnosticConsumer::new(
        std::io::stderr(),
    );
    let tb = toolchain::lexer::TokenizedBuffer::tokenize(&source, &mut diags);
    if args.phase == Some(CompilationPhase::Lex) {
        if !args.quiet {
            println!("TokenizedBuffer [");
            tb.print_tokens();
            println!("]");
        }
    }
}

fn parse_args() -> Result<SCCArgs, pico_args::Error> {
    let mut pargs = pico_args::Arguments::from_env();

    if pargs.contains(["-h", "--help"]) {
        print!("{}", HELP);
        std::process::exit(0);
    }

    let args = SCCArgs {
        phase: pargs.opt_value_from_fn("--phase", parse_phase)?,
        quiet: pargs.contains("--quiet"),
        source_file: pargs.free_from_str()?,
    };

    let remaining = pargs.finish();
    if !remaining.is_empty() {
        eprintln!("Error: unused arguments left: {:?}.", remaining);
        std::process::exit(1);
    }
    Ok(args)
}

fn parse_phase(s: &str) -> Result<CompilationPhase, &'static str> {
    match s {
        "lex" => Ok(CompilationPhase::Lex),
        "parse" => Ok(CompilationPhase::Parse),
        _ => Err("unrecognized compilation phase."),
    }
}
