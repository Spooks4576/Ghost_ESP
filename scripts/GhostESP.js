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

    if (menutype === 5)
    {
        GpsUtilsMenu();
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
    return input !== undefined;
}

function mainMenu() {
    submenu.setHeader("Select a utility:");
    submenu.addItem("Wifi Utils", 0);
    submenu.addItem("BLE Utils", 1);
    submenu.addItem("LED Utils", 2);
    submenu.addItem("GPS Utils", 3);

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
        GpsUtilsMenu();
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
    submenu.addItem("Deauth Stations", 14);
    submenu.addItem("Sniff Raw", 15);
    submenu.addItem("Sniff EPOL", 16);
    submenu.addItem("Sniff Probe", 17);
    submenu.addItem("Sniff PWN", 18);
    submenu.addItem("Calibrate", 19);
    
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
        sendSerialCommand("attack -t deauth", 1);
    }

    if (result === 15)
    {
        sendSerialCommand("sniffraw", 1);
    }

    if (result === 16)
    {
        sendSerialCommand("sniffpmkid", 1);
    }

    if (result === 17)
    {
        sendSerialCommand("sniffprobe", 1);
    }

    if (result === 18)
    {
        sendSerialCommand("sniffpwn", 1);
    }

    if (result === 19)
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
    submenu.addItem("Scan For Airtags", 7);
    submenu.addItem("Sniff BLE", 8);

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
        sendSerialCommand('airtagscan', 2);
    }

    if (result === 8)
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

