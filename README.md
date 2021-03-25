# Macchina-J2534
J2534 drivers for various Macchina hardware

This is a experimental driver which is built in Rust, and is unofficially ported to Linux and OSX as well as Windows.

The Linux and OSX port can be utilized by [OpenVehicleDiag](github.com/rnd-ash/OpenVehicleDiag)


## Feature matrix

:x: - Feature is not supported by the adapter

TODO - Feature is supported by the adapter, but work is needed on the driver side in order to utilize it

:heavy_minus_sign: - Feature works, however some parts of the full implementation are missing, so some bugs might exist

✔️ - Feature works fully according to the J2534 specification

|J2534 feature|[M2 UTD](https://www.macchina.cc/catalog/m2-boards/m2-under-dash)|[A0](https://www.macchina.cc/catalog/a0-boards/a0-under-dash)|
|---|---|---|
| Read battery voltage|:heavy_check_mark:|TODO|
| Read programming voltage|:x:|:x:|
| ISO-TP|:heavy_check_mark:|TODO|
| CAN |:heavy_minus_sign:| TODO |
| ISO9141| TODO | :x: |
| ISO14230-4| TODO | :x: |
|J1850PWM| TODO | :x: |
|J1850VPW| TODO | :x: |
|SCI|:x:|:x:|

## How to install

The process is generally the same for all supported hardware.

### Requirments
* Rust installed on your system [See here on how to](https://www.rust-lang.org/tools/install)
* Arduino IDE Installed [See here on how to](https://www.arduino.cc/en/software)

### Important information for windows users
You will need to install the i686-pc-windows-msvc toolchain!
```
$ rustup run stable-i686-pc-windows-msvc
```

### Installing the driver on Windows
1. Create the directory `C:\Program Files (x86)\macchina\passthru\`
2. Give the created `passthru` directory write permissions for your user account
3. From the repositories driver folder, run `build.bat`. This will compile and install the drive
4. Depending on your hardware, either open `driver_m2.reg` or `driver_a0.reg`, and modify the COM-PORT attribute in the reg file to match that of your adapter as listed in device manager
5. Merge the `driver.reg` file with the Windows registry

### Installing the driver on Linux and OSX
1. Create the directory `~/.passthru/`
2. From the repositories driver folder, run `build.sh`
3. In your `~/.passthru/` folder, you will find 2 JSON files. One for the M2 (`macchina_m2.json`) and one for the A0 (`macchina_a0.reg`). Change the `COM-PORT` attribute in the JSON to match that of your TTY port your adapter uses.

### Installing the adapter firmware
1. Depending on your hardware, you will need to either open the foler `firmware/A0` for the Macchina A0, or `firmware/M2` for the M2 in the Arduino IDE.
2. be sure you have read the setting up docs for your relivent adapter on Macchina's website [here for the A0](https://docs.macchina.cc/a0-docs/getting-started) or [here for the M2](https://docs.macchina.cc/m2-docs/arduino). 
3. Press the upload sketch button!


