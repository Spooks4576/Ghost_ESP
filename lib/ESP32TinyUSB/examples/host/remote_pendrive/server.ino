#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "index_html.h"
#ifndef SERVE_WEB_FROM_PENDRIVE
#include "app_js.h"
#include "app_css.h"
#endif

void handleRoot()
{
    server.send(200, "text/html", index_html);
}

void handleRoot2()
{
    String path = String(MOUNT_POINT) + "/index.html";
    server.sendHeader("Location", path);
    server.send(301, "text/html", "");
}

#ifndef SERVE_WEB_FROM_PENDRIVE

void handleJS()
{
    server.send(200, "text/javascript", app_js);
}

void handleCSS()
{
    server.send(200, "text/css", app_css);
}

#endif

bool handleFileList(String path)
{
    ESP_LOGI("", "%s", path.c_str());

    const char *dirpath = path.c_str();
    char entrypath[256];
    char entrysize[32];
    const char *entrytype;

    struct dirent *entry;
    struct stat entry_stat;
    char _dirpath[255] = {0};
    const size_t dirpath_len = strlen(dirpath);
    strncpy(_dirpath, dirpath, path.length() - 1);
    _dirpath[dirpath_len - 1] = 0x0;

    DIR *dir = opendir(_dirpath);

    /* Retrieve the base path of file storage to construct the full path */
    strlcpy(entrypath, dirpath, sizeof(entrypath));

    if (!dir)
    {
        ESP_LOGE(TAG, "Failed to stat dir : %s", _dirpath);
        return false;
    }
    String output = "[";
    // /* Iterate over all files / folders and fetch their names and sizes */
    char _script[500] = {0};
    if (strlen(dirpath) > 7)
    {
        sprintf(_script, "{\"path\":\"%s/..\", \"dir\":1,\"name\":\"Up\", \"size\":\"0\"}", _dirpath);
    }
    else
    {
        sprintf(_script, "{\"path\":\"%s\", \"dir\":1,\"name\":\"Refresh\", \"size\":\"0\"}", _dirpath);
    }
    output += _script;
    entry = readdir(dir);

    while (entry != NULL)
    {
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);

        int st = 0;
        st = stat(entrypath, &entry_stat);

        if (st == -1)
        {
            // ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
            entry = readdir(dir);
            continue;
        }

        output += ',';
        output += "{\"dir\":\"";
        output += (entry->d_type == DT_DIR) ? 1 : 0;
        output += "\",\"name\":\"";
        output += String(entry->d_name);
        output += "\",\"path\":\"";
        output += String(dirpath);
        output += String(entry->d_name);
        output += "\",\"size\":\"";
        output += String(entry_stat.st_size);
        output += "\"}";

        entry = readdir(dir);
    }

    output += "]";
    server.send(200, "text/json", output);
    return true;
}

void handleSTA()
{
    for (uint8_t i = 0; i < server.args(); i++)
    {
        if (strcmp(server.argName(i).c_str(), "ssid") == 0)
        {
            ssidSTA = server.arg(i);
        }

        if (strcmp(server.argName(i).c_str(), "pass") == 0)
        {
            passSTA = server.arg(i);
        }
    }

    handleRoot();
    WiFi.begin(ssidSTA.c_str(), passSTA.c_str());
}

void handleAP()
{
    for (uint8_t i = 0; i < server.args(); i++)
    {
        if (strcmp(server.argName(i).c_str(), "ssid") == 0)
        {
            ssidAP = server.arg(i);
        }

        if (strcmp(server.argName(i).c_str(), "pass") == 0)
        {
            passAP = server.arg(i);
        }
    }

    handleRoot();
    WiFi.softAP(ssidAP.c_str(), passAP.c_str());
}

String getContentType(String filename)
{
    if (server.hasArg("download"))
    {
        return "application/octet-stream";
    }
    else if (filename.endsWith(".htm"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".html"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".css"))
    {
        return "text/css";
    }
    else if (filename.endsWith(".js"))
    {
        return "application/javascript";
    }
    else if (filename.endsWith(".png"))
    {
        return "image/png";
    }
    else if (filename.endsWith(".gif"))
    {
        return "image/gif";
    }
    else if (filename.endsWith(".jpg"))
    {
        return "image/jpeg";
    }
    else if (filename.endsWith(".ico"))
    {
        return "image/x-icon";
    }
    else if (filename.endsWith(".xml"))
    {
        return "text/xml";
    }
    else if (filename.endsWith(".pdf"))
    {
        return "application/pdf";
    }
    else if (filename.endsWith(".zip"))
    {
        return "application/x-zip";
    }
    else if (filename.endsWith(".gz"))
    {
        return "application/x-gzip";
    }
    return "text/plain";
}

bool handleFileRead(String path)
{
    if (path.endsWith("/"))
    {
        return handleFileList(path);
    }
    Serial.println(path);

    FILE *fd = NULL;
    struct stat file_stat;
    const char *filepath = path.c_str();

    if (stat(filepath, &file_stat) == -1)
    {
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        return false;
    }

    fd = fopen(filepath, "r");
    if (!fd)
    {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        return false;
    }
    ESP_LOGI(TAG, "Sending file : %s (%ld bytes), type: %s...", filepath, file_stat.st_size, getContentType(String(filepath)).c_str());
    // server._streamFileCore(, String(filepath), getContentType(String(filepath)));

    String contentType = getContentType(String(filepath));
    server.setContentLength(file_stat.st_size);
    server.send(200, contentType, "");

    char chunk[500] = {};
    size_t chunksize;
    size_t total = 0;
    do
    {
        chunksize = fread(chunk, 1, 500, fd);
        server.client().write((uint8_t *)chunk, chunksize);
        total += chunksize;
    } while (total < file_stat.st_size);
    /* Close file after sending complete */
    fclose(fd);
    return true;
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void handleMkdir()
{
    const char* path;
    if (strcmp(server.argName(0).c_str(), "dirname") == 0)
    {
        path = server.arg(0).c_str();
    }

    // char path[256] = {};
    // sprintf(path, "%s", name + 8);
    if (mkdir(path, 0755))
    {
        ESP_LOGE("", "failed to mkdir [%d]: %s",  server.args(), path);
        server.send(409, "text/html", "");
        for (uint8_t i = 0; i < server.args(); i++)
        {
            Serial.println(server.argName(i).c_str());
        }
        return;
    }
    else
    {
        server.send(201, "text/html", "");
    }
}

void handleDelete()
{
    const char* path;
    if (strcmp(server.argName(0).c_str(), "file") == 0)
    {
        path = server.arg(0).c_str();
    }

    // char path[256] = {};
    // sprintf(path, "%s", name + 8);
    if (unlink(path))
    {
        ESP_LOGE("", "failed to unlink: %s", path);
        server.send(409, "text/html", "");
        return;
    }
    else
    {
        server.send(201, "text/html", "");
    }
    
}
