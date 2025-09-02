#include "bluelight.hpp"
#include <fstream>
#include <ncurses.h>

#define INPUT_SHOULD_EXIT 1
#define INPUT_CONTINUE 0

class Gui {
  static const unsigned int WINDOW_WIDTH = 50;

  std::vector<Device> devices;
  std::vector<std::string> keys;
  std::vector<Device> pairedDevices;

  WINDOW * pairingWindow;
  WINDOW * keyingWindow;
  
  int cursorX, cursorDevices, cursorKeys;

public:

  Gui() {
    cursorX = 0;
    cursorDevices = 0;
    cursorKeys = 0;

    int pairingWinHeight = 1;
    if(devices.size() > 1) pairingWinHeight = devices.size();

    int keyingWinHeight = 1;
    if(pairedDevices.size() > 1) keyingWinHeight = pairedDevices.size();

    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    noecho();
    cbreak();
    timeout(500);

    //init_pair(1, -1, COLOR_GREY);

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
    int newDevicesSize = d.size();

    pairedDevices.clear();
    std::copy_if(d.begin(), d.end(), std::back_inserter(pairedDevices), [](Device dev){return dev.isBonded();});

    if(newDevicesSize != devices.size()) {
      werase(pairingWindow);
      wrefresh(pairingWindow);
      wresize(pairingWindow, newDevicesSize + 2, WINDOW_WIDTH);

      werase(keyingWindow);
      wrefresh(keyingWindow);
      wresize(keyingWindow, pairedDevices.size() + 2, WINDOW_WIDTH);
    }
    devices = d;

    render();
  }

  void setKeys();

  void getKeys();

  void render() {
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
      bool highlighted = i == cursorDevices && cursorX == 0;

      Device d = devices[i];

     
      if(highlighted) wattron(pairingWindow, A_BOLD);
      
      mvwaddstr(pairingWindow, 1+i, 2, d.getAlias().c_str());
      
      if(highlighted) wattroff(pairingWindow, A_BOLD);


      const char * textPair = "[Pair]";
      const char * textUnpair = "[Forget]";


      if(highlighted) wattron(pairingWindow, A_REVERSE);

      if(d.isBonded()) {
        mvwaddstr(pairingWindow, 1+i, WINDOW_WIDTH - strlen(textUnpair) - 1, textUnpair);
      } else {
        mvwaddstr(pairingWindow, 1+i, WINDOW_WIDTH - strlen(textPair) - 1, textPair);
      }

      if(highlighted) wattroff(pairingWindow, A_REVERSE);
    }
    if(devices.size() == 0) {
      mvwaddstr(pairingWindow, 1, 2, "No Nearby Devices");
    }

    for(int i = 0; i < pairedDevices.size(); i++) {
      bool highlighted = i == cursorKeys && cursorX == 1;

      Device d = pairedDevices[i];

     
      if(highlighted) wattron(keyingWindow, A_BOLD);
      
      mvwaddstr(keyingWindow, 1+i, 2, d.getAlias().c_str());
      
      if(highlighted) wattroff(keyingWindow, A_BOLD);


      if(highlighted) wattron(keyingWindow, A_REVERSE);

      if(highlighted) wattroff(keyingWindow, A_REVERSE);
    }
    if(pairedDevices.size() == 0) {
      mvwaddstr(keyingWindow, 1, 2, "No Paired Devices");
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

    if(cursorX == 0) {
      if(key == KEY_UP && cursorDevices > 0) cursorDevices--;
      if(key == KEY_DOWN) cursorDevices++;

      if(cursorDevices > devices.size() - 1) cursorDevices = devices.size() - 1;
    } else {
      if(key == KEY_UP && cursorKeys > 0) cursorKeys--;
      if(key == KEY_DOWN) cursorKeys++;

      if(cursorKeys > pairedDevices.size() - 1) cursorKeys = pairedDevices.size() - 1;
    }



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
