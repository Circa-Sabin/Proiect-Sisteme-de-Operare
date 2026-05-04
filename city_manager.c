#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <time.h>


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

        else if (strcmp(argv[i], "--remove_district") == 0 && i + 1 < argc) {
            strcpy(command, "remove_district");
            strcpy(district, argv[i + 1]);
        }
    }
}

// logica comenzilor

void check_symlinks() {
    // Deschidem folderul curent (".")
    DIR *dir = opendir(".");
    if (dir == NULL) {
        return;
    }

    struct dirent *entry;
    
    // Parcurgem toate fisierele din folderul curent
    while ((entry = readdir(dir)) != NULL) {
        
        // Cautam doar fisierele care incep cu "active_reports-"
        if (strncmp(entry->d_name, "active_reports-", 15) == 0) {
            
            struct stat sym_stat;
            // PDF cere specific lstat() pentru a examina link-ul insusi, nu fisierul la care arata
            if (lstat(entry->d_name, &sym_stat) == 0) {
                
                // Verificam daca este intr-adevar un symlink (folosind macroul S_ISLNK)
                if (S_ISLNK(sym_stat.st_mode)) {
                    
                    // Acum incercam sa folosim stat() normal pentru a vedea daca tinta mai exista
                    struct stat target_stat;
                    if (stat(entry->d_name, &target_stat) == -1) {
                        // stat() da rateu daca fisierul tinta nu mai exista
                        printf("Avertisment: Link-ul simbolic '%s' este 'dangling' (fisierul tinta lipseste).\n", entry->d_name);
                        
                        // Oprional: Daca vrei sa fii extra ordonat, poti sa si stergi link-ul mort:
                        // unlink(entry->d_name);
                    }
                }
            }
        }
    }
    
    closedir(dir);
}

void do_district() {

    mkdir(district, 0750);
    chmod(district, 0750);

    char filepath[150];

    // partea pentru reports.dat

    sprintf(filepath, "%s/reports.dat", district);
    int fd_reports = open(filepath, O_CREAT | O_RDWR, 0664);
    if (fd_reports != -1) {
        close(fd_reports);
    }

    chmod(filepath, 0664);

    // partea pentru district.cfg

    sprintf(filepath, "%s/district.cfg", district);
    int fd_cfg = open(filepath, O_CREAT | O_RDWR, 0640);
    if (fd_cfg != -1) {
        close(fd_cfg);
    }
    chmod(filepath, 0640);

    // partea pentru logged_district

    sprintf(filepath, "%s/logged_district", district);
    int fd_log = open(filepath, O_CREAT | O_RDWR, 0644);
    if (fd_log != -1) {
        close(fd_log);
    }
    chmod(filepath, 0644);

    char symlink_name[150];
    sprintf(symlink_name, "active_reports-%s", district);

    char target_path[150];
    sprintf(target_path, "%s/reports.dat", district);

    if(symlink(target_path, symlink_name) == -1) {
        if (errno != EEXIST) {
            perror("Eroare la crearea symlink-ului!");
        }
    }

};

void do_add(){

    Report report;

    memset(&report, 0, sizeof(Report)); // curatare memorie

    printf("Introduceti latitudinea X: ");
    scanf("%f", &report.latitude);

    printf("Introduceti longitudinea Y: ");
    scanf("%f", &report.longitude);

    printf("Introduceti categoria problemei: ");
    scanf("%s", report.category);

    printf("Introduceti severitatea (1 = minor, 2 = moderate, 3 = critical): ");
    scanf("%d", &report.severity);

    printf("Introduceti o descriere a problemei: ");
    scanf(" %[^\n]", report.description);

    strcpy(report.inspector_name, user);
    report.timestamp = time(NULL);

    char filepath[150];
    sprintf(filepath, "%s/reports.dat", district);
    int fd = open(filepath, O_WRONLY | O_APPEND); // O_APPEND pentru a adauga la finalul fisierului

    if(fd == -1) {
        perror("Eroare la deschiderea fisierului reports.dat!\n");
        return;
    }

    int marime_fisier = lseek(fd, 0, SEEK_END); // lseek arata marimea totala a fisierului in bytes
    report.id = marime_fisier / sizeof(Report) + 1;

    write(fd, &report, sizeof(Report));
    close(fd);

    printf("\nRaport cu ID %d a fost adaugat cu succes in districtul '%s'!\n", report.id, district);
};

void printeaza_bucata(int cifra) {
    if (cifra == 0) printf("---");
    else if (cifra == 1) printf("--x");
    else if (cifra == 2) printf("-w-");
    else if (cifra == 3) printf("-wx");
    else if (cifra == 4) printf("r--");
    else if (cifra == 5) printf("r-x");
    else if (cifra == 6) printf("rw-");
    else if (cifra == 7) printf("rwx");
}

