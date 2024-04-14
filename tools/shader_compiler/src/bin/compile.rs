use clap::ArgAction;
use clap::Parser;
use glob::glob;
use std::fs::File;
use std::io::{Read, Write};
use std::path::PathBuf;
use std::process::Command;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[arg(short, long)]
    shader_root: PathBuf,

    #[arg(short, long)]
    output_root: PathBuf,

    #[arg(short, long, action=ArgAction::SetTrue)]
    all: bool,
}

fn read_content(path: &PathBuf) -> String {
    let mut f = File::open(path).expect("file not found");
    let mut content = String::new();
    f.read_to_string(&mut content)
        .expect("something went wrong reading the file");
    content
}

fn make_md5(input_path: &PathBuf) -> String {
    let contents = read_content(input_path);
    let md5_digest = md5::compute(contents);
    format!("{:x}", md5_digest)
}

fn write_content(path: &PathBuf, content: &str) -> Result<(), Box<dyn std::error::Error>> {
    let mut file = File::create(path).expect("Failed to create file");
    writeln!(file, "{}", content)?;
    file.flush()?;
    Ok(())
}

fn remove_breakline(mut str: String) -> String {
    if str.ends_with('\n') {
        str.pop();
        if str.ends_with('\r') {
            str.pop();
        }
    }
    str.to_string()
}

fn extract_dirs(path: PathBuf) -> String {
    let components: Vec<_> = path.components().collect();
    let dirs: Vec<_> = components
        .iter()
        .filter_map(|c| c.as_os_str().to_str())
        .collect();

    let relevant_dirs = &dirs[2..dirs.len() - 1];

    relevant_dirs.join("/")
}

fn compile_path(
    path: &PathBuf,
    processed_root: &PathBuf,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut command = Command::new("glslc");
    if !processed_root.exists() {
        std::fs::create_dir_all(processed_root)?;
    }
    let extracted_dir = extract_dirs(path.to_path_buf());

    let output_root = processed_root.join("spv").join(extracted_dir.clone());
    if !output_root.exists() {
        std::fs::create_dir_all(&output_root)?;
    }

    let cache_root = processed_root.join(".cache").join(extracted_dir);
    if !cache_root.exists() {
        std::fs::create_dir_all(&cache_root)?;
    }

    let output_path =
        output_root.join(path.file_name().unwrap().to_owned().into_string().unwrap() + ".spv");

    let cache_path = cache_root
        .join(format!(
            "{}.md5",
            path.file_name().unwrap().to_string_lossy()
        ))
        .to_owned();

    print!(
        "Processing: {}, output_path: {}",
        path.display(),
        output_path.display()
    );

    let md5 = make_md5(&path);
    if cache_path.exists() && md5 == remove_breakline(read_content(&cache_path).to_owned()) {
        println!("...skipped");
        return Ok(());
    }

    if output_path.exists() {
        std::fs::remove_file(&output_path)?;
    }
    if cache_path.exists() {
        std::fs::remove_file(&cache_path)?;
    }
    println!();

    // write md5 to cache
    write_content(&cache_path, &md5)?;

    let output = command
        .arg(path)
        .arg("-g")
        .arg("-O0")
        .arg("-o")
        .arg(output_path)
        .output()
        .expect("failed to execute process");
    std::io::stderr().write_all(&output.stderr).unwrap();
    Ok(())
}

fn compile() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();

    let extensions = vec!["vert", "frag", "comp"];

    if args.all && args.output_root.exists() {
        std::fs::remove_dir_all(&args.output_root)?;
    }

    for ext in extensions {
        for entry in glob(
            args.shader_root
                .join("**")
                .join(format!("*.{}", ext))
                .to_str()
                .unwrap(),
        )
        .expect("Failed tp read glob pattern")
        {
            match entry {
                Ok(path) => compile_path(&path, &args.output_root)?,
                Err(e) => println!("{:?}", e),
            }
        }
    }

    Ok(())
}

fn main() {
    compile().unwrap()
}
