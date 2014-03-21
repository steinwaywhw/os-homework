

#include "mainwindow.h"
#include "mainwindow_wrapper.h"

extern "C" {
	void mainwindow_draw (void *main_window, void *data, int len) {
 		static_cast<MainWindow *>(main_window)->draw(data, len);
	}

	// void mainwindow_wrong (void *main_window) {
	// 	static_cast<MainWindow *>(main_window)->wrong();
	// }
}

