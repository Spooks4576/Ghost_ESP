/*
    esp32 S2 USB host MSC example with access from website 

    author: chegewara
*/
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <esp_vfs_fat.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "usb/usb_host.h"
#include "usb_host.hpp"
#include "usb_acm.hpp"
#include "usb_msc.hpp"

#define SERVE_WEB_FROM_PENDRIVE 1

WebServer server(80);
USBhost host;
USBmscDevice *device;

#define EXAMPLE_ESP_WIFI_SSID "pendrive"
#define EXAMPLE_ESP_WIFI_PASS ""
#define MOUNT_POINT "/files"

String ssidSTA;
String passSTA;
String ssidAP;
String passAP;

bool is_mount = false;

static void inquiry_cb(usb_transfer_t *transfer)
{
    printf("inquiry_cb\n");
    device->mount(MOUNT_POINT);
    is_mount = true;
}

void client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
    {
        host.open(event_msg);
        usb_device_info_t info = host.getDeviceInfo();
        ESP_LOGI("", "device speed: %s, device address: %d, max ep_ctrl size: %d, config: %d", info.speed ? "USB_SPEED_FULL" : "USB_SPEED_LOW", info.dev_addr, info.bMaxPacketSize0, info.bConfigurationValue);
        const usb_config_desc_t *config_desc = host.getConfigurationDescriptor();
        int offset = 0;

        for (size_t n = 0; n < config_desc->bNumInterfaces; n++)
        {
            const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, n, 0, &offset);

            if (intf->bInterfaceClass == 0x08) // MSC
            {
                msc_transfer_cb_t cb = {
                    .inquiry_cb = inquiry_cb,
                };
                device = new USBmscDevice(config_desc, &host);
                ((USBmscDevice *)device)->registerCallbacks(cb);
                n = config_desc->bNumInterfaces;
            }
            else if (intf->bInterfaceClass == 0x02 || intf->bInterfaceClass == 0x0a) // CDC ACM
            {
                n = config_desc->bNumInterfaces;
            }

            if (device)
                device->init();
        }
    }
    else
    {
        ESP_LOGW("", "DEVICE gone event");
    }
}

void setup(void)
{
    Serial.begin(115200);
    host.registerClientCb(client_event_callback);
    host.init();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    if (MDNS.begin("esp32"))
    {
        Serial.println("MDNS responder started");
    }

    if (ssidSTA.length() > 1)
    {
        WiFi.begin(ssidSTA.c_str(), passSTA.c_str());
    }

#ifndef SERVE_WEB_FROM_PENDRIVE
    server.on("/", handleRoot);
    server.on("/app.js", handleJS);
    server.on("/app.css", handleCSS);
#else
    server.on("/", handleRoot2);
#endif
    server.on("/station", handleSTA);
    server.on("/wifi_ap", handleAP);
    server.on("/mkdir", handleMkdir);
    server.on("/delete", handleDelete);

    server.onNotFound([]()
    {
        if (!handleFileRead(server.uri()))
        {
            handleNotFound();
        }
    });

    server.begin();
    Serial.println("HTTP server started");

    delay(1000); // let already inserted pendrive to mount and get statistics
    if (is_mount)
    {
        read_test(0);

        uint64_t used_space;
        uint64_t max_space;
        getFreeSpace(&used_space, &max_space);

        log_i("storage used: %lld/%lld", used_space, max_space);
    }
}

void loop(void)
{
    server.handleClient();
    delay(2);
}

//--------------------- optional test functions ------------------------//
void read_test(int t)
{
    char line[500];
    FILE *f;
    char *mount;
    if (t)
    {
        f = fopen(MOUNT_POINT "/README.txt", "r");
        mount = MOUNT_POINT;
    }
    else
    {
        f = fopen(MOUNT_POINT "/README2.txt", "r");
        mount = MOUNT_POINT;
    }

    if (f == NULL)
    {
        ESP_LOGE("TAG", "Failed to open file");
    }
    else
    {
        ESP_LOGI("READ file", "%s", MOUNT_POINT "/README.txt");
        char *pos;
        fgets(line, sizeof(line), f);
        // strip newline
        pos = strchr(line, '\n');
        do
        {
            if (pos)
            {
                *pos = '\0';
            }
            ESP_LOGI("", "%s", line);
            fgets(line, sizeof(line), f);
            pos = strchr(line, '\n');
        } while (pos);
        fclose(f);
    }

    char *_dirpath = mount;
    char *dirpath = MOUNT_POINT "/";
    DIR *dir = opendir(_dirpath);

    // /* Retrieve the base path of file storage to construct the full path */
    // strlcpy(entrypath, dirpath, sizeof(entrypath));
    char entrypath[256];
    char entrysize[32];
    const char *entrytype;

    struct dirent *entry;
    struct stat entry_stat;
    const size_t dirpath_len = strlen(dirpath);
    strlcpy(entrypath, dirpath, sizeof(entrypath));

    if (!dir)
    {
        ESP_LOGE("TAG", "Failed to stat dir : %s", _dirpath);
    }
    else
    {
        entry = readdir(dir);
        while (entry != NULL)
        {
            entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

            strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);

            int st = 0;
            st = stat(entrypath, &entry_stat);
            if (st == -1)
            {
                ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entrypath);
                entry = readdir(dir);
                continue;
            }
            sprintf(entrysize, "%ld", entry_stat.st_size);
            ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entrypath, entrysize);
            entry = readdir(dir);
        }
        closedir(dir);
    }
}

static void getFreeSpace(uint64_t *used_space, uint64_t *max_space)
{

    FATFS *fs;
    DWORD c;
    if (f_getfree(MOUNT_POINT, &c, &fs) == FR_OK)
    {
        *used_space =
            ((uint64_t)fs->csize * (fs->n_fatent - 2 - fs->free_clst)) * fs->ssize;
        *max_space = ((uint64_t)fs->csize * (fs->n_fatent - 2)) * fs->ssize;
    }
}
