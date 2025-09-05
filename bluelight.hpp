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
#include <optional>

#define BT_SERVICE "org.bluez"
#define ADAPTER_PATH "/org/bluez/hci0"
#define DEVICES_PATH "/org/bluez/hci0/"
#define BT_SERVICE_PATH "/org/bluez"
#define APP_PATH "/com/nickrehac/bluelight"
#define SIGNAL_MATCH_RULES "type='signal',sender='org.bluez'"

#define NUM_HANDLERS 1

void fail(DBusError e);

class Device {
  std::string path;

  DBusConnection * connection;

  std::string alias;
  std::string address;

  bool bonded;
  bool connected;
  int rssi;

  std::optional<std::string> getString(std::string property);
  std::optional<short> getShort(std::string property);
  std::optional<bool> getBool(std::string property);

public:
  Device(std::string path, DBusConnection * connection);

  std::string getPath();
  std::string getAlias();
  std::string getAddress();

  bool isBonded();
  short getRSSI();
  bool isConnected();

  bool pair();
  bool unPair();

  std::optional<DBusError> call(std::string functionName);

  bool verifyProximity();
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

  static DBusHandlerResult methodCallHandler(DBusConnection * connection, DBusMessage * message, void * controller);

  bool pendingDevicesUpdate;
  std::function<void()> onDevicesUpdated;

  void registerForSignals();

  std::vector<Device> devices;

  void registerAgent();
  void unregisterAgent();

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
