#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "func.h"

/* =========================
   Input helpers
   ========================= */
void read_line(const char *prompt, char *buf, size_t n)
{
    if (!buf || n == 0) return;

    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

int read_int(const char *prompt, int minv, int maxv)
{
    char line[128];
    int v;

    while (1) {
        read_line(prompt, line, sizeof(line));
        if (sscanf(line, "%d", &v) == 1 && v >= minv && v <= maxv) return v;
        printf("Invalid number. Try again.\n");
    }
}

double read_double(const char *prompt, double minv, double maxv)
{
    char line[128];
    double v;

    while (1) {
        read_line(prompt, line, sizeof(line));
        if (sscanf(line, "%lf", &v) == 1 && v >= minv && v <= maxv) return v;
        printf("Invalid number. Try again.\n");
    }
}

int read_bool01(const char *prompt)
{
    return read_int(prompt, 0, 1);
}

int read_tristate(const char *prompt)
{
    return read_int(prompt, -1, 1);
}

/* =========================
   Date helpers
   ========================= */
static int date_is_valid(date d)
{
    if (d.year < 1900 || d.year > 2200) return 0;
    if (d.month < 1 || d.month > 12) return 0;
    if (d.day < 1 || d.day > 31) return 0;

    static const int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxd = mdays[d.month - 1];

    int leap = ((d.year % 4 == 0 && d.year % 100 != 0) || (d.year % 400 == 0));
    if (d.month == 2 && leap) maxd = 29;

    return (d.day <= maxd);
}

date read_date(const char *prompt)
{
    char line[128];
    date d;

    while (1) {
        read_line(prompt, line, sizeof(line));
        if (sscanf(line, "%d %d %d", &d.day, &d.month, &d.year) == 3 && date_is_valid(d)) {
            return d;
        }
        printf("Invalid date. Format: dd mm yyyy\n");
    }
}

int date_cmp(date a, date b)
{
    if (a.year != b.year) return (a.year < b.year) ? -1 : 1;
    if (a.month != b.month) return (a.month < b.month) ? -1 : 1;
    if (a.day != b.day) return (a.day < b.day) ? -1 : 1;
    return 0;
}

int date_in_range(date x, int use_min, date mn, int use_max, date mx)
{
    if (use_min && date_cmp(x, mn) < 0) return 0;
    if (use_max && date_cmp(x, mx) > 0) return 0;
    return 1;
}

/* =========================
   String helpers
   ========================= */
static int char_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 'a';
    return ch;
}

int string_contains_ci(const char *haystack, const char *needle)
{
    if (!needle || needle[0] == '\0') return 1;
    if (!haystack) return 0;

    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen > hlen) return 0;

    for (size_t i = 0; i + nlen <= hlen; i++) {
        size_t j = 0;
        for (; j < nlen; j++) {
            if (char_lower((unsigned char)haystack[i + j]) != char_lower((unsigned char)needle[j])) break;
        }
        if (j == nlen) return 1;
    }
    return 0;
}



static const char* tf_name(text_field_t tf)
{
    switch (tf) {
        case TF_MODEL: return "Model";
        case TF_MAKE:  return "Make";
        case TF_PLATE: return "Plate";
        case TF_COLOR: return "Color";
        default:       return "Unknown";
    }
}

static const char* get_field_ptr(const car *c, text_field_t tf)
{
    switch (tf) {
        case TF_MODEL: return c->model;
        case TF_MAKE:  return c->make;
        case TF_PLATE: return c->plate;
        case TF_COLOR: return c->color;
        default:       return "";
    }
}