void print_permissions(mode_t mode) {

    int perm_user = (mode / 64) % 8; // extrage permisiunile pentru user
    int perm_group = (mode / 8) % 8; // extrage perm
    int perm_other = mode % 8; // extrage permisiunile pentru others

    printeaza_bucata(perm_user);
    printeaza_bucata(perm_group);
    printeaza_bucata(perm_other);
}

void do_list(){

    char filepath[150];
    sprintf(filepath, "%s/reports.dat", district); // construieste calea

    struct stat st;
    if (stat(filepath, &st) == -1) {
        perror("Nu am gasit fisierul reports.dat!\n");
        return;
    }

    printf("\n INFO FISIER \n");
    printf("Permisiuni: ");
    print_permissions(st.st_mode);

    printf("\nMarime: %ld bytes\n", st.st_size);
    printf("Ultima modificare: %s", ctime(&st.st_mtime));
    printf("\n\n");

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului reports.dat!\n");
        return;
    }

    Report report;
    int numar_rapoarte = 0;

    printf("Lista Rapoarte \n");

    while(read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        numar_rapoarte++;
        printf("ID: %d | Inspector: %s | Severitate: %d\n", report.id, report.inspector_name, report.severity);
        printf("GPS: %.2f, %.2f | Categoria: %s\n", report.latitude, report.longitude, report.category);
        printf("Descriere: %s\n", report.description);
        printf("\n");
    }

    if(numar_rapoarte == 0) {
        printf("Nu exista rapoarte in districtul s%s.\n", district);
    }

    close(fd);

};

void do_view(){

    int target_id = atoi(variabila_extra); // converteste argumentul in int

    char filepath[150];
    sprintf(filepath, "%s/reports.dat", district); // construieste calea

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("Nu am gasit fisierul reports.dat in districtul %s!\n", district);
        return;
    }

    Report report;

    int raport_gasit = 0;

    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {

        if(report.id == target_id){
            raport_gasit = 1;


            printf("\n DETALII RAPORT ID %d\n", report.id);
            printf("Inspector: %s\n", report.inspector_name);
            printf("Data si ora: %s", ctime(&report.timestamp));
            printf("GPS: %.2f, %.2f\n", report.latitude, report.longitude);
            printf("Categoria: %s\n", report.category);
            printf("Severitate: %d\n", report.severity);
            printf("Descriere: %s\n", report.description);
            printf("\n");
            break;

        }
    }

    if (raport_gasit == 0){
        printf("Nu am gasit raportul cu ID %d in districtul %s!\n", target_id, district);
    }

    close(fd);
}

void do_remove_report(){

    int target_id = atoi(variabila_extra);

    printf("Incerc sa sterg raportul %d\n", target_id);


    char filepath[150];

    sprintf(filepath, "%s/reports.dat", district);

    int fd= open(filepath, O_RDWR);
    if (fd == -1) {
        perror("Nu am gasit fisierul vechi!\n");
        return;
    }

    Report r;

    int gasit = 0;

    off_t pozitie_gasit = 0; //salvam locul exact unde e raportul pe care l vrem sters

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == target_id) {
            gasit = 1;
            pozitie_gasit = lseek(fd,0,SEEK_CUR) - sizeof(Report);
            break;
        }
    }
    if (gasit == 0) {
        printf("Nu am gasit raportul %d!\n", target_id);
        close(fd);
        return;
    }

    off_t read_cursor = lseek(fd,0,SEEK_CUR);

    off_t write_cursor = pozitie_gasit;

    while (1) {
        lseek(fd,read_cursor,SEEK_SET);
        if (read(fd,&r,sizeof(Report)) < sizeof(Report)) {
            break;
        }
        lseek(fd,write_cursor,SEEK_SET);
        write(fd,&r,sizeof(Report));

        read_cursor += sizeof(Report);
        write_cursor += sizeof(Report);
    }

    ftruncate(fd,write_cursor);
    close(fd);
    printf("Raportul cu id %d a fost sters cu succes\n", target_id);
};

void do_update_threshold() {
    // Verificam rolul: doar managerul are voie!
    if (strcmp(role, "manager") != 0) {
        printf("Eroare: Doar utilizatorii cu rolul 'manager' pot actualiza pragul!\n");
        return;
    }

    char filepath[150];
    sprintf(filepath, "%s/district.cfg", district); // Scriem in district.cfg, nu in threshold.dat

    // Verificam permisiunile cu stat()
    struct stat st;
    if (stat(filepath, &st) == -1) {
        printf("Eroare: Nu am putut gasi fisierul %s.\n", filepath);
        return;
    }

    // Extragem doar bitii de permisiune (ultimii 9 biti)
    mode_t permisiuni = st.st_mode & 0777;

    if (permisiuni != 0640) {
        // Daca cineva a umblat la permisiuni, refuzam si printam un diagnostic (cum cere PDF-ul)
        printf("Diagnostic: Permisiunile fisierului %s au fost modificate (sunt %o, ar trebui sa fie 640). Refuz actualizarea.\n", filepath, permisiuni);
        return;
    }

    // Luam noul prag
    int nou_prag = atoi(variabila_extra);

    // Deschidem fisierul DOAR pentru scriere, stergand ce era inainte (O_TRUNC)
    int fd = open(filepath, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        printf("Eroare: Nu am putut deschide %s pentru scriere.\n", filepath);
        return;
    }

    char buffer_text[150];
    sprintf(buffer_text, "%s\n", variabila_extra);

    write(fd, buffer_text, strlen(buffer_text));
    close(fd);

    printf("Pragul de severitate pentru %s a fost actualizat cu succes la %d.\n", district, nou_prag);
}

