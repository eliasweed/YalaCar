#include "func.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* =========================
   Internal helpers
   ========================= */
static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void create_empty_binary_file(const char *path) {
    FILE *f = fopen(path, "wb");
    if (f) fclose(f);
}

static const char* action_to_string(action_t a) {
    switch (a) {
        case ACT_LOGIN_SUCCESS:   return "LOGIN_SUCCESS";
        case ACT_LOGIN_FAIL:      return "LOGIN_FAIL";
        case ACT_LOGOUT:          return "LOGOUT";
        case ACT_SEARCH_CAR:      return "SEARCH_CAR";
        case ACT_ADD_CAR:         return "ADD_CAR";
        case ACT_UPDATE_CAR:      return "UPDATE_CAR";
        case ACT_DELETE_CAR:      return "DELETE_CAR";
        case ACT_SHOW_USERS:      return "SHOW_USERS";
        case ACT_ADD_USER:        return "ADD_USER";
        case ACT_UPDATE_PROFILE:  return "UPDATE_PROFILE";
        default:                  return "UNKNOWN";
    }
}

/* =========================
   System / Files (Ilya)
   ========================= */
void ensure_system_files_exist(void) {
    if (!file_exists(USERS_FILE)) create_empty_binary_file(USERS_FILE);
    if (!file_exists(CARS_FILE))  create_empty_binary_file(CARS_FILE);
    if (!file_exists(LOG_FILE)) { FILE *f = fopen(LOG_FILE, "w"); if (f) fclose(f); }
}

void ensure_admin_user_exists(void) {
    FILE *f = fopen(USERS_FILE, "rb");
    fseek(f, 0, SEEK_END);
    if (ftell(f) > 0) { fclose(f); return; }
    fclose(f);

    user_t admin = {"admin", "admin", 3, "System_Manager"};
    f = fopen(USERS_FILE, "ab");
    fwrite(&admin, sizeof(user_t), 1, f);
    fclose(f);
    log_action(&admin, ACT_ADD_USER, "Initial admin created");
}

/* =========================
   Authentication (Ilya)
   ========================= */
int login_flow(user_t *out_user) {
    int tries = 0;
    while (tries < LOGIN_MAX_TRIES) {
        char uname[MAX_USERNAME], pass[MAX_PASSWORD];
        read_line("\nUsername: ", uname, sizeof(uname));
        read_line("Password: ", pass, sizeof(pass));

        FILE *f = fopen(USERS_FILE, "rb");
        user_t tmp;
        int found = 0;
        while (fread(&tmp, sizeof(tmp), 1, f) == 1) {
            if (strcmp(tmp.username, uname) == 0 && strcmp(tmp.password, pass) == 0) {
                found = 1; break;
            }
        }
        fclose(f);
        if (found) { 
            *out_user = tmp; 
            log_action(out_user, ACT_LOGIN_SUCCESS, "Login success"); 
            return 1; 
        }
        tries++;
        log_action(NULL, ACT_LOGIN_FAIL, "Failed login attempt");
        printf("Invalid credentials. Tries left: %d\n", LOGIN_MAX_TRIES - tries);
    }
    return 0;
}

/* =========================
   Main Menu (Integrated)
   ========================= */
void run_main_menu(user_t *current_user) {
    while (1) {
        printf("\nWelcome %s | Level %d\n", current_user->fullname, current_user->level);
        printf("1) Search car\n2) Add car\n3) List all cars\n4) Update profile\n");
        if (current_user->level >= 2) printf("5) Update/Delete car\n");
        if (current_user->level == 3) printf("6) List users\n7) Add user\n");
        printf("0) Logout\nChoose: ");

        int choice = read_int("", 0, 7);
        if (choice == 0) { log_action(current_user, ACT_LOGOUT, "Logout"); break; }

        switch (choice) {
            case 1: cars_search_flow(current_user); break;
            case 2: cars_add_flow(current_user); break;
            case 3: cars_list_all_flow(current_user); break;
            case 4: change_personal_info(current_user); break;
            case 5: if(current_user->level >= 2) {
                        int s = read_int("Enter serial: ", 1, 1e9);
                        printf("1) Update 2) Delete: ");
                        if(read_int("", 1, 2) == 1) cars_update_by_serial(current_user, s);
                        else cars_delete_by_serial(current_user, s);
                    } break;
            case 6: if(current_user->level == 3) users_list_flow(current_user); break;
            case 7: if(current_user->level == 3) add_user(current_user); break;
        }
    }
}

/* =========================
   Car Management (Integrated)
   ========================= */
