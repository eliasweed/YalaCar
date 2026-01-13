#include "func.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* =========================
   Internal helpers (Ilya)
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
        case ACT_DELETE_USER:     return "DELETE_USER";
        case ACT_CHANGE_LEVEL:    return "CHANGE_LEVEL";
        case ACT_UPDATE_PROFILE:  return "UPDATE_PROFILE";
        case ACT_EXIT:            return "EXIT";
        default:                  return "UNKNOWN";
    }
}

/* =========================
   System Bootstrap (Ilya)
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

    user admin = {"admin", "admin", 3, "System_Manager"};
    f = fopen(USERS_FILE, "ab");
    fwrite(&admin, sizeof(user), 1, f);
    fclose(f);
    update_log(&admin, ACT_ADD_USER, "Initial admin created");
}

/* =========================
   Authentication (Ilya)
   ========================= */
int login_flow(user *out_user) {
    int tries = 0;
    while (tries < LOGIN_MAX_TRIES) {
        char uname[MAX_USERNAME], pass[MAX_PASSWORD];
        read_line("\nUsername: ", uname, sizeof(uname));
        read_line("Password: ", pass, sizeof(pass));

        FILE *f = fopen(USERS_FILE, "rb");
        user tmp;
        int found = 0;
        while (fread(&tmp, sizeof(tmp), 1, f) == 1) {
            if (strcmp(tmp.username, uname) == 0 && strcmp(tmp.password, pass) == 0) {
                found = 1; break;
            }
        }
        fclose(f);
        if (found) {
            *out_user = tmp;
            update_log(out_user, ACT_LOGIN_SUCCESS, "Success");
            return 1;
        }
        tries++;
        update_log(NULL, ACT_LOGIN_FAIL, "Failed attempt");
        printf("Access Denied (%d/%d)\n", tries, LOGIN_MAX_TRIES);
    }
    printf("Too many failed attempts. Goodbye.\n");
    exit(1);
}

/* =========================
   Main Menu (Eyal & Ilya)
   ========================= */
void run_main_menu(user *current_user) {
    while (1) {
        printf("\nWelcome %s | Level %d\n", current_user->fullname, current_user->level);
        printf("1) Search Car\n2) Add Car\n3) List All Cars\n4) Edit Profile\n");
        if (current_user->level >= 2) printf("5) Update Car (by serial)\n6) Delete Car (by serial)\n");
        if (current_user->level == 3) printf("7) List Users\n8) Add User\n9) Delete User\n");
        printf("0) Logout\n");

        int choice = read_int("Choose: ", 0, 9);
        if (choice == 0) { update_log(current_user, ACT_LOGOUT, "User logged out"); break; }

        switch (choice) {
            case 1: cars_search_flow(current_user); break;
            case 2: cars_add_flow(current_user); break;
            case 3: cars_list_all_flow(current_user); break;
            case 4: change_personal_info(current_user); break;
            case 5: if (current_user->level >= 2) cars_update_by_serial(current_user, read_int("Serial: ", 1, 1000000)); break;
            case 6: if (current_user->level >= 2) cars_delete_by_serial(current_user, read_int("Serial: ", 1, 1000000)); break;
            case 7: if (current_user->level == 3) users_list_flow(current_user); break;
            case 8: if (current_user->level == 3) add_user(current_user); break;
            case 9: if (current_user->level == 3) users_delete_flow(current_user); break;
            default: printf("Permission denied or invalid option.\n");
        }
    }
}

/* =========================
   Personal Info (Eyal)
   ========================= */
