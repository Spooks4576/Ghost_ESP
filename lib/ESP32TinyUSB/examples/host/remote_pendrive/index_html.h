const char* index_html = "<!DOCTYPE html>"
"<html>"
""
"<head>"
"  <title>Remote pendrive</title>"
"  <script src='app.js'></script>"
"  <link rel='stylesheet' href='app.css'>"
"</head>"
""
"<body>"
"  <div class='modal' id='spinner'>"
"    <div style='width: 320px;'>"
"    </div>"
"  </div>"
"  <div class='main-content'>"
"    <div class='connect-container'>"
"      <span id='status'></span>"
"      <button id='wifi' class='button black'>Setup wifi</button>"
"      <button id='tree' class='button black'>Files tree</button>"
"    </div>"
""
"    <div id='myModal1' class='modal'>"
"      <!-- Modal content -->"
"      <div class='modal-content' style='background-color: #216cb7;max-width: 400px;'>"
"        <span id='close2' class='close'>&times;</span>"
"        <div class='container'>"
"          <div class='receiver' style='max-width: 400px; min-width: 320px;'>"
"            <input type='checkbox' id='en_ap'>AP </input>"
"            <div id='ap_content' hidden>"
"              <div class='lines-header'>Wifi access point:</div>"
"              <label for='ap_ssid'>AP ssid: </label><input id='ap_ssid' style='position: relative; left: 75px;'/></br>"
"              <label for='ap_pass'>AP password: </label><input id='ap_pass' style='position: relative; left: 40px;' /></br>"
"              <label for='enc_type'>Auth mode: </label>"
"              <select id='auth' style='position: relative; left: 8px;'>"
"                <option value=0>WIFI_AUTH_OPEN</option>"
"                <option value=1>WIFI_AUTH_WEP</option>"
"                <option value=2>WIFI_AUTH_WPA_PSK</option>"
"                <option value=3>WIFI_AUTH_WPA2_PSK</option>"
"                <option value=4>WIFI_AUTH_WPA_WPA2_PSK</option>"
"                <option value=5>WIFI_AUTH_WPA2_ENTERPRISE</option>"
"                <option value=6>WIFI_AUTH_WPA3_PSK</option>"
"                <option value=7>WIFI_AUTH_WPA2_WPA3_PSK</option>"
"              </select></br>"
""
"              <a href='' id='ap_ip' hidden>Connect device</a>"
"              <button id='ap_btn' class='button black'>Setup wifi</button>"
"              </br></br>"
"            </div>"
""
"            <input type='checkbox' id='en_sta'>STA</input>"
"            <div id='sta_content' hidden>"
"              <div class='lines-header'>Wifi station:</div>"
"              <label for='sta_ssid'>STA ssid: </label><input id='sta_ssid'  style='position: relative; left: 70px;'/></br>"
"              <label for='sta_pass'>STA password: </label><input id='sta_pass'  style='position: relative; left: 35px;'/></br>"
"              <a href='' id='sta_ip' hidden>Connect device</a>"
"              <button id='sta_btn' class='button black'>Connect wifi</button>"
"            </div>"
"          </div>"
"        </div>"
"      </div>"
"    </div>"
""
"    <div id='myModal2' class='modal' >"
"      <!-- Modal content -->"
"      <div class='modal-content' style='background-color: #216cb7; max-width: 600px; min-width: fit-content;'>"
"        <div class='container'>"
"          <div class='receiver' style='max-width: 400px'>"
"            <div>"
"              <label style='font-size: large; font-weight: bold; '>Path:</label>"
"                <label id='full_path' style='font-size: large; font-weight: bold;'></label>"
"              </div>"
"            <table id='filestree' class='filestree'></table>"
"          </div>"
"        </div>"
"      </div>"
"    </div>"
""
"  </div>"
"</body>"
""
"</html>";