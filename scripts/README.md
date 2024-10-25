# Rave Mode on Ghost ESP - README

Follow these instructions to set up and run Rave Mode on your Ghost ESP with display or LED visualizations. Rave Mode syncs your ESP32's visuals with the audio playing on your computer, creating a dynamic light show!

## Step 1: Connect Your ESP32 to the Internet

Before you can enable Rave Mode, you need to connect your ESP32 to a Wi-Fi network. You can do this using one of the following methods:

### Method 1: Using Serial Command

1. Open your serial terminal and connect to your ESP32.
2. Enter the command:

 ```connect SSID PASSWORD```


Replace `SSID` with your Wi-Fi network name and `PASSWORD` with your network password.

### Method 2: Using the Web UI

1. Connect to the ESP32's access point.
2. Navigate to the Web UI in your browser.
3. Enter your Wi-Fi credentials and connect to the network.

After connecting, verify that your ESP32 has an IP address. You will use this IP address to communicate with the visualizer script.

## Step 2: Choose Your Visualizer Script

Depending on your setup, you can choose between two scripts for Rave Mode:

- **Display_Visualizer.py**: For ESP32 devices with a display.
- **LED_Visualizer.py**: For ESP32 devices without a display (uses LEDs only).

### Step 3: Run the Script

Both scripts should work out of the box. To run a script:

1. Download the script (either `Display_Visualizer.py` or `LED_Visualizer.py`).
2. Run the script using Python:


 ```python Display_Visualizer.py```

  or 

 ```python LED_Visualizer.py```


### Step 4: Adjusting the Multicast Address (if needed)

If the script doesn't work right away, you may need to adjust the multicast or broadcast IP address:

1. Open the script in a text editor.
2. Find the line with the IP address:

```UDP_IP = "192.168.1.105"```

or

```BROADCAST_IP = '192.168.1.255'```


3. Make sure that the IP address matches your network's subnet. For example, if your ESP32's IP is `192.168.2.x`, the address in the script should be `192.168.2.255`.

After ensuring the IP address matches your network's subnet, Rave Mode should be ready to go!

## Step 5: Install Virtual Audio Cable

Rave Mode requires **Virtual Audio Cable** to capture your computer's audio and sync it with your ESP32. Follow these steps to install it:

1. Download **Virtual Audio Cable** from its official website: [https://vb-audio.com/Cable/](https://vb-audio.com/Cable/).
2. Follow the installation instructions provided on the website.
3. Once installed, open the **Sound Control Panel** on your computer (usually found in the Control Panel).

### Step 6: Set Up Audio Playback Through Virtual Audio Cable

To ensure audio is played through both **Virtual Audio Cable** and your speakers:

1. In the **Sound Control Panel**, go to the **Playback** tab.
2. Set your regular speakers as the default playback device.
3. Select **CABLE Input (Virtual Audio Cable)** and click **Properties**.
4. Go to the **Listen** tab.
5. Check the box **Listen to this device** and select your speakers as the playback device.
6. Click **Apply** and **OK**.

This setup allows the audio to be captured by the Virtual Audio Cable while playing through your speakers.

### Step 7: Enjoy Rave Mode!

With everything set up, your ESP32 should now synchronize its display or LEDs with the audio playing on your computer. Sit back and enjoy the light show!

---

If you encounter any issues, double-check your Wi-Fi connection, ensure that the IP addresses match your network, and verify that Virtual Audio Cable is set up correctly. Happy raving! 🎶
