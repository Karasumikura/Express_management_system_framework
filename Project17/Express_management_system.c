#define _CRT_SECURE_NO_WARNINGS // 忽略scanf警告(vs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SHELF_CAPACITY 50
#define WARNING_THRESHOLD 0.8
#define BASE_PRICE 10.0

// 枚举定义
typedef enum {
    SIZE_EXTRA_LARGE,
    SIZE_LARGE,
    SIZE_MEDIUM,
    SIZE_SMALL,
    SIZE_TINY
} PackageSize;

typedef enum {
    WEIGHT_LEVEL1,
    WEIGHT_LEVEL2,
    WEIGHT_LEVEL3,
    WEIGHT_LEVEL4,
    WEIGHT_LEVEL5
} PackageWeight;

typedef enum {
    SPECIAL_NONE,
    SPECIAL_FRAGILE,
    SPECIAL_UPRIGHT,
    SPECIAL_HAZARDOUS,
    SPECIAL_LIGHT_SENSITIVE,
    SPECIAL_REFRIGERATED
} SpecialFlags;

typedef enum {
    SHIPPING_STANDARD_TRUCK,
    SHIPPING_EXPRESS_ROAD,
    SHIPPING_EXPRESS_AIR,
    SHIPPING_SUPER_EXPRESS
} ShippingMethod;

// 用户数据结构
typedef struct User {
    int id;
    char name[50];
    char phone[20];
    int membership; // 0-新 1-白银 2-黄金
    double total_spent;
    time_t last_purchase;
    int purchase_count;
    struct User* next;
} User;
// 包裹数据结构
typedef struct Package {
    int id;
    int user_id;  // 用户关联字段
    double content_value; // 包裹内容物价值
    PackageSize size;     // 包裹尺寸
    PackageWeight weight; // 包裹重量 
    SpecialFlags special; // 特殊标志
    ShippingMethod shipping; // 运输方式
    char shelf_code[10];  // 货架编码
    char pickup_code[10]; // 取件码
    time_t arrival;       // 入库时间
    time_t pickup;        // 出库时间
    int status;           // 包裹状态
    double storage_fee;   // 存储费用
    struct Package* next; // 链表指针
} Package;

// 财务记录
typedef struct Finance {
    int type; // 1-计件费 2-派送费 3-保存费
    double amount;
    time_t timestamp;
    struct Finance* next;
} Finance;

// 全局链表
User* users = NULL;
Package* packages = NULL;
Finance* finances = NULL;

// 文件操作函数
void create_data_dir() {
    system("mkdir data 2>nul"); // 创建数据目录
}

void save_list(const char* filename, void* head, size_t elem_size) {
    char path[100];
    sprintf(path, "data/%s", filename);
    FILE* fp = fopen(path, "wb");
    if (!fp) return;

    while (head) {
        fwrite(head, elem_size, 1, fp);
        if (elem_size == sizeof(User)) head = ((User*)head)->next;
        else if (elem_size == sizeof(Package)) head = ((Package*)head)->next;
        else if (elem_size == sizeof(Finance)) head = ((Finance*)head)->next;
    }
    fclose(fp);
}

void load_list(const char* filename, void** head, size_t elem_size) {
    char path[100];
    sprintf(path, "data/%s", filename);
    FILE* fp = fopen(path, "rb");
    if (!fp) return;

    void* prev = NULL;
    while (1) {
        void* curr = malloc(elem_size);
        if (fread(curr, elem_size, 1, fp) != 1) {
            free(curr);
            break;
        }

        if (prev) {
            if (elem_size == sizeof(User)) ((User*)prev)->next = curr;
            else if (elem_size == sizeof(Package)) ((Package*)prev)->next = curr;
            else if (elem_size == sizeof(Finance)) ((Finance*)prev)->next = curr;
        }
        else {
            *head = curr;
        }

        prev = curr;
        // 清空链表指针
        if (elem_size == sizeof(User)) ((User*)curr)->next = NULL;
        else if (elem_size == sizeof(Package)) ((Package*)curr)->next = NULL;
        else if (elem_size == sizeof(Finance)) ((Finance*)curr)->next = NULL;
    }
    fclose(fp);
}

