# **How to Flash Firmware on Development Boards**

This guide provides instructions on how to flash firmware onto development boards. The method you choose depends on the type of USB port available on your board (Micro USB or USB-C) or if you prefer using a Flipper Zero device for the flashing process.

## **Preparation**

Before you start, ensure you have a file extraction program like 7-Zip installed for extracting firmware files. You can download 7-Zip from [7-Zip's official website](https://www.7-zip.org/download.html).

---

## **Method 1: USB Connection**

This method is suitable for boards equipped with either a Micro USB or USB-C port and will use the ESP flashing tool website as an example for the flashing process.

### **Step 1: Download and Extract the Firmware**
- Visit the firmware's release page to download the latest firmware version for your board.
- Extract the downloaded `.tar.gz` or `.zip` file using 7-Zip or another extraction tool.

### **Step 2: Connect Your Board**
- Identify the boot button on your board. For Flipper Dev Boards, it's the button closest to the USB port. On other boards, this button should be labeled "Boot" or "Select," but it can vary, especially if your board has a case.
- Enter the board's bootloader mode by holding the identified boot button while connecting the board to your PC via a Micro USB or USB-C cable.

### **Step 3: Flash the Firmware**
- Go to [https://esp.huhn.me/](https://esp.huhn.me/) and click on "Connect".
- Select the COM port that your board is connected to. It should be labeled with your board's chipset, like "ESP32-S2".
- **Important Offsets:**
  - For **ESP32-S2** and similar boards, use the following offsets:
    - `bootloader.bin` at `0x1000`
    - `partitions.bin` at `0x8000`
    - `firmware.bin` at `0x10000`
  - For **ESP32-S3** boards, the bootloader offset changes to:
    - `bootloader.bin` at `0x0`
    - Continue using `0x8000` for `partitions.bin` and `0x10000` for `firmware.bin`.
- Click "Flash" and wait until the process completes.

### **Step 4: Verify the Flash**
- After flashing, disconnect and reconnect your board to ensure the firmware update was successful.

---

## **Method 2: Using Flipper Zero**

If your board lacks a USB port or you prefer using a Flipper Zero, follow these steps:

### **Step 1: Prepare the Firmware Files**
- Download and extract the firmware files for your board as described in Method 1, Step 1.

### **Step 2: Transfer Files to Flipper Zero**
- Ensure qFlipper is installed on your PC. You can download it from [the Flipper Zero update page](https://flipperzero.one/update).
- Connect the Flipper Zero to your PC and use qFlipper to transfer the firmware files to `SDCard/apps_data/esp_flasher` (avoid placing them in the assets folder).

### **Step 3: Connect the Board to Flipper Zero**
- With your board in bootloader mode (use the boot and reset buttons as needed), connect it to the Flipper Zero's GPIO Header.

### **Step 4: Flash the Firmware**
- Navigate to the ESP flasher app on the Flipper Zero and choose "Manual Flash".
- Select the appropriate files for bootloader, partitions, and firmware, then initiate the flash process.
- Once completed, reset your board to finish the firmware update.
