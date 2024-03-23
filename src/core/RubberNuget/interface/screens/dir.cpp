#include "dir.h"
#include "../graphics.h"
#include "../../RubberNugget.h"
#include "../../utils.h"

DirScreen::DirScreen(String path) {
  this->path = path;
  this->files = nullptr;
  this->numFiles = 0;
  this->selected = 0;
  this->top = 0;

  // TODO: files could be null; draw/update should handle this
  this->files = newFileList(path.c_str(), numFiles);
}

DirScreen::~DirScreen() {
  if (files){
    delete this->files;
  }
}

int DirScreen::update(int btn) {
  switch(btn){
    case BTN_UP:
      if (this->selected != 0) {
          this->selected--;
      }
      if (this->selected < this->top) {
          this->top--;
      }
      break;
    case BTN_DOWN:
      if (this->selected < this->numFiles-1) {
          this->selected++;
      }
      if (this->selected-this->top > DIR_SCREEN_MAX_SHOWN-1){
          this->top++;
      }
      break;
    case BTN_LEFT:
      return SCREEN_BACK;
    case BTN_RIGHT:
      if(files[selected].fattrib & AM_DIR){ // directory; enter it
        String nextPath(path);
        if (nextPath.length()!=1){
          nextPath += "/";
        }
        nextPath += files[selected].fname;
        Serial.printf("Before dir screen alloc. Mem: %d\n", ESP.getFreeHeap());
        NuggetScreen* subdir = new DirScreen(nextPath);
        this->pushScreen(subdir);
        return SCREEN_PUSH;
      } else { // file; assume it's a payload and run it
        String payloadPath = path;
        if (payloadPath.length()!=1){
          payloadPath += "/";
        }
        payloadPath += files[selected].fname;
        Serial.printf("Before runner alloc. Mem: %d\n", ESP.getFreeHeap());
        fileOp op = readFile(payloadPath);
        if (op.ok) {
          NuggetScreen* runner = new ScriptRunnerScreen(op.result);
          this->pushScreen(runner);
          return SCREEN_PUSH;
        } else {
          // TODO: inform the user the payload could not be opened
        }
      }
  }
  return SCREEN_REDRAW;
}


//     item0             <-- files; item0 extends "above" screen
// ┌──────────────────┐
// │   item1          │  <-- top=1
// │   item2          │
// │ > item3          │  <-- cursor=3; third item, not third from top
// │   item4          │  <-- numFiles=5
// ├──────────────────┤
// │/path/items       │  <-- footer
// └──────────────────┘
bool DirScreen::draw() {
  if (!files || this->selected >= this->numFiles || (this->top > 0 && this->numFiles < DIR_SCREEN_MAX_SHOWN)) {
    Serial.printf("[displayFiles] condition error: (files:%p)(numFiles:%d)(selected:%d)(top:%d)\n");
    // TODO: display error screen
    return false;
  }

  // loop condition: display only DIR_SCREEN_MAX_SHOWN at a time, and not more
  // than files we actually have.
  for (int i = top; i < top+DIR_SCREEN_MAX_SHOWN && i < numFiles; i++) {
    String fileName(files[i].fname);
    if (fileName.length() > 12) {
      fileName = fileName.substring(0, 9) + "...";
    }
    display->drawString(6, 10*(i-top), fileName);
  }
  // Cursor
  display->drawRect(2,10*(selected-top)+6,2,2);

  // Footer
  display->drawLine(0, 54, 127, 54);
  display->drawLine(0, 53, 127, 53);
  display->drawString(0,54,"Dir:");
  if(path.length() > 17) {
    display->drawString(25, 54, path.substring(0,14)+"...");
  } else {
    display->drawString(25, 54, path);
  }
  // Cat image
  display->drawXbm(0, 0, 128, 64, main_cat_image_bits);
  return true;
}

