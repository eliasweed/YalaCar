#ifndef FUNC_H_
#define FUNC_H_

#include <stddef.h>

/* =========================
   Constants / File names
   ========================= */
#define USERS_FILE "users.dat"
#define CARS_FILE  "cars.dat"
#define LOG_FILE   "log.txt"

#define MAX_USERNAME 15
#define MAX_PASSWORD 15
#define MAX_FULLNAME 20

#define MAX_MODEL    30
#define MAX_PLATE    15
#define MAX_MAKE     30
#define MAX_COLOR    20

#define LOGIN_MAX_TRIES 3

/* =========================
   Data Structures
   ========================= */

typedef struct user {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int  level;
    char fullname[MAX_FULLNAME];
} user;

typedef struct date {
    int day;
    int month;
    int year;
} date;

typedef struct car{
    /*Textual parameters*/
    char model[MAX_MODEL];
    char plate[MAX_PLATE];
    char make[MAX_MAKE];
    char color[MAX_COLOR];

    /*Numeral parameters*/
    int seats;
    double mileage;
    double price;
    int engine_cc;
    int battery_size;
    double range_km;

    /*Boolean parameters*/
    int is_electric; 
    int is_luxury;   
    int is_automatic;
    int is_family;   
    int test_valid;  

    /*Date parameters*/
    date manufacture_date;
    date road_date;
}car;

typedef struct car_node{
    car car;
    struct car_node* next;
    struct car_node* prev;
}car_node;

/* Which text field to use */
typedef enum {
    TF_MODEL = 1,
    TF_MAKE  = 2,
    TF_PLATE = 3,
    TF_COLOR = 4
} text_field_t;

typedef enum {
    ACT_LOGIN_SUCCESS,
    ACT_LOGIN_FAIL,
    ACT_LOGOUT,
    ACT_SEARCH_CAR,
    ACT_ADD_CAR,
    ACT_UPDATE_CAR,
    ACT_DELETE_CAR,
    ACT_SHOW_USERS,
    ACT_ADD_USER,
    ACT_DELETE_USER,
    ACT_CHANGE_LEVEL,
    ACT_UPDATE_PROFILE,
    ACT_EXIT
} action_t;

/* =========================
   Internal helpers (file)
   ========================= */
static int  file_exists(const char *path);
static void create_empty_binary_file(const char *path);
static const char* action_to_string(action_t a);

/* cars file helpers */
static int  load_all_cars(car **out_arr, size_t *out_n);
static int  save_all_cars(const car *arr, size_t n);
static int  car_serial_exists(const car *arr, size_t n, int serial);

/* users file helpers */
static int  load_all_users(user **out_arr, size_t *out_n);
static int  save_all_users(const user *arr, size_t n);

void update_log(const user *u, action_t act, const char *details);
void change_personal_info(user* User);
void add_user(user* currentUser);
void update_car(user* currentUser, car_node* head);

/* =========================
   Public API
   ========================= */

/* System bootstrap */
void ensure_system_files_exist(void);
void ensure_admin_user_exists(void);

/* Auth + main loop */
int  login_flow(user *out_user);
void run_main_menu(user *current_user); /* note: may update profile */

/* Cars (operations) */
void cars_search_flow(const user *current_user);
void cars_add_flow(const user *current_user);
void cars_update_flow(const user *current_user); /* prompts for serial */
void cars_delete_flow(const user *current_user); /* prompts for serial */
void cars_list_all_flow(const user *current_user);
void print_car(const car *c);

/* Cars (direct actions by serial, used from search) */
int  cars_update_by_serial(const user *current_user, int serial);
int  cars_delete_by_serial(const user *current_user, int serial);

/* Users (admin only) */
void users_list_flow(const user *current_user);
void users_add_flow(const user *current_user);
void users_delete_flow(const user *current_user);
void users_change_level_flow(const user *current_user);

/* Profile */
void update_personal_details_flow(user *current_user);

/* Logging */
void log_action(const user *u, action_t act, const char *details);

/* Utilities */
void read_line(const char *prompt, char *buf, size_t n);
int  read_int(const char *prompt, int minv, int maxv);
double read_double(const char *prompt, double minv, double maxv);
int  read_tristate(const char *prompt);
int  read_bool01(const char *prompt);
date read_date(const char *prompt);
int  date_cmp(date a, date b);
int  date_in_range(date x, int use_min, date mn, int use_max, date mx);
int  string_contains_ci(const char *haystack, const char *needle);
void print_car(const car *c);

/* Utils (string/date helpers) */
int    string_contains_ci(const char *haystack, const char *needle); /* empty needle => match */
int    date_cmp(date a, date b); /* -1/0/1 */
int    date_in_range(date x, int use_min, date mn, int use_max, date mx);

#endif