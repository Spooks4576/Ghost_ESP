#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "usb/usb_host.h"

#include "usb_host.hpp"
#include "usb_acm.hpp"
#include "usb_msc.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <esp_vfs_fat.h>

USBhost host;
// USBhostDevice* device;
USBmscDevice *device;

void cbw_cb(usb_transfer_t *transfer)
{
    printf("USER CBW CB\n");
}

void data_cb(usb_transfer_t *transfer)
{
    printf("USER data CB\n");
}

static void csw_cb(usb_transfer_t *transfer)
{
    printf("USER CSW CB\n");
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    ESP_LOGE("", "stack overflow: %s\n", pcTaskName);
}

#define MOUNT_POINT "/msc"
#define MOUNT_POINT1 "/msc1"
char line[5000];
bool is_mount = false;
static void capacity_cb(usb_transfer_t *transfer)
{
    printf("capacity_cb: block_size: %d, block_count: %d\n", device->getBlockSize(is_mount), device->getBlockCount(is_mount));
}

static void inquiry_cb(usb_transfer_t *transfer)
{
    printf("inquiry_cb\n");
    if(!is_mount){
        device->mount(MOUNT_POINT, 0);
        device->mount(MOUNT_POINT1, 1);
    }
    is_mount = true;
}

static void unit_ready_cb(usb_transfer_t *transfer)
{
    printf("unit_ready_cb\n");
}

static void max_luns_cb(usb_transfer_t *transfer)
{
    printf("max_luns_cb\n");
}

static void sense_cb(usb_transfer_t *transfer)
{
    printf("sense_cb\n");
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
                    // .cbw_cb = cbw_cb,
                    // .data_cb = data_cb,
                    // .csw_cb = csw_cb,
                    .capacity_cb = capacity_cb,
                    .inquiry_cb = inquiry_cb,
                    // .unit_ready_cb = unit_ready_cb,
                    // .max_luns_cb = max_luns_cb,
                    // .sense_cb = sense_cb,
                };
                device = new USBmscDevice(config_desc, &host);
                ((USBmscDevice *)device)->registerCallbacks(cb);
                n = config_desc->bNumInterfaces;
            }
            else if (intf->bInterfaceClass == 0x02 || intf->bInterfaceClass == 0x0a) // CDC ACM
            {
                // device = new USBacmDevice(config_desc, &host);
                n = config_desc->bNumInterfaces;
            }

            if (device)
                device->init();

            // heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);

        }
    } else {
        ESP_LOGW("", "DEVICE gone event");
    }
}

#define TAG ""

void read_test(int t)
{
    FILE *f;
    char* mount;
    if (t)
    {
        f = fopen(MOUNT_POINT"/README.txt", "r");
        mount = MOUNT_POINT;
    } else
    {
        f = fopen(MOUNT_POINT"/README2.txt", "r");
        mount = MOUNT_POINT;
    }
    
    if (f == NULL)
    {
        ESP_LOGE("TAG", "Failed to open file");
    }
    else
    {
        ESP_LOGI("READ file", "%s", MOUNT_POINT"/README.txt");
        char *pos;
        fgets(line, sizeof(line), f);
        // strip newline
        pos = strchr(line, '\n');        
        do{
            if (pos)
            {
                *pos = '\0';
            }
            ESP_LOGI("", "%s", line);
            fgets(line, sizeof(line), f);
            pos = strchr(line, '\n');
        }while(pos);
        fclose(f);
    }

    char* _dirpath = mount;
    char* dirpath = "/msc/";
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
                if(unlink(entrypath))
                    ESP_LOGE(TAG, "unlink failed");
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

void write_test()
{
    FILE* f = fopen("/msc/README.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(TAG, "File written");
    read_test(1);

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/msc/README1.txt", &st) == 0) {
        // Delete it if it exists
        if(unlink("/msc/README1.txt"))
            ESP_LOGE(TAG, "unlink failed");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename("/msc/README.txt", "/msc/README1.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        // return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen("/msc/README1.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file %s: '%s'", "/msc/README1.txt", line);
}


static void getFreeSpace(uint64_t* used_space, uint64_t* max_space)
{

    FATFS *fs;
    DWORD c;
    if (f_getfree("/files", &c, &fs) == FR_OK)
    {
        *used_space =
            ((uint64_t)fs->csize * (fs->n_fatent - 2 - fs->free_clst)) * fs->ssize;
        *max_space = ((uint64_t)fs->csize * (fs->n_fatent - 2)) * fs->ssize;
    }
}


void setup()
{
    host.registerClientCb(client_event_callback);
    host.init();

    while (!is_mount)
    {
        vTaskDelay(1);
    }

    write_test();
    read_test(0);

    uint64_t used_space; 
    uint64_t max_space;
    getFreeSpace(&used_space, &max_space);

    ESP_LOGI("TEST", "storage used: %lld/%lld", used_space, max_space);
}

void loop()
{
}