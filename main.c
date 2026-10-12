#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_PRODUCTS 200
#define NAME_LEN 100
#define DATE_LEN 32
#define PRODUCT_FILE "products.csv"
#define WORK_LOG_FILE "work_log.txt"

#define BASE_BALANCE 390500
#define PAY_PER_MINUTE 9800.0

#define LOGIN_USER "happytue772"
#define LOGIN_PASSWORD "1234"

typedef struct {
    char name[NAME_LEN];
    int price;
    int stock;
    int adultOnly;
    char expiryAt[DATE_LEN];   /* YYYY-MM-DD_HH:MM:SS or - */
} Product;

Product products[MAX_PRODUCTS];
int productCount = 0;
long long currentBalance = BASE_BALANCE;
long long grossSales = 0;
long long refundTotal = 0;
time_t loginTime = 0;
time_t workStartTime = 0;

void readLine(const char *prompt, char *buf, size_t size) {
    if (prompt != NULL) {
        printf("%s", prompt);
    }

    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }

    buf[strcspn(buf, "\r\n")] = '\0';
}

int readInt(const char *prompt) {
    char buf[128];
    char *end;
    long value;

    while (1) {
        readLine(prompt, buf, sizeof(buf));
        value = strtol(buf, &end, 10);

        while (*end != '\0' && isspace((unsigned char)*end)) {
            end++;
        }

        if (buf[0] != '\0' && *end == '\0') {
            return (int)value;
        }
        printf("숫자로 다시 입력해주세요.\n");
    }
}

long long readLongLong(const char *prompt) {
    char buf[128];
    char *end;
    long long value;

    while (1) {
        readLine(prompt, buf, sizeof(buf));
        value = strtoll(buf, &end, 10);

        while (*end != '\0' && isspace((unsigned char)*end)) {
            end++;
        }

        if (buf[0] != '\0' && *end == '\0') {
            return value;
        }
        printf("숫자로 다시 입력해주세요.\n");
    }
}

