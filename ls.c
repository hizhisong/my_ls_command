/**
 * `ls` command
 * 参数说明
 * 1. ls -l
 *  total:总用量（https://stackoverflow.com/questions/7401704/what-is-that-total-in-the-very-first-line-after-ls-l）<单位kb(yte)> Extend with BLOCK
 *  表格：（不包含隐藏文件）
 *  param1 --> 文件类型、权限
 *  param2 --> 当文件是普通文件为硬链接数
 *         --> 当文件是目录时为子目录数 <权限前文件类型标识为d的>
 *  param3 --> 文件所属者
 *  param4 --> 文件所属组
 *  param5 --> 文件字节<byte>
 *  param6 --> 创建mm dd time
 *  param7 --> 文件名
 * 
 * 2. ls -a
 *  显示全部文件(包括隐藏文件)
 * 
 * 3. ls -R     -->注意实现对根目录的查找
 *  递归显示
 *  例如：
 *  .:
 *   2.c  3.c  4.c  dwsdw  uheuh.dg
 *
 *  ./dwsdw:
 *  12dcds.cd  uhdh.cdcd
 * 
 * 4. ls -t
 *  根据时间排序
 *
 * 5. ls -r
 *  翻转顺序 --> ls -r 依据字母序翻转
 *              ls -rt 依据修改时间翻转(ls -t最新修改排前面， ls-rt最新修改在后面)
 * 
 * 功能：
 * 对所有目录能使用my_ls
 * 能屏蔽Ctrl+C杀死程序
 * 实现-t   <回忆整理几种排序算法>
 * 实现-r
 * 
 * 输出：
 * 大致上分为两种格式输出：
 * 只输出文件名
 * 输出详细信息
 * 
 * 
 *             -- display_dir  <-- reverse  <for dir file>
 *            /       |
 *   main ---         |                            ---> display_single
 *            \      \|/                          /
 *             -- display       ------------------ 
 *         [display file items by paramFlag]      \
 *                                                 ---> display_attribute
 *                
*/      


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>


#define PARAM_NONE 0    // 00...00000000                                // 完成
#define PARAM_L 1       // 00...00000001                                // 完成
#define PARAM_A 2       // 00...00000010                                // 完成
#define PARAM_UPR 4     // 00...00000100        // Upper R              
#define PARAM_T 8       // 00...00001000                                // 完成
#define PARAM_R 16      // 00...00010000        // low r -- reverse     

// 存目录下文件信息的链表
struct node{
    struct dirent* item;
    char path[PATH_MAX];
    struct node* next;
};

// 建栈，看是否有-R，有则将入栈，再递归调用，无则略过此部分
typedef struct unit{
    int front;
    int tail;
    char* path[PATH_MAX];
}Queue;

void
display(char* path, int paramFlag);

void
display_dir(char* path, int paramFlag);

struct node*
reverse(struct node* head);

struct node* 
sort(struct node* head, char* path, int paramFlag);

void
display_attribute(char* name, struct stat *buf);

void
display_single(const char* name);

void
my_err(const char* msg, int line);


int
main(int argc, char** argv)
{
    char param[32] = {0};     // 将参数中所有选项拼接起来
    int j = 0;      // param游标

    int hyphenCounter = 0;  // 参数中'-'个数
    // 获取参数
    for(int i = 0; i < argc; i++){
        // 参数前带 '-'
        if(argv[i][0] == '-'){
            for(int k = 1; k < strlen(argv[i]); k++, j++){       // k start with 1
                param[j] = argv[i][k];
            }
            hyphenCounter++;
        }
    }
    


    int flag_param = PARAM_NONE;

    // 对参数合法性处理
    for(int i = 0; i < strlen(param); i++){     // 利用或位运算特点：有一位真则真
        switch (param[i])
        {
            case 'l':
                flag_param |= PARAM_L;
                break;
            case 'a': 
                flag_param |= PARAM_A;
                break;
            case 'R':
                flag_param |= PARAM_UPR;
                break;
            case 't':
                flag_param |= PARAM_T;
                break;
            case 'r':
                flag_param |= PARAM_R;
                break;
            default:
                printf("Invalid parameter:%c\n", param[i]);
                exit(1);
        }
    }

    // 参数没有文件名/目录名
    if( hyphenCounter + 1 == argc){
        display_dir("./", flag_param);
    }else{
        // 再次遍历命令行参数，获得文件/目录名
        char path[PATH_MAX+1] = {0};        // linux/limits.h  -->  #define PATH_MAX        4096    /* # chars in a path name including nul */
        struct stat buf;
        
        for(int i = 1; i < argc; i++){
            if(argv[i][0] == '-'){
                continue;
            }else{
                strcpy(path, argv[i]);
                
                // 获取文件/目录文件的struct stat结构体信息
                if(stat(path, &buf) == -1){
                    my_err("stat", __LINE__);
                }

                // 分目录文件、普通文件解析
                if(S_ISDIR(buf.st_mode)){
                    // 检测目录路径字符串最后是否以'/'结尾
                    if(path[strlen(argv[i])-1] != '/'){
                        path[strlen(argv[i])] = '/';
                        path[strlen(argv[i])+1] = '\0';
                    }else{
                        path[strlen(argv[i])] = '\0';
                    }

                    display_dir(path, flag_param);
                }else{
                    display(path, flag_param);
                }
            }
        }
    }

    return 0;
}