static void build_log_details(char *dst, size_t n,
                              const char *text1_label, const char *text1_val,
                              const char *text2_label, const char *text2_val,
                              double min_price, double max_price,
                              int min_mileage, int max_mileage,
                              int min_seats, int max_seats,
                              double min_engine, double max_engine,
                              double min_range, double max_range,
                              int q_electric, int q_luxury, int q_auto, int q_family, int q_test)
{
    /* Keep it concise but informative */
    snprintf(dst, n,
        "criteria: %s='%s'%s%s%s price[%.2f..%.2f] mileage[%d..%d] seats[%d..%d] engine[%.2f..%.2f] range[%.2f..%.2f] bool(e=%d l=%d a=%d f=%d t=%d)",
        text1_label, text1_val,
        (text2_label ? " AND " : ""),
        (text2_label ? text2_label : ""),
        (text2_label ? "='" : ""),
        min_price, max_price,
        min_mileage, max_mileage,
        min_seats, max_seats,
        min_engine, max_engine,
        min_range, max_range,
        q_electric, q_luxury, q_auto, q_family, q_test
    );

    /* fix formatting if second field exists */
    if (text2_label) {
        size_t len = strlen(dst);
        if (len < n - 2) {
            /* append second value close */
            strncat(dst, text2_val, n - strlen(dst) - 1);
            strncat(dst, "'", n - strlen(dst) - 1);
        }
    }
}

