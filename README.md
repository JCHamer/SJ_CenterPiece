# Getting started with Raspberry Pi Pico/Pico W

- Install pico-sdk `yay pico-sdk` or download from [git](https://github.com/raspberrypi/pico-sdk) directly
- Follow the quick start on [git](https://github.com/raspberrypi/pico-sdk#quick-start-your-own-project) or on the [Raspberry Pi website](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- Depending on the install, you may have automatically installed, but if not, you'll need to install the following packages `arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib cmake arm-none-eabi-gdb` though they're named a bit differently on Debian, it seems like in general the `arm-none-eabi` string comes last instead of first.
- Set the environment variable `PICO_SDK_PATH` to point toward where the SDK was installed in your system. You likely want to add a line like `export PICO_SDK_PATH=/usr/share/pico-sdk` to your `~/.bashrc` file. This assumes the install location from the AUR package, adjust accordingly if you installed from a different source.
- You might want to install the pico-examples [git repo](https://github.com/raspberrypi/pico-examples) with the command `git clone https://github.com/raspberrypi/pico-examples.git`
- To build the pico-examples project, `cd pico-examples` to get into the new project folder, note that the required cmake files are already present, then run the following commands to make a build directory and then build inside of it. You may want to use the alternate cmake command to define a board and indicate building for a board other than the default `pico`, for example, the `pico_w` or another RP2040 board using the board names defined in `pico-sdk/src/boards/include/boards/`.
```bash
$ mkdir build
$ cd build
$ cmake ..
$ #cmake -DPICO_BOARD=pico_w ..
```
    - CMake flags of interest include, they can be placed in the cmake command before the `..`
        - `PICO_BOARD` can be `pico`, `pico_w`, and other RP2040 boards
        - `CMAKE_BUILD_TYPE` can be `Release` or `Debug`
        - `WIFI_SSID` and `WIFI_PASSWORD` when building for the Pico W
- Build all projects with `make` or a specific project by first entering a subdirectory and then running `make` inside of there, for example `cd blink && make`
- Load the `.elf` file via a debugger or drag-and-drop the `.uf2` over USB.
    - On the final board, you may need to swap a jumper to ensure the system is powered over USB rather than the batteries.
    - You'll first need to get the Pico into it's bootloader, hold down the white button on the Pico while you plug it in via USB and you should see a new device, like `/dev/sdf1`, appear on your system.
    - To drag-and-drop via USB, you'll first mount the RPi Pico as a USB mass storage device, for example, on Manjaro you can use `udiskctl mount -b /dev/sdf1` where you'll replace `sdf1` with the newest device plugged in according to `sudo dmesg`, note the mountpoint provided by udiskctl. Then copy your uf2 file to onto the root of USB device. After copying the file, the Pico will reboot and start running the code. The reboot should also unmount your USB device.

## Serial Comms
- When the project is built with DEBUG flags set, there will be logging transmitted over the serial port that autopopulates on the Pico USB port
- To connect to the Pico's USB port you can use a tool like PuTTY or Moserial, you're looking for a new device that showed up when you plugged in the Pico, for example, `/dev/tty/ACM0` on Linux or a new COM number on Windows
- You'll want to set the baud rate to 115200, 8 data bits, 1 stop bits, no parity, and use a hardware handshake