void
my_err(const char* msg, int line){
    fprintf(stderr, "line:%d ", line);
    perror(msg);
    exit(1);
}

// 仅仅打印单个文件名   ----------------------------> 待优化
void
display_single(const char* name){
    printf("%s ", name);
}

//打印该文件-l属性
void
display_attribute(char* name, struct stat *buf){
    //============================================================= 打印total待解决http://git.savannah.gnu.org/cgit/coreutils.git/plain/src/ls.c

    char file_type;
    // 判断文件类型
    if(S_ISREG(buf->st_mode))
        file_type = '-';
    else if(S_ISDIR(buf->st_mode))
        file_type = 'd';
    else if(S_ISBLK(buf->st_mode))
        file_type = 'b';
    else if(S_ISCHR(buf->st_mode))
        file_type = 'c';
    else if(S_ISFIFO(buf->st_mode))
        file_type = 'f';
    else if(S_ISLNK(buf->st_mode))
        file_type = 'l';
    else if(S_ISSOCK(buf->st_mode))
        file_type = 's';
        
    printf("%c", file_type);


    // 打印文件所有者权限
    if((*buf).st_mode & S_IRUSR){
        printf("r");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IWUSR){
        printf("w");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IXUSR){
        printf("r");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IWGRP){
        printf("w");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IXGRP){
        printf("x");
    }else{
        printf("-");
    }

    // other
    if((*buf).st_mode & S_IROTH){
        printf("r");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IWOTH){
        printf("w");
    }else{
        printf("-");
    }

    if((*buf).st_mode & S_IXOTH){
        printf("x");
    }else{
        printf("-");
    }

    // 文件链接数/目录下子目录数

    printf("%4ld ", buf->st_nlink);


    // 所有者名字
    printf("%-8s ", getpwuid(buf->st_uid)->pw_name);
    // 组名
    printf("%-8s ", getgrgid(buf->st_gid)->gr_name);
    

    // 文件大小 byte(s)
    printf("%6ld ", buf->st_size);

    // ===================================================================== 上次修改时间 ？与ls -l多了许多其他时间格式信息
    char buf_time[32];

    // printf("%ld\n", (buf->st_mtim).tv_sec);
    strcpy(buf_time, ctime(&((buf->st_mtim).tv_sec)));
    buf_time[strlen(buf_time) - 1] = '\0';  // replace '\n' with '\0'
    printf("%s ", buf_time);
    
    printf("%-s\n", name);
}

