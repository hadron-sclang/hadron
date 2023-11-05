use std::{
    env,
    path::{Path, PathBuf},
};

use argh::FromArgs;

#[derive(FromArgs, PartialEq, Debug)]
/// Top-level command.
struct Args {
    #[argh(subcommand)]
    subcommand: SubCommand,
}

#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum SubCommand {
    Coverage(CoverageArgs),
    ContinuousIntegration(CIArgs),
}

#[derive(FromArgs, PartialEq, Debug)]
/// Collect test coverage data.
#[argh(subcommand, name = "cov")]
struct CoverageArgs {
    #[argh(switch)]
    /// produce an html report instead of the default .lcov file.
    report: bool,
}

#[derive(FromArgs, PartialEq, Debug)]
/// Run the continuous integration validation tests.
#[argh(subcommand, name = "ci")]
struct CIArgs {}

fn main() -> Result<(), DynError> {
    let args: Args = argh::from_env();
    match args.subcommand {
        SubCommand::Coverage(cov_args) => {
            coverage(cov_args.report)?;
        }
        SubCommand::ContinuousIntegration(_) => {
            ci()?;
        }
    };

    Ok(())
}

type DynError = Box<dyn std::error::Error>;

fn coverage(report: bool) -> Result<(), DynError> {
    let cov_path = project_root().join("target").join("coverage");
    fs_extra::dir::create_all(&cov_path, true)?;

    // Collect the coverage information by invoking `cargo test` with the appropriate environment
    // variables.
    println!("** collecting coverage information.");
    let cargo = cargo();
    duct::cmd!(cargo, "test", "--tests")
        .env("CARGO_INCREMENTAL", "0")
        .env("RUSTFLAGS", "-C instrument-coverage")
        .env("LLVM_PROFILE_FILE", "cargo-test-%p-%m.profraw")
        .run()?;

    // Now invoke grcov to coallate the coverage data into either html report or lcov file.
    println!("** coallating coverage data.");
    let (output_format, output_path) = if report {
        ("html", cov_path.join("html"))
    } else {
        ("lcov", cov_path.join("tests.lcov"))
    };
    duct::cmd!(
        "grcov",
        ".",
        "--binary-path",
        project_root().join("target").join("debug").join("deps"),
        "-s",
        ".",
        "-t",
        output_format,
        "--branch",
        "--ignore-not-existing",
        "--ignore", // ignore stuff outside root project directory
        "../*",
        "--ignore", // ignore stuff with a fully specified path (likely system libs)
        "/*",
        "--ignore", // ignore everything in this xtask folder
        "xtask/*",
        "--ignore", // ignore integration test code, this always runs in a test build!
        "lang/tests/*",
        "--ignore", // ignore the binaries, they aren't exercised in test
        "lang/src/bin/*",
        "--ignore", // ignore the unittest code, always runs.
        "*_unittests.rs",
        "-o",
        output_path,
    )
    .run()?;

    // Cleanup any .profraw files in the project.
    println!("** cleaning up *.profraw files.");
    let profraw_files: Result<Vec<PathBuf>, _> = glob::glob("**/*.profraw")?.collect();
    profraw_files.unwrap().iter().try_for_each(fs_extra::file::remove)?;

    if report {
        println!("** done. Generated coverage report in target/coverage/html/index.html.");
    } else {
        println!("** done. Machine-readable coverage report in target/coverage/tests.lcov")
    }
    Ok(())
}

fn ci() -> Result<(), DynError> {
    let cargo = cargo();
    println!("** checking code formatting with `cargo fmt --all -- --check`");
    duct::cmd!(&cargo, "fmt", "--all", "--", "--check",).run()?;

    println!("** linting code with `cargo clippy -- --deny all`");
    duct::cmd!(&cargo, "clippy", "--", "--deny", "clippy::all",).run()?;

    println!("** building and testing code with `RUSTFLAGS=\"-D warnings\" cargo test`");
    duct::cmd!(&cargo, "test").env("RUSTFLAGS", "-D warnings").run()?;

    println!("** all checks passed!");
    Ok(())
}

fn project_root() -> PathBuf {
    Path::new(&env!("CARGO_MANIFEST_DIR")).ancestors().nth(1).unwrap().to_path_buf()
}

fn cargo() -> String {
    env::var("CARGO").unwrap_or_else(|_| "cargo".to_string())
}