void save_all_data() {
    save_list("users.dat", users, sizeof(User));
    save_list("packages.dat", packages, sizeof(Package));
    save_list("finances.dat", finances, sizeof(Finance));
}

void load_all_data() {
    load_list("users.dat", (void**)&users, sizeof(User));
    load_list("packages.dat", (void**)&packages, sizeof(Package));
    load_list("finances.dat", (void**)&finances, sizeof(Finance));
}

// 函数声明
void generate_pickup_code(char* code);
void calculate_pricing(User* user, Package* pkg);
void add_package();
void inventory_check();
void package_management();
void add_user();
void find_user_menu();
void update_membership();
void handle_exception(int pkg_id);
void financial_management();
void generate_reports();
Package* find_package(int pkg_id);

// 生成取件码（示例实现）
void generate_pickup_code(char* code) {
    static int counter = 1000;
    sprintf(code, "PK%d%03d", (int)time(NULL) % 10000, counter++);
}

// 价格计算（包含杀熟逻辑和动态调价）
// 参数：
//   user - 用户指针，包含会员信息和消费记录
//   pkg - 包裹指针，包含包裹属性和费用信息
// 功能：
//   1. 根据会员等级应用折扣
//   2. 根据消费行为实施动态定价（大数据杀熟）
//   3. 计算特殊处理和运输方式的附加费
void calculate_pricing(User* user, Package* pkg) {
    double base = BASE_PRICE;
    time_t now = time(NULL);

    /* 会员折扣策略：
     * 新用户首单9折
     * 黄金会员永久8折
     * 白银会员无折扣 */
    if (user->membership == 0 && user->purchase_count == 0) { // 新用户首单
        base *= 0.9;
    }
    else if (user->membership == 2) { // 黄金会员
        base *= 0.8;
    }

    /* 大数据杀熟逻辑：
     * 最近30天消费3次以上 + 15%
     * 总消费超过5000元 + 20%
     * 高频消费（每周超过2次） + 10% */
    time_t period = now - user->last_purchase;
    if (user->purchase_count > 5 || user->total_spent > 1000) {
        base *= 1.2; // 基础加价20%

        // 动态调价
        if (user->total_spent > 5000) {
            base *= 1.2;  // 高消费用户额外加20%
        }
        if (user->purchase_count / (difftime(now, user->last_purchase) / 86400) > 0.3) { // 日均消费0.3次以上
            base *= 1.1; // 高频加价10%
        }
    }

    // 特殊处理附加费
    switch (pkg->special) {
    case SPECIAL_FRAGILE:      base += 8.0; break;
    case SPECIAL_UPRIGHT:      base += 5.0; break;
    case SPECIAL_HAZARDOUS:    base += 15.0; break;
    case SPECIAL_LIGHT_SENSITIVE: base += 3.0; break;
    case SPECIAL_REFRIGERATED: base += 10.0; break;
    default: break;
    }

    // 运输方式附加费
    switch (pkg->shipping) {
    case SHIPPING_EXPRESS_ROAD: base += 5.0; break;
    case SHIPPING_EXPRESS_AIR:  base += 15.0; break;
    case SHIPPING_SUPER_EXPRESS: base += 20.0; break;
    default: break;
    }

    pkg->storage_fee = round(base * 100) / 100; // 保留两位小数
}
// 用户ID管理
static int user_id = 0;
static int user_id_initialized = 0;

// 更新会员等级（自动根据消费行为调整）
void update_membership() {
    User* curr = users;
    time_t now = time(NULL);

    while (curr) {
        // 自动升级逻辑
        if (curr->total_spent > 5000 && difftime(now, curr->last_purchase) < 2592000) {
            curr->membership = 2; // 黄金会员
        }
        else if (curr->total_spent > 1000 && difftime(now, curr->last_purchase) < 2592000) {
            curr->membership = 1; // 白银会员
        }

        // 自动降级逻辑
        if (curr->membership == 2 && difftime(now, curr->last_purchase) > 7776000) {
            curr->membership = 1; // 黄金降为白银
        }
        else if (curr->membership == 1 && difftime(now, curr->last_purchase) > 15552000) {
            curr->membership = 0; // 白银降为新用户
        }

        curr = curr->next;
    }
    printf("会员等级已自动更新！\n");
}

