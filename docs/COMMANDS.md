# Ghost ESP Commands

## General Commands

- **`help`**  
  **Description:** Display this help message.  
  **Usage:** `help`

- **`scanap`**  
  **Description:** Start a Wi-Fi access point (AP) scan.  
  **Usage:** `scanap`

- **`scansta`**  
  **Description:** Start scanning for Wi-Fi stations.  
  **Usage:** `scansta`

- **`stopscan`**  
  **Description:** Stop any ongoing Wi-Fi scan.  
  **Usage:** `stopscan`

- **`list`**  
  **Description:** List Wi-Fi scan results or connected stations.  
  **Usage:** `list -a | list -s`  
  **Arguments:**  
    - `-a`: Show access points from Wi-Fi scan  
    - `-s`: List connected stations

## Attack Commands

- **`attack`**  
  **Description:** Launch an attack (e.g., deauthentication attack).  
  **Usage:** `attack -d`  
  **Arguments:**  
    - `-d`: Start deauth attack

- **`beaconspam`**  
  **Description:** Start beacon spam with different modes.  
  **Usage:** `beaconspam [OPTION]`  
  **Arguments:**  
    - `-r`: Start random beacon spam  
    - `-rr`: Start Rickroll beacon spam  
    - `-l`: Start AP List beacon spam  
    - `[SSID]`: Use specified SSID for beacon spam

- **`stopspam`**  
  **Description:** Stop ongoing beacon spam.  
  **Usage:** `stopspam`

- **`stopdeauth`**  
  **Description:** Stop ongoing deauthentication attack.  
  **Usage:** `stopdeauth`

## Selection Commands

- **`select`**  
  **Description:** Select an access point by index from the scan results.  
  **Usage:** `select -a <number>`  
  **Arguments:**  
    - `-a`: AP selection index (must be a valid number)

## Settings Commands

- **`setsetting`**  
  **Description:** Set various device settings.  
  **Usage:** `setsetting <index> <value>`  
  **Arguments:**  
    - `<index>`: Setting index (1: RGB mode, 2: Channel switch delay, 3: Channel hopping, 4: Random BLE MAC)  
    - `<value>`: Value corresponding to the setting (varies by setting index)

### RGB Mode Values
- `1`: Stealth Mode  
- `2`: Normal Mode  
- `3`: Rainbow Mode

### Channel Switch Delay Values
- `1`: 0.5s  
- `2`: 1s  
- `3`: 2s  
- `4`: 3s  
- `5`: 4s

### Channel Hopping Values
- `1`: Disabled  
- `2`: Enabled

### Random BLE MAC Values
- `1`: Disabled  
- `2`: Enabled

## Evil Portal Commands

- **`startportal`**  
  **Description:** Start a portal with specified SSID and password.  
  **Usage:** `startportal <URL> <SSID> <Password> <AP_ssid>`  
  **Arguments:**  
    - `<URL>`: URL for the portal  
    - `<SSID>`: Wi-Fi SSID for the portal  
    - `<Password>`: Wi-Fi password for the portal  
    - `<AP_ssid>`: SSID for the access point  
    - `<Domain>`: Custom Domain to spoof in the address bar

- **`stopportal`**  
  **Description:** Stop the Evil Portal.  
  **Usage:** `stopportal`

## Capture Commands

- **`capture`**  
  **Description:** Start a Wi-Fi capture (Requires SD Card or Flipper).  
  **Usage:** `capture [OPTION]`  
  **Arguments:**  
    - `-probe`: Start capturing probe packets  
    - `-beacon`: Start capturing beacon packets  
    - `-deauth`: Start capturing deauth packets  
    - `-raw`: Start capturing raw packets  
    - `-wps`: Start capturing WPS packets and their auth type  
    - `-stop`: Stop the active capture

## Bluetooth (BLE) Commands (If BLE is enabled)

- **`blescan`**  
  **Description:** Handle BLE scanning with various modes.  
  **Usage:** `blescan [OPTION]`  
  **Arguments:**  
    - `-f`: Start "Find the Flippers" mode  
    - `-ds`: Start BLE spam detector  
    - `-a`: Start AirTag scanner  
    - `-r`: Scan for raw BLE packets  
    - `-s`: Stop BLE scanning

## New Network Commands

- **`connect`**  
  **Description:** Connects to a specific Wi-Fi network.  
  **Usage:** `connect <SSID> <Password>`

- **`dialconnect`**  
  **Description:** Cast a random YouTube video on all smart TVs on your LAN (Requires connection via `connect`).  
  **Usage:** `dialconnect`

- **`powerprinter`**  
  **Description:** Print custom text to a printer on your LAN (Requires connection via `connect`).  
  **Usage:** `powerprinter <Printer IP> <Text> <FontSize> <Alignment>`  
  **Arguments:**  
    - **`Alignment` Options:**  
      - `CM`: Center Middle  
      - `TL`: Top Left  
      - `TR`: Top Right  
      - `BR`: Bottom Right  
      - `BL`: Bottom Left
