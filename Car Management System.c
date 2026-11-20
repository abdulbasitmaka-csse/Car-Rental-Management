/* ==============================================================
   CAR RENTAL MANAGEMENT SYSTEM  (simple console version)

   Topics used: structures, arrays, functions, loops, text files.
   There are ten menu options. No frontend is needed.

   Build : gcc -std=c99 -Wall -Wextra main.c -o car_rental.exe
   Run   : .\car_rental.exe
   All records are saved in car_rental.txt in the current folder.

   The program is divided into these parts:
     1. Structures and global variables
     2. Input helper functions (reading and checking user input)
     3. Search functions
     4. File functions (save and load)
     5. Menu option functions
     6. main function
   ============================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CARS 200
#define MAX_RENTALS 1000
#define FILE_NAME "car_rental.txt"
#define FILE_HEADING "CRMS_TEXT_V1"


/* ==============================================================
   PART 1: STRUCTURES AND GLOBAL VARIABLES
   ============================================================== */

/* A structure groups the details of one car. */
struct Car {
    int id;
    char brand[32];
    char model[32];
    int year;
    double dailyRate;
    int isRented;               /* 0 = available, 1 = rented */
};

/* A second structure stores one booking (rental).
   It keeps its own copy of the car name and price, so the history
   stays correct even if the car is edited or deleted later. */
struct Rental {
    int rentalId;
    int carId;
    char vehicle[80];           /* Example: "Toyota Corolla (2020)" */
    char customer[64];
    char phone[24];
    int rentalDays;
    double dailyRate;           /* Price per day at the time of renting */
    double totalAmount;         /* dailyRate * rentalDays */
    int active;                 /* 0 = returned, 1 = still active */
};

/* Arrays store all the records.
   The counts tell us how many positions of each array are used. */
struct Car cars[MAX_CARS];
struct Rental rentals[MAX_RENTALS];
int carCount = 0;
int rentalCount = 0;
int nextRentalId = 1;           /* The ID given to the next new rental */


/* ==============================================================
   PART 2: INPUT HELPER FUNCTIONS
   ============================================================== */

/* Reads one line of text from the keyboard or from a file.
   A string is a character array that ends with '\0'.
   Returns 1 if the line was read correctly.
   Returns 0 if reading failed or the line was too long. */
int readLine(FILE *file, char text[], int size) {
    int length, ch;

    if (fgets(text, size, file) == NULL) {
        return 0;
    }

    length = strlen(text);

    /* Normal case: the line ends with a new line, so we remove it. */
    if (length > 0 && text[length - 1] == '\n') {
        text[length - 1] = '\0';
        length--;
        /* Windows files can also have '\r' before the new line. */
        if (length > 0 && text[length - 1] == '\r') {
            text[length - 1] = '\0';
        }
        return 1;
    }

    /* The array is full. Check if the line ends right here. */
    if (length == size - 1) {
        ch = fgetc(file);
        if (ch == '\n' || ch == EOF) {
            return 1;
        }
        /* The line is too long, so we throw away the rest of it. */
        while (ch != '\n' && ch != EOF) {
            ch = fgetc(file);
        }
        return 0;
    }

    /* The last line of a file may have no new line. That is fine. */
    return 1;
}

/* Returns 1 if the text contains only spaces or tabs (or nothing). */
int isBlank(char text[]) {
    int i;
    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] != ' ' && text[i] != '\t') {
            return 0;
        }
    }
    return 1;
}

/* Shows the prompt and keeps asking until the user types some text. */
void readText(char prompt[], char text[], int size) {
    while (1) {
        printf("%s", prompt);
        if (readLine(stdin, text, size) && !isBlank(text)) {
            return;
        }
        /* If the keyboard input has ended, stop the program. */
        if (feof(stdin)) {
            exit(0);
        }
        printf("Enter text with 1 to %d characters.\n", size - 1);
    }
}

/* Asks the user for a number. The number is stored using the pointer
   "number". Returns 1 if the user typed exactly one number and
   nothing else (like letters). Returns 0 otherwise. */
