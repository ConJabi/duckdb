cd ../opendp/rust/build
sed '56d' derive.rs > derive.rs.tmp && mv derive.rs.tmp derive.rs
cd ../
cargo build --release --all-features