// sort -- 链表的归并排序  O(nlogn)
struct node* 
sort(struct node* head, char* path, int paramFlag){
    if(head == NULL || head->next == NULL){
        return head;
    }

    // cut
    struct node* fast = head->next, *slow = head;
    while(fast != NULL && fast->next != NULL){
        slow = slow->next;
        fast = fast->next->next;
    }
    struct node* tmp = slow->next;
    slow->next = NULL;
    struct node* left = sort(head, path, paramFlag);
    struct node* right = sort(tmp, path, paramFlag);
    
    // sort
    struct node* h = (struct node*)malloc(sizeof(struct node));
    struct node* res = h;



    while(left != NULL && right != NULL){
        if(paramFlag & PARAM_T){
            struct stat buf1;
            struct stat buf2;
            if(lstat(left->path, &buf1) == -1 && lstat(right->path, &buf2) == -1){
                my_err("stat", __LINE__);
            }

            if(buf1.st_mtim.tv_sec > buf2.st_mtim.tv_sec){
                h->next = left;
                left = left->next;
            }else{
                h->next = right;
                right = right->next;
            }
            h = h->next;
        }else{
            if(strcmp(left->item->d_name, right->item->d_name) < 0){
                h->next = left;
                left = left->next;
            }else{
                h->next = right;
                right = right->next;
            }
            h = h->next;
        }
    }
    h->next = left != NULL ? left : right;
    return res->next;
    
}

// reverse
struct node*
reverse(struct node* head){
    struct node* ptr1 = NULL, *ptr2 = head;
    while(head){
        head = head->next;
        ptr2->next = ptr1;
        ptr1 = ptr2;
        ptr2 = head;
    }

    return ptr1;
}

// 传入目录名和展示规格要求(paramFlag)，对目录下文件使用其路径逐个调用display函数根据paramFlag打印其相关信息
// 只处理-R（递归）、-r（翻转）、-t（按时间序）
void
display_dir(char* path, int paramFlag){
    DIR* stream = opendir(path);

    struct dirent* tmp;
    struct node* newNode;
    struct node* head = (struct node*)malloc(sizeof(struct node));
    head->next = NULL;

    // 创建队列 && 初始化
    Queue queue;
    queue.front = queue.tail = -1;
    

    while(tmp = readdir(stream)){
        newNode = (struct node*)malloc(sizeof(struct node));
        newNode->item = tmp;
        strcat(newNode->path, path);
        strcat(newNode->path, newNode->item->d_name);

        newNode->next = head->next;
        head->next = newNode;
    }

    // sort  -> 1.by name<无-t> 2.by time
    head->next = sort(head->next, path, paramFlag);

    // 是否有-r reverse
    if(paramFlag & PARAM_R){
        head->next = reverse(head->next);
    }

    struct node* t = head->next;
    while(t){
        // 看是否有-R，有则将入栈，再递归调用，无则略过此部分
        // 入队列时排除 '.'、'..' 目录
        if((paramFlag & PARAM_UPR) && strcmp(t->item->d_name, ".") != 0 && strcmp(t->item->d_name, "..") != 0){
           struct stat buf;
           if(lstat(t->path, &buf)){
               my_err("lstat", __LINE__);
           }
           if(S_ISDIR(buf.st_mode)){
                // 入队列
                queue.tail++;
                queue.path[queue.tail] = (char*)malloc(sizeof(char) * PATH_MAX);
                strcpy(queue.path[queue.tail], t->path);
                strcat(queue.path[queue.tail], "/");
                // printf("%s\n", queue.path[queue.tail]);
           } 
        }

        // printf("%s\n", t->path);
        display(t->path, paramFlag);
        t = t->next;
    }


    while(queue.front != queue.tail){
        queue.front++;
        printf("\n%s:\n", queue.path[queue.front]);
        // 处理-R的递归打印
        display_dir(queue.path[queue.front], paramFlag);
    }
}

// 对单个文件根据其paramFlag参数打印信息
void
display(char* path, int paramFlag){
    // get file name
    char filename[NAME_MAX] = {0};             // #define NAME_MAX         255	/* # chars in a file name */
    int j;
    for(int i = 0; i < strlen(path); i++){
        if(path[i] == '/'){
            j = 0;
        }else{
            filename[j] = path[i];
            j++;
        }
    }

    filename[j] = '\0';

    struct stat buf;
    if(lstat(path, &buf) == -1){
        my_err("stat", __LINE__);
    }

    if(paramFlag == PARAM_NONE){
        if(filename[0] != '.'){     // mention: without -a
            display_single(filename);
            printf("\n");
        }    

    }else if((paramFlag & PARAM_A) && (paramFlag & PARAM_L)){
        display_attribute(filename, &buf);
    
    }else if(paramFlag & PARAM_A){      // -a
        display_single(filename);
        printf("\n");
    
    }else if(paramFlag & PARAM_L){
        if(filename[0] != '.'){
            display_attribute(filename, &buf);
        }
    }
}
