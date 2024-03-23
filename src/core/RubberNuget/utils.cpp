#include "utils.h"

fileOp saveFile(String path, String content) {
  fileOp ret;
  ret.ok = false;

  // create parent directories as needed
  FILINFO filinfo;
  int pathSoFarIdx = 1;
  while(true) {
    int nextDir = path.indexOf("/", pathSoFarIdx);
    if (nextDir == -1){
      break;
    }
    String pathSoFar = path.substring(0, nextDir);
    if (FR_OK != f_stat(pathSoFar.c_str(), &filinfo)){
      if (f_mkdir(pathSoFar.c_str()) != FR_OK) {
        ret.result = String("could not create directory");
        return ret;
      }
    }
    pathSoFarIdx = nextDir+1;
  }

  // Create file
  FIL file;
  if (FR_OK != f_open(&file, path.c_str(), FA_WRITE | FA_CREATE_ALWAYS)){
    ret.result = String("could not open file for writing");
    return ret;
  }

  // Write to file
  UINT written = 0;
  if (FR_OK != f_write(&file, content.c_str(), content.length(), &written)){
    f_close(&file);
    ret.result = String("could not write to file");
    return ret;
  }
  f_close(&file);
  ret.ok = true;
  return ret;
}

fileOp readFile(String path){
  fileOp ret;
  ret.ok = false;

  // open file
  FIL file;
  FRESULT fr = f_open(&file, path.c_str(), FA_READ);
  if (fr != FR_OK) {
    // TODO: add more specific reason for failure. For now, return most likely
    // reason for failure
    ret.result = String("file doesn't exist");
    return ret;
  }

  // read file contents
  uint16_t size = f_size(&file);
  char* data = (char*) malloc(size+1);
  UINT bytesRead;
  fr = f_read(&file, data, (UINT)size, &bytesRead);

  if (fr == FR_OK && bytesRead==size) {
    ret.result = String(data);
    ret.result = ret.result.substring(0, bytesRead);
    ret.ok = true;
    Serial.println("[readFile]");
    Serial.println(ret.result);
  } else {
    ret.result = String("error reading file");
  }
  f_close(&file);
  return ret;
}