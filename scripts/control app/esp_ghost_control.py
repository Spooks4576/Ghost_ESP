import sys
import json
import queue
import serial
import threading
from datetime import datetime
from serial.tools import list_ports
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QComboBox, QPushButton, QLabel, QTextEdit,
                             QTabWidget, QGroupBox, QGridLayout, QLineEdit, QMessageBox,
                             QSplitter, QInputDialog, QSpinBox, QFormLayout)
from PyQt6.QtCore import Qt, pyqtSignal, QThread
from PyQt6.QtGui import QFont, QTextCursor, QPalette, QColor
from functools import partial

class SerialMonitorThread(QThread):
    data_received = pyqtSignal(str)

    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port
        self.running = True

    def run(self):
        while self.running and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting:
                    data = self.serial_port.readline().decode().strip()
                    if data:
                        self.data_received.emit(data)
            except Exception as e:
                self.data_received.emit(f"Error reading serial: {str(e)}")
                break
            self.msleep(10)

    def stop(self):
        self.running = False

class ESP32ControlGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Ghost ESP Control Panel")
        self.setGeometry(100, 100, 1400, 900)

        # Initialize serial communication variables
        self.serial_port = None
        self.monitor_thread = None

        # Set dark theme
        self.setup_dark_theme()

        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # Create main layout for central widget
        main_layout = QVBoxLayout(central_widget)

        # Set up the main layout
        self.setup_ui(main_layout)

        # Refresh available ports
        self.refresh_ports()

    def setup_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.WindowText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Base, QColor(25, 25, 25))
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.ToolTipBase, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.ToolTipText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Text, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Button, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.ButtonText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.BrightText, Qt.GlobalColor.red)
        palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
        palette.setColor(QPalette.ColorRole.Highlight, QColor(42, 130, 218))
        palette.setColor(QPalette.ColorRole.HighlightedText, Qt.GlobalColor.black)
        self.setPalette(palette)

    def setup_ui(self, main_layout):
        # Create top bar for serial connection
        self.setup_connection_bar(main_layout)

        # Create splitter for resizable sections
        splitter = QSplitter(Qt.Orientation.Vertical)
        main_layout.addWidget(splitter)

        # Create main content area
        content_widget = QWidget()
        content_layout = QHBoxLayout(content_widget)
        splitter.addWidget(content_widget)

        # Left side - Command panels
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        self.setup_command_tabs(left_layout)
        content_layout.addWidget(left_widget)

        # Right side - Display area
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        self.setup_display_area(right_layout)
        content_layout.addWidget(right_widget)

        # Set content layout stretch factors
        content_layout.setStretch(0, 1)  # Left side
        content_layout.setStretch(1, 1)  # Right side

        # Bottom - Log area
        self.setup_log_area()
        splitter.addWidget(self.log_group)

        # Set splitter stretch factors
        splitter.setStretchFactor(0, 3)  # Content area
        splitter.setStretchFactor(1, 1)  # Log area

    def setup_connection_bar(self, main_layout):
        connection_group = QGroupBox("Serial Connection")
        connection_layout = QHBoxLayout(connection_group)

        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        connection_layout.addWidget(QLabel("Port:"))
        connection_layout.addWidget(self.port_combo)

        refresh_btn = QPushButton("Refresh Ports")
        refresh_btn.clicked.connect(self.refresh_ports)
        refresh_btn.setFixedWidth(100)
        connection_layout.addWidget(refresh_btn)

        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        self.connect_btn.setFixedWidth(100)
        connection_layout.addWidget(self.connect_btn)

        connection_layout.addStretch()
        main_layout.addWidget(connection_group)

    def setup_command_tabs(self, layout):
        self.tab_widget = QTabWidget()

        # WiFi Operations Tab
        self.tab_widget.addTab(self.create_wifi_tab(), "WiFi Operations")

        # Network Operations Tab
        self.tab_widget.addTab(self.create_network_tab(), "Network Operations")

        # BLE Operations Tab
        self.tab_widget.addTab(self.create_ble_tab(), "BLE Operations")

        # Capture Operations Tab
        self.tab_widget.addTab(self.create_capture_tab(), "Capture Operations")

        # Evil Portal Tab
        self.tab_widget.addTab(self.create_evil_portal_tab(), "Evil Portal")

        # Settings Tab
        self.tab_widget.addTab(self.create_settings_tab(), "Settings")

        # Custom Command Area
        custom_group = QGroupBox("Custom Command")
        custom_layout = QVBoxLayout(custom_group)

        # Custom command input
        self.cmd_entry = QLineEdit()
        self.cmd_entry.setPlaceholderText("Enter custom command...")
        self.cmd_entry.returnPressed.connect(self.send_custom_command)
        custom_layout.addWidget(self.cmd_entry)

        # Send Command button
        send_btn = QPushButton("Send Command")
        send_btn.clicked.connect(self.send_custom_command)
        custom_layout.addWidget(send_btn)

        # Help Command button
        help_btn = QPushButton("Help Command")
        help_btn.clicked.connect(lambda: self.send_command("help"))
        custom_layout.addWidget(help_btn)

        # Adding widgets to layout
        layout.addWidget(self.tab_widget)
        layout.addWidget(custom_group)

    def create_wifi_tab(self):
        wifi_widget = QWidget()
        wifi_layout = QGridLayout(wifi_widget)

        # Scanning Operations
        self.create_command_group("WiFi Scanning", [
            ("Scan Access Points", "scanap"),
            ("Scan Stations", "scansta"),
            ("Stop Scan", "stopscan"),
            ("List APs", "list -a"),
            ("List Stations", "list -s")
        ], wifi_layout, 0, 0)

        # Attack Operations
        self.create_command_group("Attack Operations", [
            ("Start Deauth", "attack -d"),
            ("Stop Deauth", "stopdeauth"),
            ("Select AP", self.show_select_ap_dialog)
        ], wifi_layout, 0, 1)

        # Beacon Operations
        self.create_command_group("Beacon Operations", [
            ("Random Beacon Spam", "beaconspam -r"),
            ("Rickroll Beacon", "beaconspam -rr"),
            ("AP List Beacon", "beaconspam -l"),
            ("Custom SSID Beacon", self.show_custom_beacon_dialog),
            ("Stop Spam", "stopspam")
        ], wifi_layout, 1, 0)

        return wifi_widget

    def create_network_tab(self):
        network_widget = QWidget()
        network_layout = QGridLayout(network_widget)

        # WiFi Connection
        wifi_connect_group = QGroupBox("WiFi Connection")
        wifi_connect_layout = QFormLayout(wifi_connect_group)

        self.wifi_ssid = QLineEdit()
        self.wifi_password = QLineEdit()
        self.wifi_password.setEchoMode(QLineEdit.EchoMode.Password)

        wifi_connect_layout.addRow("SSID:", self.wifi_ssid)
        wifi_connect_layout.addRow("Password:", self.wifi_password)

        connect_btn = QPushButton("Connect to Network")
        connect_btn.clicked.connect(self.connect_to_wifi)
        wifi_connect_layout.addRow(connect_btn)

        network_layout.addWidget(wifi_connect_group, 0, 0)

        # Network Tools
        self.create_command_group("Network Tools", [
            ("Cast Random YouTube Video", "dialconnect"),
            ("Print to Network Printer", self.show_printer_dialog)
        ], network_layout, 0, 1)

        return network_widget

    def create_ble_tab(self):
        ble_widget = QWidget()
        ble_layout = QGridLayout(ble_widget)

        self.create_command_group("BLE Scanning", [
            ("Find Flippers", "blescan -f"),
            ("BLE Spam Detector", "blescan -ds"),
            ("AirTag Scanner", "blescan -a"),
            ("Raw BLE Scan", "blescan -r"),
            ("Stop BLE Scan", "blescan -s")
        ], ble_layout, 0, 0)

        return ble_widget

    def create_capture_tab(self):
        capture_widget = QWidget()
        capture_layout = QGridLayout(capture_widget)

        self.create_command_group("Packet Capture (Requires SD Card or Flipper)", [
            ("Capture Probes", "capture -probe"),
            ("Capture Beacons", "capture -beacon"),
            ("Capture Deauth", "capture -deauth"),
            ("Capture Raw", "capture -raw"),
            ("Capture WPS", "capture -wps"),
            ("Capture Pwnagotchi", "capture -pwn"),
            ("Stop Capture", "capture -stop")
        ], capture_layout, 0, 0)

        return capture_widget

    def create_evil_portal_tab(self):
        portal_widget = QWidget()
        portal_layout = QFormLayout(portal_widget)

        # Portal Settings
        self.portal_url = QLineEdit()
        self.portal_ssid = QLineEdit()
        self.portal_password = QLineEdit()
        self.portal_ap_ssid = QLineEdit()
        self.portal_domain = QLineEdit()

        portal_layout.addRow("Portal URL:", self.portal_url)
        portal_layout.addRow("Portal SSID:", self.portal_ssid)
        portal_layout.addRow("Portal Password:", self.portal_password)
        portal_layout.addRow("AP SSID:", self.portal_ap_ssid)
        portal_layout.addRow("Custom Domain:", self.portal_domain)

        # Control buttons
        button_layout = QHBoxLayout()
        start_portal_btn = QPushButton("Start Portal")
        start_portal_btn.clicked.connect(self.start_evil_portal)
        stop_portal_btn = QPushButton("Stop Portal")
        stop_portal_btn.clicked.connect(lambda: self.send_command("stopportal"))

        button_layout.addWidget(start_portal_btn)
        button_layout.addWidget(stop_portal_btn)
        portal_layout.addRow(button_layout)

        return portal_widget

    def create_settings_tab(self):
        settings_widget = QWidget()
        settings_layout = QFormLayout(settings_widget)

        # RGB Mode
        rgb_mode = QComboBox()
        rgb_mode.addItems(["Stealth Mode", "Normal Mode", "Rainbow Mode"])
        rgb_mode.currentIndexChanged.connect(lambda i: self.send_command(f"setsetting 1 {i+1}"))
        settings_layout.addRow("RGB Mode:", rgb_mode)

        # Channel Switch Delay
        channel_delay = QComboBox()
        channel_delay.addItems(["0.5s", "1s", "2s", "3s", "4s"])
        channel_delay.currentIndexChanged.connect(lambda i: self.send_command(f"setsetting 2 {i+1}"))
        settings_layout.addRow("Channel Switch Delay:", channel_delay)

        # Channel Hopping
        channel_hopping = QComboBox()
        channel_hopping.addItems(["Disabled", "Enabled"])
        channel_hopping.currentIndexChanged.connect(lambda i: self.send_command(f"setsetting 3 {i+1}"))
        settings_layout.addRow("Channel Hopping:", channel_hopping)

        # Random BLE MAC
        ble_mac = QComboBox()
        ble_mac.addItems(["Disabled", "Enabled"])
        ble_mac.currentIndexChanged.connect(lambda i: self.send_command(f"setsetting 4 {i+1}"))
        settings_layout.addRow("Random BLE MAC:", ble_mac)

        # Reboot and Save buttons side by side
        button_layout = QHBoxLayout()
        reboot_btn = QPushButton("Reboot")
        reboot_btn.clicked.connect(lambda: self.send_command("reboot"))
        button_layout.addWidget(reboot_btn)

        save_btn = QPushButton("Save Settings")
        save_btn.clicked.connect(lambda: self.send_command("savesetting"))
        button_layout.addWidget(save_btn)

        # Add the button layout to settings layout
        settings_layout.addRow(button_layout)

        return settings_widget

    def create_command_group(self, title, commands, layout, row, col):
        group = QGroupBox(title)
        group_layout = QVBoxLayout(group)

        for text, command in commands:
            btn = QPushButton(text)
            if callable(command):
                btn.clicked.connect(command)
            else:
                btn.clicked.connect(partial(self.send_command, command))
            group_layout.addWidget(btn)

        group_layout.addStretch()
        layout.addWidget(group, row, col)

    def setup_display_area(self, layout):
        display_group = QGroupBox("Display")
        display_layout = QVBoxLayout(display_group)

        self.display_text = QTextEdit()
        self.display_text.setReadOnly(True)
        display_layout.addWidget(self.display_text)

        button_layout = QHBoxLayout()

        clear_display_btn = QPushButton("Clear Display")
        clear_display_btn.clicked.connect(self.display_text.clear)
        button_layout.addWidget(clear_display_btn)

        save_log_btn = QPushButton("Save Log")
        save_log_btn.clicked.connect(self.save_log)
        button_layout.addWidget(save_log_btn)
        display_layout.addLayout(button_layout)

        layout.addWidget(display_group)

    def setup_log_area(self):
        self.log_group = QGroupBox("Log")
        log_layout = QVBoxLayout(self.log_group)

        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(200)
        log_layout.addWidget(self.log_text)

        button_layout = QHBoxLayout()
        clear_log_btn = QPushButton("Clear Log")
        clear_log_btn.clicked.connect(self.log_text.clear)
        button_layout.addWidget(clear_log_btn)

        save_log_btn = QPushButton("Save Log")
        save_log_btn.clicked.connect(self.save_log)
        button_layout.addWidget(save_log_btn)

        log_layout.addLayout(button_layout)

    def refresh_ports(self):
        self.port_combo.clear()
        ports = [port.device for port in list_ports.comports()]
        self.port_combo.addItems(ports)

    def toggle_connection(self):
        if not self.serial_port or not self.serial_port.is_open:
            try:
                port = self.port_combo.currentText()
                self.serial_port = serial.Serial(port, 115200, timeout=1)
                self.connect_btn.setText("Disconnect")
                self.connect_btn.setStyleSheet("background-color: #ff4444;")
                self.log_message(f"Connected to {port}")

                # Start monitor thread
                self.monitor_thread = SerialMonitorThread(self.serial_port)
                self.monitor_thread.data_received.connect(self.process_response)
                self.monitor_thread.start()

            except Exception as e:
                QMessageBox.critical(self, "Connection Error", str(e))
                self.log_message(f"Connection error: {str(e)}")
        else:
            self.disconnect()

    def disconnect(self):
        if self.monitor_thread:
            self.monitor_thread.stop()
            self.monitor_thread.wait()

        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()

        self.connect_btn.setText("Connect")
        self.connect_btn.setStyleSheet("")
        self.log_message("Disconnected")

    def send_command(self, command):
        if not self.serial_port or not self.serial_port.is_open:
            QMessageBox.warning(self, "Not Connected", "Please connect to ESP32 first")
            return

        self.log_message(f"Sending command: {command}")
        try:
            self.serial_port.write(f"{command}\n".encode())
        except Exception as e:
            self.log_message(f"Error sending command: {str(e)}")

    def send_custom_command(self):
        command = self.cmd_entry.text().strip()
        if command:
            self.send_command(command)
            self.cmd_entry.clear()

    def process_response(self, response):
        try:
            # Try to parse as JSON for structured data
            data = json.loads(response)
            if 'scan_result' in data:
                self.update_display_scan(data['scan_result'])
            elif 'status' in data:
                self.update_display_status(data['status'])
            else:
                self.display_text.append(response)

        except json.JSONDecodeError:
            # Format the text with timestamp
            timestamp = datetime.now().strftime("%H:%M:%S")
            formatted_text = f"[{timestamp}] {response}"
            self.display_text.append(formatted_text)

        self.display_text.ensureCursorVisible()

    def log_message(self, message):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
        self.log_text.ensureCursorVisible()

    def save_log(self):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"ghost_esp_log_{timestamp}.txt"
        try:
            with open(filename, 'w') as f:
                f.write(self.display_text.toPlainText())
            self.log_message(f"Log saved to {filename}")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save log: {str(e)}")

    def show_select_ap_dialog(self):
        selected_ap, ok = QInputDialog.getText(self, "Select Access Point", "Enter Access Point name:")
        if ok and selected_ap:
            self.send_command(f"select -a {selected_ap}")

    def show_custom_beacon_dialog(self):
        ssid, ok = QInputDialog.getText(self, "Custom Beacon", "Enter SSID for beacon spam:")
        if ok and ssid:
            self.send_command(f'beaconspam "{ssid}"')

    def show_printer_dialog(self):
        dialog = QDialog(self)
        dialog.setWindowTitle("Print to Network Printer")
        layout = QFormLayout(dialog)

        ip_input = QLineEdit()
        text_input = QTextEdit()
        font_size = QSpinBox()
        font_size.setRange(8, 72)
        font_size.setValue(12)

        alignment = QComboBox()
        alignment.addItems(["Center Middle (CM)", "Top Left (TL)", "Top Right (TR)",
                          "Bottom Right (BR)", "Bottom Left (BL)"])

        layout.addRow("Printer IP:", ip_input)
        layout.addRow("Text:", text_input)
        layout.addRow("Font Size:", font_size)
        layout.addRow("Alignment:", alignment)

        buttons = QHBoxLayout()
        ok_button = QPushButton("Print")
        cancel_button = QPushButton("Cancel")
        buttons.addWidget(ok_button)
        buttons.addWidget(cancel_button)
        layout.addRow(buttons)

        ok_button.clicked.connect(dialog.accept)
        cancel_button.clicked.connect(dialog.reject)

        if dialog.exec() == QDialog.DialogCode.Accepted:
            align_map = {"Center Middle (CM)": "CM", "Top Left (TL)": "TL",
                        "Top Right (TR)": "TR", "Bottom Right (BR)": "BR",
                        "Bottom Left (BL)": "BL"}
            cmd = f'powerprinter {ip_input.text()} "{text_input.toPlainText()}" {font_size.value()} {align_map[alignment.currentText()]}'
            self.send_command(cmd)

    def connect_to_wifi(self):
        ssid = self.wifi_ssid.text()
        password = self.wifi_password.text()
        if ssid and password:
            self.send_command(f"connect {ssid} {password}")
        else:
            QMessageBox.warning(self, "Input Error", "Please enter both SSID and password")

    def start_evil_portal(self):
        url = self.portal_url.text()
        ssid = self.portal_ssid.text()
        password = self.portal_password.text()
        ap_ssid = self.portal_ap_ssid.text()
        domain = self.portal_domain.text()

        if all([url, ssid, password, ap_ssid]):
            cmd = f"startportal {url} {ssid} {password} {ap_ssid}"
            if domain:
                cmd += f" {domain}"
            self.send_command(cmd)
        else:
            QMessageBox.warning(self, "Input Error", "Please fill all required fields")

    def update_display_scan(self, scan_data):
        self.display_text.append("\n=== Scan Results ===")
        for item in scan_data:
            self.display_text.append(f"- {item}")
        self.display_text.append("==================\n")
        self.display_text.ensureCursorVisible()

    def update_display_status(self, status):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.display_text.append(f"[{timestamp}] Status: {status}")
        self.display_text.ensureCursorVisible()

    def closeEvent(self, event):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        super().closeEvent(event)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    font = QFont("Arial", 10)
    app.setFont(font)
    window = ESP32ControlGUI()
    window.show()
    sys.exit(app.exec())