void formatTime(time_t value, char *buf, size_t size) {
    struct tm *t = localtime(&value);
    if (t == NULL) {
        snprintf(buf, size, "시간 오류");
        return;
    }
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

void formatTimeForCsv(time_t value, char *buf, size_t size) {
    struct tm *t = localtime(&value);
    if (t == NULL) {
        snprintf(buf, size, "-");
        return;
    }
    strftime(buf, size, "%Y-%m-%d_%H:%M:%S", t);
}

time_t parseCsvDate(const char *text) {
    int y, m, d, hh, mm, ss;
    struct tm t;

    if (strcmp(text, "-") == 0 || text[0] == '\0') {
        return (time_t)0;
    }

    if (sscanf(text, "%d-%d-%d_%d:%d:%d", &y, &m, &d, &hh, &mm, &ss) != 6) {
        return (time_t)-1;
    }

    memset(&t, 0, sizeof(t));
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = hh;
    t.tm_min = mm;
    t.tm_sec = ss;
    t.tm_isdst = -1;

    return mktime(&t);
}

int sameName(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

int findProductIndex(const char *name) {
    int i;
    for (i = 0; i < productCount; i++) {
        if (sameName(products[i].name, name)) {
            return i;
        }
    }
    return -1;
}

void setDefaultExpiry(Product *p) {
    time_t now = time(NULL);
    time_t expiry = 0;

    if (strcmp(p->name, "우유") == 0) {
        expiry = now + 24 * 60 * 60;
    } else if (strcmp(p->name, "두부") == 0) {
        expiry = now + 24 * 60 * 60;
    } else if (strcmp(p->name, "오뎅") == 0) {
        expiry = now + 60 * 60;
    }

    if (expiry != 0) {
        formatTimeForCsv(expiry, p->expiryAt, sizeof(p->expiryAt));
    }
}

void saveProducts(void) {
    FILE *fp = fopen(PRODUCT_FILE, "w");
    int i;

    if (fp == NULL) {
        printf("[오류] %s 저장에 실패했습니다.\n", PRODUCT_FILE);
        return;
    }

    fprintf(fp, "name,price,stock,adult_only,expiry_at\n");
    for (i = 0; i < productCount; i++) {
        fprintf(fp, "%s,%d,%d,%d,%s\n",
                products[i].name,
                products[i].price,
                products[i].stock,
                products[i].adultOnly,
                products[i].expiryAt);
    }

    fclose(fp);
}

void createDefaultProducts(void) {
    Product defaults[] = {
        {"빵", 1800, 8, 0, "-"},
        {"우유", 2200, 5, 0, "-"},
        {"달걀", 7000, 4, 0, "-"},
        {"두부", 2000, 5, 0, "-"},
        {"오뎅", 1500, 6, 0, "-"},
        {"라면", 1200, 10, 0, "-"},
        {"생수", 1000, 12, 0, "-"},
        {"콜라", 2000, 7, 0, "-"},
        {"삼각김밥", 1600, 9, 0, "-"},
        {"초콜릿", 1800, 6, 0, "-"},
        {"담배", 4500, 10, 1, "-"},
        {"맥주", 3000, 8, 1, "-"},
        {"소주", 2000, 8, 1, "-"}
    };
    int count = (int)(sizeof(defaults) / sizeof(defaults[0]));
    int i;

    productCount = count;
    for (i = 0; i < count; i++) {
        products[i] = defaults[i];
        if (strcmp(products[i].expiryAt, "-") == 0) {
            setDefaultExpiry(&products[i]);
        }
    }
    saveProducts();
}

void loadProducts(void) {
    FILE *fp = fopen(PRODUCT_FILE, "r");
    char line[512];

    if (fp == NULL) {
        printf("%s 파일이 없어 기본 상품 파일을 자동 생성합니다.\n", PRODUCT_FILE);
        createDefaultProducts();
        return;
    }

    productCount = 0;
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        createDefaultProducts();
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL && productCount < MAX_PRODUCTS) {
        Product p;
        int parsed;

        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') {
            continue;
        }

        memset(&p, 0, sizeof(p));
        parsed = sscanf(line, "%99[^,],%d,%d,%d,%31[^\n]",
                        p.name, &p.price, &p.stock, &p.adultOnly, p.expiryAt);

        if (parsed == 5) {
            if (strcmp(p.expiryAt, "-") == 0) {
                setDefaultExpiry(&p);
            }
            products[productCount++] = p;
        }
    }

    fclose(fp);
    saveProducts();
}

double getWorkedMinutes(void) {
    if (workStartTime == 0) {
        return 0.0;
    }
    return difftime(time(NULL), workStartTime) / 60.0;
}

double getCurrentPay(void) {
    return getWorkedMinutes() * PAY_PER_MINUTE;
}

void printCurrentTime(void) {
    char buf[64];
    formatTime(time(NULL), buf, sizeof(buf));
    printf("현재 날짜/시간 : %s\n", buf);
}

int login(void) {
    char user[50];
    char password[50];
    int attempts = 3;

    printf("=====================================\n");
    printf("        C 편의점 POS 시스템\n");
    printf("=====================================\n");

    while (attempts > 0) {
        readLine("사용자 이름 : ", user, sizeof(user));
        readLine("암호        : ", password, sizeof(password));

        if (strcmp(user, LOGIN_USER) == 0 && strcmp(password, LOGIN_PASSWORD) == 0) {
            loginTime = time(NULL);
            printf("\n로그인 성공!\n");
            printCurrentTime();
            return 1;
        }

        attempts--;
        printf("로그인 정보가 올바르지 않습니다. 남은 횟수: %d\n", attempts);
    }

    return 0;
}

void printStockStars(int stock) {
    int i;
    if (stock <= 0) {
        printf("(재고없음)");
        return;
    }

    for (i = 0; i < stock; i++) {
        putchar('*');
    }
}

void showInventory(void) {
    int i;
    printf("\n========== 재고 체크 ==========\n");
    for (i = 0; i < productCount; i++) {
        printf("%-14s ", products[i].name);
        printStockStars(products[i].stock);
        printf("  (%d개)\n", products[i].stock);
    }
    printf("===============================\n");
}

void showBalance(void) {
    printf("\n기본 잔고(전일 매출) : %lld원\n", BASE_BALANCE);
    printf("현재 잔고           : %lld원\n", currentBalance);
}

