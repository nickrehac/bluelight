#include "bluelight.hpp"
#include <fstream>
#include <ncurses.h>

#define INPUT_SHOULD_EXIT 1
#define INPUT_CONTINUE 0

class Gui {
  static const unsigned int WINDOW_WIDTH = 50;

  std::vector<Device> devices;
  std::vector<std::string> keys;

  WINDOW * pairingWindow;
  WINDOW * keyingWindow;
  
  int cursorX, cursorY;

public:

  Gui() {
    cursorX = 0;
    cursorY = 0;

    int pairingWinHeight = 1;
    if(devices.size() > 1) pairingWinHeight = devices.size();

    int keyingWinHeight = 1;
    if(keys.size() > 1) keyingWinHeight = keys.size();

    initscr();
    curs_set(0);
    noecho();
    cbreak();
    timeout(500);

    pairingWindow = newwin(2 + pairingWinHeight, WINDOW_WIDTH, 0, 0);
    keyingWindow = newwin(2 + keyingWinHeight, WINDOW_WIDTH, 0, WINDOW_WIDTH + 5);

    keypad(pairingWindow, true);
    keypad(keyingWindow, true);
    keypad(stdscr, true);
  }

  ~Gui() {
    delwin(pairingWindow);
    delwin(keyingWindow);

    endwin();
  }

  void setDevices(std::vector<Device> d) {
    int newSize = d.size();
    if(newSize != devices.size()) {
      werase(pairingWindow);
      wrefresh(pairingWindow);
      wresize(pairingWindow, newSize + 2, WINDOW_WIDTH);
    }
    devices = d;
    render();
  }

  void setKeys();

  void getKeys();

  void render() {
    //erase();
    //refresh();

    werase(pairingWindow);
    werase(keyingWindow);

    if(cursorX == 0) {
      box(pairingWindow, 0, 0);
      box(keyingWindow, '.', '.');
    }
    else {
      box(pairingWindow, '.', '.');
      box(keyingWindow, 0, 0);
    }

    mvwaddstr(pairingWindow, 0, 2, "Devices");
    mvwaddstr(keyingWindow, 0, 2, "Keys");

    for(int i = 0; i < devices.size(); i++) {
      Device d = devices[i];
      mvwaddstr(pairingWindow, 1+i, 2, d.getAlias().c_str());
    }
    if(devices.size() == 0) {
      mvwaddstr(pairingWindow, 1, 2, "No Nearby Devices");
    }

    wrefresh(pairingWindow);
    wrefresh(keyingWindow);
  }

  int doInput() {
    int key;
    key = getch();
    /*
    if(cursorX == 0) {
      key = wgetch(pairingWindow);
    }
    else {
      key = wgetch(keyingWindow);
    }*/

    if(key == ERR) return INPUT_CONTINUE;

    
    if(key == KEY_LEFT && cursorX == 1) cursorX = 0;
    if(key == KEY_RIGHT && cursorX == 0) cursorX = 1;

    if(key == KEY_UP && cursorY > 0) cursorY--;
    if(key == KEY_DOWN) cursorY++;

    if(cursorX == 0 && cursorY > devices.size() - 1) cursorY = devices.size() - 1;
    if(cursorX == 1 && cursorY > keys.size() - 1) cursorY = keys.size() - 1;


    if(key == 'q') return INPUT_SHOULD_EXIT;

    render();

    return INPUT_CONTINUE;
  }
};

int main() {
  BluetoothController controller;

  controller.startDiscovery();
  controller.updateDevices();


  Gui gui;

  gui.render();

  controller.setOnDevicesUpdated([&](){
      gui.setDevices(
        controller.getDevices()
      );
  });


  while(true) {
    controller.poll();
    if(gui.doInput() == INPUT_SHOULD_EXIT) return 0;
    controller.updateDevices();
  }
}