int readNumberFromKeyboard(char prompt[], double *number) {
    char line[100];
    char extra;

    printf("%s", prompt);
    if (!readLine(stdin, line, 100)) {
        if (feof(stdin)) {
            exit(0);
        }
        return 0;
    }

    /* sscanf returns how many values it filled.
       1 means a number was found and nothing came after it.
       2 means there were extra characters after the number. */
    if (sscanf(line, "%lf %c", number, &extra) == 1) {
        return 1;
    }
    return 0;
}

/* Keeps asking until the user enters a whole number
   between minimum and maximum (both included). */
int readNumber(char prompt[], int minimum, int maximum) {
    double number;

    while (1) {
        if (readNumberFromKeyboard(prompt, &number)) {
            /* The range is checked first, so the conversion to int is safe. */
            if (number >= minimum && number <= maximum && number == (int)number) {
                return (int)number;
            }
        }
        printf("Enter a whole number from %d to %d.\n", minimum, maximum);
    }
}

/* Keeps asking until the user enters a valid daily rent. */
double readRate(void) {
    double rate;

    while (1) {
        if (readNumberFromKeyboard("Daily rent in PKR (100-1000000): ", &rate)) {
            if (rate >= 100 && rate <= 1000000) {
                return rate;
            }
        }
        printf("Please enter a valid daily rate.\n");
    }
}

/* A phone number may contain digits, +, - and spaces only.
   It must have 7 to 20 digits. Returns 1 if valid, 0 if not. */
int validPhone(char phone[]) {
    int i, digits = 0;

    for (i = 0; phone[i] != '\0'; i++) {
        if (phone[i] >= '0' && phone[i] <= '9') {
            digits++;
        } else if (phone[i] != '+' && phone[i] != '-' && phone[i] != ' ') {
            return 0;
        }
    }

    if (digits >= 7 && digits <= 20) {
        return 1;
    }
    return 0;
}


/* ==============================================================
   PART 3: SEARCH FUNCTIONS
   They return the array position of the record, or -1 if not found.
   ============================================================== */

int findCar(int id) {
    int i;
    for (i = 0; i < carCount; i++) {
        if (cars[i].id == id) {
            return i;
        }
    }
    return -1;
}

int findRental(int id) {
    int i;
    for (i = 0; i < rentalCount; i++) {
        if (rentals[i].rentalId == id) {
            return i;
        }
    }
    return -1;
}


/* ==============================================================
   PART 4: FILE FUNCTIONS (SAVE AND LOAD)
   fopen opens a file, fprintf writes into it, fclose closes it.
   %.17g writes a decimal number without losing any precision.
   ============================================================== */

/* Writes the car at position i into the file. */
void writeCar(FILE *file, int i) {
    fprintf(file, "%d %d %.17g %d\n", cars[i].id, cars[i].year,
            cars[i].dailyRate, cars[i].isRented);
    fprintf(file, "%s\n", cars[i].brand);
    fprintf(file, "%s\n", cars[i].model);
}

/* Writes the rental at position i into the file. */
void writeRental(FILE *file, int i) {
    fprintf(file, "%d %d %d %.17g %.17g %d\n", rentals[i].rentalId,
            rentals[i].carId, rentals[i].rentalDays, rentals[i].dailyRate,
            rentals[i].totalAmount, rentals[i].active);
    fprintf(file, "%s\n", rentals[i].vehicle);
    fprintf(file, "%s\n", rentals[i].customer);
    fprintf(file, "%s\n", rentals[i].phone);
}

/* Saves everything into car_rental.txt.
   Returns 1 if saving worked, 0 if it failed. */