void cars_search_flow(const user *current_user)
{
    /* Choose text search mode */
    printf("\n[SEARCH] Text search mode:\n");
    printf("1) Single text field\n");
    printf("2) Two text fields together (AND)\n");
    int mode = read_int("Choose (1-2): ", 1, 2);

    text_field_t f1 = (text_field_t)read_int("Choose field #1 (1=Model 2=Make 3=Plate 4=Color): ", 1, 4);
    char q1[64] = {0};
    {
        char p[128];
        snprintf(p, sizeof(p), "Enter value for %s (empty=ignore): ", tf_name(f1));
        read_line(p, q1, sizeof(q1));
    }

    text_field_t f2 = TF_MODEL;
    char q2[64] = {0};
    int use_two = 0;
    if (mode == 2) {
        use_two = 1;
        f2 = (text_field_t)read_int("Choose field #2 (1=Model 2=Make 3=Plate 4=Color): ", 1, 4);
        {
            char p[128];
            snprintf(p, sizeof(p), "Enter value for %s (empty=ignore): ", tf_name(f2));
            read_line(p, q2, sizeof(q2));
        }
    }

    /* Numeric filters (use sentinel to ignore) */
    printf("\n[SEARCH] Numeric filters (enter -1 to ignore):\n");
    double min_price = read_double("Min price (-1 ignore): ", -1.0, 1e12);
    double max_price = read_double("Max price (-1 ignore): ", -1.0, 1e12);
    if (min_price >= 0 && max_price >= 0 && min_price > max_price) { double t=min_price; min_price=max_price; max_price=t; }
    if (min_price < 0) min_price = -1.0;
    if (max_price < 0) max_price = -1.0;

    int min_mileage = read_int("Min mileage (-1 ignore): ", -1, 3000000);
    int max_mileage = read_int("Max mileage (-1 ignore): ", -1, 3000000);
    if (min_mileage >= 0 && max_mileage >= 0 && min_mileage > max_mileage) { int t=min_mileage; min_mileage=max_mileage; max_mileage=t; }

    int min_seats = read_int("Min seats (-1 ignore): ", -1, 12);
    int max_seats = read_int("Max seats (-1 ignore): ", -1, 12);
    if (min_seats >= 0 && max_seats >= 0 && min_seats > max_seats) { int t=min_seats; min_seats=max_seats; max_seats=t; }

    double min_engine = read_double("Min engine CC (-1 ignore): ", -1.0, 1e12);
    double max_engine = read_double("Max engine CC (-1 ignore): ", -1.0, 1e12);
    if (min_engine >= 0 && max_engine >= 0 && min_engine > max_engine) { double t=min_engine; min_engine=max_engine; max_engine=t; }

    double min_range = read_double("Min range km (-1 ignore): ", -1.0, 1e12);
    double max_range = read_double("Max range km (-1 ignore): ", -1.0, 1e12);
    if (min_range >= 0 && max_range >= 0 && min_range > max_range) { double t=min_range; min_range=max_range; max_range=t; }

    /* Boolean filters tristate */
    printf("\n[SEARCH] Boolean filters (-1 any, 0 no, 1 yes):\n");
    int q_electric  = read_tristate("Is electric? (-1/0/1): ");
    int q_luxury    = read_tristate("Is luxury?   (-1/0/1): ");
    int q_auto      = read_tristate("Is automatic?(-1/0/1): ");
    int q_family    = read_tristate("Is family?   (-1/0/1): ");
    int q_test      = read_tristate("Test valid?  (-1/0/1): ");

    /* Date ranges */
    printf("\n[SEARCH] Date filters:\n");
    int use_manu_min = read_bool01("Filter Manufacture MIN date? (0/1): ");
    date manu_min = {1,1,1900};
    if (use_manu_min) manu_min = read_date("Manufacture MIN (dd mm yyyy): ");

    int use_manu_max = read_bool01("Filter Manufacture MAX date? (0/1): ");
    date manu_max = {31,12,2200};
    if (use_manu_max) manu_max = read_date("Manufacture MAX (dd mm yyyy): ");
    if (use_manu_min && use_manu_max && date_cmp(manu_min, manu_max) > 0) { date t=manu_min; manu_min=manu_max; manu_max=t; }

    int use_road_min = read_bool01("Filter Road MIN date? (0/1): ");
    date road_min = {1,1,1900};
    if (use_road_min) road_min = read_date("Road MIN (dd mm yyyy): ");

    int use_road_max = read_bool01("Filter Road MAX date? (0/1): ");
    date road_max = {31,12,2200};
    if (use_road_max) road_max = read_date("Road MAX (dd mm yyyy): ");
    if (use_road_min && use_road_max && date_cmp(road_min, road_max) > 0) { date t=road_min; road_min=road_max; road_max=t; }

    /* Log criteria */
    char details[900];
    build_log_details(details, sizeof(details),
                      tf_name(f1), q1,
                      (use_two ? tf_name(f2) : NULL), q2,
                      (min_price < 0 ? 0.0 : min_price), (max_price < 0 ? 0.0 : max_price),
                      (min_mileage < 0 ? 0 : min_mileage), (max_mileage < 0 ? 0 : max_mileage),
                      (min_seats < 0 ? 0 : min_seats), (max_seats < 0 ? 0 : max_seats),
                      (min_engine < 0 ? 0.0 : min_engine), (max_engine < 0 ? 0.0 : max_engine),
                      (min_range < 0 ? 0.0 : min_range), (max_range < 0 ? 0.0 : max_range),
                      q_electric, q_luxury, q_auto, q_family, q_test);
    update_log(current_user, ACT_SEARCH_CAR, details);

    /* Scan cars file */
    FILE *f = fopen(CARS_FILE, "rb");
    if (!f) {
        printf("Cannot open cars file.\n");
        return;
    }

    printf("\n============= SEARCH RESULTS =============\n");
    car car;
    int match_count = 0;

    while (fread(&car, sizeof(car), 1, f) == 1) {
        /* text filters */
        if (q1[0] != '\0' && !string_contains_ci(get_field_ptr(&car, f1), q1)) continue;
        if (use_two && q2[0] != '\0' && !string_contains_ci(get_field_ptr(&car, f2), q2)) continue;

        /* numeric filters */
        if (min_price >= 0 && car.price < min_price) continue;
        if (max_price >= 0 && car.price > max_price) continue;

        if (min_mileage >= 0 && car.mileage < min_mileage) continue;
        if (max_mileage >= 0 && car.mileage > max_mileage) continue;

        if (min_seats >= 0 && car.seats < min_seats) continue;
        if (max_seats >= 0 && car.seats > max_seats) continue;

        /* engine filter: use engine_cc (electric cars have 0) */
        if (min_engine >= 0 && car.engine_cc < min_engine) continue;
        if (max_engine >= 0 && car.engine_cc > max_engine) continue;

        if (min_range >= 0 && car.range_km < min_range) continue;
        if (max_range >= 0 && car.range_km > max_range) continue;

        /* boolean tristate */
        if (q_electric != -1 && car.is_electric != q_electric) continue;
        if (q_luxury   != -1 && car.is_luxury   != q_luxury) continue;
        if (q_auto     != -1 && car.is_automatic!= q_auto) continue;
        if (q_family   != -1 && car.is_family   != q_family) continue;
        if (q_test     != -1 && car.test_valid  != q_test) continue;

        /* date ranges */
        if (!date_in_range(car.manufacture_date, use_manu_min, manu_min, use_manu_max, manu_max)) continue;
        if (!date_in_range(car.road_date,        use_road_min, road_min, use_road_max, road_max)) continue;

        print_car(&car);
        match_count++;
    }

    fclose(f);

    if (match_count == 0) {
        printf("(No matches)\n");
        return;
    }

    printf("==========================================\n");
    printf("Matches found: %d\n", match_count);

    /* Per assignment: allow edit by typing the serial number */
    int serial = read_int("Enter serial to edit (0 to go back): ", 0, 1000000000);
    if (serial == 0) return;

    /* Level-based actions */
    if (current_user->level < 2) {
        printf("Level 1: view-only. Ask admin to modify.\n");
        return;
    }

    printf("\nActions:\n");
    printf("1) Update car (by serial)\n");
    printf("2) Delete car (by serial)\n");
    printf("0) Back\n");
    int act = read_int("Choose: ", 0, 2);

    if (act == 1) {
        cars_update_by_serial(current_user, serial);
    } else if (act == 2) {
        cars_delete_by_serial(current_user, serial);
    }
}