// 查找包裹实现
Package* find_package(int pkg_id) {
    if (pkg_id == 0) {
        printf("输入包裹ID: ");
        scanf("%d", &pkg_id);
    }

    Package* curr = packages;
    while (curr) {
        if (curr->id == pkg_id) {
            printf("找到包裹%d\n", pkg_id);
            return curr;
        }
        curr = curr->next;
    }
    printf("未找到包裹%d\n", pkg_id);
    return NULL;
}

// 包裹入库
// 包裹ID管理
static int pkg_id = 0;
static int pkg_id_initialized = 0;

void init_package_id() {
    if (!pkg_id_initialized) {
        FILE* fp = fopen("data/max_ids.dat", "rb");
        if (fp) {
            fseek(fp, sizeof(int), SEEK_SET); // 跳过用户ID
            fread(&pkg_id, sizeof(int), 1, fp);
            fclose(fp);
        }
        else {
            pkg_id = 1; // 默认起始ID
        }
        pkg_id_initialized = 1;
    }
}

void save_package_id() {
    FILE* fp = fopen("data/max_ids.dat", "rb+");
    if (fp) {
        fseek(fp, sizeof(int), SEEK_SET); // 定位到包裹ID位置
        fwrite(&pkg_id, sizeof(int), 1, fp);
        fclose(fp);
    }
}

// 输入验证函数
int get_valid_input(const char* prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) != 1) {
            printf("输入错误，请输入数字！\n");
            while (getchar() != '\n'); // 清空输入缓冲区
            continue;
        }
        if (value >= min && value <= max) {
            return value;
        }
        printf("输入无效，请输入%d到%d之间的数字！\n", min, max);
    }
}

void add_package() {
    init_package_id();

    Package* new_pkg = (Package*)malloc(sizeof(Package));
    memset(new_pkg, 0, sizeof(Package));
    new_pkg->next = NULL;

    new_pkg->id = pkg_id++;
    save_package_id();

    // 获取包裹详细信息（使用输入验证）
    new_pkg->size = get_valid_input(
        "包裹尺寸（0-极大 1-大 2-中 3-小 4-极小）: ",
        0, 4);

    new_pkg->weight = get_valid_input(
        "包裹重量等级（0-5kg 1-10kg 2-20kg 3-30kg 4-50kg）: ",
        0, 4);

    new_pkg->special = get_valid_input(
        "特殊标志（0-无 1-易碎 2-不可倒放 3-危险品 4-避光 5-冷藏）: ",
        0, 5);

    new_pkg->shipping = get_valid_input(
        "运输方式（0-标准货车 1-加急公路 2-特快空运 3-特快公路）: ",
        0, 3);

    // 输入内容物价值
    printf("输入包裹内容物价值: ");
    scanf("%lf", &new_pkg->content_value);

    new_pkg->arrival = time(NULL);  // 记录入库时间

    // 用户关联处理
    int user_input_id;
    User* target_user = NULL;
    do {
        printf("输入用户ID（0表示新建用户）: ");
        scanf("%d", &user_input_id);

        if (user_input_id == 0) {
            add_user(); // 转到新建用户模块
            user_input_id = user_id - 1; // 获取最新创建的用户ID
            target_user = users; // 新用户位于链表头部
        }
        else {
            // 查找用户
            User* curr = users;
            while (curr) {
                if (curr->id == user_input_id) {
                    target_user = curr;
                    break;
                }
                curr = curr->next;
            }
            if (!target_user) {
                printf("未找到用户%d，请重新输入或新建用户（输入0）\n", user_input_id);
            }
        }
    } while (!target_user);

    new_pkg->user_id = target_user->id;
    target_user->total_spent += new_pkg->content_value; // 累计消费金额

    // 生成取件码和货架码
    generate_pickup_code(new_pkg->pickup_code);
    sprintf(new_pkg->shelf_code, "SH%02d", rand() % 100);

    // 加入链表
    new_pkg->next = packages;
    packages = new_pkg;

    printf("包裹%d入库成功！取件码：%s\n", new_pkg->id, new_pkg->pickup_code);
}