void print_car(const car_t *c) {
    printf("Serial: %d | Plate: %s | Model: %s | Price: %.2f\n", c->serial, c->plate, c->model, c->price);
    printf("Year: %d | Mileage: %d | Electric: %s\n", c->manufacture_date.year, c->mileage, c->is_electric ? "Yes" : "No");
}

static int cmp_car_serial(const void *a, const void *b) {
    return ((car_t*)a)->serial - ((car_t*)b)->serial;
}

void cars_add_flow(const user_t *current_user) {
    car_t c; memset(&c, 0, sizeof(c));
    c.serial = read_int("Serial: ", 1, 1e9);
    read_line("Model: ", c.model, MAX_MODEL);
    read_line("Plate: ", c.plate, MAX_PLATE);
    c.price = read_double("Price: ", 0, 1e12);
    c.manufacture_date = read_date("Manufacture date (dd mm yyyy): ");
    
    FILE *f = fopen(CARS_FILE, "rb");
    fseek(f, 0, SEEK_END);
    long n = ftell(f) / sizeof(car_t);
    car_t *arr = malloc((n + 1) * sizeof(car_t));
    fseek(f, 0, SEEK_SET);
    fread(arr, sizeof(car_t), n, f);
    fclose(f);

    arr[n] = c;
    qsort(arr, n + 1, sizeof(car_t), cmp_car_serial);

    f = fopen(CARS_FILE, "wb");
    fwrite(arr, sizeof(car_t), n + 1, f);
    fclose(f); free(arr);
    log_action(current_user, ACT_ADD_CAR, c.plate);
    printf("Car added successfully.\n");
}

/* =========================
   User Management (Eyal)
   ========================= */
void change_personal_info(user_t *User) {
    int choice, stay = 1;
    user_t temp;
    while (stay) {
        printf("\n1. Password 2. Name 3. Save: ");
        choice = read_int("", 1, 3);
        if (choice == 1) { read_line("New pass: ", User->password, MAX_PASSWORD); }
        else if (choice == 2) { read_line("New name: ", User->fullname, MAX_FULLNAME); }
        else stay = 0;
    }
    FILE* f = fopen(USERS_FILE, "rb+");
    while (fread(&temp, sizeof(user_t), 1, f)) {
        if (strcmp(temp.username, User->username) == 0) {
            fseek(f, -((long)sizeof(user_t)), SEEK_CUR);
            fwrite(User, sizeof(user_t), 1, f); break;
        }
    }
    fclose(f);
}

void add_user(user_t *currentUser) {
    user_t nu;
    read_line("Username: ", nu.username, MAX_USERNAME);
    read_line("Password: ", nu.password, MAX_PASSWORD);
    nu.level = read_int("Level (1-3): ", 1, 3);
    read_line("Full name: ", nu.fullname, MAX_FULLNAME);
    FILE *f = fopen(USERS_FILE, "ab");
    fwrite(&nu, sizeof(user_t), 1, f);
    fclose(f);
    log_action(currentUser, ACT_ADD_USER, nu.username);
}

/* =========================
   Logging & Utils (Integrated)
   ========================= */
void log_action(const user_t *u, action_t act, const char *details) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] user=%s action=%s details=%s\n",
            tmv->tm_year+1900, tmv->tm_mon+1, tmv->tm_mday, tmv->tm_hour, tmv->tm_min, tmv->tm_sec,
            (u ? u->username : "N/A"), action_to_string(act), details);
    fclose(f);
}

void read_line(const char *p, char *b, size_t n) {
    printf("%s", p); fflush(stdout);
    if (!fgets(b, (int)n, stdin)) return;
    b[strcspn(b, "\n")] = 0;
}

int read_int(const char *p, int min, int max) {
    char line[128]; int v;
    while (1) {
        read_line(p, line, sizeof(line));
        if (sscanf(line, "%d", &v) == 1 && v >= min && v <= max) return v;
        printf("Invalid input.\n");
    }
}

double read_double(const char *p, double min, double max) {
    char line[128]; double v;
    while (1) {
        read_line(p, line, sizeof(line));
        if (sscanf(line, "%lf", &v) == 1 && v >= min && v <= max) return v;
        printf("Invalid input.\n");
    }
}

date_t read_date(const char *p) {
    date_t d;
    while (1) {
        char line[128]; read_line(p, line, sizeof(line));
        if (sscanf(line, "%d %d %d", &d.day, &d.month, &d.year) == 3) return d;
        printf("Invalid date.\n");
    }
}