# Description

This is cool example which allows to make USB host to wifi gateway with esp32 S2. It is opposite of USB device with sd card connected to it over SPI.
The idea is to connect for example normal pendrive to esp32 S2 and open files, make dirs or delete files/folders. With some mode effort it will be possible to upload files to pendrive.
At the same time it is possible to perform all normal operations on files from app level using POSIX functions.
In addition there is possibility to setup wifi STA and AP credentials from website.

Maybe it s not very impresive usage case, but with it it is possible to build app that will log everything to pendrive, maybe to add crash dump to pendrive or just device update from pendrive, which can be inserted with new firmware or just uploaded to it and triggered from website update.
I can see more possibilities and i would like to see how others will use it.

# Why posix

It is possible to use only posix functions, because library is not prepared to work like FATFS or SPIFFS libraries included in arduino.
I understand it would be nice to have such integration, but i decided to make this library arduino and esp-idf compatible.
If anyone would like to add arduino-ish implementation of FS then i am open to merge PRs.


# USAGE

With `#define SERVE_WEB_FROM_PENDRIVE 1` (line 25) it is possible to control where web UI files are read from:
1. files embedded into app, which will increase app size
2. files are on pendrive, which not only let to decrease app size, but also allow to customize web UI without re-flashing app
    exmaple files are in embedded folder

