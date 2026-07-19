/* ============================================================
   BANK MANAGEMENT SYSTEM
   Language : C
   Concepts : Structures, File Handling, Input Validation
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ACC_FILE        "accounts.dat"
#define TXN_FILE        "transactions.dat"
#define STARTING_ACC_NO 1001
#define MIN_DEPOSIT     500.0

/* ---------------------------------------------------------
   STRUCTURES
   --------------------------------------------------------- */

typedef struct {
    int    accNo;
    char   name[50];
    char   pin[5];      /* 4-digit PIN stored as a string (keeps leading zeros) */
    double balance;
} Account;

typedef struct {
    int    accNo;
    char   type[12];    /* "CREATE", "DEPOSIT", "WITHDRAW" */
    double amount;
    double balanceAfter;
    char   timestamp[26];
} Transaction;

/* ---------------------------------------------------------
   VALIDATION HELPERS
   --------------------------------------------------------- */

int isValidName(const char *name) {
    if (strlen(name) == 0) return 0;
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalpha((unsigned char)name[i]) && name[i] != ' ') return 0;
    }
    return 1;
}

int isValidPin(const char *pin) {
    if (strlen(pin) != 4) return 0;
    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)pin[i])) return 0;
    }
    return 1;
}

int isValidAmount(double amt) {
    return amt > 0;
}

