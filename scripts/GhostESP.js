let submenu = require("submenu");
let serial = require("serial");
let keyboard = require("keyboard");
let dialog = require("dialog");

serial.setup("usart", 115200);

let shouldexit = false;

function sendSerialCommand(command) {
    serial.write(command);
    receiveSerialData();
}

function receiveSerialData() {
    let data = '';
    let char = '';
    while (true) {
        let rx_data = serial.readBytes(1, 1000); // Timeout set to 1000 ms
        if (rx_data !== undefined) {
            let data_view = Uint8Array(rx_data);
            char = chr(data_view[0]);
            if (char === '\n')
            {
                print(data);
                data = '';
            }
            data += char;
        } else {
            print("Delaying 2 seconds and Trying again else exiting")
            delay(10000);
            rx_data = serial.readBytes(1, 1000);
            if (rx_data !== undefined) {
                let data_view = Uint8Array(rx_data);
                char = chr(data_view[0]);
                if (char === '\n')
                {
                    print(data);
                    data = '';
                }
                data += char;
            }
            else {
                delay(2000);
                break; // Break if no data received
            }
        }
    }
}

function promptForText(header, defaultValue) {
    keyboard.setHeader(header);
    return keyboard.text(100, defaultValue, true);
}

function validateAndSelectAPStation(type, commandPrefix) {
    let input = promptForText("Enter " + to_string(type) + " Index");
    if (validateNumber(input)) {
        sendSerialCommand(commandPrefix + " " + input);
    } else {
        dialog.message("Error", "Invalid number entered.");
    }
}

function validateNumber(input) {
    return !isNaN(input) && Number.isInteger(parseFloat(input));
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

    let result = submenu.show();

    if (result === 0) {
        sendSerialCommand('scanap');
    }

    if (result === 1) {
        sendSerialCommand('scansta');
    }

    if (result === 2) {
        sendSerialCommand('ssid -a -g');
    }

    if (result === 3) {
        let ssid = promptForText("Enter SSID");
        sendSerialCommand('ssid -a -n ' + ssid);
    }

    if (result === 4) {
        sendSerialCommand('list -a');
    }

    if (result === 5) {
        sendSerialCommand('list -c');
    }

    if (result === 6) {
        validateAndSelectAPStation("AP", "select -a");
    }

    if (result === 7) {
        validateAndSelectAPStation("Station", "select -s");
    }

    if (result === 8) {
        sendSerialCommand('attack beacon -l');
    }

    if (result === 9) {
        sendSerialCommand('attack beacon -r');
    }

    if (result === 10) {
        sendSerialCommand('attack rickroll');
    }

    if (result === 11)
    {
        // Its better if we input this info manually so ill let you guys decide this 
        sendSerialCommand("castv2connect -s SSID -p PASSWORD -v Y7uhkyameuk");
    }

    if (result === 12)
    {
        sendSerialCommand("dialconnect -s SSID -p PASSWORD -t youtube -v Y7uhkyameuk");
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
        sendSerialCommand('blespam -t samsung');
    }

    if (result === 1) {
        sendSerialCommand('blespam -t apple');
    }

    if (result === 2) {
        sendSerialCommand('blespam -t google');
    }

    if (result === 3) {
        sendSerialCommand('blespam -t windows');
    }

    if (result === 4) {
        sendSerialCommand('blespam -t all');
    }
}

function ledUtilsMenu() {
    submenu.setHeader("LED Utilities:");
    submenu.addItem("Rainbow LED", 0);

    let result = submenu.show();
    if (result === 0) {
        sendSerialCommand('led');
    }
}

function mainLoop() {
    while (!shouldexit) {
        mainMenu(); // Navigate through the main menu
        let confirm = dialog.message("Exit", "Press OK to exit, Cancel to return.");
        if (confirm === 'OK') {
            sendSerialCommand('stop'); // Send a stop command before exiting
            break;
        } else {
            // If the user cancels, it will loop back and show the main menu again.
        }
    }
}


mainLoop();