// 库存盘点
void inventory_check() {
    int counts[5] = { 0 };
    Package* curr = packages;

    while (curr) {
        if (curr->status == 0) { // 仅统计在库
            counts[curr->size]++;
        }
        curr = curr->next;
    }

    printf("\n当前库存：\n");
    const char* size_names[] = { "极大", "大", "中", "小", "极小" };
    for (int i = 0; i < 5; i++) {
        printf("%s: %d件 (%.1f%%)\n",
            size_names[i],
            counts[i],
            (float)counts[i] / SHELF_CAPACITY * 100);

        if (counts[i] > SHELF_CAPACITY * WARNING_THRESHOLD) {
            printf("⚠️ 库存预警！%s包裹超过阈值\n", size_names[i]);
        }
    }
}

// 用户管理菜单
void user_management() {
    int choice;
    do {
        printf("\n用户管理\n");
        printf("1. 添加用户\n");
        printf("2. 查询用户\n");
        printf("3. 更新会员等级\n");
        printf("0. 返回主菜单\n");
        printf("请选择操作: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1: add_user(); break;
        case 2: find_user_menu(); break;
        case 3: update_membership(); break;
        case 0: return;
        default: printf("无效选择!\n");
        }
    } while (1);
}

// 添加用户
// 从文件加载最大用户ID
int load_max_user_id() {
    FILE* fp = fopen("data/max_ids.dat", "rb");
    int max_id = 1000;
    if (fp) {
        fread(&max_id, sizeof(int), 1, fp);
        fclose(fp);
    }
    return max_id;
}

// 保存最大用户ID
void save_max_user_id(int id) {
    FILE* fp = fopen("data/max_ids.dat", "wb");
    if (fp) {
        fwrite(&id, sizeof(int), 1, fp);
        fclose(fp);
    }
}



void init_user_id() {
    if (!user_id_initialized) {
        user_id = load_max_user_id();
        user_id_initialized = 1;
    }
}

void add_user() {
    init_user_id();  // 确保只初始化一次

    User* new_user = (User*)malloc(sizeof(User));
    memset(new_user, 0, sizeof(User));
    new_user->next = NULL;

    new_user->id = user_id++;
    save_max_user_id(user_id);

    printf("输入用户名: ");
    scanf("%49s", new_user->name);
    printf("输入联系电话: ");
    scanf("%19s", new_user->phone);

    new_user->membership = 0; // 默认新用户
    new_user->last_purchase = time(NULL);

    // 加入链表
    new_user->next = users;
    users = new_user;

    printf("用户添加成功！ID: %d\n", new_user->id);
}

// 包裹异常处理
void handle_exception(int pkg_id) {
    Package* pkg = find_package(pkg_id);
    if (!pkg) {
        printf("包裹不存在！\n");
        return;
    }

    printf("选择异常类型:\n");
    printf("1. 损坏\n2. 丢失\n3. 误领\n4. 拒收\n");
    int choice;
    scanf("%d", &choice);

    pkg->status = 2; // 标记为异常
    Finance* f = (Finance*)malloc(sizeof(Finance));
    f->type = 3; // 保存费
    f->amount = pkg->storage_fee * 2; // 双倍赔偿
    f->timestamp = time(NULL);
    f->next = finances;
    finances = f;

    printf("已记录异常并生成赔偿账单\n");
}