void getTimestamp(char *buffer, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

/* ---------------------------------------------------------
   FILE HELPERS
   --------------------------------------------------------- */

int getAccountCount(void) {
    FILE *fp = fopen(ACC_FILE, "rb");
    if (fp == NULL) return 0;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return (int)(size / sizeof(Account));
}

int findAccountByNo(int accNo, Account *result) {
    FILE *fp = fopen(ACC_FILE, "rb");
    if (fp == NULL) return 0;
    Account temp;
    while (fread(&temp, sizeof(Account), 1, fp) == 1) {
        if (temp.accNo == accNo) {
            *result = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int updateAccountRecord(Account updated) {
    FILE *fp = fopen(ACC_FILE, "rb+");
    if (fp == NULL) return 0;
    Account temp;
    long pos = 0;
    while (fread(&temp, sizeof(Account), 1, fp) == 1) {
        if (temp.accNo == updated.accNo) {
            fseek(fp, pos, SEEK_SET);
            fwrite(&updated, sizeof(Account), 1, fp);
            fclose(fp);
            return 1;
        }
        pos += sizeof(Account);
    }
    fclose(fp);
    return 0;
}

void logTransaction(int accNo, const char *type, double amount, double balanceAfter) {
    FILE *fp = fopen(TXN_FILE, "ab");
    if (fp == NULL) {
        printf("Warning: could not log transaction.\n");
        return;
    }
    Transaction t;
    t.accNo = accNo;
    strncpy(t.type, type, sizeof(t.type) - 1);
    t.type[sizeof(t.type) - 1] = '\0';
    t.amount = amount;
    t.balanceAfter = balanceAfter;
    getTimestamp(t.timestamp, sizeof(t.timestamp));
    fwrite(&t, sizeof(Transaction), 1, fp);
    fclose(fp);
}

/* ---------------------------------------------------------
   PIN AUTHENTICATION
   --------------------------------------------------------- */

int authenticate(int accNo, Account *acc) {
    if (!findAccountByNo(accNo, acc)) {
        printf("Account not found.\n");
        return 0;
    }
    char pin[10];
    int attempts = 3;
    while (attempts--) {
        printf("Enter 4-digit PIN: ");
        scanf("%9s", pin);
        if (strcmp(pin, acc->pin) == 0) return 1;
        printf("Incorrect PIN. %d attempt(s) left.\n", attempts);
    }
    printf("Authentication failed.\n");
    return 0;
}

/* ---------------------------------------------------------
   CORE FEATURES
   --------------------------------------------------------- */

void createAccount(void) {
    Account acc;
    char name[50], pin[10], pin2[10];

    printf("\n--- Create New Account ---\n");
    printf("Enter full name: ");
    scanf(" %[^\n]", name);
    while (!isValidName(name)) {
        printf("Invalid name (letters and spaces only). Re-enter: ");
        scanf(" %[^\n]", name);
    }

    do {
        printf("Set a 4-digit PIN: ");
        scanf("%9s", pin);
        if (!isValidPin(pin)) { printf("PIN must be exactly 4 digits.\n"); continue; }
        printf("Confirm PIN: ");
        scanf("%9s", pin2);
        if (strcmp(pin, pin2) != 0) printf("PINs do not match. Try again.\n");
    } while (!isValidPin(pin) || strcmp(pin, pin2) != 0);

    double initialDeposit;
    printf("Enter initial deposit amount (min %.2f): ", MIN_DEPOSIT);
    scanf("%lf", &initialDeposit);
    while (initialDeposit < MIN_DEPOSIT) {
        printf("Minimum initial deposit is %.2f. Enter again: ", MIN_DEPOSIT);
        scanf("%lf", &initialDeposit);
    }

    acc.accNo = STARTING_ACC_NO + getAccountCount();
    strcpy(acc.name, name);
    strcpy(acc.pin, pin);
    acc.balance = initialDeposit;

    FILE *fp = fopen(ACC_FILE, "ab");
    if (fp == NULL) { printf("Error creating account.\n"); return; }
    fwrite(&acc, sizeof(Account), 1, fp);
    fclose(fp);

    logTransaction(acc.accNo, "CREATE", initialDeposit, acc.balance);

    printf("\nAccount created successfully!\n");
    printf("Your Account Number is: %d\n", acc.accNo);
    printf("Please save this number, you will need it every time you log in.\n");
}

void deposit(void) {
    int accNo;
    printf("\n--- Deposit ---\n");
    printf("Enter account number: ");
    scanf("%d", &accNo);

    Account acc;
    if (!authenticate(accNo, &acc)) return;

    double amount;
    printf("Enter amount to deposit: ");
    scanf("%lf", &amount);
    if (!isValidAmount(amount)) {
        printf("Invalid amount. Deposit must be greater than 0.\n");
        return;
    }

    acc.balance += amount;
    updateAccountRecord(acc);
    logTransaction(accNo, "DEPOSIT", amount, acc.balance);

    printf("Deposit successful. New balance: %.2f\n", acc.balance);
}

void withdraw(void) {
    int accNo;
    printf("\n--- Withdraw ---\n");
    printf("Enter account number: ");
    scanf("%d", &accNo);

    Account acc;
    if (!authenticate(accNo, &acc)) return;

    double amount;
    printf("Enter amount to withdraw: ");
    scanf("%lf", &amount);
    if (!isValidAmount(amount)) {
        printf("Invalid amount. Withdrawal must be greater than 0.\n");
        return;
    }
    if (amount > acc.balance) {
        printf("Insufficient balance. Current balance: %.2f\n", acc.balance);
        return;
    }

    acc.balance -= amount;
    updateAccountRecord(acc);
    logTransaction(accNo, "WITHDRAW", amount, acc.balance);

    printf("Withdrawal successful. New balance: %.2f\n", acc.balance);
}

void balanceEnquiry(void) {
    int accNo;
    printf("\n--- Balance Enquiry ---\n");
    printf("Enter account number: ");
    scanf("%d", &accNo);

    Account acc;
    if (!authenticate(accNo, &acc)) return;

    printf("\nAccount Holder : %s\n", acc.name);
    printf("Account Number : %d\n", acc.accNo);
    printf("Current Balance: %.2f\n", acc.balance);
}

void transactionHistory(void) {
    int accNo;
    printf("\n--- Transaction History ---\n");
    printf("Enter account number: ");
    scanf("%d", &accNo);

    Account acc;
    if (!authenticate(accNo, &acc)) return;

    FILE *fp = fopen(TXN_FILE, "rb");
    if (fp == NULL) {
        printf("No transactions found.\n");
        return;
    }

    Transaction t;
    int found = 0;
    printf("\n%-20s %-10s %-12s %-12s\n", "Date/Time", "Type", "Amount", "Balance");
    printf("--------------------------------------------------------------\n");
    while (fread(&t, sizeof(Transaction), 1, fp) == 1) {
        if (t.accNo == accNo) {
            printf("%-20s %-10s %-12.2f %-12.2f\n", t.timestamp, t.type, t.amount, t.balanceAfter);
            found = 1;
        }
    }
    fclose(fp);
    if (!found) printf("No transactions found for this account.\n");
}

/* ---------------------------------------------------------
   MAIN MENU
   --------------------------------------------------------- */

void showMenu(void) {
    printf("\n===== BANK MANAGEMENT SYSTEM =====\n");
    printf("1. Create Account\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Balance Enquiry\n");
    printf("5. Transaction History\n");
    printf("6. Exit\n");
    printf("Enter your choice: ");
}

int main(void) {
    int choice;
    do {
        showMenu();
        if (scanf("%d", &choice) != 1) {
            /* clear bad input so we don't loop forever */
            while (getchar() != '\n');
            printf("Invalid input.\n");
            continue;
        }
        switch (choice) {
            case 1: createAccount();        break;
            case 2: deposit();              break;
            case 3: withdraw();             break;
            case 4: balanceEnquiry();       break;
            case 5: transactionHistory();   break;
            case 6: printf("Thank you for using our Bank System. Goodbye!\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (choice != 6);
    return 0;
}