void showSales(void) {
    printf("\n========== 오늘 매출 ==========\n");
    printf("총 판매액 : %lld원\n", grossSales);
    printf("총 환불액 : %lld원\n", refundTotal);
    printf("순 매출액 : %lld원\n", grossSales - refundTotal);
    printf("===============================\n");
}

void showExpireProducts(void) {
    int i;
    int found = 0;
    time_t now = time(NULL);

    printf("\n========== 유통기한 체크 ==========\n");
    for (i = 0; i < productCount; i++) {
        time_t expiry;
        double remain;

        if (strcmp(products[i].expiryAt, "-") == 0) {
            continue;
        }

        found = 1;
        expiry = parseCsvDate(products[i].expiryAt);
        if (expiry == (time_t)-1) {
            printf("%s : 날짜 형식 오류 (%s)\n", products[i].name, products[i].expiryAt);
            continue;
        }

        remain = difftime(expiry, now);
        printf("%-12s | %s | ", products[i].name, products[i].expiryAt);

        if (remain <= 0) {
            printf("[EXPIRED - 판매 금지]\n");
        } else if (remain < 3600) {
            printf("남은 시간 약 %.0f분\n", remain / 60.0);
        } else {
            printf("남은 시간 약 %.1f시간\n", remain / 3600.0);
        }
    }

    if (!found) {
        printf("유통기한 관리 대상 상품이 없습니다.\n");
    }
    printf("===================================\n");
}

int isExpired(const Product *p) {
    time_t expiry;

    if (strcmp(p->expiryAt, "-") == 0) {
        return 0;
    }

    expiry = parseCsvDate(p->expiryAt);
    if (expiry == (time_t)-1) {
        return 1;
    }

    return difftime(expiry, time(NULL)) <= 0;
}

int checkAdultAge(void) {
    char birth[32];
    int y, m, d;
    int age;
    time_t now = time(NULL);
    struct tm *today = localtime(&now);

    readLine("생년월일 입력 (YYYYMMDD) : ", birth, sizeof(birth));

    if (strlen(birth) != 8 || sscanf(birth, "%4d%2d%2d", &y, &m, &d) != 3) {
        printf("생년월일 형식이 올바르지 않습니다.\n");
        return 0;
    }

    if (today == NULL || m < 1 || m > 12 || d < 1 || d > 31) {
        printf("생년월일이 올바르지 않습니다.\n");
        return 0;
    }

    age = (today->tm_year + 1900) - y;
    if ((today->tm_mon + 1 < m) || ((today->tm_mon + 1 == m) && today->tm_mday < d)) {
        age--;
    }

    printf("만 나이 : %d세\n", age);

    if (age <= 19) {
        printf("[판매 금지] 19세 이하이므로 술/담배를 판매할 수 없습니다.\n");
        return 0;
    }

    printf("성인 확인 완료: 술/담배 판매 가능\n");
    return 1;
}

void sellProduct(void) {
    char name[NAME_LEN];
    int idx;
    int qty;
    long long total;
    long long paid;

    printf("\n========== 물건 판매 ==========\n");
    readLine("판매할 상품명 : ", name, sizeof(name));
    idx = findProductIndex(name);

    if (idx < 0) {
        printf("상품을 찾을 수 없습니다.\n");
        return;
    }

    if (products[idx].stock <= 0) {
        printf("재고가 없습니다.\n");
        return;
    }

    if (isExpired(&products[idx])) {
        printf("[판매 금지] 유통기한이 지난 상품입니다.\n");
        return;
    }

    if (products[idx].adultOnly) {
        printf("이 상품은 19금 상품입니다. 성인 확인이 필요합니다.\n");
        if (!checkAdultAge()) {
            return;
        }
    }

    printf("상품 가격 : %d원\n", products[idx].price);
    printf("현재 재고 : %d개\n", products[idx].stock);
    qty = readInt("판매 수량 : ");

    if (qty <= 0 || qty > products[idx].stock) {
        printf("판매 수량이 올바르지 않습니다.\n");
        return;
    }

    total = (long long)products[idx].price * qty;
    printf("총 결제 금액 : %lld원\n", total);

    paid = readLongLong("받은 금액(입금액) : ");
    if (paid < total) {
        printf("받은 금액이 부족하여 판매를 취소합니다.\n");
        return;
    }

    products[idx].stock -= qty;
    grossSales += total;
    currentBalance += total;
    saveProducts();

    printf("거스름돈 : %lld원\n", paid - total);
    printf("예상 잔고 변동 : +%lld원\n", total);
    printf("판매 후 현재 잔고 : %lld원\n", currentBalance);
    printf("판매 완료. 재고가 CSV 파일에 즉시 반영되었습니다.\n");
}

