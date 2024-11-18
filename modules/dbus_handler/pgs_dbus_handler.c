#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <dbus/dbus.h>
#include "pgs_modules.h"
#include "pgs_dbus_handler.h"
#include "pgs_utils.h"

static char * dbus_name;
static char * dbus_path;
static char * dbus_intf;

static void check_and_abort(DBusError * error);
static void respond_to_introspect(DBusConnection * connection, DBusMessage * request);
static void respond_to_states(DBusConnection * connection, DBusMessage * request);
static DBusHandlerResult messages_handler(DBusConnection * connection, DBusMessage * message, void * user_data);

static void respond_to_states(DBusConnection * connection, DBusMessage * request)
{
    int pid    = 0;
    int states = 1;
    DBusError error;
    DBusMessage * reply;

    dbus_error_init(&error);

    printf("Respond to %s\n", dbus_name);

    dbus_message_get_args(request, &error, DBUS_TYPE_INT32, &states, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);
    if(dbus_error_is_set(&error)) {
        reply = dbus_message_new_error(request, "wrong_arguments", "Illegal arguments to Sum");
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        return;
    }

    pgs_dispatch_dbus_message(states);

    if(pid != 0) {
        printf("Kill %d by %s\n", pid, dbus_name);
        kill(pid, SIGKILL);
    }

    // if(!strcmp(dbus_name, PGS_DBUS_MENU_NAME)) {
    //     if(states == 0) {
    //         pgs_lvgl_suspend();
    //         exit(0);
    //     }

    //     if(pid != 0) {
    //         printf("Kill %d by %s\n", pid, dbus_name);
    //         kill(pid, SIGKILL);
    //     }
    // }
}

static void check_and_abort(DBusError * error)
{
    if(dbus_error_is_set(error)) {
        puts(error->message);
        abort();
    }
}

static void respond_to_introspect(DBusConnection * connection, DBusMessage * request)
{
    DBusMessage * reply;
    char introspection_data[1024];
    sprintf(introspection_data,
            " <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
            " <!-- dbus-sharp 0.8.1 -->"
            " <node>"
            "   <interface name=\"org.freedesktop.DBus.Introspectable\">"
            "     <method name=\"Introspect\">"
            "       <arg name=\"data\" direction=\"out\" type=\"s\" />"
            "     </method>"
            "   </interface>"
            "   <interface name=\"%s\">"
            "     <method name=\"states\">"
            "       <arg name=\"states\" direction=\"in\" type=\"i\" />"
            "       <arg name=\"pid\" direction=\"in\" type=\"i\" />"
            "     </method>"
            "     <property name=\"Status\" type=\"u\" access=\"readwrite\"/>"
            "   </interface>"
            " </node>",
            dbus_name);

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_data, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
}

void pgs_dbus_method_call(const char * name, const char * path, const int states, const int pid)
{
    DBusConnection * connection;
    DBusError error;
    DBusMessage * message;
    const char * intf   = name;
    const char * method = "States";

    printf("Call from %s to %s %s %s kill %d\n", dbus_name, name, method, states ? "resume" : "suspend", pid);

    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

    if(dbus_error_is_set(&error)) {
        printf("Getting dbus connection: %s\n", error.message);
        dbus_error_free(&error);
        dbus_connection_unref(connection);
        return;
    }

    message = dbus_message_new_method_call(name, path, intf, method);
    if(message == NULL) {
        printf("New dbus message NULL\n");
        return;
    }

    /* append the argument to the message */
    dbus_message_append_args(message, DBUS_TYPE_INT32, &states, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

    /* We don't want to receive a reply */
    dbus_message_set_no_reply(message, TRUE);

    if(!dbus_connection_send(connection, message, NULL)) {
        printf("Send dbus message of memory\n");
        exit(-1);
    }

    dbus_message_unref(message);
    dbus_connection_unref(connection);
}

static DBusHandlerResult messages_handler(DBusConnection * connection, DBusMessage * message, void * user_data)
{
    const char * intf   = dbus_message_get_interface(message);
    const char * member = dbus_message_get_member(message);

    if(0 == strcmp("org.freedesktop.DBus.Introspectable", intf) && 0 == strcmp("Introspect", member)) {
        respond_to_introspect(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;

    } else if(0 == strcmp(dbus_intf, intf) && 0 == strcmp("States", member)) {
        respond_to_states(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;

    } else {

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;
int flag              = 0;

static void * dbus_dispatch_thread_func(void * param)
{
    if(strcmp(dbus_name, PGS_DBUS_MENU_NAME)) {
        pgs_dispatch_dbus_message(0);
        pthread_mutex_lock(&mutex);
        flag = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    while(1) {
        dbus_connection_read_write_dispatch((DBusConnection *)param, 1000); /* 0 to wait forever */
    }
    return NULL;
}

void pgs_dbus_handler_init(const char * name, const char * path)
{
    pthread_t tid;
    DBusError error;
    DBusConnection * connection;
    DBusObjectPathVTable vtable;

    dbus_name = name;
    dbus_path = path;
    dbus_intf = name; /* same as name */

    vtable.unregister_function = NULL;
    vtable.message_function    = messages_handler;

    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    check_and_abort(&error);

    dbus_bus_request_name(connection, dbus_name, 0, &error);
    check_and_abort(&error);

    dbus_connection_try_register_object_path(connection, dbus_path, &vtable, NULL, &error);
    check_and_abort(&error);

    pthread_create(&tid, NULL, dbus_dispatch_thread_func, connection);

    if(strcmp(dbus_name, PGS_DBUS_MENU_NAME)) {
        pthread_mutex_lock(&mutex);
        while(flag == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
}
