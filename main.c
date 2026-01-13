#include <stdio.h>
#include <stdlib.h>
#include "func.h"

int main() {
    user_t currentUser;

    // 1. אתחול המערכת: יצירת קבצים חסרים (users.dat, cars.dat, log.txt)
    ensure_system_files_exist();

    // 2. יצירת משתמש admin:admin ברירת מחדל אם הקובץ ריק
    ensure_admin_user_exists();

    printf("========================================\n");
    printf("      Welcome to YALA CAR System       \n");
    printf("========================================\n");

    // 3. לולאת התחברות ראשית
    while (1) {
        printf("\nPlease login to continue (or press Ctrl+C to exit)\n");
        
        if (login_flow(&currentUser)) {
            // אם ההתחברות הצליחה, עוברים לתפריט הראשי
            run_main_menu(&currentUser);
            
            // לאחר יציאה מהתפריט הראשי (Logout), חוזרים למסך הלוגין
            printf("\nLogged out successfully.\n");
        } else {
            // אם המשתמש נכשל ב-3 ניסיונות
            printf("\nToo many failed attempts. System locked briefly.\n");
        }
    }

    return 0;
}