int saveData(void) {
    int i, failed;
    FILE *file = fopen(FILE_NAME, "w");

    if (file == NULL) {
        printf("Could not open %s for saving.\n", FILE_NAME);
        return 0;
    }

    /* First line: heading. Second line: the three counters. */
    fprintf(file, "%s\n", FILE_HEADING);
    fprintf(file, "%d %d %d\n", carCount, rentalCount, nextRentalId);

    for (i = 0; i < carCount; i++) {
        writeCar(file, i);
    }
    for (i = 0; i < rentalCount; i++) {
        writeRental(file, i);
    }

    failed = ferror(file);          /* Did any write fail? */
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        printf("Saving failed. Check disk space and folder permissions.\n");
        return 0;
    }
    return 1;
}

/* Reads the heading and the three counters. Returns 1 if they are valid. */
int readHeader(FILE *file) {
    char heading[40];

    if (!readLine(file, heading, 40)) {
        return 0;
    }
    if (strcmp(heading, FILE_HEADING) != 0) {
        return 0;
    }
    if (fscanf(file, "%d %d %d\n", &carCount, &rentalCount, &nextRentalId) != 3) {
        return 0;
    }
    if (carCount < 0 || carCount > MAX_CARS) {
        return 0;
    }
    if (rentalCount < 0 || rentalCount > MAX_RENTALS) {
        return 0;
    }
    if (nextRentalId < 1) {
        return 0;
    }
    return 1;
}

/* Reads one car from the file into position i. Returns 1 if it worked. */
int readCar(FILE *file, int i) {
    if (fscanf(file, "%d %d %lf %d\n", &cars[i].id, &cars[i].year,
               &cars[i].dailyRate, &cars[i].isRented) != 4) {
        return 0;
    }
    if (!readLine(file, cars[i].brand, 32)) {
        return 0;
    }
    if (!readLine(file, cars[i].model, 32)) {
        return 0;
    }
    if (cars[i].brand[0] == '\0' || cars[i].model[0] == '\0') {
        return 0;
    }
    return 1;
}

/* Checks that the car loaded at position i has sensible values. */
int carIsValid(int i) {
    if (cars[i].id < 1 || cars[i].id > 999999) {
        return 0;
    }
    if (cars[i].year < 1980 || cars[i].year > 2100) {
        return 0;
    }
    /* Written with ! so that invalid decimal values are also rejected. */
    if (!(cars[i].dailyRate >= 100 && cars[i].dailyRate <= 1000000)) {
        return 0;
    }
    if (cars[i].isRented != 0 && cars[i].isRented != 1) {
        return 0;
    }
    return 1;
}

/* Reads one rental from the file into position i. Returns 1 if it worked. */
int readRental(FILE *file, int i) {
    if (fscanf(file, "%d %d %d %lf %lf %d\n", &rentals[i].rentalId,
               &rentals[i].carId, &rentals[i].rentalDays, &rentals[i].dailyRate,
               &rentals[i].totalAmount, &rentals[i].active) != 6) {
        return 0;
    }
    if (!readLine(file, rentals[i].vehicle, 80)) {
        return 0;
    }
    if (!readLine(file, rentals[i].customer, 64)) {
        return 0;
    }
    if (!readLine(file, rentals[i].phone, 24)) {
        return 0;
    }
    if (rentals[i].vehicle[0] == '\0' || rentals[i].customer[0] == '\0' ||
        rentals[i].phone[0] == '\0') {
        return 0;
    }
    return 1;
}

/* Checks that the rental loaded at position i has sensible values. */
int rentalIsValid(int i) {
    if (rentals[i].rentalId < 1 || rentals[i].rentalId >= nextRentalId) {
        return 0;
    }
    if (rentals[i].rentalDays < 1 || rentals[i].rentalDays > 365) {
        return 0;
    }
    if (!(rentals[i].dailyRate >= 100 && rentals[i].dailyRate <= 1000000)) {
        return 0;
    }
    if (!(rentals[i].totalAmount >= 100 && rentals[i].totalAmount <= 365000000)) {
        return 0;
    }
    if (rentals[i].active != 0 && rentals[i].active != 1) {
        return 0;
    }
    /* An active rental must still have its car in the car list. */
    if (rentals[i].active == 1 && findCar(rentals[i].carId) == -1) {
        return 0;
    }
    return 1;
}

