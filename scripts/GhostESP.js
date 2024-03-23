let submenu = require("submenu");
let serial = require("serial");
let keyboard = require("keyboard");
let textbox = require("textbox");
let dialog = require("dialog");

serial.setup("usart", 115200);

let shouldexit = false;

function sendSerialCommand(command, menutype) {
    serial.write(command);
    receiveSerialData(menutype);
}

function receiveSerialData(menutype) {
    textbox.setConfig("end", "text");
    textbox.show();
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
    submenu.addItem("BLE Spam", 1);
    submenu.addItem("LED Utils", 2);

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

    if (result === undefined) {
        shouldexit = true;
    }
}

function wifiUtilsMenu() {
    submenu.setHeader("Wifi Utilities:");
    submenu.addItem("Scan Wifi", 0);
    submenu.addItem("Scan Stations", 1);
    submenu.addItem("Add SSID Random", 2);
    submenu.addItem("Add SSID", 3);
    submenu.addItem("List AP", 4);
    submenu.addItem("List Stations", 5);
    submenu.addItem("Select AP", 6);
    submenu.addItem("Select Station", 7);
    submenu.addItem("Beacon Spam SSID List", 8);
    submenu.addItem("Beacon Spam Random", 9);
    submenu.addItem("Beacon Spam Rickroll", 10);
    submenu.addItem("Cast V2 Connect", 11);
    submenu.addItem("Dial Connect", 12);
    submenu.addItem("Deauth Detector", 13);

    let result = submenu.show();

    if (result === 0) {
        sendSerialCommand('scanap', 1);
    }

    if (result === 1) {
        sendSerialCommand('scansta', 1);
    }

    if (result === 2) {
        sendSerialCommand('ssid -a -g', 1);
    }

    if (result === 3) {
        let ssid = promptForText("Enter SSID");
        sendSerialCommand('ssid -a -n ' + ssid, 1);
    }

    if (result === 4) {
        sendSerialCommand('list -a', 1);
    }

    if (result === 5) {
        sendSerialCommand('list -c', 1);
    }

    if (result === 6) {
        validateAndSelectAPStation("AP", "select -a", 1);
    }

    if (result === 7) {
        validateAndSelectAPStation("Station", "select -s", 1);
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

    if (result === undefined)
    {
        mainMenu();
    }
}

function bleSpamMenu() {
    submenu.setHeader("BLE Spam Options:");
    submenu.addItem("Samsung Spam", 0);
    submenu.addItem("Apple Spam", 1);
    submenu.addItem("Google Spam", 2);
    submenu.addItem("Windows Spam", 3);
    submenu.addItem("All", 4);

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


mainLoop();