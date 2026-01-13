#include "func.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/* ==========================================================
   SECTION 1: INTERNAL HELPERS & FILE UTILS
   ========================================================== */

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

static int load_all_cars(car_t **out_arr, size_t *out_n) {
    FILE *f = fopen(CARS_FILE, "rb");
    if (!f) { *out_arr = NULL; *out_n = 0; return 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t n = (size_t)sz / sizeof(car_t);
    if (n == 0) { *out_arr = NULL; *out_n = 0; fclose(f); return 1; }
    *out_arr = (car_t*)malloc(n * sizeof(car_t));
    fread(*out_arr, sizeof(car_t), n, f);
    *out_n = n;
    fclose(f);
    return 1;
}

static int save_all_cars(const car_t *arr, size_t n) {
    FILE *f = fopen(CARS_FILE, "wb");
    if (!f) return 0;
    if (n > 0) fwrite(arr, sizeof(car_t), n, f);
    fclose(f);
    return 1;
}

/* ==========================================================
   SECTION 2: SYSTEM BOOTSTRAP & LOGGING
   ========================================================== */

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
    user_t admin = {"admin", "admin", 3, "Manager_System"};
    f = fopen(USERS_FILE, "ab");
    fwrite(&admin, sizeof(user_t), 1, f);
    fclose(f);
    log_action(&admin, ACT_ADD_USER, "Initial admin created");
}

void log_action(const user_t *u, action_t act, const char *details) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] user=%s level=%d action=%s details=%s\n",
            tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
            tmv->tm_hour, tmv->tm_min, tmv->tm_sec,
            (u ? u->username : "N/A"), (u ? u->level : -1),
            action_to_string(act), (details ? details : ""));
    fclose(f);
}

/* ==========================================================
   SECTION 3: AUTHENTICATION & MENUS
   ========================================================== */

int login_flow(user_t *out_user) {
    int tries = 0;
    while (tries < LOGIN_MAX_TRIES) {
        char uname[MAX_USERNAME], pass[MAX_PASSWORD];
        read_line("\nUsername: ", uname, sizeof(uname));
        read_line("Password: ", pass, sizeof(pass));

        FILE *f = fopen(USERS_FILE, "rb"); // קריאה בינארית
        if (!f) return 0;

        user_t tmp;
        int found = 0;
        while (fread(&tmp, sizeof(user_t), 1, f)) {
            // השוואה נקייה של מחרוזות
            if (strcmp(tmp.username, uname) == 0 && strcmp(tmp.password, pass) == 0) {
                *out_user = tmp;
                found = 1;
                break;
            }
        }
        fclose(f);

        if (found) {
            log_action(out_user, ACT_LOGIN_SUCCESS, "Login success");
            return 1;
        }
        tries++;
        printf("Invalid credentials. Tries left: %d\n", LOGIN_MAX_TRIES - tries);
    }
    return 0;
}

void run_main_menu(user_t *current_user) {
    while (1) {
        printf("\nWelcome %s | Level %d\n", current_user->fullname, current_user->level);
        printf("1) Search Car\n2) Add Car\n3) List All Cars\n4) Update Profile\n");
        if (current_user->level >= 2) printf("5) Update Car\n6) Delete Car\n");
        if (current_user->level == 3) printf("7) List Users\n8) Add User\n");
        printf("0) Logout\nChoose: ");
        int choice = read_int("", 0, 8);
        if (choice == 0) break;
        switch (choice) {
            case 1: cars_search_flow(current_user); break;
            case 2: cars_add_flow(current_user); break;
            case 3: cars_list_all_flow(current_user); break;
            case 4: change_personal_info(current_user); break;
            case 5: if(current_user->level >= 2) cars_update_by_serial(current_user, read_int("Serial: ", 1, 1e9)); break;
            case 6: if(current_user->level >= 2) cars_delete_by_serial(current_user, read_int("Serial: ", 1, 1e9)); break;
            case 7: if(current_user->level == 3) users_list_flow(current_user); break;
            case 8: if(current_user->level == 3) add_user(current_user); break;
        }
    }
}

/* ==========================================================
   SECTION 4: CAR SEARCH & LISTING
   ========================================================== */

void print_car(const car_t *c) {
    printf("--------------------------------------------------\n");
    printf("Serial: %d | Plate: %s | Model: %s | Make: %s\n", c->serial, c->plate, c->model, c->make);
    printf("Price: %.2f | Mileage: %d | Electric: %s\n", c->price, c->mileage, c->is_electric ? "Yes" : "No");
}

void cars_list_all_flow(const user_t *current_user) {
    FILE *f = fopen(CARS_FILE, "rb");
    if (!f) return;
    car_t c;
    while (fread(&c, sizeof(car_t), 1, f)) print_car(&c);
    fclose(f);
}

static const char* get_field_ptr(const car_t *c, int tf) {
    switch (tf) {
        case 1: return c->model; case 2: return c->make;
        case 3: return c->plate; case 4: return c->color;
        default: return "";
    }
}

void cars_search_flow(const user_t *current_user) {
    int f1 = read_int("Search by (1=Model 2=Make 3=Plate 4=Color): ", 1, 4);
    char q1[64]; read_line("Enter search value: ", q1, sizeof(q1));
    double min_p = read_double("Min Price (-1 ignore): ", -1.0, 1e12);

    FILE *f = fopen(CARS_FILE, "rb");
    car_t c; int found = 0;
    while (fread(&c, sizeof(car_t), 1, f)) {
        if (q1[0] && !string_contains_ci(get_field_ptr(&c, f1), q1)) continue;
        if (min_p >= 0 && c.price < min_p) continue;
        print_car(&c); found++;
    }
    fclose(f);
    printf("Total matches: %d\n", found);
}