function GpsUtilsMenu()
{
    submenu.setHeader("GPS Utils");
    submenu.addItem("Street Detector", 0);
    submenu.addItem("Wardrive", 1);
    let result = submenu.show();

    if (result === 0)
    {
        sendSerialCommand("streetdetector", 5);
    }

    if (result === 1)
    {
        sendSerialCommand("wardrive", 5);
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function promptUSBType(){
    submenu.setHeader("Pick USB Type:");
    submenu.addItem("Nintendo Switch", 0);
    submenu.addItem("Playstation", 1)
    //submenu.addItem("Xbox One", 1);
    submenu.addItem("Choose Script", 2);

    let result = submenu.show();
    if (result === 0) {
        promptNSWControls();
    }

    if (result === 1)
    {
        promptplaystationControls();
    }

    if (result === 2)
    {
        let path = dialog.pickFile("/ext", "*");

        let data = storage.read(path);

        sendSerialCommand("controller -s \n" + arraybuf_to_string(data) + "\n \f", 4);
    }

    if (result === 3)
    {
        
    }

    if (result === 4)
    {
        
    }

    if (result === undefined)
    {
        mainMenu();
    }
}

function promptXInputControls()
{
    submenu.setHeader("Pick Button Press:");
    submenu.addItem("Button: Y", 0);
    submenu.addItem("Button: B", 1);
    submenu.addItem("Button: A", 2);
    submenu.addItem("Button: X", 3);
    submenu.addItem("Button: Back", 4);
    submenu.addItem("Button: Start", 5);
    submenu.addItem("Button: Home", 6);
    submenu.addItem("Button: LB", 7);
    submenu.addItem("Button: RB", 8);
    submenu.addItem("Button: LT", 9);
    submenu.addItem("Button: RT",10);
    submenu.addItem("Button: L-Thumb", 11);
    submenu.addItem("Button: R-Thumb",12);
    submenu.addItem("Button: Up", 13);
    submenu.addItem("Button: Left",14);
    submenu.addItem("Button: Down", 15);
    submenu.addItem("Button: Right",16);

    let result = submenu.show();
    if (result === 0){
        sendSerialCommand("controller -t xinput -b BUTTON_Y", 4);
    }
    if (result === 1){
        sendSerialCommand("controller -t xinput -b BUTTON_B", 4);
    }
    if (result === 2){
        sendSerialCommand("controller -t xinput -b BUTTON_A", 4);
    }
    if (result === 3){
        sendSerialCommand("controller -t xinput -b BUTTON_X", 4);
    }
    if (result === 4){
        sendSerialCommand("controller -t xinput -b BUTTON_BACK", 4);
    }
    if (result === 5){
        sendSerialCommand("controller -t xinput -b BUTTON_START", 4);
    }
    if (result === 6){
        sendSerialCommand("controller -t xinput -b BUTTON_HOME", 4);
    }
    if (result === 7){
        sendSerialCommand("controller -t xinput -b BUTTON_LBUMPER", 4);
    }
    if (result === 8){
        sendSerialCommand("controller -t xinput -b BUTTON_RBUMPER", 4);
    }
    if (result === 9){
        sendSerialCommand("controller -t xinput -b BUTTON_LTRIGGER", 4);
    }
    if (result === 10){
        sendSerialCommand("controller -t xinput -b BUTTON_RTRIGGER", 4);
    }
    if (result === 11){
        sendSerialCommand("controller -t xinput -b BUTTON_LTHUMBSTICK", 4);
    }
    if (result === 12){
        sendSerialCommand("controller -t xinput -b BUTTON_RTHUMBSTICK", 4);
    }
    if (result === 13){
        sendSerialCommand("controller -t xinput -b DPAD_UP" , 4);
    }
    if (result === 14){
        sendSerialCommand("controller -t xinput -b DPAD_LEFT ", 4);
    }
    if (result === 15){
        sendSerialCommand("controller -t xinput -b DPAD_DOWN", 4);
    }
    if (result === 16){
        sendSerialCommand("controller -t xinput -b DPAD_RIGHT", 4);
    }
}

function promptplaystationControls()
{
    submenu.setHeader("Pick Button Press:");
    submenu.addItem("Button: Triangle", 0);
    submenu.addItem("Button: Circle", 1);
    submenu.addItem("Button: Cross", 2);
    submenu.addItem("Button: Square", 3);
    submenu.addItem("Button: Back", 4);
    submenu.addItem("Button: Start", 5);
    submenu.addItem("Button: PS", 6);
    submenu.addItem("Button: LB", 7);
    submenu.addItem("Button: RB", 8);
    submenu.addItem("Button: LT", 9);
    submenu.addItem("Button: RT",10);
    submenu.addItem("Button: L-Thumb", 11);
    submenu.addItem("Button: R-Thumb",12);
    submenu.addItem("Button: Up", 13);
    submenu.addItem("Button: Left",14);
    submenu.addItem("Button: Down", 15);
    submenu.addItem("Button: Right",16);

    let result = submenu.show();
    if (result === 0){
        sendSerialCommand("controller -t playstation -b BUTTON_Y", 4);
    }
    if (result === 1){
        sendSerialCommand("controller -t playstation -b BUTTON_B", 4);
    }
    if (result === 2){
        sendSerialCommand("controller -t playstation -b BUTTON_A", 4);
    }
    if (result === 3){
        sendSerialCommand("controller -t playstation -b BUTTON_X", 4);
    }
    if (result === 4){
        sendSerialCommand("controller -t playstation -b BUTTON_BACK", 4);
    }
    if (result === 5){
        sendSerialCommand("controller -t playstation -b BUTTON_START", 4);
    }
    if (result === 6){
        sendSerialCommand("controller -t playstation -b BUTTON_HOME", 4);
    }
    if (result === 7){
        sendSerialCommand("controller -t playstation -b BUTTON_LBUMPER", 4);
    }
    if (result === 8){
        sendSerialCommand("controller -t playstation -b BUTTON_RBUMPER", 4);
    }
    if (result === 9){
        sendSerialCommand("controller -t playstation -b BUTTON_LTRIGGER", 4);
    }
    if (result === 10){
        sendSerialCommand("controller -t playstation -b BUTTON_RTRIGGER", 4);
    }
    if (result === 11){
        sendSerialCommand("controller -t playstation -b BUTTON_LTHUMBSTICK", 4);
    }
    if (result === 12){
        sendSerialCommand("controller -t playstation -b BUTTON_RTHUMBSTICK", 4);
    }
    if (result === 13){
        sendSerialCommand("controller -t playstation -b DPAD_UP" , 4);
    }
    if (result === 14){
        sendSerialCommand("controller -t playstation -b DPAD_LEFT ", 4);
    }
    if (result === 15){
        sendSerialCommand("controller -t playstation -b DPAD_DOWN", 4);
    }
    if (result === 16){
        sendSerialCommand("controller -t playstation -b DPAD_RIGHT", 4);
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

    let result = submenu.show();
    if (result === 0){
        sendSerialCommand("controller -t nsw -b BUTTON_Y", 4);
    }
    if (result === 1){
        sendSerialCommand("controller -t nsw -b BUTTON_B", 4);
    }
    if (result === 2){
        sendSerialCommand("controller -t nsw -b BUTTON_A", 4);
    }
    if (result === 3){
        sendSerialCommand("controller -t nsw -b BUTTON_X", 4);
    }
    if (result === 4){
        sendSerialCommand("controller -t nsw -b BUTTON_MINUS", 4);
    }
    if (result === 5){
        sendSerialCommand("controller -t nsw -b BUTTON_PLUS", 4);
    }
    if (result === 6){
        sendSerialCommand("controller -t nsw -b BUTTON_SHARE", 4);
    }
    if (result === 7){
        sendSerialCommand("controller -t nsw -b BUTTON_HOME", 4);
    }
    if (result === 9){
        sendSerialCommand("controller -t nsw -b BUTTON_LBUMPER", 4);
    }
    if (result === 9){
        sendSerialCommand("controller -t nsw -b BUTTON_RBUMPER", 4);
    }
    if (result === 10){
        sendSerialCommand("controller -t nsw -b BUTTON_LTRIGGER", 4);
    }
    if (result === 11){
        sendSerialCommand("controller -t nsw -b BUTTON_RTRIGGER", 4);
    }
    if (result === 12){
        sendSerialCommand("controller -t nsw -b BUTTON_LTHUMBSTICK", 4);
    }
    if (result === 13){
        sendSerialCommand("controller -t nsw -b BUTTON_RTHUMBSTICK", 4);
    }
    if (result === 14){
        sendSerialCommand("controller -t nsw -b DPAD_UP", 4);
    }
    if (result === 15){
        sendSerialCommand("controller -t nsw -b DPAD_LEFT", 4);
    }
    if (result === 16){
        sendSerialCommand("controller -t nsw -b DPAD_DOWN", 4);
    }
    if (result === 17){
        sendSerialCommand("controller -t nsw -b DPAD_RIGHT", 4);
    }
    if (result === 18){
        sendSerialCommand("controller -t nsw -b LSTICK_UP", 4);
    }
    if (result === 19){
        sendSerialCommand("controller -t nsw -b LSTICK_LEFT", 4);
    }
    if (result === 20){
        sendSerialCommand("controller -t nsw -b LSTICK_DOWN", 4);
    }
    if (result === 21){
        sendSerialCommand("controller -t nsw -b LSTICK_RIGHT", 4);
    }
    if (result === 22){
        sendSerialCommand("controller -t nsw -b RSTICK_UP", 4);
    }
    if (result === 23){
        sendSerialCommand("controller -t nsw -b RSTICK_LEFT", 4);
    }
    if (result === 24){
        sendSerialCommand("controller -t nsw -b RSTICK_DOWN", 4);
    }
    if (result === 25){
        sendSerialCommand("controller -t nsw -b RSTICK_RIGHT", 4);
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