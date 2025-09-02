#include "bluelight.hpp"


DBusHandlerResult BluetoothController::incomingMessageHandler(DBusConnection * connection, DBusMessage * message, void * userData) {
  int type = dbus_message_get_type(message);

  if(type == DBUS_MESSAGE_TYPE_SIGNAL) return signalHandler(connection, message, userData);

  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


DBusHandlerResult BluetoothController::signalHandler(DBusConnection * connection, DBusMessage * message, void * userData) {
  DBusError err;
  dbus_error_init(&err);

  BluetoothController * controller = static_cast<BluetoothController*>(userData);

  std::string method = dbus_message_get_member(message);

  if(!method.compare("InterfacesAdded")) {
    controller->updateDevices();
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  if(!method.compare("InterfacesRemoved")) {
    controller->updateDevices();
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


void BluetoothController::registerForSignals() {
  DBusError err;
  dbus_error_init(&err);
  dbus_bus_add_match(connection, SIGNAL_MATCH_RULES, &err);
}


BluetoothController::BluetoothController() {
  DBusError err;
  dbus_error_init(&err);
  
  connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if(!connection) fail(err);

  onDevicesUpdated = nullptr;

  DBusObjectPathVTable vtable = {
    .message_function = BluetoothController::incomingMessageHandler
  };
  dbus_connection_register_object_path(connection, "/", &vtable, this);

  dbus_connection_set_watch_functions(connection, addWatchFunction, removeWatchFunction, NULL, &watches, freeWatchFunction);

  registerForSignals();
}


unsigned int BluetoothController::addWatchFunction(DBusWatch * watch, void * data) {
  std::vector<DBusWatch*> * watches = static_cast<std::vector<DBusWatch*>*>(data);
  watches->push_back(watch);
  return true;
}

void BluetoothController::removeWatchFunction(DBusWatch * watch, void * data) {
  std::vector<DBusWatch*> * watches = static_cast<std::vector<DBusWatch*>*>(data);
  for(int i = 0; i < watches->size(); i++) {
    if((*watches)[i] == watch) {
      watches->erase(watches->begin() + i);
      return;
    }
  }
}

void BluetoothController::freeWatchFunction(void* memory){}


void BluetoothController::pollWatches() {
  for(DBusWatch * watch : watches) {
    if(dbus_watch_get_enabled(watch)) {
      int fd = dbus_watch_get_unix_fd(watch);
      int flags = dbus_watch_get_flags(watch);
      pollfd pollResults{
        fd,
        POLLIN | POLLOUT,
      };
      ::poll(&pollResults, 1, 0);

      if(flags & DBUS_WATCH_READABLE && pollResults.revents && POLLIN) {
        dbus_watch_handle(watch, DBUS_WATCH_READABLE);
      }
      if(flags & DBUS_WATCH_WRITABLE && pollResults.revents && POLLOUT) {
        dbus_watch_handle(watch, DBUS_WATCH_WRITABLE);
      }
    }
  }
}


BluetoothController::~BluetoothController() {
  if(connection) {
    dbus_connection_unregister_object_path(connection, "/");
    dbus_connection_unref(connection);
  }
}


void BluetoothController::updateDevices() {
  //TODO: sort by rssi (paired first)
  std::vector<Device> newDevices;

  DBusMessage * query;
  DBusMessage * reply;
  DBusError err;
  dbus_error_init(&err);

  query = dbus_message_new_method_call(BT_SERVICE, "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

  reply = dbus_connection_send_with_reply_and_block(connection, query, DBUS_TIMEOUT_USE_DEFAULT, &err);
  if(!reply) {
    dbus_message_unref(query);
    dbus_message_unref(reply);
    return;
  }

  DBusMessageIter objects;

  dbus_message_iter_init(reply, &objects);
  dbus_message_iter_recurse(&objects, &objects);

  int curType = 0;

  while((curType = dbus_message_iter_get_arg_type(&objects)) != DBUS_TYPE_INVALID) {
    DBusMessageIter object;
    dbus_message_iter_recurse(&objects, &object);

    const char * path_cstr;

    dbus_message_iter_get_basic(&object, &path_cstr);

    std::string path(path_cstr);

    if(path.compare(0, strlen(DEVICES_PATH), DEVICES_PATH) == 0) {//if device
      newDevices.push_back(Device(path, connection));
    }

    dbus_message_iter_next(&objects);
  }

  dbus_message_unref(query);
  dbus_message_unref(reply);

  devices = newDevices;

  std::sort(devices.begin(), devices.end(), [](Device a, Device b){
      return a.isBonded();
  });

  if(onDevicesUpdated) onDevicesUpdated();
}


std::vector<Device> BluetoothController::getDevices() {
  return devices;
}


void BluetoothController::setOnDevicesUpdated(std::function<void()> callback) {
  onDevicesUpdated = callback;
}


void BluetoothController::startDiscovery() {
  DBusMessage * msg;
  DBusError err;
  dbus_error_init(&err);

  msg = dbus_message_new_method_call(BT_SERVICE, ADAPTER_PATH, "org.bluez.Adapter1", "StartDiscovery");

  if(!dbus_connection_send_with_reply_and_block(connection, msg, -1, &err)) {
    fail(err);
  }


  dbus_message_unref(msg);
}


void BluetoothController::stopDiscovery() {
  DBusMessage * msg;
  DBusError err;
  dbus_error_init(&err);

  msg = dbus_message_new_method_call(BT_SERVICE, ADAPTER_PATH, "org.bluez.Adapter1", "StartDiscovery");

  dbus_connection_send(connection, msg, nullptr);
  dbus_connection_flush(connection);

  dbus_message_unref(msg);
}


void BluetoothController::dispatch() {
  dbus_connection_read_write_dispatch(connection, -1);
}


void BluetoothController::poll() {
  pollWatches();

  int status;
  while((status = dbus_connection_get_dispatch_status(connection) == DBUS_DISPATCH_DATA_REMAINS))
      dbus_connection_dispatch(connection);
}


void fail(DBusError e) {
  std::cout << e.name;
  std::cout << e.message;
  exit(1);
}


Device::Device(std::string path, DBusConnection * connection) {
  this->connection = connection;
  this->path = path;
  alias = "NO_NAME";
  connected = false;
  bonded = false;
  rssi = 0;

  auto deviceAlias = getString("Alias");
  auto deviceConnected = getBool("Connected");
  auto deviceBonded = getBool("Bonded");
  auto deviceAddress = getString("Address");
  auto deviceRSSI = getShort("RSSI");

  if(deviceAlias) alias = *deviceAlias;

  if(deviceConnected) connected = *deviceConnected;

  if(deviceBonded) bonded = *deviceBonded;

  if(deviceAddress) address = *deviceAddress;

  if(deviceRSSI) rssi = *deviceRSSI;
}


std::optional<std::string> Device::getString(std::string property) {
  const char * interface = "org.bluez.Device1";
  const char * propertyCStr = property.c_str();
  std::optional<std::string> retval;


  DBusMessage * msg = dbus_message_new_method_call(BT_SERVICE, path.c_str(), "org.freedesktop.DBus.Properties", "Get");

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &propertyCStr, DBUS_TYPE_INVALID);


  DBusError err;
  dbus_error_init(&err);
  DBusMessage * reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &err);

  if(!reply) return retval;

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse(&iter, &iter);

  const char * result;

  if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
    dbus_message_iter_get_basic(&iter, &result);
    retval = std::string(result);
  }

  dbus_message_unref(reply);
  dbus_message_unref(msg);

  return retval;
}

std::optional<short> Device::getShort(std::string property) {
  const char * interface = "org.bluez.Device1";
  const char * propertyCStr = property.c_str();
  std::optional<bool> retval;

  DBusMessage * msg = dbus_message_new_method_call(BT_SERVICE, path.c_str(), "org.freedesktop.DBus.Properties", "Get");

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &propertyCStr, DBUS_TYPE_INVALID);


  DBusError err;
  dbus_error_init(&err);
  DBusMessage * reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &err);

  if(!reply) return retval;

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse(&iter, &iter);

  if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT16) {
    int result = 0;
    dbus_message_iter_get_basic(&iter, &result);
    retval = result;
  }

  dbus_message_unref(reply);
  dbus_message_unref(msg);

  return retval;
}

