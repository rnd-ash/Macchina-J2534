use std::env;
fn main() {
    let target_os = env::var("CARGO_CFG_TARGET_OS");
    match target_os.as_ref().map(|x| &**x) {
        Ok("macos") | Ok("linux") => {}
        Ok("windows") => println!("cargo:rustc-cdylib-link-arg=/DEF:driver.def"),
        tos => panic!("unknown target os {:?}!", tos),
    }
}