// 财务统计（增强版）
void financial_management() {
    printf("\n=== 财务统计 ===\n");
    double income[4] = { 0 }; // 0:总 1:计件 2:派送 3:保存
    double monthly_growth[12] = { 0 }; // 每月增长
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;

    // 详细统计
    Finance* curr = finances;
    while (curr) {
        struct tm* tm_record = localtime(&curr->timestamp);
        if (tm_record->tm_year + 1900 == current_year) {
            monthly_growth[tm_record->tm_mon] += curr->amount;
        }

        income[0] += curr->amount;
        if (curr->type >= 1 && curr->type <= 3) {
            income[curr->type] += curr->amount;
        }
        curr = curr->next;
    }

    // 基础统计
    printf("【基础统计】\n");
    printf("总收入: ￥%.2f\n", round(income[0] * 100) / 100);
    printf("├─ 计件费: ￥%.2f\n", round(income[1] * 100) / 100);
    printf("├─ 派送费: ￥%.2f\n", round(income[2] * 100) / 100);
    printf("└─ 保存费: ￥%.2f\n", round(income[3] * 100) / 100);

    // 增长趋势
    printf("\n【月增长趋势】\n");
    for (int i = 0; i < 12; i++) {
        printf("%02d月: ￥%-8.2f", i + 1, monthly_growth[i]);
        if ((i + 1) % 3 == 0) printf("\n");
    }

    // 分类统计
    printf("\n【分类统计】\n");
    printf("会员等级分布:\n");
    User* u = users;
    double member_income[3] = { 0 };
    while (u) {
        member_income[u->membership] += u->total_spent;
        u = u->next;
    }
    printf("新用户: ￥%.2f\n", member_income[0]);
    printf("白银会员: ￥%.2f\n", member_income[1]);
    printf("黄金会员: ￥%.2f\n", member_income[2]);

    // 图表显示（待定）
}



// 包裹管理菜单
void package_management() {
    int choice;
    do {
        printf("\n包裹管理\n");
        printf("1. 包裹入库\n");
        printf("2. 包裹出库\n");
        printf("3. 查询包裹\n");
        printf("4. 异常处理\n");
        printf("0. 返回主菜单\n");
        printf("请选择操作: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1: add_package(); break;
        case 2:
            printf("输入包裹ID: ");
            int out_id;
            scanf("%d", &out_id);
            Package* out_pkg = find_package(out_id);
            if (out_pkg && out_pkg->status == 0) {
                printf("输入取件码: ");
                char input_code[10];
                scanf("%9s", input_code);

                if (strcmp(input_code, out_pkg->pickup_code) == 0) {
                    out_pkg->status = 1;
                    out_pkg->pickup = time(NULL);

                    // 记录计件费和派送费
                    Finance* f = (Finance*)malloc(sizeof(Finance));
                    f->type = 1; // 计件费
                    f->amount = out_pkg->storage_fee * 0.7; // 70%为计件费
                    f->timestamp = time(NULL);
                    f->next = finances;
                    finances = f;

                    // 更新用户消费记录
                    User* curr_user = users;
                    while (curr_user) {
                        if (curr_user->id == out_pkg->user_id) {
                            curr_user->total_spent += out_pkg->storage_fee;
                            curr_user->last_purchase = time(NULL);
                            curr_user->purchase_count++;
                            break;
                        }
                        curr_user = curr_user->next;
                    }
                    printf("包裹%d出库成功！\n", out_id);
                }
                else {
                    printf("取件码错误！\n");
                }
            }
            else {
                printf("包裹不存在或不可出库\n");
            }
            break;
        case 3: find_package(0); break;
        case 4:
            printf("输入包裹ID: ");
            int id;
            scanf("%d", &id);
            handle_exception(id);
            break;
        case 0: return;
        default: printf("无效选择!\n");
        }
    } while (1);
}