std::optional<bool> Device::getBool(std::string property) {
  const char * interface = "org.bluez.Device1";
  const char * propertyCStr = property.c_str();
  std::optional<bool> retval;

  DBusMessage * msg = dbus_message_new_method_call(BT_SERVICE, path.c_str(), "org.freedesktop.DBus.Properties", "Get");

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &propertyCStr, DBUS_TYPE_INVALID);


  DBusError err;
  dbus_error_init(&err);
  DBusMessage * reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &err);

  if(!reply) return retval;

  DBusMessageIter iter;
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse(&iter, &iter);


  if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_BOOLEAN) {
    bool result = 0;
    dbus_message_iter_get_basic(&iter, &result);
    retval = result;
  }

  dbus_message_unref(reply);
  dbus_message_unref(msg);

  return retval;
}


std::string Device::getAlias() {
  return alias;
}


bool Device::isBonded() {
  return bonded;
}

bool Device::isConnected() {
  return connected;
}

short Device::getRSSI() {
  return rssi;
}

std::optional<DBusError> Device::call(std::string functionName) {
  DBusMessage * msg = dbus_message_new_method_call(BT_SERVICE, path.c_str(), "org.bluez.Device1", "functionName");

  DBusError err;
  dbus_error_init(&err);
  DBusMessage * reply = dbus_connection_send_with_reply_and_block(connection, msg, -1, &err);

  std::optional<DBusError> retval;

  if(!reply) {
    retval = err;
  }

  dbus_message_unref(reply);
  dbus_message_unref(msg);
  return retval;
}

bool Device::verifyProximity() {
  if(connected) {
    auto deviceConnected = getBool("Connected");
    if(deviceConnected) connected = *deviceConnected;
    if(connected) return true;
  }

  if(!bonded) return false;

  auto connectionStatus = call("Connect");

  if(connectionStatus) {
    const char * err = connectionStatus.value().name;
    if(strcmp(err, "org.bluez.Error.AlreadyConnected")) return true;
    return false;
  } else {
    call("Disconnect");
    return true;
  }
}

