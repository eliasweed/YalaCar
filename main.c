#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "func.h"

/* פונקציית עזר ליצירת צומת רכב לבדיקה - מעודכנת לשמות השדות החדשים */
car_node* create_test_car(const char* plate, const char* model, double price) {
    car_node* newNode = (car_node*)malloc(sizeof(car_node));
    if (!newNode) return NULL;

    /* אתחול נתונים טקסטואליים */
    strncpy(newNode->car.plate, plate, MAX_PLATE - 1);
    strncpy(newNode->car.model, model, MAX_MODEL - 1);
    strncpy(newNode->car.make, "Toyota", MAX_MAKE - 1);
    strncpy(newNode->car.color, "Silver", MAX_COLOR - 1);
    
    /* אתחול נתונים מספריים ובוליאניים */
    newNode->car.price = price;
    newNode->car.mileage = 10000.5;
    newNode->car.seats = 5;
    newNode->car.test_valid = 1;
    newNode->car.is_electric = 0;

    /* אתחול תאריכים (באמצעות ה-struct date החדש) */
    newNode->car.manufacture_date.day = 1;
    newNode->car.manufacture_date.month = 1;
    newNode->car.manufacture_date.year = 2022;
    
    newNode->next = NULL;
    newNode->prev = NULL;
    
    return newNode;
}

int main() {
    /* 1. יצירת משתמש מדומה (מנהל) לצורך ה-Log */
    user tester = {
        .username = "admin_test",
        .level = 3,
        .fullname = "Test Manager"
    };

    /* 2. הקמת רשימה מקושרת עם רכב אחד */
    car_node* head = create_test_car("123-45-678", "Corolla", 130000.0);
    if (!head) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("--- Test: update_car function ---\n");
    printf("Initial State: Plate: %s, Model: %s, Price: %.2f\n\n", 
            head->car.plate, head->car.model, head->car.price);

    /* 3. הרצת הפונקציה - יש להקיש לוחית 123-45-678 */
    update_car(&tester, head);

    /* 4. הדפסת תוצאות לאחר העדכון בזיכרון */
    printf("\n--- Results After Update in Memory ---\n");
    printf("Plate: %s\n", head->car.plate);
    printf("New Price: %.2f\n", head->car.price);
    printf("New Mileage: %.2f\n", head->car.mileage);
    printf("New Color: %s\n", head->car.color);
    printf("Test Valid: %s\n", head->car.test_valid ? "Yes" : "No");

    /* 5. וידוא רישום ב-Log */
    printf("\nChecking %s for UPDATE_CAR action...\n", LOG_FILE);
    FILE* logF = fopen(LOG_FILE, "r");
    if (logF) {
        char line[512];
        while (fgets(line, sizeof(line), logF)) {
            if (strstr(line, "UPDATE_CAR")) {
                printf("Log entry: %s", line);
            }
        }
        fclose(logF);
    }

    /* 6. שחרור זיכרון */
    free(head);

    return 0;
}