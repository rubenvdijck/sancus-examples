# Sancus Event Manager

**DISCLAIMER:** This application was developed for an outdated Sancus' RIOT OS port. Therefore, it does not work as is on the [most recent version](https://github.com/sancus-tee/sancus-riot) but it requires some changes.

## TODO

- Either use the latest `sancus-sim`, or create a `.mcs` Sancus image using the latest [sancus-core](https://github.com/sancus-tee/sancus-core) code
- Add this folder as an application to the latest `sancus-riot` code
- Make the modifications required to make it compile (especially on OS features such as timers, UART communication, threads, etc.)
- Create the ELF file
    - if SMs are embedded into the ELF, make sure you compile the code using the most recent [sancus-compiler](https://github.com/sancus-tee/sancus-compiler) and [sancus-support](https://github.com/sancus-tee/sancus-support).
        - Also update [reactive-base](https://github.com/AuthenticExecution/reactive-base) using these repositories instead of the "patched" ones (check [here](https://github.com/AuthenticExecution/reactive-base/blob/main/scripts/install_sancus.sh)).
- Make sure everything works. If something does not work properly for unexplicable reasons (especially regarding the SMs), it is recommended to use `sancus-sim` and `gtkwave`.

## Features

### Secure I/O on LED and button

`pmodled.{c,h}` and `pmodbtn.{c,h}` declare two SMs for Secure I/O on an LED and a button respectively.

- Both can be enabled by setting the corresponding parameter in the Makefile to 1
- The actual addresses of the MMIO region and the registers might be different
    - check the msp430 manual and the physical position of the I/O devices
- The button currently only works by polling the device periodically. For a better use, secure interrupts could be implemented.

### Networking

Data to/from the Sancus device was exchanged via UART, a serial communication media in which both uplink and downlink are on the same physical cable. For this reason, `networking.{c,h}` (as well as [reactive-uart2ip](https://github.com/AuthenticExecution/reactive-uart2ip)) use a lot of tricks and hacks to send/receive data correctly.

- These two files could be hugely simplified in different conditions (e.g., separate uplink/downlink, or even a wireless media)

### Threads

We used three threads:
- Main thread: this is the actual event manager, it receives events from the other threads and handles them, eventually calling the SMs
- UART thread: this thread sends/receives data to/from the UART channel. In particular, when a new event arrives, it forwards it to the main thread using a queue
- Periodic event thread: see below (by default, this thread is disabled - you can enable it in the Makefile)

### Periodic events

Periodic events are a feature that allows to call a specific entry point of an SM regularly. This can be for example used to trigger a sensor reading every second or so.
- Periodic events can be defined in the deployment descriptor, afterwards our [reactive-tools](https://github.com/AuthenticExecution/reactive-tools) can automatically initialize them on the Event Manager.
- This feature was disabled by default due to some bugs on the `xtimer` module in the old Riot OS.