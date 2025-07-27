#include "bluelight.hpp"

BluetoothController::BluetoothController() {
  DBusError err;
  
  connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if(!connection) fail(err);
}

BluetoothController::getDevices() {
  std::vector<Device> devices;

  DBusMessage * query;
  DBusMessage * reply;
  DBusError err;

  query = dbus_message_new_method_call(BT_SERVICE, "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

  reply = dbus_connection_send_with_reply_and_block(connection, query, DBUS_TIMEOUT_USE_DEFAULT, &err);
  if(!reply) fail(err);

  DBusMessageIter objects;

  dbus_message_iter_init(reply, &objects);
  dbus_message_iter_recurse(&objects, &objects);

  int curType = 0;

  while((curType = dbus_message_iter_get_arg_type(objects)) != DBUS_TYPE_INVALID) {
    DBusMessageIter object;
    dbus_message_iter_recurse(&objects, &object);

    const char * path_cstr;

    dbus_message_iter_get_basic(&object, &path);

    std::string path(path_cstr);

    if(path.starts_with(ADAPTER_PATH)) {//if device
      devices.push_back(Device(path, connection));
    }
  }

  dbus_message_unref(query);
  dbus_message_unref(reply);

  return devices;
}

void BluetoothController::fail(DBusError e) {
  std::cout << e.name;
  std::cout << e.message;
  exit(1);
}

Device::Device(std::string path, DBusConnection * connection) {
  this->connection = connection;
  this->path = path;
}

int main() {
  
}