void change_personal_info(user *User) {
    int choice, stay = 1;
    user temp;
    while (stay) {
        printf("\n1. Edit Password\n2. Edit Name\n3. Save & Exit\nChoice: ");
        choice = read_int("", 1, 3);
        if (choice == 1) { read_line("New password: ", User->password, MAX_PASSWORD); update_log(User, ACT_UPDATE_PROFILE, "Password changed"); }
        else if (choice == 2) { read_line("New name: ", User->fullname, MAX_FULLNAME); update_log(User, ACT_UPDATE_PROFILE, "Name changed"); }
        else stay = 0;
    }
    FILE* f = fopen(USERS_FILE, "rb+");
    while (fread(&temp, sizeof(user), 1, f)) {
        if (strcmp(temp.username, User->username) == 0) {
            fseek(f, -((long)sizeof(user)), SEEK_CUR);
            fwrite(User, sizeof(user), 1, f); break;
        }
    }
    fclose(f);
}

/* =========================
   Car Management (Ilya & Eyal)
   ========================= */
void print_car(const car *c) {
    printf("Serial: %d | Plate: %s | Model: %s | Price: %.2f\n", c->serial, c->plate, c->model, c->price);
    printf("Year: %d | Mileage: %.1f | Color: %s | Electric: %s\n", c->manufacture_date.year, c->mileage, c->color, c->is_electric ? "Yes" : "No");
    printf("--------------------------------------------------\n");
}

void cars_list_all_flow(const user *current_user) {
    FILE *f = fopen(CARS_FILE, "rb");
    car c; int count = 0;
    printf("\n--- Car Inventory ---\n");
    while (fread(&c, sizeof(car), 1, f)) { print_car(&c); count++; }
    fclose(f);
    if (count == 0) printf("Inventory is empty.\n");
}

/* Utility to keep file sorted by serial */
static int cmp_car_serial(const void *a, const void *b) {
    return ((car*)a)->serial - ((car*)b)->serial;
}

void cars_add_flow(const user *current_user) {
    car c; memset(&c, 0, sizeof(car));
    c.serial = read_int("Serial: ", 1, 1000000);
    read_line("Model: ", c.model, MAX_MODEL);
    read_line("Plate: ", c.plate, MAX_PLATE);
    c.price = read_double("Price: ", 0, 1e9);
    c.manufacture_date = read_date("Date (dd mm yyyy): ");

    /* Load all, add new, sort and save */
    FILE *f = fopen(CARS_FILE, "rb");
    fseek(f, 0, SEEK_END);
    long n = ftell(f) / sizeof(car);
    car *arr = malloc((n + 1) * sizeof(car));
    fseek(f, 0, SEEK_SET);
    fread(arr, sizeof(car), n, f);
    fclose(f);

    arr[n] = c;
    qsort(arr, n + 1, sizeof(car), cmp_car_serial);

    f = fopen(CARS_FILE, "wb");
    fwrite(arr, sizeof(car), n + 1, f);
    fclose(f); free(arr);
    update_log(current_user, ACT_ADD_CAR, c.plate);
    printf("Car added and file sorted.\n");
}

/* =========================
   Logging & Utils
   ========================= */
void update_log(const user *u, action_t act, const char *details) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "[%ld] User: %s | Action: %s | Details: %s\n", (long)now, u ? u->username : "N/A", action_to_string(act), details);
    fclose(f);
}

void read_line(const char *p, char *b, size_t n) {
    printf("%s", p); fflush(stdout);
    fgets(b, (int)n, stdin);
    b[strcspn(b, "\n")] = 0;
}

int read_int(const char *p, int min, int max) {
    char line[128]; int v;
    while (1) {
        read_line(p, line, sizeof(line));
        if (sscanf(line, "%d", &v) == 1 && v >= min && v <= max) return v;
        printf("Invalid number.\n");
    }
}

double read_double(const char *p, double min, double max) {
    char line[128]; double v;
    while (1) {
        read_line(p, line, sizeof(line));
        if (sscanf(line, "%lf", &v) == 1 && v >= min && v <= max) return v;
        printf("Invalid number.\n");
    }
}

date read_date(const char *p) {
    date d;
    printf("%s", p);
    scanf("%d %d %d", &d.day, &d.month, &d.year);
    getchar();
    return d;
}