/* After the last record, the file may only contain empty lines.
   Returns 1 if that is true. */
int onlyBlankLinesLeft(FILE *file) {
    int ch;

    /* Check every remaining character, including spaces before newlines. */
    while ((ch = fgetc(file)) != EOF) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return 0;
        }
    }
    if (ferror(file)) {
        return 0;
    }
    return 1;
}

/* Loads the saved records when the program starts.
   Returns 1 if everything is fine, 0 if the file is damaged. */
int loadData(void) {
    int i, ok = 1;

    /* "a+" creates the file on the first run without erasing old data. */
    FILE *file = fopen(FILE_NAME, "a+");
    if (file == NULL) {
        printf("Cannot open %s.\n", FILE_NAME);
        return 0;
    }

    rewind(file);                   /* Go to the start of the file. */

    /* If the file is empty, this is the first run. Nothing to load. */
    if (fgetc(file) == EOF && !ferror(file)) {
        fclose(file);
        return 1;
    }
    rewind(file);

    if (!readHeader(file)) {
        ok = 0;
    }

    for (i = 0; ok && i < carCount; i++) {
        if (!readCar(file, i) || !carIsValid(i)) {
            ok = 0;
        }
    }

    for (i = 0; ok && i < rentalCount; i++) {
        if (!readRental(file, i) || !rentalIsValid(i)) {
            ok = 0;
        }
    }

    if (ok && !onlyBlankLinesLeft(file)) {
        ok = 0;
    }

    fclose(file);
    if (!ok) {
        printf("Invalid %s. The file has not been changed.\n", FILE_NAME);
        printf("Back up %s, then repair it or move it before restarting.\n", FILE_NAME);
    }
    return ok;
}


/* ==============================================================
   PART 5: MENU OPTION FUNCTIONS
   ============================================================== */

/* Prints the heading line of the car table. */
void printCarHeading(void) {
    printf("ID      BRAND            MODEL             YEAR    PKR/DAY       STATUS\n");
}

/* Prints one car as one row of the table.
   %-16.16s means: left aligned, width 16, at most 16 characters. */
void showCar(int i) {
    printf("%-7d %-16.16s %-17.17s %-7d %-13.2f ", cars[i].id,
           cars[i].brand, cars[i].model, cars[i].year, cars[i].dailyRate);
    if (cars[i].isRented == 1) {
        printf("RENTED\n");
    } else {
        printf("AVAILABLE\n");
    }
}

/* Menu 2: display all cars */
void displayCars(void) {
    int i;

    printf("\n--- ALL CARS ---\n");
    if (carCount == 0) {
        printf("No car records found.\n");
        return;
    }

    printCarHeading();
    for (i = 0; i < carCount; i++) {
        showCar(i);
    }
    printf("Total cars: %d\n", carCount);
}

/* Menu 1: add a new car */
void addCar(void) {
    struct Car car;

    printf("\n--- ADD NEW CAR ---\n");
    if (carCount == MAX_CARS) {
        printf("Car storage is full.\n");
        return;
    }

    car.id = readNumber("Car ID (1-999999): ", 1, 999999);
    if (findCar(car.id) != -1) {
        printf("This Car ID already exists.\n");
        return;
    }

    readText("Brand: ", car.brand, 17);
    readText("Model: ", car.model, 18);
    car.year = readNumber("Year (1980-2100): ", 1980, 2100);
    car.dailyRate = readRate();
    car.isRented = 0;               /* A new car is available */

    cars[carCount] = car;           /* Put it in the first free position */
    carCount++;
    printf("Car added.\n");
}

/* Menu 3: search a car by its ID */
void searchCar(void) {
    int id = readNumber("Enter Car ID: ", 1, 999999);
    int i = findCar(id);

    if (i == -1) {
        printf("Car not found.\n");
    } else {
        printCarHeading();
        showCar(i);
    }
}

