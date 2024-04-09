# Description

Library allows to build USB class devices and to make it usable with minimal effort:
- CDC, communication device class,
- MSC, mass storage class,
- HID, human interface device class: keyboard, mouse, gamepad, generic IN/OUT,
- MIDI, musical instrument digital interface class,
- DFU, device firmware update class,
- WebUSB, its using vendor class to show webusb usage.

# Hardware
To use native USB we need to connect pins 19 and 20 to usb cable or with similar connectors:
![](https://ae01.alicdn.com/kf/HTB1MFvqNgHqK1RjSZJnq6zNLpXaR/10-sztuk-Mini-Micro-USB-do-DIP-2-54mm-Adapter-z-cze-modu-u-Panel-kobiet.jpg)
![](https://ae01.alicdn.com/kf/HTB1cfmCgcnI8KJjSspeq6AwIpXa6/AMS1117-3-3V-AMS1117-3-3V-Mini-USB-5V-3-3V-DC-Perfect-Power-Supply-Module.jpg)

# How to
Library allows to set all values in standard USB device like:
- manufacturer
- product name
- serial number
- revision
- VID and PID

```
ANYusb device;  // any USB class like HID, MSC, CDC
device.manufacturer(char*);
device.product(char*); // product name
device.serial(char*);  // serial number SN
device.revision(uint16_t); // product revison
device.deviceID(uint16_t VID, uint16_t PID);
device.deviceID(uint16_t* VID, uint16_t* PID);
```

# Default EP numbers
In case of using more than 1 USB class in 1 device please make sure there is no conflict between EP numbers and eventuallu use `setBaseEP`:
- CDC - EP1 and EP2,
- HID - keyboard - EP2, mouse - EP3,
- MSC - EP4,
- WebUSB - EP4,
- MIDI - EP5
- DFU - not required

# Contributions
Issues and PRs are welcome.

# USB host (WIP)

I have some basic implementation of USB host which i decided to add. This is still experimental version as i dont know what tpe of design to implement (events or callbacks).

Here is log from MSC host reading files from pendrive:

```
[   960][I][test.ino:86] client_event_callback(): device speed: USB_SPEED_FULL, device address: 1, max ep_ctrl size: 64, config: 1
EP num: 1/2, len: 32, address: 0x81, EP max size: 64, dir: IN
EP num: 2/2, len: 32, address: 0x02, EP max size: 64, dir: OUT
capacity_cb: block_size: 512, block_count: 60948479
inquiry_cb
[  1038][I][test.ino:221] write_test(): File written
[  1049][I][test.ino:149] read_test(): /msc/README.txt
[  1051][I][test.ino:159] read_test(): Hello World!
[  1055][I][test.ino:205] read_test(): Found file : /msc/fanet_module.pdf (735686 bytes)
[  1056][I][test.ino:205] read_test(): Found file : /msc/06639_datasheet.pdf (205240 bytes)
[  1063][I][test.ino:205] read_test(): Found file : /msc/Bluetooth_5-FINAL.pdf (2100903 bytes)
[  1072][I][test.ino:205] read_test(): Found file : /msc/USB-Basics-USB-2.0-.pdf (113669 bytes)
[  1080][I][test.ino:205] read_test(): Found file : /msc/DS_SX1280-1_V2.2.book.pdf (197260 bytes)
[  1096][I][test.ino:205] read_test(): Found file : /msc/USB-Basics-USB-2.0- (1).pdf (113669 bytes)
[  1102][I][test.ino:205] read_test(): Found file : /msc/USB---Enumeration-States.pdf (29098 bytes)
[  1110][I][test.ino:205] read_test(): Found file : /msc/USB---Protocol---bits4device.pdf (1022853 bytes)
[  1125][I][test.ino:205] read_test(): Found file : /msc/esp32-s2_technical_reference_manual_en.pdf (6769251 bytes)
[  1132][I][test.ino:205] read_test(): Found file : /msc/cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf (4613315 bytes)
[  1146][I][test.ino:205] read_test(): Found file : /msc/USB Complete The Developer's Guide 4th Ed.pdf (6936935 bytes)
[  1161][I][test.ino:205] read_test(): Found file : /msc/USB Mass Storage Designing and Programming Devices and Embedded Hosts.pdf (3360734 bytes)
[  1173][I][test.ino:205] read_test(): Found file : /msc/README.txt (13 bytes)
[  1181][I][test.ino:233] write_test(): Renaming file
[  1212][I][test.ino:240] write_test(): Reading file
[  1225][I][test.ino:254] write_test(): Read from file /msc/README1.txt: 'Hello World!'
[  1233][E][test.ino:145] read_test(): Failed to open file
[  1237][I][test.ino:205] read_test(): Found file : /msc/fanet_module.pdf (735686 bytes)
[  1238][I][test.ino:205] read_test(): Found file : /msc/06639_datasheet.pdf (205240 bytes)
[  1245][I][test.ino:205] read_test(): Found file : /msc/Bluetooth_5-FINAL.pdf (2100903 bytes)
[  1253][I][test.ino:205] read_test(): Found file : /msc/USB-Basics-USB-2.0-.pdf (113669 bytes)
[  1262][I][test.ino:205] read_test(): Found file : /msc/DS_SX1280-1_V2.2.book.pdf (197260 bytes)
[  1278][I][test.ino:205] read_test(): Found file : /msc/USB-Basics-USB-2.0- (1).pdf (113669 bytes)
[  1284][I][test.ino:205] read_test(): Found file : /msc/USB---Enumeration-States.pdf (29098 bytes)
[  1292][I][test.ino:205] read_test(): Found file : /msc/USB---Protocol---bits4device.pdf (1022853 bytes)
[  1307][I][test.ino:205] read_test(): Found file : /msc/esp32-s2_technical_reference_manual_en.pdf (6769251 bytes)
[  1314][I][test.ino:205] read_test(): Found file : /msc/cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf (4613315 bytes)
[  1327][I][test.ino:205] read_test(): Found file : /msc/USB Complete The Developer's Guide 4th Ed.pdf (6936935 bytes)
[  1342][I][test.ino:205] read_test(): Found file : /msc/USB Mass Storage Designing and Programming Devices and Embedded Hosts.pdf (3360734 bytes)
[  1352][I][test.ino:205] read_test(): Found file : /msc/README1.txt (13 bytes)
[  1352][I][test.ino:289] setup(): storage used: 26329088/31189319680
```
