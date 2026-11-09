// The library is installed as libomarchy-theme.so.<version> (see ./stage); its soname is the ABI
// major, so apps link against libomarchy-theme.so.0 whatever minor version is installed.
fn main() {
    println!("cargo:rustc-cdylib-link-arg=-Wl,-soname,libomarchy-theme.so.0");
    println!("cargo:rerun-if-changed=build.rs");
}
