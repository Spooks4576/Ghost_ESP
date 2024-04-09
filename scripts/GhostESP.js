let submenu = require("submenu");
let serial = require("serial");
let keyboard = require("keyboard");
let textbox = require("textbox");
let dialog = require("dialog");
let storage = require("storage");

serial.setup("usart", 115200);

let shouldexit = false;
let path = "/ext/storage.test";

function sendSerialCommand(command, menutype) {
    serial.write(command);
    receiveSerialData(menutype);
}

function receiveSerialData(menutype) {
    textbox.setConfig("end", "text");
    textbox.show();

    serial.readAny(0);


    while (textbox.isOpen()) {
        let rx_data = serial.readAny(250);
        if (rx_data !== undefined) {
            textbox.addText(rx_data);
        }
    }
    serial.write("stop");

    if (menutype === 0)
    {
        mainMenu();
    }
    if (menutype === 1)
    {
        wifiUtilsMenu();
    }
    if (menutype === 2)
    {
        bleSpamMenu();
    }
    if (menutype === 3)
    {
        ledUtilsMenu();
    }
    if (menutype === 4)
    {
        promptUSBType();
    }
}

function promptForText(header, defaultValue) {
    keyboard.setHeader(header);
    return keyboard.text(100, defaultValue, true);
}

function validateAndSelectAPStation(type, commandPrefix) {
    let input = promptForText("Enter " + to_string(type) + " Index");
    if (validateNumber(input)) {
        sendSerialCommand(commandPrefix + " " + input, 1);
    } else {
        dialog.message("Error", "Invalid number entered.");
    }
}

function validateNumber(input) {
    return true;
}

function mainMenu() {
    submenu.setHeader("Select a utility:");
    submenu.addItem("Wifi Utils", 0);
    submenu.addItem("BLE Utils", 1);
    submenu.addItem("LED Utils", 2);
    submenu.addItem("Evil Portal", 3);
    submenu.addItem("USB Emulation", 4);

    let result = submenu.show();

    if (result === 0) {
        wifiUtilsMenu();
    }

    if (result === 1) {
        bleSpamMenu();
    }

    if (result === 2) {
        ledUtilsMenu();
    }

    if (result === 3)
    {
        sendSerialCommand("evilportal -s KillShot", 1);
    }

    if (result === 4)
    {
        promptUSBType();
    }

    if (result === undefined) {
        shouldexit = true;
    }
}