/* Menu 4: update the details of a car */
void updateCar(void) {
    struct Car edited;
    int id = readNumber("Enter Car ID to update: ", 1, 999999);
    int i = findCar(id);

    if (i == -1) {
        printf("Car not found.\n");
        return;
    }

    /* Work on a copy. The real record changes only after all
       the new values have been entered correctly. */
    edited = cars[i];
    readText("New brand: ", edited.brand, 17);
    readText("New model: ", edited.model, 18);
    edited.year = readNumber("New year (1980-2100): ", 1980, 2100);
    edited.dailyRate = readRate();
    cars[i] = edited;

    printf("Car updated. Existing rentals keep their original price.\n");
}

/* Menu 5: delete a car */
void deleteCar(void) {
    int j;
    int id = readNumber("Enter Car ID to delete: ", 1, 999999);
    int i = findCar(id);

    if (i == -1) {
        printf("Car not found.\n");
        return;
    }
    if (cars[i].isRented == 1) {
        printf("Return this rented car first.\n");
        return;
    }

    /* Move every car after position i one place to the left. */
    for (j = i; j < carCount - 1; j++) {
        cars[j] = cars[j + 1];
    }
    carCount--;
    printf("Car deleted.\n");
}

/* Menu 6: rent a car */
void rentCar(void) {
    struct Rental rental;
    int i, id;

    if (rentalCount == MAX_RENTALS || nextRentalId == 2147483647) {
        printf("Rental history storage is full.\n");
        return;
    }

    id = readNumber("Car ID to rent: ", 1, 999999);
    i = findCar(id);
    if (i == -1) {
        printf("Car not found.\n");
        return;
    }
    if (cars[i].isRented == 1) {
        printf("This car is already rented.\n");
        return;
    }

    readText("Customer full name: ", rental.customer, 64);

    /* Ask again and again until the phone number is valid. */
    do {
        readText("Customer phone: ", rental.phone, 24);
        if (!validPhone(rental.phone)) {
            printf("Use 7-20 digits, optionally with +, - or spaces.\n");
        }
    } while (!validPhone(rental.phone));

    rental.rentalDays = readNumber("Rental duration (days, 1-365): ", 1, 365);

    /* Fill in the rest of the rental record. */
    rental.rentalId = nextRentalId;
    nextRentalId++;
    rental.carId = cars[i].id;
    /* snprintf joins the car details into one string, for example
       "Toyota Corolla (2020)". */
    snprintf(rental.vehicle, 80, "%s %s (%d)", cars[i].brand, cars[i].model,
             cars[i].year);
    rental.dailyRate = cars[i].dailyRate;
    rental.totalAmount = rental.dailyRate * rental.rentalDays;
    rental.active = 1;

    rentals[rentalCount] = rental;
    rentalCount++;
    cars[i].isRented = 1;           /* The car is no longer available */

    printf("Rental ID: %d | Total bill: PKR %.2f\n", rental.rentalId,
           rental.totalAmount);
}

/* Menu 7: return a rented car */
void returnCar(void) {
    int carIndex;
    int id = readNumber("Rental ID to close: ", 1, 2147483647);
    int i = findRental(id);

    if (i == -1) {
        printf("Rental ID not found.\n");
        return;
    }
    if (rentals[i].active == 0) {
        printf("This rental has already been returned.\n");
        return;
    }

    carIndex = findCar(rentals[i].carId);
    if (carIndex == -1) {
        printf("Associated car is missing.\n");
        return;
    }

    rentals[i].active = 0;          /* The rental is finished */
    cars[carIndex].isRented = 0;    /* The car is available again */
    printf("Car returned. Final bill: PKR %.2f\n", rentals[i].totalAmount);
}

/* Menu 8: show all rentals */
void rentalHistory(void) {
    int i;

    printf("\n--- RENTAL HISTORY ---\n");
    if (rentalCount == 0) {
        printf("No rentals recorded yet.\n");
        return;
    }

    for (i = 0; i < rentalCount; i++) {
        printf("Rental #%d | Car %d: %s | %s | %s | %d day(s) | PKR %.2f | ",
               rentals[i].rentalId, rentals[i].carId, rentals[i].vehicle,
               rentals[i].customer, rentals[i].phone, rentals[i].rentalDays,
               rentals[i].totalAmount);
        if (rentals[i].active == 1) {
            printf("ACTIVE\n");
        } else {
            printf("RETURNED\n");
        }
    }
    printf("Total rental records: %d\n", rentalCount);
}

