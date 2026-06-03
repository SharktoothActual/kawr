//kawr v0.1
//updated 6/2/26

//headers
//----------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dbus/dbus.h>

//variables
//----------
typedef enum {
    MODE_UNKNOWN,
    MODE_GRAPHICAL,
    MODE_CONSOLE
} InhibitMode;

//functions
//----------
//command_exists - a function to detect available command
int command_exists(const char *cmd) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "command -v %s >/dev/null 2>&1", cmd);
    return (system(buffer) == 0);
}

//auto_detect_mode - find a suitable resource
InhibitMode auto_detect_mode(void) {
    char *dbus_address = getenv("DBUS_SESSION_BUS_ADDRESS");
    if (dbus_address != NULL && strlen(dbus_address) > 0) {
        return MODE_GRAPHICAL;
    }
    char *term_env = getenv("TERM");
    if (term_env != NULL && strcmp(term_env, "linux") == 0) {
        if (command_exists("setterm") && isatty(0)) {
            return MODE_CONSOLE;
        }
    }
    return MODE_UNKNOWN;
}

//handle stateful dbus inhibition
uint32_t set_graphical_inhibit(int execute_lock) {
    DBusError err;
    DBusConnection *conn;
    DBusMessage *msg;
    DBusMessageIter args;
    uint32_t cookie =0;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set (&err)) {
        dbus_error_free(&err);
        return 0;
    }
    if (execute_lock) {
        msg = dbus_message_new_method_call("org.freedesktop.ScreenSaver",
                                           "/org/freedesktop/ScreenSaver",
                                           "org.freedesktop.ScreenSaver",
                                           "Inhibit");
        const char *app_name = "kawr";
        const char *reason = "Running Target App";
        dbus_message_iter_init_append(msg, &args);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &app_name);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reason);
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
        dbus_message_unref(msg);
        if (reply && dbus_message_get_args(reply, &err, DBUS_TYPE_UINT32, &cookie, DBUS_TYPE_INVALID)) {
            dbus_message_unref(reply);
        }
    }
    return cookie;
}

//main
//----------
//set the default state to be unknown (as we haven't checked it yet)
int main(int argc, char *argv[]) {
    InhibitMode mode = MODE_UNKNOWN;
    int target_start_index = 1;

    //check for arguments and override output to match
    if (argc > 1) {
        if (strcmp(argv[1], "-g") == 0 || strcmp(argv[1], "--graphical") == 0) {
            mode = MODE_GRAPHICAL;
            target_start_index = 2;
        }
        else if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "--terminal") == 0 ||
                 strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--console") == 0) {
            mode = MODE_CONSOLE;
            target_start_index = 2;
        }
    }

    //privilege guard
    char *sudo_user = getenv("SUDO_USER");
    char *doas_user = getenv("DOAS_USER");
    if (sudo_user != NULL || doas_user != NULL) {
        fprintf(stderr, "You cannot run kawr using sudo or doas, as this can break your graphical display bus.\n");
        fprintf(stderr, "Correct usage: kawr %s %s\n",
                (sudo_user != NULL) ? "sudo" : "doas",
                argv[target_start_index]);
        return 1;
    }

    //auto-detect display manager if not manually set
    if (mode == MODE_UNKNOWN) {
        mode = auto_detect_mode();
    }

    //ERROR HANDLER: auto_detect_mode fails to detect anything
    if (mode == MODE_UNKNOWN) {
        char *term_env = getenv("TERM");
        if (term_env == NULL || strlen(term_env) == 0) {
            fprintf(stderr, "ERROR: Your environment doesn't appear to have a terminal context.\n");
        }
        else if (isatty(0) == 0 ) {
            fprintf(stderr, "Don't run kawr as root. Place `kawr` in front of `sudo`/`doas`.\n");
        }
        else {
            fprintf(stderr, "No active display server or TTY found.\n");
        }
        return 1;
    }

    //ERROR HANDLER: no program specified
    if (target_start_index >= argc) {
        fprintf(stderr, "Specify a program to run.\nUsage: kawr [options] {target program}\n");
        return 1;
    }

    //memory allocation
    int total_length = 0;
    for (int i = target_start_index; i < argc; i++) {
        total_length += strlen(argv[i]) + 1; 
    }
    char *target_program_string = malloc(total_length);
    if (target_program_string == NULL) {
        //ERROR HANDLER: out of memory
        fprintf(stderr, "Not enough RAM available.\n");
        return 1;
    }
    target_program_string[0] = '\0';
    for (int i = target_start_index; i < argc; i++) {
        strcat(target_program_string, argv[i]);
        if (i < argc - 1) {
            strcat(target_program_string, " "); 
        }
    }

    //find misplaced flags
    int misplaced_flag_found = 0;
    if (strstr(target_program_string, "-g") != NULL || strstr(target_program_string, "--graphical") != NULL ||
        strstr(target_program_string, "-t") != NULL || strstr(target_program_string, "--terminal") != NULL ||
        strstr(target_program_string, "-c") != NULL || strstr(target_program_string, "--console") != NULL) {
        misplaced_flag_found = 1;
    }

    //if no misplaced flags, confirm init
    printf("Inhibiting system while (%s) is running.\n", argv[target_start_index]);

    //WARNING HANDLER: misplaced flag found
    if (misplaced_flag_found) {
        char *suggested_flag = "-g";
        char *mode_name = "Graphical";
        if (mode == MODE_CONSOLE) {
            suggested_flag = "-c";
            mode_name = "Console";
        }
        printf ("INFO: If you meant to run kawr in (%s) mode, close %s and run: \"kawr %s %s\"\n",
                mode_name, argv[target_start_index], suggested_flag, argv[target_start_index]);
    }

    //inhibit
    int execution_status = 0;
    uint32_t dbus_cookie =0;
    switch (mode) {
        case MODE_GRAPHICAL: {
            dbus_cookie = set_graphical_inhibit(1);
            execution_status = system(target_program_string);
            break;
        }
        case MODE_CONSOLE: 
            system("setterm -blank 0 -powerdown 0");
            execution_status = system(target_program_string);
            break;
        
        default:
            break;
    }

    //exiting
    if (mode == MODE_CONSOLE) {
        system("setterm -blank 10 -powerdown 10");
    }     
    
    free(target_program_string);
    return WEXITSTATUS(execution_status);
}