function wifiUtilsMenu() {
    submenu.setHeader("Wifi Utilities:");
    submenu.addItem("Scan Wifi", 0);
    submenu.addItem("Scan Stations", 1);
    submenu.addItem("List AP", 2);
    submenu.addItem("List Stations", 3);
    submenu.addItem("Select AP", 4);
    submenu.addItem("Select Station", 5);
    submenu.addItem("Add SSID Random", 6);
    submenu.addItem("Add SSID", 7);
    submenu.addItem("Beacon Spam SSID List", 8);
    submenu.addItem("Beacon Spam Random", 9);
    submenu.addItem("Beacon Spam Rickroll", 10);
    submenu.addItem("Cast V2 Connect", 11);
    submenu.addItem("Dial Connect", 12);
    submenu.addItem("Deauth Detector", 13);
    submenu.addItem("Sniff Raw", 14);
    submenu.addItem("Sniff EPOL", 15);
    submenu.addItem("Sniff Probe", 16);
    submenu.addItem("Sniff PWN", 17);
    submenu.addItem("Calibrate", 18);

    let result = submenu.show();

    if (result === 0) {
        sendSerialCommand('scanap', 1);
    }

    if (result === 1) {
        sendSerialCommand('scansta', 1);
    }

    if (result === 2) {
        sendSerialCommand('list -a', 1);
    }

    if (result === 3) {
        sendSerialCommand('list -c', 1);
    }

    if (result === 4) {
        validateAndSelectAPStation("AP", "select -a", 1);
    }

    if (result === 5) {
        validateAndSelectAPStation("Station", "select -s", 1);
    }

    if (result === 6) {
        sendSerialCommand('ssid -a -g', 1);
    }

    if (result === 7) {
        let ssid = promptForText("Enter SSID");
        sendSerialCommand('ssid -a -n ' + ssid, 1);
    }

    if (result === 8) {
        sendSerialCommand('attack -t beacon -l', 1);
    }

    if (result === 9) {
        sendSerialCommand('attack -t beacon -r', 1);
    }

    if (result === 10) {
        sendSerialCommand('attack -t rickroll', 1);
    }

    if (result === 11)
    {
        // Its better if we input this info manually so ill let you guys decide this 
        sendSerialCommand("castv2connect -s SSID -p PASSWORD -v Y7uhkyameuk", 1);
    }

    if (result === 12)
    {
        sendSerialCommand("dialconnect -s SSID -p PASSWORD -t youtube -v Y7uhkyameuk", 1);
    }

    if (result === 13)
    {
        sendSerialCommand("deauthdetector -s SSID -p PASSWORD -w WebHookUrl", 1);
    }

    if (result === 14)
    {
        sendSerialCommand("sniffraw", 1);
    }

    if (result === 15)
    {
        sendSerialCommand("sniffpmkid", 1);
    }

    if (result === 16)
    {
        sendSerialCommand("sniffprobe", 1);
    }

    if (result === 17)
    {
        sendSerialCommand("sniffpwn", 1);
    }

    if (result === 18)
    {
        sendSerialCommand("calibrate", 1);
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function bleSpamMenu() {
    submenu.setHeader("BLE Options:");
    submenu.addItem("Samsung Spam", 0);
    submenu.addItem("Apple Spam", 1);
    submenu.addItem("Google Spam", 2);
    submenu.addItem("Windows Spam", 3);
    submenu.addItem("Kitchen Sink", 4);
    submenu.addItem("Find the Flippers", 5);
    submenu.addItem("BLE Spam Detector", 6);
    submenu.addItem("Sniff BLE", 7);

    let result = submenu.show();

    if (result === 0) {
        sendSerialCommand('blespam -t samsung', 2);
    }

    if (result === 1) {
        sendSerialCommand('blespam -t apple', 2);
    }

    if (result === 2) {
        sendSerialCommand('blespam -t google', 2);
    }

    if (result === 3) {
        sendSerialCommand('blespam -t windows', 2);
    }

    if (result === 4) {
        sendSerialCommand('blespam -t all', 2);
    }

    if (result === 5) {
        sendSerialCommand('findtheflippers', 2);
    }

    if (result === 6)
    {
        sendSerialCommand('detectblespam', 2);
    }

    if (result === 7)
    {
        sendSerialCommand('sniffbt', 2);
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function ledUtilsMenu() {
    submenu.setHeader("LED Utilities:");
    submenu.addItem("Rainbow LED", 0);

    let result = submenu.show();
    if (result === 0) {
        sendSerialCommand('led -p', 3);
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function promptUSBType(){
    submenu.setHeader("Pick USB Type:");
    submenu.addItem("Nintendo Switch", 0);
    submenu.addItem("PS5 / DualSense", 1);
    submenu.addItem("Xbox One", 2);

    let result = submenu.show();
    if (result === 0) {
        promptNSWControls();
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function promptNSWControls(){
    submenu.setHeader("Pick Button Press:");
    submenu.addItem("Button: Y", 0);
    submenu.addItem("Button: B", 1);
    submenu.addItem("Button: A", 2);
    submenu.addItem("Button: X", 3);
    submenu.addItem("Button: -", 4);
    submenu.addItem("Button: +", 5);
    submenu.addItem("Button: Share", 6);
    submenu.addItem("Button: Home", 7);
    submenu.addItem("Button: LB", 8);
    submenu.addItem("Button: RB", 9);
    submenu.addItem("Button: LT", 10);
    submenu.addItem("Button: RT",11);
    submenu.addItem("Button: L-Thumb", 12);
    submenu.addItem("Button: R-Thumb",13);
    submenu.addItem("Button: Up", 14);
    submenu.addItem("Button: Left",15);
    submenu.addItem("Button: Down", 16);
    submenu.addItem("Button: Right",17);
    submenu.addItem("Button: L-Stick-Up", 18);
    submenu.addItem("Button: L-Stick-Left",19);
    submenu.addItem("Button: L-Stick-Down", 20);
    submenu.addItem("Button: L-Stick-Right",21);
    submenu.addItem("Button: R-Stick-Up", 22);
    submenu.addItem("Button: R-Stick-Left",23);
    submenu.addItem("Button: R-Stick-Down", 24);
    submenu.addItem("Button: R-Stick-Right",25);
    submenu.addItem("Choose Script", 26);

    let result = submenu.show();
    if (result === 0){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_Y", 4);
    }
    if (result === 1){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_B", 4);
    }
    if (result === 2){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_A", 4);
    }
    if (result === 3){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_X", 4);
    }
    if (result === 4){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_MINUS", 4);
    }
    if (result === 5){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_PLUS", 4);
    }
    if (result === 6){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_SHARE", 4);
    }
    if (result === 7){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_HOME", 4);
    }
    if (result === 9){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_LBUMPER", 4);
    }
    if (result === 9){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_RBUMPER", 4);
    }
    if (result === 10){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_LTRIGGER", 4);
    }
    if (result === 11){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_RTRIGGER", 4);
    }
    if (result === 12){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_LTHUMBSTICK", 4);
    }
    if (result === 13){
        sendSerialCommand("usbcontrol -t nsw -b BUTTON_RTHUMBSTICK", 4);
    }
    if (result === 14){
        sendSerialCommand("usbcontrol -t nsw -b DPAD_UP", 4);
    }
    if (result === 15){
        sendSerialCommand("usbcontrol -t nsw -b DPAD_LEFT", 4);
    }
    if (result === 16){
        sendSerialCommand("usbcontrol -t nsw -b DPAD_DOWN", 4);
    }
    if (result === 17){
        sendSerialCommand("usbcontrol -t nsw -b DPAD_RIGHT", 4);
    }
    if (result === 18){
        sendSerialCommand("usbcontrol -t nsw -b LSTICK_UP", 4);
    }
    if (result === 19){
        sendSerialCommand("usbcontrol -t nsw -b LSTICK_LEFT", 4);
    }
    if (result === 20){
        sendSerialCommand("usbcontrol -t nsw -b LSTICK_DOWN", 4);
    }
    if (result === 21){
        sendSerialCommand("usbcontrol -t nsw -b LSTICK_RIGHT", 4);
    }
    if (result === 22){
        sendSerialCommand("usbcontrol -t nsw -b RSTICK_UP", 4);
    }
    if (result === 23){
        sendSerialCommand("usbcontrol -t nsw -b RSTICK_LEFT", 4);
    }
    if (result === 24){
        sendSerialCommand("usbcontrol -t nsw -b RSTICK_DOWN", 4);
    }
    if (result === 25){
        sendSerialCommand("usbcontrol -t nsw -b RSTICK_RIGHT", 4);
    }
    if (result === 26)
    {
        let path = dialog.pickFile("/ext", "*");

        let data = storage.read(path);

        sendSerialCommand("usbcontrol -s \n" + arraybuf_to_string(data) + "\n \f", 4);
    }
}

function mainLoop() {
    while (!shouldexit) {
        mainMenu(); // Navigate through the main menu
        let confirm = dialog.message("Exit", "Press OK to exit, Cancel to return.");
        if (confirm === 'OK') {
            sendSerialCommand('stop', 0); // Send a stop command before exiting
            break;
        } else {
            // If the user cancels, it will loop back and show the main menu again.
        }
    }
}

function arraybuf_to_string(arraybuf) {
    let string = "";
    let data_view = Uint8Array(arraybuf);
    for (let i = 0; i < data_view.length; i++) {
        string += chr(data_view[i]);
    }
    return string;
}


mainLoop();