/* ==========================================================
   SECTION 5: CAR UPDATE/DELETE/ADD
   ========================================================== */

static int cmp_car_serial(const void *a, const void *b) {
    return ((car_t*)a)->serial - ((car_t*)b)->serial;
}

void cars_add_flow(const user_t *current_user) {
    car_t c; memset(&c, 0, sizeof(c));
    c.serial = read_int("New Serial: ", 1, 1e9);
    read_line("Model: ", c.model, MAX_MODEL);
    read_line("Plate: ", c.plate, MAX_PLATE);
    read_line("Make: ", c.make, MAX_MAKE);
    c.price = read_double("Price: ", 0, 1e12);
    c.is_electric = read_bool01("Electric? (0/1): ");
    c.manufacture_date = read_date("Manufacture date (dd mm yyyy): ");

    car_t *arr; size_t n;
    load_all_cars(&arr, &n);
    car_t *new_arr = realloc(arr, (n + 1) * sizeof(car_t));
    new_arr[n] = c;
    qsort(new_arr, n + 1, sizeof(car_t), cmp_car_serial);
    save_all_cars(new_arr, n + 1);
    free(new_arr);
    log_action(current_user, ACT_ADD_CAR, "Car added sorted");
}

int cars_update_by_serial(const user_t *current_user, int serial) {
    car_t *arr; size_t n;
    if (!load_all_cars(&arr, &n)) return 0;
    int idx = -1;
    for (size_t i = 0; i < n; i++) if (arr[i].serial == serial) { idx = i; break; }
    if (idx == -1) { free(arr); printf("Serial not found.\n"); return 0; }

    printf("Updating Serial %d. Enter new price (current %.2f, -1 keep): ", serial, arr[idx].price);
    double np = read_double("", -1.0, 1e12);
    if (np >= 0) arr[idx].price = np;
    
    save_all_cars(arr, n);
    free(arr);
    log_action(current_user, ACT_UPDATE_CAR, "Price updated");
    return 1;
}

int cars_delete_by_serial(const user_t *current_user, int serial) {
    car_t *arr; size_t n;
    if (!load_all_cars(&arr, &n)) return 0;
    int idx = -1;
    for (size_t i = 0; i < n; i++) if (arr[i].serial == serial) { idx = i; break; }
    if (idx == -1) { free(arr); return 0; }

    for (size_t i = idx; i < n - 1; i++) arr[i] = arr[i + 1];
    save_all_cars(arr, n - 1);
    free(arr);
    log_action(current_user, ACT_DELETE_CAR, "Deleted car");
    return 1;
}

/* ==========================================================
   SECTION 6: USER & UTILS
   ========================================================= */

void change_personal_info(user_t *User) {
    read_line("New Full Name: ", User->fullname, MAX_FULLNAME);
    FILE *f = fopen(USERS_FILE, "rb+");
    user_t temp;
    while (fread(&temp, sizeof(user_t), 1, f)) {
        if (strcmp(temp.username, User->username) == 0) {
            fseek(f, -((long)sizeof(user_t)), SEEK_CUR);
            fwrite(User, sizeof(user_t), 1, f); break;
        }
    }
    fclose(f);
    log_action(User, ACT_UPDATE_PROFILE, "Name updated");
}

void add_user(user_t *currentUser) {
    user_t nu;
    read_line("Username: ", nu.username, MAX_USERNAME);
    read_line("Password: ", nu.password, MAX_PASSWORD);
    nu.level = read_int("Level (1-3): ", 1, 3);
    read_line("Full Name: ", nu.fullname, MAX_FULLNAME);
    FILE *f = fopen(USERS_FILE, "ab");
    fwrite(&nu, sizeof(user_t), 1, f);
    fclose(f);
    log_action(currentUser, ACT_ADD_USER, nu.username);
}

void users_list_flow(const user_t *current_user) {
    FILE *f = fopen(USERS_FILE, "rb");
    user_t u;
    printf("\n--- System Users ---\n");
    while (fread(&u, sizeof(user_t), 1, f))
        printf("User: %-15s | Level: %d | Name: %s\n", u.username, u.level, u.fullname);
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
        printf("Invalid input. Try again: ");
    }
}

double read_double(const char *p, double min, double max) {
    char line[128]; double v;
    while (1) {
        read_line(p, line, sizeof(line));
        if (sscanf(line, "%lf", &v) == 1 && v >= min && v <= max) return v;
        printf("Invalid input. Try again: ");
    }
}

int read_bool01(const char *p) { return read_int(p, 0, 1); }
int read_tristate(const char *p) { return read_int(p, -1, 1); }

date_t read_date(const char *p) {
    date_t d;
    while (1) {
        char line[128]; read_line(p, line, sizeof(line));
        if (sscanf(line, "%d %d %d", &d.day, &d.month, &d.year) == 3) return d;
        printf("Invalid date (dd mm yyyy): ");
    }
}

int date_cmp(date_t a, date_t b) {
    if (a.year != b.year) return a.year - b.year;
    if (a.month != b.month) return a.month - b.month;
    return a.day - b.day;
}

int date_in_range(date_t x, int use_min, date_t mn, int use_max, date_t mx) {
    if (use_min && date_cmp(x, mn) < 0) return 0;
    if (use_max && date_cmp(x, mx) > 0) return 0;
    return 1;
}

int string_contains_ci(const char *haystack, const char *needle) {
    if (!needle || !*needle) return 1;
    char *h = strdup(haystack), *n = strdup(needle);
    for (int i = 0; h[i]; i++) h[i] = tolower(h[i]);
    for (int i = 0; n[i]; i++) n[i] = tolower(n[i]);
    int res = strstr(h, n) != NULL;
    free(h); free(n); return res;
}