int parse_condition(const char *input, char *field, char *op, char * value) {
    int bucati_gasite = sscanf(input,"%[^:]:%[^:]:%s", field, op, value);
    if (bucati_gasite == 3) {
        return 1;
    }
    return 0;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    // Daca filtram dupa SEVERITATE (int)
    if (strcmp(field, "severity") == 0) {
        int val_cautata = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val_cautata;
        if (strcmp(op, "!=") == 0) return r->severity != val_cautata;
        if (strcmp(op, "<") == 0) return r->severity < val_cautata;
        if (strcmp(op, "<=") == 0) return r->severity <= val_cautata;
        if (strcmp(op, ">") == 0) return r->severity > val_cautata;
        if (strcmp(op, ">=") == 0) return r->severity >= val_cautata;
    }
    // Daca filtram dupa CATEGORIE (string)
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    // Daca filtram dupa INSPECTOR (string)
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector_name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector_name, value) != 0;
    }

    return 0; // Nu s-a gasit campul sau operatorul
}

void do_filter() {
    char filepath[150];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("Eroare: Nu am gasit rapoarte in districtul %s.\n", district);
        return;
    }

    // Variabile pentru a stoca rezultatul spargerii conditiei
    char field[50], op[10], value[100];

    // Spargem sirul din "variabila_extra" folosind functia generata
    if (!parse_condition(variabila_extra, field, op, value)) {
        printf("Eroare: Conditia '%s' nu respecta formatul field:operator:value\n", variabila_extra);
        close(fd);
        return;
    }

    Report r;
    int rapoarte_gasite = 0;

    printf("\n=== RAPOARTE CARE RESPECTA CONDITIA: %s %s %s ===\n", field, op, value);

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {

        // Verificam folosind functia generata
        if (match_condition(&r, field, op, value) == 1) {
            printf("ID: %d | Severitate: %d | Categorie: %s | Inspector: %s\n",
            r.id, r.severity, r.category, r.inspector_name);
            rapoarte_gasite++;
        }
    }

    if (rapoarte_gasite == 0) {
        printf("Nu s-a gasit niciun raport care sa corespunda.\n");
    }
    printf("===================================================\n");

    close(fd);
}



void do_remove_district() {
    if (strcmp(role, "manager") != 0) {
        printf("Eroare: doar managerii");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Eroare: fork");
        return;
    }
    else if (pid == 0) {
        execlp("rm", "rm", "-rf", district, NULL);
        perror("Eroare: execlp");
        exit(1);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        printf("Districtul '%s' a fost sters cu succes\n", district);
    }
}

int main(int argc, char *argv[]) {

    get_argv(argc, argv);

    check_symlinks();

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

    else if (strcmp(command, "remove_district") == 0) {
        do_remove_district();
    }

    else {

        printf("Invalid Command : %s\n", command);
        return -1;
    }

    char log_filepath[150];
    sprintf(log_filepath, "%s/logged_district", district);

    struct stat st;

    if(stat(log_filepath, &st) != -1) {

        int are_voie_sa_scrie = 0;

        // Verificam strict permisiunile folosind MACROURI din st_mode, asa cum cere PDF-ul
        if (strcmp(role, "manager") == 0) {
            if (st.st_mode & S_IWUSR) { // S_IWUSR verifica daca Owner-ul (Managerul) are drept de scriere
                are_voie_sa_scrie = 1;
            }
        } 
        else if (strcmp(role, "inspector") == 0) {
            if (st.st_mode & S_IWGRP) { // S_IWGRP verifica daca Grupul (Inspectorul) are drept de scriere
                are_voie_sa_scrie = 1;
            }
        }

        // Daca nu are permisiunea, detectam si refuzam cu un mesaj clar
        if (are_voie_sa_scrie == 0) {
            printf("Avertisment permisiuni: Rolul '%s' nu are drept de scriere in logged_district!\n", role);
        } 
        else {
            // Daca are voie, deschidem si scriem (O_APPEND ca sa adaugam la final)
            int fd_log = open(log_filepath, O_WRONLY | O_APPEND);
            if (fd_log != -1) {
                char log_entry[300];
                // Construim textul log-ului: timestamp, user, rol comanda
                sprintf(log_entry, "%ld\n%s\n%s %s\n", time(NULL), user, role, command);
                
                write(fd_log, log_entry, strlen(log_entry));
                close(fd_log);
            }
        }

    }

    return 0;
}
