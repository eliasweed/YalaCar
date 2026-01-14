#include "func.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/* ==========================================================
   SECTION 0: INTERNAL HELPER PROTOTYPES
   ========================================================== */

static int file_exists(const char *path);
static void create_empty_binary_file(const char *path);
static const char* action_to_string(action_t a);
static const char* get_field_ptr(const car_t *c, int tf);

/* Linked List Internal Management */
static car_node* create_node(const car_t *c);
static void insert_sorted(car_node **head, car_node *new_node);
static car_node* load_cars_to_list();
static void sync_list_to_file(car_node *head);
static void free_list(car_node *head);

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
        case ACT_DELETE_USER:     return "DELETE_USER";
        case ACT_CHANGE_LEVEL:    return "CHANGE_LEVEL";
        case ACT_UPDATE_PROFILE:  return "UPDATE_PROFILE";
        case ACT_EXIT:            return "EXIT";
        default:                  return "UNKNOWN";
    }
}

static const char* get_field_ptr(const car_t *c, int tf) {
    switch (tf) {
        case 1: return c->model;
        case 2: return c->make;
        case 3: return c->plate;
        case 4: return c->color;
        default: return "";
    }
}

/* ==========================================================
   SECTION 2: LINKED LIST MANAGEMENT
   ========================================================== */

