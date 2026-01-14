#include <stdio.h>
#include <stdlib.h>
#include "func.h"

int main() {
    user_t currentUser;

    // 1. System initialization: create missing files (users.dat, cars.dat, log.txt)
    ensure_system_files_exist();

    // 2. Create default admin:admin user if the file is empty
    ensure_admin_user_exists();

    printf("========================================\n");
    printf("      Welcome to YALA CAR System       \n");
    printf("========================================\n");

    // 3. Main login loop
    while (1) {
        printf("\nPlease login to continue (or press Ctrl+C to exit)\n");
        
        if (login_flow(&currentUser)) {
            // If login is successful, move to the main menu
            run_main_menu(&currentUser);
            
            // After exiting the main menu (Logout), return to the login screen
            printf("\nLogged out successfully.\n");
        } else {
            // If the user fails after 3 attempts
            printf("\nToo many failed attempts. Exiting.\n");
            return 1;
        }
    }

    return 0;
}