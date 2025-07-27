#include <dbus/dbus.h>

#include <vector>
#include <string>
#include <ncurses>

#define BT_SERVICE "org.bluez"
#define ADAPTER_PATH "/org/bluez/hci0/"

class Device {
  std::string path;

  DBusConnection * busConnection;

public:
  Device(std::string path);

  bool isPaired();
  std::string path();
}

class BluetoothController {
  DBusConnection * connection;

  void fail(DBusError e);

public:
  BluetoothController();
  ~BluetoothController();

  vector<Device> getDevices();
  bool setPairing(bool);
  bool startDiscovery();
}