static car_node* create_node(const car_t *c) {
    car_node *new_node = (car_node*)malloc(sizeof(car_node));
    if (!new_node) return NULL;
    new_node->car = *c;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

static void insert_sorted(car_node **head, car_node *new_node) {
    if (*head == NULL || (*head)->car.serial >= new_node->car.serial) {
        new_node->next = *head;
        if (*head) (*head)->prev = new_node;
        *head = new_node;
        return;
    }
    car_node *current = *head;
    while (current->next != NULL && current->next->car.serial < new_node->car.serial) {
        current = current->next;
    }
    new_node->next = current->next;
    if (current->next) current->next->prev = new_node;
    current->next = new_node;
    new_node->prev = current;
}

static car_node* load_cars_to_list() {
    FILE *f = fopen(CARS_FILE, "rb");
    if (!f) return NULL;
    car_node *head = NULL;
    car_t temp_car;
    while (fread(&temp_car, sizeof(car_t), 1, f)) {
        car_node *node = create_node(&temp_car);
        insert_sorted(&head, node);
    }
    fclose(f);
    return head;
}

static void sync_list_to_file(car_node *head) {
    FILE *f = fopen(CARS_FILE, "wb");
    if (!f) return;
    car_node *current = head;
    while (current) {
        fwrite(&(current->car), sizeof(car_t), 1, f);
        current = current->next;
    }
    fclose(f);
}

static void free_list(car_node *head) {
    car_node *temp;
    while (head) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

/* ==========================================================
   SECTION 3: SYSTEM, AUTHENTICATION & LOGGING
   ========================================================== */

void ensure_system_files_exist(void) {
    if (!file_exists(USERS_FILE)) create_empty_binary_file(USERS_FILE);
    if (!file_exists(CARS_FILE))  create_empty_binary_file(CARS_FILE);
    if (!file_exists(LOG_FILE)) { FILE *f = fopen(LOG_FILE, "w"); if (f) fclose(f); }
}

void ensure_admin_user_exists(void) {
    FILE *f = fopen(USERS_FILE, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    if (ftell(f) > 0) { fclose(f); return; }
    fclose(f);

    user_t admin = {"admin", "admin", 3, "System_Manager"};
    f = fopen(USERS_FILE, "ab");
    fwrite(&admin, sizeof(user_t), 1, f);
    fclose(f);
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

int login_flow(user_t *out_user) {
    int tries = 0;
    while (tries < LOGIN_MAX_TRIES) {
        char uname[MAX_USERNAME], pass[MAX_PASSWORD];
        read_line("\nUsername: ", uname, sizeof(uname));
        read_line("Password: ", pass, sizeof(pass));

        FILE *f = fopen(USERS_FILE, "rb");
        if (!f) return 0;
        user_t tmp;
        int found = 0;
        while (fread(&tmp, sizeof(user_t), 1, f)) {
            if (strcmp(tmp.username, uname) == 0 && strcmp(tmp.password, pass) == 0) {
                *out_user = tmp; found = 1; break;
            }
        }
        fclose(f);
        if (found) { log_action(out_user, ACT_LOGIN_SUCCESS, "Login success"); return 1; }
        tries++;
        printf("Invalid credentials. Tries left: %d\n", LOGIN_MAX_TRIES - tries);
    }
    return 0;
}

void run_main_menu(user_t *current_user) {
    car_node *car_list = load_cars_to_list();
    while (1) {
        printf("\nWelcome %s | Level %d\n", current_user->fullname, current_user->level);
        printf("1) Search Car\n2) Add Car\n3) List All Cars\n4) Update Profile\n");
        if (current_user->level >= 2) printf("5) Update Car\n6) Delete Car\n");
        if (current_user->level == 3) printf("7) List Users\n8) Add User\n9) Delete User\n10) Change Level\n");
        printf("0) Logout\nChoose: ");
        int choice = read_int("", 0, 10);
        if (choice == 0) break;
        switch (choice) {
            case 1: cars_search_flow(current_user, car_list); break;
            case 2: cars_add_flow(current_user, &car_list); break;
            case 3: cars_list_all_flow(current_user, car_list); break;
            case 4: change_personal_info(current_user); break;
            case 5: if(current_user->level >= 2) cars_update_by_serial(current_user, car_list, read_int("Serial: ", 1, 1e9)); break;
            case 6: if(current_user->level >= 2) cars_delete_by_serial(current_user, &car_list, read_int("Serial: ", 1, 1e9)); break;
            case 7: if(current_user->level == 3) users_list_flow(current_user); break;
            case 8: if(current_user->level == 3) add_user(current_user); break;
            case 9: if(current_user->level == 3) users_delete_flow(current_user); break;
            case 10: if(current_user->level == 3) users_change_level_flow(current_user); break;
        }
    }
    free_list(car_list);
}

/* ==========================================================
   SECTION 4: CAR OPERATIONS (LinkedList Based)
   ========================================================== */

void print_car(const car_t *c) {
    printf("--------------------------------------------------\n");
    printf("Serial: %d | Plate: %s | Model: %s | Make: %s\n", c->serial, c->plate, c->model, c->make);
    printf("Price: %.2f | Mileage: %d | Electric: %s\n", c->price, c->mileage, c->is_electric ? "Yes" : "No");
}

void cars_list_all_flow(const user_t *current_user, car_node *head) {
    if (!head) { printf("Inventory empty.\n"); return; }
    while (head) { print_car(&(head->car)); head = head->next; }
}

void cars_search_flow(const user_t *current_user, car_node *head) {
    int tf = read_int("Search field (1=Model 2=Make 3=Plate 4=Color): ", 1, 4);
    char q[64]; read_line("Value: ", q, sizeof(q));
    int found = 0;
    while (head) {
        if (!q[0] || string_contains_ci(get_field_ptr(&(head->car), tf), q)) {
            print_car(&(head->car)); found++;
        }
        head = head->next;
    }
    printf("Matches: %d\n", found);
}

void cars_add_flow(const user_t *current_user, car_node **head) {
    car_t c; memset(&c, 0, sizeof(c));
    c.serial = read_int("Serial: ", 1, 1e9);
    read_line("Model: ", c.model, MAX_MODEL);
    read_line("Plate: ", c.plate, MAX_PLATE);
    read_line("Make: ", c.make, MAX_MAKE);
    c.price = read_double("Price: ", 0, 1e12);
    c.is_electric = read_bool01("Electric? (0/1): ");
    c.manufacture_date = read_date("Date (dd mm yyyy): ");
    insert_sorted(head, create_node(&c));
    sync_list_to_file(*head);
    log_action(current_user, ACT_ADD_CAR, "Added car");
}

void cars_update_by_serial(const user_t *current_user, car_node *head, int serial) {
    while (head && head->car.serial != serial) head = head->next;
    if (!head) { printf("Serial not found.\n"); return; }
    head->car.price = read_double("New price: ", 0, 1e12);
    sync_list_to_file(head);
    log_action(current_user, ACT_UPDATE_CAR, "Updated price");
}

void cars_delete_by_serial(const user_t *current_user, car_node **head, int serial) {
    car_node *curr = *head;
    while (curr && curr->car.serial != serial) curr = curr->next;
    if (!curr) { printf("Not found.\n"); return; }
    if (curr->prev) curr->prev->next = curr->next;
    if (curr->next) curr->next->prev = curr->prev;
    if (curr == *head) *head = curr->next;
    free(curr);
    sync_list_to_file(*head);
    log_action(current_user, ACT_DELETE_CAR, "Deleted car");
}

/* ==========================================================
   SECTION 5: USER MANAGEMENT (Admin Only)
   ========================================================== */

void users_list_flow(const user_t *current_user) {
    FILE *f = fopen(USERS_FILE, "rb");
    user_t u;
    printf("\n--- System Users ---\n");
    while (fread(&u, sizeof(user_t), 1, f))
        printf("User: %-15s | Level: %d | Name: %s\n", u.username, u.level, u.fullname);
    fclose(f);
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

void users_delete_flow(const user_t *current_user) {
    char target[MAX_USERNAME];
    read_line("Delete username: ", target, MAX_USERNAME);
    if (strcmp(target, "admin") == 0) { printf("Cannot delete admin.\n"); return; }
    FILE *f = fopen(USERS_FILE, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    size_t n = sz / sizeof(user_t);
    user_t *arr = (user_t*)malloc(sz);
    fseek(f, 0, SEEK_SET);
    fread(arr, sizeof(user_t), n, f); fclose(f);
    FILE *wf = fopen(USERS_FILE, "wb");
    for (int i = 0; i < n; i++) if (strcmp(arr[i].username, target) != 0) fwrite(&arr[i], sizeof(user_t), 1, wf);
    fclose(wf); free(arr);
    log_action(current_user, ACT_DELETE_USER, target);
}

void users_change_level_flow(const user_t *current_user) {
    char target[MAX_USERNAME];
    read_line("Username: ", target, MAX_USERNAME);
    FILE *f = fopen(USERS_FILE, "rb+");
    user_t tmp; int found = 0;
    while (fread(&tmp, sizeof(user_t), 1, f)) {
        if (strcmp(tmp.username, target) == 0) {
            tmp.level = read_int("New level (1-3): ", 1, 3);
            fseek(f, -((long)sizeof(user_t)), SEEK_CUR);
            fwrite(&tmp, sizeof(user_t), 1, f); found = 1; break;
        }
    }
    fclose(f);
    if (found) log_action(current_user, ACT_CHANGE_LEVEL, target);
}

void change_personal_info(user_t *User) {
    read_line("New Full Name: ", User->fullname, MAX_FULLNAME);
    FILE *f = fopen(USERS_FILE, "rb+");
    user_t tmp;
    while (fread(&tmp, sizeof(user_t), 1, f)) {
        if (strcmp(tmp.username, User->username) == 0) {
            fseek(f, -((long)sizeof(user_t)), SEEK_CUR);
            fwrite(User, sizeof(user_t), 1, f); break;
        }
    }
    fclose(f);
}

/* ==========================================================
   SECTION 6: UTILITIES
   ========================================================== */

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
        printf("Invalid input [%d-%d].\n", min, max);
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

int read_bool01(const char *p) { return read_int(p, 0, 1); }
int read_tristate(const char *p) { return read_int(p, -1, 1); }

date_t read_date(const char *p) {
    date_t d;
    while (1) {
        char line[128]; read_line(p, line, sizeof(line));
        if (sscanf(line, "%d %d %d", &d.day, &d.month, &d.year) == 3) return d;
        printf("Invalid date (dd mm yyyy).\n");
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

int string_contains_ci(const char *h, const char *n) {
    if (!n || !*n) return 1;
    char *hl = _strdup(h), *nl = _strdup(n);
    for (int i = 0; hl[i]; i++) hl[i] = (char)tolower(hl[i]);
    for (int i = 0; nl[i]; i++) nl[i] = (char)tolower(nl[i]);
    int res = (strstr(hl, nl) != NULL);
    free(hl); free(nl); return res;
}