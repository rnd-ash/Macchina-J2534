@echo off

cargo build --target i686-pc-windows-msvc --release --features M2 || echo "M2 BUILD FAILED" && exit 1
copy ".\target\i686-pc-windows-msvc\release\macchina_pt_driver.dll" "C:\Program Files (x86)\macchina\passthru\driver_m2.dll" || echo "FAILED TO COPY M2 DLL" && exit 1

cargo build --target i686-pc-windows-msvc --release --features A0 || echo "M2 BUILD FAILED" && exit 1
copy ".\target\i686-pc-windows-msvc\release\macchina_pt_driver.dll" "C:\Program Files (x86)\macchina\passthru\driver_a0.dll" || echo "FAILED TO COPY A0 DLL" && exit 1

echo "Install complete! - Please merge registry the appropriate registry entry"
