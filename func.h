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
    int  level;                 /* 1..3 */
    char fullname[MAX_FULLNAME];
} user_t;

typedef struct date {
    int day;
    int month;
    int year;
} date_t;

typedef struct car {
    int  serial;                /* unique serial */
    char model[MAX_MODEL];
    char plate[MAX_PLATE];
    char make[MAX_MAKE];
    char color[MAX_COLOR];

    int    seats;
    int    mileage;
    double price;               /* non-integer numeric */

    double engine_cc;           /* 0 if electric */
    double battery_kwh;         /* 0 if not electric */
    double range_km;

    int is_electric;            /* 0/1 */
    int is_luxury;              /* 0/1 */
    int is_automatic;           /* 0/1 */
    int is_family;              /* 0/1 */
    int test_valid;             /* 0/1 */

    date_t manufacture_date;
    date_t road_date;
} car_t;

typedef struct car_node {
    car_t car;
    struct car_node *next;
    struct car_node *prev;
} car_node;

typedef enum {
    ACT_LOGIN_SUCCESS, ACT_LOGIN_FAIL, ACT_LOGOUT, ACT_SEARCH_CAR,
    ACT_ADD_CAR, ACT_UPDATE_CAR, ACT_DELETE_CAR, ACT_SHOW_USERS,
    ACT_ADD_USER, ACT_DELETE_USER, ACT_CHANGE_LEVEL, ACT_UPDATE_PROFILE, ACT_EXIT
} action_t;

/* =========================
   Public API
   ========================= */

/* System bootstrap */
void ensure_system_files_exist(void);
void ensure_admin_user_exists(void);

/* Auth + main loop */
int  login_flow(user_t *out_user);
void run_main_menu(user_t *current_user);

/* Cars Operations */
void cars_search_flow(const user_t *current_user);
void cars_add_flow(const user_t *current_user);
void cars_list_all_flow(const user_t *current_user);
void print_car(const car_t *c);
int  cars_update_by_serial(const user_t *current_user, int serial);
int  cars_delete_by_serial(const user_t *current_user, int serial);

/* Users & Profile (Eyal) */
void add_user(user_t *currentUser);
void users_list_flow(const user_t *current_user);
void change_personal_info(user_t *User);

/* Logging */
void log_action(const user_t *u, action_t act, const char *details);

/* Utilities */
void   read_line(const char *prompt, char *buf, size_t n);
int    read_int(const char *prompt, int minv, int maxv);
double read_double(const char *prompt, double minv, double maxv);
int    read_bool01(const char *prompt);
int    read_tristate(const char *prompt);
date_t read_date(const char *prompt);
int    date_cmp(date_t a, date_t b);
int    date_in_range(date_t x, int use_min, date_t mn, int use_max, date_t mx);
int    string_contains_ci(const char *haystack, const char *needle);

#endif