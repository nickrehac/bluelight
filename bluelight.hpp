#include <dbus/dbus.h>
#include <poll.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <vector>
#include <string>
#include <utility>
#include <array>
#include <iostream>
#include <cstring>
#include <functional>

#define BT_SERVICE "org.bluez"
#define ADAPTER_PATH "/org/bluez/hci0"
#define DEVICES_PATH "/org/bluez/hci0/"
#define APP_PATH "/com/nickrehac/bluelight"
#define SIGNAL_MATCH_RULES "type='signal',sender='org.bluez'"

#define NUM_HANDLERS 1

void fail(DBusError e);

class Device {
  std::string path;

  DBusConnection * connection;

  std::string alias;
  std::string address;
  bool inRange;


public:
  Device(std::string path, DBusConnection * connection);

  std::string getAlias();
  bool isPaired();
};

class BluetoothController {
  DBusConnection * connection;

  std::vector<DBusWatch*> watches;

  static unsigned int addWatchFunction(DBusWatch * watch, void * data);
  static void removeWatchFunction(DBusWatch * watch, void * data);
  static void freeWatchFunction(void * memory);

  void pollWatches();

  static DBusHandlerResult incomingMessageHandler(DBusConnection * connection, DBusMessage * message, void * controller);

  static DBusHandlerResult signalHandler(DBusConnection * connection, DBusMessage * message, void * controller);

  std::function<void()> onDevicesUpdated;

  void registerForSignals();

  std::vector<Device> devices;

public:
  BluetoothController();
  ~BluetoothController();

  void updateDevices();

  std::vector<Device> getDevices();
  bool setPairing(bool);
  void startDiscovery();
  void stopDiscovery();

  void dispatch();
  void poll();

  void setOnDevicesUpdated(std::function<void()> callback);
};

class LEDConnection {

};
