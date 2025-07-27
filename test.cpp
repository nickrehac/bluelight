#include <dbus/dbus.h>
#include <iostream>

bool doErr(void* handle, DBusError e) {
  if(handle == nullptr) {
    std::cerr << e.name << '\n';
    std::cerr << e.message << std::endl;
    return true;
  }
  return false;
}

class Device {
  std::string objectPath;
}


int main() {
  DBusConnection * conn;
  DBusError err;
  DBusMessage * msg;
  DBusMessage * reply;
  int numObjects;
  const char * objName;
  int curType = DBUS_TYPE_INVALID;

  dbus_error_init(&err);

  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if(doErr(conn, err)) goto fail;

  msg = dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
  if(doErr(msg, err)) goto fail;

  reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_USE_DEFAULT, &err);
  if(doErr(reply, err)) goto fail;

  DBusMessageIter msgIter;

  dbus_message_iter_init(reply, &msgIter);
  dbus_message_iter_recurse(&msgIter, &msgIter);

  while((curType = dbus_message_iter_get_arg_type(&msgIter)) != DBUS_TYPE_INVALID) {
    if(curType == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter entry;
      dbus_message_iter_recurse(&msgIter, &entry);
      dbus_message_iter_get_basic(&entry, &objName);
      std::cout << "found object: " << objName << '\n';
    } else {
      std::cout << "type found: " << (char) curType << '\n';
    }
    dbus_message_iter_next(&msgIter);
  }

  std::cout << dbus_bus_get_unique_name(conn) << '\n';


fail:
  if(conn != nullptr) dbus_connection_unref(conn);
  if(msg != nullptr) dbus_message_unref(msg);
  if(reply != nullptr) dbus_message_unref(reply);
}