void refundProduct(void) {
    char name[NAME_LEN];
    int idx;
    int qty;
    int inputPrice;
    long long refund;

    printf("\n========== 물건 환불 ==========\n");
    readLine("환불할 상품명 : ", name, sizeof(name));
    idx = findProductIndex(name);

    if (idx < 0) {
        printf("상품을 찾을 수 없습니다.\n");
        return;
    }

    printf("등록된 상품 가격 : %d원\n", products[idx].price);
    inputPrice = readInt("환불 가격 입력 : ");

    if (inputPrice != products[idx].price) {
        printf("입력 가격이 등록 가격과 다릅니다. 환불을 취소합니다.\n");
        return;
    }

    qty = readInt("환불 수량 : ");
    if (qty <= 0) {
        printf("환불 수량이 올바르지 않습니다.\n");
        return;
    }

    refund = (long long)inputPrice * qty;
    products[idx].stock += qty;
    refundTotal += refund;
    currentBalance -= refund;
    saveProducts();

    printf("환불 금액 : %lld원\n", refund);
    printf("예상 잔고 변동 : -%lld원\n", refund);
    printf("환불 후 현재 잔고 : %lld원\n", currentBalance);
    printf("환불 완료. 재고가 CSV 파일에 즉시 반영되었습니다.\n");
}

void stockInProduct(void) {
    char name[NAME_LEN];
    char expiry[DATE_LEN];
    int idx;
    int qty;

    printf("\n========== 물품 입고 ==========\n");
    readLine("상품명 : ", name, sizeof(name));
    idx = findProductIndex(name);

    if (idx >= 0) {
        int choice;
        printf("[중복 확인] 이미 등록된 상품입니다.\n");
        printf("상품명: %s / 가격: %d원 / 재고: %d개\n",
               products[idx].name, products[idx].price, products[idx].stock);

        choice = readInt("기존 상품에 수량을 추가할까요? (1:예 / 0:아니오) : ");
        if (choice != 1) {
            printf("입고를 취소합니다.\n");
            return;
        }

        qty = readInt("추가 입고 수량 : ");
        if (qty <= 0) {
            printf("수량이 올바르지 않습니다.\n");
            return;
        }

        products[idx].stock += qty;

        if (strcmp(name, "우유") == 0 || strcmp(name, "두부") == 0 || strcmp(name, "오뎅") == 0) {
            setDefaultExpiry(&products[idx]);
            printf("해당 상품의 유통기한을 입고 시점 기준으로 자동 갱신했습니다: %s\n",
                   products[idx].expiryAt);
        } else {
            readLine("새 유통기한 (YYYY-MM-DD_HH:MM:SS, 없으면 -) : ", expiry, sizeof(expiry));
            if (strcmp(expiry, "-") != 0 && parseCsvDate(expiry) == (time_t)-1) {
                printf("날짜 형식이 잘못되어 기존 유통기한을 유지합니다.\n");
            } else {
                strncpy(products[idx].expiryAt, expiry, sizeof(products[idx].expiryAt) - 1);
                products[idx].expiryAt[sizeof(products[idx].expiryAt) - 1] = '\0';
            }
        }

        saveProducts();
        printf("입고 완료. 현재 재고: %d개\n", products[idx].stock);
        return;
    }

    if (productCount >= MAX_PRODUCTS) {
        printf("상품 저장 공간이 가득 찼습니다.\n");
        return;
    }

    memset(&products[productCount], 0, sizeof(Product));
    strncpy(products[productCount].name, name, NAME_LEN - 1);
    products[productCount].price = readInt("가격 입력 : ");
    products[productCount].stock = readInt("입고 수량 : ");
    products[productCount].adultOnly = readInt("19금 상품 여부 (1:술/담배 등, 0:일반) : ");

    if (products[productCount].price < 0 || products[productCount].stock < 0) {
        printf("가격 또는 재고 수량이 올바르지 않습니다.\n");
        memset(&products[productCount], 0, sizeof(Product));
        return;
    }

    if (strcmp(name, "우유") == 0 || strcmp(name, "두부") == 0 || strcmp(name, "오뎅") == 0) {
        strcpy(products[productCount].expiryAt, "-");
        setDefaultExpiry(&products[productCount]);
        printf("기본 유통기한 자동 적용: %s\n", products[productCount].expiryAt);
    } else {
        while (1) {
            readLine("Expire date (YYYY-MM-DD_HH:MM:SS, 없으면 -) : ", expiry, sizeof(expiry));
            if (strcmp(expiry, "-") == 0 || parseCsvDate(expiry) != (time_t)-1) {
                strncpy(products[productCount].expiryAt, expiry, DATE_LEN - 1);
                products[productCount].expiryAt[DATE_LEN - 1] = '\0';
                break;
            }
            printf("날짜 형식이 올바르지 않습니다.\n");
        }
    }

    productCount++;
    saveProducts();
    printf("새 상품 입고 완료. 이제 검색할 수 있습니다.\n");
}

