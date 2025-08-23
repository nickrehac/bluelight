#include "bluelight.hpp"
#include <fstream>
#include <ncurses.h>

int main() {
  BluetoothController controller;

  std::vector<Device> devices;

  controller.setOnDevicesUpdated([&](){
      devices = controller.getDevices();

      std::cout << "new device list! \n\n";

      for(Device d : devices) {
        std::cout << "Device: " << d.getAlias() << '\n';
      }

      std::cout << std::endl;
  });

  controller.startDiscovery();

  controller.updateDevices();

  int i = 0;

  while(true) {
    controller.poll();
  }
}
