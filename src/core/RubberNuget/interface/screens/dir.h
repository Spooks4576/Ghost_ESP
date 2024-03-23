#ifndef DIR_SCREEN_H
#define DIR_SCREEN_H

#define DIR_SCREEN_MAX_SHOWN 4

#include "cdcusb.h"
#include "mscusb.h"
#include "flashdisk.h"

#include "../lib/NuggetInterface.h"
#include "runner.h"


class DirScreen : public NuggetScreen {
	public:
		DirScreen(String path);
		~DirScreen();
		bool draw();
		int update(int);
	private:
		FILINFO* files;
		String path;
		int numFiles;
		int selected;
		int top;
};

#endif
