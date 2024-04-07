# **How to Flash the Flipper Dev Board**

Flashing the Flipper Dev Board can be accomplished through different methods. This guide will walk you through each method step-by-step. You can choose the method that best suits your setup and preferences.

## **Preparation**

Before starting, ensure you have a file extraction program like 7-Zip installed. If you don't have it, you can download it from [7-Zip's official website](https://www.7-zip.org/download.html).

---

## **Method 1: Connecting through USB**

### **Step 1: Download the Firmware**
- Navigate to the firmware's releases page and download the latest release for your board.
- Use 7-Zip or another file extraction program to extract the `.tar.gz` file.

### **Step 2: Connect the Dev Board**
- Hold the boot button on the dev board (the button closest to the USB port).
- While holding the button, connect it to your PC via a USB-C cable. If you're on Windows, you should hear a sound indicating the device is recognized.

### **Step 3: Flash the Firmware**
- Visit [https://esp.huhn.me/](https://esp.huhn.me/) and click on "Connect".
- Choose the COM port that your board is connected to. It should be labeled as "ESP32-S2".
- For the offset `0x1000`, click "Select" and choose the `bootloader.bin` file.
- Repeat the process for offset `0x8000` with the `partitions.bin` file, and for `0x10000` with the `firmware.bin` file.
- Click "Flash" and wait until the process finishes.

### **Step 4: Completion**
- Once the flashing process is complete, your firmware should be successfully updated.

---

## **Method 2: Flashing through Flipper Zero**

### **Step 1: Download and Extract the Firmware**
- Follow the same downloading and extracting process as mentioned in Method 1.

### **Step 2: Connect the Dev Board to Flipper Zero**
- Connect your dev board to the Flipper Zero's GPIO Header.
- Connect the Flipper Zero to your PC using a USB-C cable.

### **Step 3: Transfer the Firmware Files**
- Download qFlipper from [the qFlipper update page](https://flipperzero.one/update).
- Using qFlipper, transfer all three files (`bootloader`, `firmware`, and `partitions`) to `SDCard/apps_data/esp_flasher`. Ensure they are not placed in the assets folder.

### **Step 4: Flash the Firmware**
- Ensure your dev board is in bootloader mode:
  - Press and hold the boot button.
  - While holding the boot button, press the reset button once.
- On the Flipper Zero, navigate to the ESP flasher and select "Manual Flash".
- Assign each file to its corresponding section: `bootloader file` for bootloader, `partitions file` for partitions, and `firmware file` for firmware.
- Click on "Flash Fast". If an error occurs, ensure your board is in download mode as previously described.

### **Step 5: Completion**
- Wait for the flashing process to finish. Once done, press the reset button to enjoy your newly flashed firmware.

---

This guide aims to provide clear and straightforward steps for flashing the Flipper Dev Board. Whether you're a first-time user or have some experience, following these steps should lead to a successful firmware update.