void searchProduct(void) {
    char name[NAME_LEN];
    int idx;

    printf("\n========== 물품 검색 ==========\n");
    readLine("검색할 상품명 : ", name, sizeof(name));
    idx = findProductIndex(name);

    if (idx < 0) {
        printf("검색 결과: 해당 상품이 없습니다.\n");
        return;
    }

    printf("상품명       : %s\n", products[idx].name);
    printf("가격         : %d원\n", products[idx].price);
    printf("재고         : %d개 ", products[idx].stock);
    printStockStars(products[idx].stock);
    printf("\n19금 상품    : %s\n", products[idx].adultOnly ? "예" : "아니오");
    printf("유통기한     : %s\n", products[idx].expiryAt);
    printf("유통기한 상태: %s\n", isExpired(&products[idx]) ? "만료/판매금지" : "정상");
}

void deleteProduct(void) {
    char name[NAME_LEN];
    int idx;
    int i;
    int confirm;

    printf("\n========== 물품 삭제 ==========\n");
    readLine("삭제할 상품명 : ", name, sizeof(name));
    idx = findProductIndex(name);

    if (idx < 0) {
        printf("해당 상품이 없습니다.\n");
        return;
    }

    printf("삭제 대상: %s / 가격 %d원 / 재고 %d개\n",
           products[idx].name, products[idx].price, products[idx].stock);
    confirm = readInt("정말 삭제할까요? (1:삭제 / 0:취소) : ");

    if (confirm != 1) {
        printf("삭제를 취소했습니다.\n");
        return;
    }

    for (i = idx; i < productCount - 1; i++) {
        products[i] = products[i + 1];
    }
    productCount--;
    saveProducts();

    printf("삭제 완료. CSV 저장 데이터에서도 삭제되었습니다.\n");
    if (findProductIndex(name) < 0) {
        printf("검색 확인 결과: '%s' 상품이 존재하지 않습니다.\n", name);
    }
}

void workMenu(void) {
    int choice;

    if (workStartTime == 0) {
        workStartTime = time(NULL);
        printf("\n업무 시작 시간이 기록되었습니다.\n");
        printCurrentTime();
    }

    while (1) {
        printf("\n=========== 업무 메뉴 ===========\n");
        printf("[1] 물건 판매\n");
        printf("[2] 물건 환불\n");
        printf("[3] 19금 상품 성인 확인\n");
        printf("[4] 물품 입고\n");
        printf("[5] 물품 검색\n");
        printf("[6] 물품 삭제\n");
        printf("[0] 메인 메뉴로\n");
        printf("---------------------------------\n");
        printf("현재 근무 시간 : %.2f분\n", getWorkedMinutes());
        printf("현재 예상 수당 : %.0f원\n", getCurrentPay());
        printf("=================================\n");

        choice = readInt("선택 : ");

        switch (choice) {
            case 1:
                sellProduct();
                break;
            case 2:
                refundProduct();
                break;
            case 3:
                checkAdultAge();
                break;
            case 4:
                stockInProduct();
                break;
            case 5:
                searchProduct();
                break;
            case 6:
                deleteProduct();
                break;
            case 0:
                return;
            default:
                printf("메뉴 번호를 다시 선택해주세요.\n");
        }
    }
}

