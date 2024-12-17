// changelog.md 

# 1.4.5
## 🛠️ General Improvements
- Added starting logs to capture commands - @jaylikesbunda  
- Improved WiFi connection logic - @jaylikesbunda  
- Removed Wi-Fi warnings and color codes for cleaner logs - @jaylikesbunda  
- Added support for variable display timeout on TWatch S3 - @jaylikesbunda 
- Revise stop command callbacks to be more consistent - @jaylikesbunda, @Spooks4576
- Miscellaneous fixes and improvements - @jaylikesbunda, @Spooks4576  

## 🌐 Network and Communication Features
- Enhanced Deauth Attack with bidirectional frames, proper 802.11 sequencing, and rate limiting (thank you @SpacehuhnTech for amazing reference code) - @jaylikesbunda  
- Added BLE Packet Capture support - @jaylikesbunda  
- Added BLE Wardriving - @jaylikesbunda  
- Added support for detecting and capturing packets from card skimmers - @jaylikesbunda  
- Added "gpsinfo" command to retrieve and display GPS information - @jaylikesbunda 

## 🖥️ User Interface Updates
- Added more terminal view logs - @jaylikesbunda, @Spooks4576  
- WebUI fixes for better functionality - @Spooks4576
- Better access for shared lvgl thread for panels where other work needs to be performed - @i-am-shodan
- Revised the WebUI styling to be more consistent with GhostESP.net - @jaylikesbunda
- Fixed GPS buffer overflow issue that could cause logging to stop - @jaylikesbunda
- Improved UART buffer handling to prevent task crashes in terminal view - @jaylikesbunda
- Terminal View trunication and cleanup to prevent overflow - @jaylikesbunda
- Terminal View scrolling improvements - @jaylikesbunda

## Non Firmware Changes
- New https://ghostesp.net website! - @jaylikesbunda
- Ghost ESP Flipper App v1.1.8 - @jaylikesbunda
- Cleanup README.md - @jaylikesbunda



...changelog starts here... 