# Ghost ESP Commands

## General Commands

- **`help`**  
  **Description:** Display this help message.  
  **Usage:** `help`

- **`scanap`**  
  **Description:** Start a Wi-Fi access point (AP) scan.  
  **Usage:** `scanap`

- **`scansta`**  
  **Description:** Start scanning for Wi-Fi stations.  
  **Usage:** `scansta`

- **`stopscan`**  
  **Description:** Stop any ongoing Wi-Fi scan.  
  **Usage:** `stopscan`

- **`list`**  
  **Description:** List Wi-Fi scan results or connected stations.  
  **Usage:** `list -a | list -s`  
  **Arguments:**  
    - `-a`: Show access points from Wi-Fi scan  
    - `-s`: List connected stations

## Attack Commands

- **`attack`**  
  **Description:** Launch an attack (e.g., deauthentication attack).  
  **Usage:** `attack -d`  
  **Arguments:**  
    - `-d`: Start deauth attack

- **`beaconspam`**  
  **Description:** Start beacon spam with different modes.  
  **Usage:** `beaconspam [OPTION]`  
  **Arguments:**  
    - `-r`: Start random beacon spam  
    - `-rr`: Start Rickroll beacon spam  
    - `-l`: Start AP List beacon spam  
    - `[SSID]`: Use specified SSID for beacon spam

- **`stopspam`**  
  **Description:** Stop ongoing beacon spam.  
  **Usage:** `stopspam`

- **`stopdeauth`**  
  **Description:** Stop ongoing deauthentication attack.  
  **Usage:** `stopdeauth`

## Selection Commands

- **`select`**  
  **Description:** Select an access point by index from the scan results.  
  **Usage:** `select -a <number>`  
  **Arguments:**  
    - `-a`: AP selection index (must be a valid number)

## Settings Commands

- **`setsetting`**  
  **Description:** Set various device settings.  
  **Usage:** `setsetting <index> <value>`  
  **Arguments:**  
    - `<index>`: Setting index (1: RGB mode, 2: Channel switch delay, 3: Channel hopping, 4: Random BLE MAC)  
    - `<value>`: Value corresponding to the setting (varies by setting index)

### RGB Mode Values
- `1`: Stealth Mode  
- `2`: Normal Mode  
- `3`: Rainbow Mode

### Channel Switch Delay Values
- `1`: 0.5s  
- `2`: 1s  
- `3`: 2s  
- `4`: 3s  
- `5`: 4s

### Channel Hopping Values
- `1`: Disabled  
- `2`: Enabled

### Random BLE MAC Values
- `1`: Disabled  
- `2`: Enabled

## Evil Portal Commands

- **`startportal`**  
  **Description:** Start a portal with specified SSID and password.  
  **Usage:** `startportal <URL> <SSID> <Password> <AP_ssid>`  
  **Arguments:**  
    - `<URL>`: URL for the portal  
    - `<SSID>`: Wi-Fi SSID for the portal  
    - `<Password>`: Wi-Fi password for the portal  
    - `<AP_ssid>`: SSID for the access point  
    - `<Domain>`: Custom Domain to spoof in the address bar

- **`stopportal`**  
  **Description:** Stop the Evil Portal.  
  **Usage:** `stopportal`

## Capture Commands

- **`capture`**  
  **Description:** Start a Wi-Fi capture (Requires SD Card or Flipper).  
  **Usage:** `capture [OPTION]`  
  **Arguments:**  
    - `-probe`: Start capturing probe packets  
    - `-beacon`: Start capturing beacon packets  
    - `-deauth`: Start capturing deauth packets  
    - `-raw`: Start capturing raw packets  
    - `-wps`: Start capturing WPS packets and their auth type  
    - `-stop`: Stop the active capture

## Bluetooth (BLE) Commands (If BLE is enabled)

- **`blescan`**  
  **Description:** Handle BLE scanning with various modes.  
  **Usage:** `blescan [OPTION]`  
  **Arguments:**  
    - `-f`: Start "Find the Flippers" mode  
    - `-ds`: Start BLE spam detector  
    - `-a`: Start AirTag scanner  
    - `-r`: Scan for raw BLE packets  
    - `-s`: Stop BLE scanning

## Network Commands

- **`connect`**  
  **Description:** Connects to a specific Wi-Fi network.  
  **Usage:** `connect <SSID> <Password>`

- **`dialconnect`**  
  **Description:** Cast a random YouTube video on all smart TVs on your LAN (Requires connection via `connect`).  
  **Usage:** `dialconnect`

- **`powerprinter`**  
  **Description:** Print custom text to a printer on your LAN (Requires connection via `connect`).  
  **Usage:** `powerprinter <Printer IP> <Text> <FontSize> <Alignment>`  
  **Arguments:**  
    - **`Alignment` Options:**  
      - `CM`: Center Middle  
      - `TL`: Top Left  
      - `TR`: Top Right  
      - `BR`: Bottom Right  
      - `BL`: Bottom Left