int verifyExitPassword(void) {
    char password[50];
    readLine("종료 패스워드 입력 : ", password, sizeof(password));
    return strcmp(password, LOGIN_PASSWORD) == 0;
}

void appendWorkLog(time_t logoutTime) {
    FILE *fp = fopen(WORK_LOG_FILE, "a");
    char loginBuf[64];
    char logoutBuf[64];

    if (fp == NULL) {
        return;
    }

    formatTime(loginTime, loginBuf, sizeof(loginBuf));
    formatTime(logoutTime, logoutBuf, sizeof(logoutBuf));

    fprintf(fp,
            "LOGIN=%s | LOGOUT=%s | WORK_MIN=%.2f | PAY=%.0f | GROSS=%lld | REFUND=%lld | NET=%lld | BALANCE=%lld\n",
            loginBuf,
            logoutBuf,
            getWorkedMinutes(),
            getCurrentPay(),
            grossSales,
            refundTotal,
            grossSales - refundTotal,
            currentBalance);

    fclose(fp);
}

void showExitLog(void) {
    time_t logoutTime = time(NULL);
    char loginBuf[64];
    char workStartBuf[64];
    char logoutBuf[64];

    formatTime(loginTime, loginBuf, sizeof(loginBuf));
    formatTime(logoutTime, logoutBuf, sizeof(logoutBuf));

    if (workStartTime != 0) {
        formatTime(workStartTime, workStartBuf, sizeof(workStartBuf));
    } else {
        strcpy(workStartBuf, "업무 시작 안 함");
    }

    printf("\n=========================================\n");
    printf("              최종 업무 로그\n");
    printf("=========================================\n");
    printf("로그인 시간       : %s\n", loginBuf);
    printf("업무 시작 시간    : %s\n", workStartBuf);
    printf("종료 시간         : %s\n", logoutBuf);
    printf("일한 시간         : %.2f분\n", getWorkedMinutes());
    printf("오늘 수당         : %.0f원\n", getCurrentPay());
    printf("총 판매액         : %lld원\n", grossSales);
    printf("총 환불액         : %lld원\n", refundTotal);
    printf("오늘 총 매출액    : %lld원\n", grossSales - refundTotal);
    printf("최종 현재 잔고    : %lld원\n", currentBalance);
    printf("=========================================\n");

    appendWorkLog(logoutTime);
    printf("업무 로그가 %s 파일에 저장되었습니다.\n", WORK_LOG_FILE);
}

int main(void) {
    int choice;

    loadProducts();

    if (!login()) {
        printf("로그인 실패로 프로그램을 종료합니다.\n");
        return 0;
    }

    while (1) {
        printf("\n============= 메인 메뉴 =============\n");
        printCurrentTime();
        printf("[1] 재고 체크\n");
        printf("[2] 현재 잔고 체크\n");
        printf("[3] 매출액\n");
        printf("[4] Expire(유통기한 물품 체크)\n");
        printf("[5] 업무 시작\n");
        printf("[6] 종료\n");
        if (workStartTime != 0) {
            printf("-------------------------------------\n");
            printf("실시간 근무 시간 : %.2f분\n", getWorkedMinutes());
            printf("실시간 예상 수당 : %.0f원\n", getCurrentPay());
        }
        printf("=====================================\n");

        choice = readInt("선택 : ");

        switch (choice) {
            case 1:
                showInventory();
                break;
            case 2:
                showBalance();
                break;
            case 3:
                showSales();
                break;
            case 4:
                showExpireProducts();
                break;
            case 5:
                workMenu();
                break;
            case 6:
                if (verifyExitPassword()) {
                    showExitLog();
                    return 0;
                }
                printf("패스워드가 틀렸습니다. 종료하지 않습니다.\n");
                break;
            default:
                printf("메뉴 번호를 다시 선택해주세요.\n");
        }
    }
}