/* Menu 9: sort the cars by daily rate (lowest first) */
void sortCars(void) {
    int i, j;
    struct Car temp;

    /* Bubble sort: compare two neighbours and swap them if they are
       in the wrong order. After each round of the inner loop, the
       biggest value moves to the end. */
    for (i = 0; i < carCount - 1; i++) {
        for (j = 0; j < carCount - i - 1; j++) {
            if (cars[j].dailyRate > cars[j + 1].dailyRate) {
                temp = cars[j];
                cars[j] = cars[j + 1];
                cars[j + 1] = temp;
            }
        }
    }
    displayCars();
}

/* Menu 10: management summary */
void summaryReport(void) {
    int i;
    int rented = 0, active = 0, completed = 0;
    double earned = 0, outstanding = 0;

    /* Count the cars that are rented right now. */
    for (i = 0; i < carCount; i++) {
        if (cars[i].isRented == 1) {
            rented++;
        }
    }

    /* Count the rentals and add up their bills. */
    for (i = 0; i < rentalCount; i++) {
        if (rentals[i].active == 1) {
            active++;
            outstanding = outstanding + rentals[i].totalAmount;
        } else {
            completed++;
            earned = earned + rentals[i].totalAmount;
        }
    }

    printf("\n--- MANAGEMENT SUMMARY ---\n");
    printf("Total cars             : %d\n", carCount);
    printf("Available cars         : %d\n", carCount - rented);
    printf("Currently rented cars  : %d\n", rented);
    printf("Active rentals         : %d\n", active);
    printf("Completed rentals      : %d\n", completed);
    printf("Completed rental value : PKR %.2f\n", earned);
    printf("Active booking value   : PKR %.2f\n", outstanding);
    printf("Values are booking charges, not proof of cash collection.\n");
}


/* ==============================================================
   PART 6: MAIN FUNCTION
   ============================================================== */

/* Returns 1 if the chosen menu option changes the data
   (so the data must be saved), otherwise 0. */
int needsSaving(int choice) {
    if (choice == 0 || choice == 1 || choice == 4 || choice == 5 ||
        choice == 6 || choice == 7 || choice == 9) {
        return 1;
    }
    return 0;
}

int main(void) {
    int choice;

    /* Load old records. Stop if the file is damaged. */
    if (!loadData()) {
        return 1;
    }

    /* Show the menu again and again until the user chooses 0. */
    do {
        printf("\n===== CAR RENTAL MANAGEMENT SYSTEM =====\n");
        printf(" 1. Add new car\n");
        printf(" 2. Display all cars\n");
        printf(" 3. Search car by ID\n");
        printf(" 4. Update car details\n");
        printf(" 5. Delete car\n");
        printf(" 6. Rent a car\n");
        printf(" 7. Return a car\n");
        printf(" 8. View rental history\n");
        printf(" 9. Sort cars by daily rate\n");
        printf("10. Management summary\n");
        printf(" 0. Save and exit\n");

        choice = readNumber("Enter choice (0-10): ", 0, 10);

        switch (choice) {
            case 1:  addCar();        break;
            case 2:  displayCars();   break;
            case 3:  searchCar();     break;
            case 4:  updateCar();     break;
            case 5:  deleteCar();     break;
            case 6:  rentCar();       break;
            case 7:  returnCar();     break;
            case 8:  rentalHistory(); break;
            case 9:  sortCars();      break;
            case 10: summaryReport(); break;
            case 0:                   break;
        }

        /* Save after every change, and when the user exits. */
        if (needsSaving(choice)) {
            if (!saveData()) {
                return 1;
            }
        }
    } while (choice != 0);

    printf("Records saved. Goodbye!\n");
    return 0;
}