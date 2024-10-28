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
                             QSplitter, QInputDialog)
from PyQt6.QtCore import Qt, pyqtSignal, QThread
from PyQt6.QtGui import QFont, QTextCursor
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
        self.setGeometry(100, 100, 1200, 800)

        # Initialize serial communication variables
        self.serial_port = None
        self.monitor_thread = None

        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # Create main layout for central widget
        main_layout = QVBoxLayout(central_widget)

        # Set up the main layout
        self.setup_ui(main_layout)

        # Refresh available ports
        self.refresh_ports()

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

        # Bottom - Log area
        self.setup_log_area()
        splitter.addWidget(self.log_group)

    def setup_connection_bar(self, main_layout):
        connection_group = QGroupBox("Serial Connection")
        connection_layout = QHBoxLayout(connection_group)

        self.port_combo = QComboBox()
        connection_layout.addWidget(QLabel("Port:"))
        connection_layout.addWidget(self.port_combo)

        refresh_btn = QPushButton("Refresh Ports")
        refresh_btn.clicked.connect(self.refresh_ports)
        connection_layout.addWidget(refresh_btn)

        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        connection_layout.addWidget(self.connect_btn)

        main_layout.addWidget(connection_group)

    def setup_command_tabs(self, layout):
        self.tab_widget = QTabWidget()

        # WiFi Operations Tab
        wifi_widget = QWidget()
        wifi_layout = QGridLayout(wifi_widget)
        self.create_command_group("WiFi Scanning", [
            ("Scan Access Points", "scanap"),
            ("Scan Stations", "scansta"),
            ("Stop Scan", "stopscan"),
            ("List APs", "list -a"),
            ("List Stations", "list -s")
        ], wifi_layout, 0, 0)

        self.create_command_group("Attack Operations", [
            ("Start Deauth", "attack -d"),
            ("Stop Deauth", "stopdeauth"),
            ("Select AP", self.show_select_ap_dialog)
        ], wifi_layout, 0, 1)

        self.create_command_group("Beacon Operations", [
            ("Random Beacon Spam", "beaconspam -r"),
            ("Rickroll Beacon", "beaconspam -rr"),
            ("AP List Beacon", "beaconspam -l"),
            ("Stop Spam", "stopspam")
        ], wifi_layout, 1, 0)

        self.tab_widget.addTab(wifi_widget, "WiFi Operations")

        # BLE Operations Tab
        ble_widget = QWidget()
        ble_layout = QGridLayout(ble_widget)
        self.create_command_group("BLE Scanning", [
            ("Find Flippers", "blescan -f"),
            ("BLE Spam Detector", "blescan -ds"),
            ("AirTag Scanner", "blescan -a"),
            ("Raw BLE Scan", "blescan -r"),
            ("Stop BLE Scan", "blescan -s")
        ], ble_layout, 0, 0)

        self.tab_widget.addTab(ble_widget, "BLE Operations")

        # Capture Operations Tab
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

        self.tab_widget.addTab(capture_widget, "Capture Operations")

        # Custom Command Area
        custom_group = QGroupBox("Custom Command")
        custom_layout = QVBoxLayout(custom_group)

        self.cmd_entry = QLineEdit()
        self.cmd_entry.setPlaceholderText("Enter custom command...")
        self.cmd_entry.returnPressed.connect(self.send_custom_command)
        custom_layout.addWidget(self.cmd_entry)

        send_btn = QPushButton("Send Command")
        send_btn.clicked.connect(self.send_custom_command)
        custom_layout.addWidget(send_btn)

        layout.addWidget(self.tab_widget)
        layout.addWidget(custom_group)

    def create_command_group(self, title, commands, layout, row, col):
        group = QGroupBox(title)
        group_layout = QVBoxLayout(group)

        for text, command in commands:
            btn = QPushButton(text)
            if callable(command):
                btn.clicked.connect(command)
            else:
                # Use partial to pass the command directly
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

        clear_display_btn = QPushButton("Clear Display")
        clear_display_btn.clicked.connect(self.display_text.clear)
        display_layout.addWidget(clear_display_btn)

        layout.addWidget(display_group)

    def setup_log_area(self):
        self.log_group = QGroupBox("Log")
        log_layout = QVBoxLayout(self.log_group)

        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(150)
        log_layout.addWidget(self.log_text)

        clear_log_btn = QPushButton("Clear Log")
        clear_log_btn.clicked.connect(self.log_text.clear)
        log_layout.addWidget(clear_log_btn)

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
            self.display_text.append(response)

        self.display_text.ensureCursorVisible()

    def log_message(self, message):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
        self.log_text.ensureCursorVisible()

    def show_select_ap_dialog(self):
        selected_ap, ok = QInputDialog.getText(self, "Select Access Point", "Enter Access Point name:")
        if ok and selected_ap:
            self.send_command(f"select -a {selected_ap}")

    def update_display_scan(self, scan_data):
        # Append scan result to display text
        self.display_text.append("Scan Results:")
        for item in scan_data:
            self.display_text.append(f"- {item}")
        self.display_text.ensureCursorVisible()

    def update_display_status(self, status):
        self.display_text.append(f"Status: {status}")
        self.display_text.ensureCursorVisible()

    def closeEvent(self, event):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        super().closeEvent(event)

# Application entry point
if __name__ == "__main__":
    app = QApplication(sys.argv)
    font = QFont("Arial", 10)
    app.setFont(font)
    window = ESP32ControlGUI()
    window.show()
    sys.exit(app.exec())
