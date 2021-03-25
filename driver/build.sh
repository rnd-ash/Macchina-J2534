#!/usr/bin/env bash

path="release"

echo "Macchina Driver Installer"

UNAME=$(uname)

if [ ! -d ~/.passthru ]; then
    mkdir -p ~/.passthru;
fi

if [ $UNAME = Darwin ]; then
    echo "Building the cargo - M2"
    cargo build --release --features M2
    echo "Copying the M2 driver to ~/.passthru/"
    cp target/${path}/libmacchina_pt_driver.dylib ~/.passthru/macchina_driver_m2.so
    echo "Building the A0 cargo - A0"
    cargo build --release --features A0
    echo "Copying the driver to ~/.passthru/"
    cp target/${path}/libmacchina_pt_driver.dylib ~/.passthru/macchina_driver_a0.so
    echo "Copying JSON to ~/.passthru/"
    cp macchina_m2.json ~/.passthru/macchina_m2.json
    cp macchina_a0.json ~/.passthru/macchina_a0.json
    
else
    echo "Building the cargo - M2"
    cargo build --release --features M2
    echo "Copying the M2 driver to ~/.passthru/"
    cp target/${path}/libmacchina_pt_driver.so ~/.passthru/macchina_driver_m2.so
    echo "Building the A0 cargo - A0"
    cargo build --release --features A0
    echo "Copying the driver to ~/.passthru/"
    cp target/${path}/libmacchina_pt_driver.so ~/.passthru/macchina_driver_a0.so
    echo "Copying JSON to ~/.passthru/"
    cp macchina_m2.json ~/.passthru/macchina_m2.json
    cp macchina_a0.json ~/.passthru/macchina_a0.json
fi

echo "Driver install is complete. Happy car hacking!"