// 查找用户菜单
void find_user_menu() {
    int choice;
    do {
        printf("\n用户查询\n");
        printf("1. 按ID查询\n");
        printf("2. 按姓名查询\n");
        printf("3. 按电话查询\n");
        printf("0. 返回\n");
        printf("请选择: ");
        scanf("%d", &choice);

        if (choice == 0) return;

        char search_str[50];
        User* curr = users;
        int found = 0;
        int search_id = 0;  // 将search_id声明移到外部

        switch (choice) {
        case 1: {
            printf("输入用户ID: ");
            if (scanf("%d", &search_id) != 1) {
                printf("无效的ID输入!\n");
                while (getchar() != '\n'); // 清空输入缓冲区
                continue;
            }
            while (curr) {
                if (curr->id == search_id) {
                    found = 1;
                    break;
                }
                curr = curr->next;
            }
            break;
        }
        case 2: {
            printf("输入姓名: ");
            scanf("%49s", search_str);
            while (curr) {
                if (strcmp(curr->name, search_str) == 0) {
                    found++;
                }
                curr = curr->next;
            }
            break;
        }
        case 3: {
            printf("输入电话: ");
            scanf("%19s", search_str);
            while (curr) {
                if (strcmp(curr->phone, search_str) == 0) {
                    found++;
                }
                curr = curr->next;
            }
            break;
        }
        default: {
            printf("无效选择!\n");
            continue;
        }
        }

        // 显示结果
        if (found) {
            printf("\n找到%d个匹配用户:\n", found);
            curr = users;
            time_t now = time(NULL);
            while (curr) {
                int match = 0;
                switch (choice) {
                case 1: match = (curr->id == search_id); break;
                case 2: match = (strcmp(curr->name, search_str) == 0); break;
                case 3: match = (strcmp(curr->phone, search_str) == 0); break;
                }

                if (match) {
                    printf("ID: %d\n", curr->id);
                    printf("姓名: %s\n", curr->name);
                    printf("电话: %s\n", curr->phone);
                    printf("会员等级: %s\n",
                        curr->membership == 0 ? "新用户" :
                        curr->membership == 1 ? "白银会员" : "黄金会员");
                    printf("最近消费: %.2f天前\n",
                        difftime(now, curr->last_purchase) / 86400);
                    printf("累计消费: ￥%.2f\n", curr->total_spent);
                    printf("--------------------------------\n");
                }
                curr = curr->next;
            }
        }
        else {
            printf("未找到匹配用户\n");
        }
    } while (1);
}



// 财务统计

// 生成报表
void generate_reports() {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);

    printf("\n报表生成\n");
    printf("1. 日报表\n");
    printf("2. 周报表\n");
    printf("3. 月报表\n");
    printf("请选择: ");

    int choice;
    scanf("%d", &choice);

    // 计算时间范围
    time_t start, end;
    if (choice == 1) { // 日报
        start = now - 86400;
        end = now;
    }
    else if (choice == 2) { // 周报
        start = now - 604800;
        end = now;
    }
    else if (choice == 3) { // 月报
        start = now - 2592000;
        end = now;
    }

    // 统计包裹数据
    int counts[5] = { 0 };
    Package* pkg = packages;
    while (pkg) {
        if (pkg->arrival >= start && pkg->arrival <= end) {
            counts[pkg->size]++;
        }
        pkg = pkg->next;
    }

    // 显示统计结果
    printf("\n时间段统计结果:\n");
    const char* sizes[] = { "极大", "大", "中", "小", "极小" };
    for (int i = 0; i < 5; i++) {
        printf("%s包裹数量: %d\n", sizes[i], counts[i]);
    }
}

// 主菜单实现
int main() {
    system("chcp 65001"); // 设置控制台编码为UTF-8,避免不使用visual studio时中文乱码
    srand(time(NULL)); // 初始化随机数
    create_data_dir(); // 创建数据目录
    load_all_data();   // 加载已有数据

    int choice;
    do {
        printf("\n菜鸟驿站管理系统\n");
        printf("1. 用户管理\n");
        printf("2. 包裹管理\n");
        printf("3. 库存管理\n");
        printf("4. 财务统计\n");
        printf("5. 生成报表\n");
        printf("0. 退出系统\n");
        printf("请选择操作: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1: user_management(); break;
        case 2: package_management(); break;
        case 3: inventory_check(); break;
        case 4: financial_management(); break;
        case 5: generate_reports(); break;
        case 0:
            save_all_data();
            // 释放内存
            while (users) {
                User* temp = users;
                users = users->next;
                free(temp);
            }
            while (packages) {
                Package* temp = packages;
                packages = packages->next;
                free(temp);
            }
            while (finances) {
                Finance* temp = finances;
                finances = finances->next;
                free(temp);
            }
            printf("数据已保存，系统安全退出！\n");
            exit(0);
        default: printf("无效选择!\n");
        }
    } while (1);

    return 0;
}
//git测试