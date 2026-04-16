#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#define MAX_NAME 50
#define MAX_CAT 30
#define MAX_DESC 256

// Struct pentru report

typedef struct {
    int id;
    char inspector_name[MAX_NAME];
    float latitude;  // pentru gps
    float longitude; // pentru gps
    char category[MAX_CAT]; // issue category
    int severity; // 1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char description[MAX_DESC];
} Report;

char role[20];
char user[50];
char command[20];
char district[50];
char variabila_extra[100]; // pentru comenzile care au nevoie de argument in plus

// functie pentru citirea argumentelor

void get_argv(int argc, char *argv[]) {

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            strcpy(role, argv[i + 1]);
        }

        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            strcpy(user, argv[i + 1]);
        }

        // comenzile add si list nu au nevoie de argument extra

        else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            strcpy(command, "add");
            strcpy(district, argv[i + 1]);
        }

        else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            strcpy(command, "list");
            strcpy(district, argv[i + 1]);
        }

        // comenzile view, remove report, update threshold, filter au nevoie de argument

        else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            strcpy(command, "view");
            strcpy(district, argv[i + 1]);
            strcpy(variabila_extra, argv[i + 2]);
        }

        else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            strcpy(command, "remove_report");
            strcpy(district, argv[i + 1]);
            strcpy(variabila_extra, argv[i + 2]);
        }

        else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            strcpy(command, "update_threshold");
            strcpy(district, argv[i + 1]);
            strcpy(variabila_extra, argv[i + 2]);
        }

        else if (strcmp(argv[i], "--filter") == 0 && i + 2 < argc) {
            strcpy(command, "filter");
            strcpy(district, argv[i + 1]);
            strcpy(variabila_extra, argv[i + 2]);
        }
    }
}

// logica comenzilor

void do_district() {};

void do_add(){};

void do_list(){};

void do_view(){};

void do_remove_report(){};

void do_update_threshold(){};

void do_filter(){};



int main(int argc, char *argv[]) {

    get_argv(argc, argv);

    if (strlen(role) == 0 || strlen(command) == 0 || strlen(district) == 0) {
        printf("Error at the arguments\n");
        return -1;
    }

    do_district();

    if (strcmp(command, "add") == 0) {
        do_add();
    }

    else if (strcmp(command, "list") == 0) {
        do_list();
    }

    else if (strcmp(command, "view") == 0) {
        do_view();
    }

    else if (strcmp(command, "remove_report") == 0) {
        do_remove_report();
    }

    else if (strcmp(command, "update_threshold") == 0) {
        do_update_threshold();
    }

    else if (strcmp(command, "filter") == 0) {
        do_filter();
    }

    else {

        printf("Invalid Command : %s\n", command);
    }

    return 0;
}