static const char* action_to_string(action_t a)
{
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

void update_log(const user *u, action_t act, const char *details) {
    FILE* f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        printf("Error: Could not open log file.\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);

    fprintf(f,
        "[%04d-%02d-%02d %02d:%02d:%02d] user=%s level=%d action=%s details=%s\n",
        tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
        tmv->tm_hour, tmv->tm_min, tmv->tm_sec,
        (u ? u->username : "N/A"),
        (u ? u->level : -1),
        action_to_string(act),
        (details ? details : "")
    );

    fclose(f);
}

void change_personal_info(user* User) {
    int choice;//to choose which parameter to change
    int stay = 1;//for the parameter choosing part
    user temp;
    
    printf("Editing Personal Information for %s\n", User->username);//starting message

    /*Choose parameter to change*/
    while (stay) {
        printf("Which parameter would you like to change?\n");
        printf("1. Password\n");
        printf("2. Full Name\n");
        printf("3. Finish and Save\n");
        printf("Your choice: ");
        
        if (scanf("%d", &choice) != 1) { //get number and check if valid
            printf("Invalid input. Please enter a number.\n");

            
            while(getchar() != '\n'); //Clean buffer from invalid input

            continue;//try again if failed
        }
        getchar(); //Clean buffer from \n

        /*Switch case for chosen parameter*/
        switch (choice) {
            case 1:
                printf("Enter new password (up to 14 characters): ");
                scanf("%14s", User->password);//get up to 14 characters and update the user's password
                update_log(User, ACT_UPDATE_PROFILE, "Updated password");//log update
                break;
            case 2:
                printf("Enter new full name (up to 19 characters): ");
                fgets(User->fullname, MAX_FULLNAME, stdin);//get full name (with spaces)
                User->fullname[strcspn(User->fullname, "\n")] = 0;//remove the \n in the end
                update_log(User, ACT_UPDATE_PROFILE, "Updated full name");//log update
                break;
            case 3:
                stay = 0;//to exit the parameter choosing part
                continue;
            default:
                printf("Parameter does not exist. Please try again.\n");
                continue;
        }

        if (stay) {
            printf("Would you like to change another parameter? (1-Yes / 0-No): ");
            scanf("%d", &stay);
        }
    }

    /*update file*/
    FILE* f = fopen(USERS_FILE, "rb+"); //Open the binary file in read and write mode
    if (!f) {//error
        printf("Error: Could not open users file.\n");
        return;
    }

    while (fread(&temp, sizeof(user), 1, f)) {
        if (strcmp(temp.username, User->username) == 0) {//find the username we want to change
            fseek(f, -((long)sizeof(user)), SEEK_CUR); // point to the start of the username just read
            fwrite(User, sizeof(user), 1, f);   // update parameters
            break;
        }
    }
    fclose(f);
    printf("Changes saved successfully. Returning to main menu.\n");
}

void add_user(user* currentUser) {
    user newUser;
    FILE* f;
    char details[50];

    printf("Adding a New User to the system\n"); //starting message

    /* Get username */
    printf("Enter username (up to 14 characters): ");
    scanf("%14s", newUser.username);
    getchar(); // Clean buffer from \n

    /* Get password */
    printf("Enter password (up to 14 characters): ");
    scanf("%14s", newUser.password);
    getchar(); // Clean buffer from \n

    /* Get authorization level */
    printf("Enter authorization level (1-Viewer, 2-Editor, 3-Manager): ");
    if (scanf("%d", &newUser.level) != 1) {
        printf("Invalid input. Please enter a number.\n");
        while(getchar() != '\n'); // Clean buffer from invalid input
        return;
    }
    getchar(); // Clean buffer from \n

    /* Get full name */
    printf("Enter full name (up to 19 characters): ");
    fgets(newUser.fullname, MAX_FULLNAME, stdin);
    newUser.fullname[strcspn(newUser.fullname, "\n")] = 0; // remove the \n in the end

    /* update file */
    f = fopen("users.bin", "ab"); // Open the binary file in append mode 
    if (!f) { // error
        printf("Error: Could not open users file.\n");
        return;
    }

    /* write new user to file */
    if (fwrite(&newUser, sizeof(user), 1, f) == 1) {
        printf("User %s added successfully.\n", newUser.username);
        
        /* log update */
        sprintf(details, "Added new user: %s", newUser.username);
        update_log(currentUser, ACT_ADD_USER, details);
    } else {
        printf("Error: Failed to write user to file.\n");
    }

    fclose(f);
    printf("Returning to main menu.\n");
}

void update_car(user* currentUser, car_node* head) {
    char plate[MAX_PLATE];
    car_node* temp = head;
    int choice, stay = 1;
    char details[100];

    printf("Editing Car Information\n");

    /* Search for the car by license plate */
    printf("Enter the license plate of the car to edit: ");
    scanf("%14s", plate);
    getchar(); // Clean buffer

    while (temp != NULL) {
        if (strcmp(temp->car.plate, plate) == 0) {
            break;
        }
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("Car with license plate %s not found.\n", plate);
        return;
    }

    /* Menu for editing specific parameters */
    while (stay) {
        printf("Editing Car: %s (%s), enter the desired option\n", temp->car.model, temp->car.plate);
        printf("1. Price\n");
        printf("2. Kilometrage\n");
        printf("3. Color\n");
        printf("4. Test Validity (1-Yes, 0-No)\n");
        printf("5. Finish and Save\n");
        printf("Your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar(); // Clean buffer

        switch (choice) {
            case 1:
                printf("Enter new price: ");
                scanf("%lf", &temp->car.price);
                break;
            case 2:
                printf("Enter new kilometrage: ");
                scanf("%lf", &temp->car.mileage);
                break;
            case 3:
                printf("Enter new color: ");
                scanf("%19s", temp->car.color);
                break;
            case 4:
                printf("Is the test valid? (1/0): ");
                scanf("%d", &temp->car.test_valid);
                break;
            case 5:
                stay = 0;
                continue;
            default:
                printf("Option not available.\n");
                continue;
        }
        
        if (stay) {
            printf("Change another parameter for this car? (1-Yes / 0-No): ");
            scanf("%d", &stay);
            getchar();
        }
    }

    sprintf(details, "Updated car details: %s", temp->car.plate);
    update_log(currentUser, ACT_UPDATE_CAR, details);

    printf("Car details updated in memory and log. Returning to main menu.\n");

    /*Then use the save all cars function*/
}