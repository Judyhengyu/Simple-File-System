#include <stdio.h>
#include <string.h>
#include <process.h>
#include <malloc.h>
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#define getb(type) (type *)malloc(sizeof(type))
using namespace std;
namespace fs = std::filesystem;
int usernowpride;     // 全局变量，当前用户类型
char usernowname[10]; // 全局变量，当前用户名
int nowlevel;         // 全局变量，当前目录层次
char usernowpath[200] = "\\\0";
// 用户类型定义
struct user
{
    char name[10]; // 用户名
    int pride;     // 用户权限，1为管理员，0为普通用户
    char pass[10]; // 用户密码
};
// 定义空白区项
struct freeblock
{
    int number;
    struct freeblock *next;
};
struct freeblock *fblock = NULL; // 全局变量，系统空闲区链
// 定义文件打开项
struct fileb
{
    int parent;    // 所在父节点
    char name[15]; // 文件名
    int pride;     // 读写权限，0只读，1读写
    int rex;       // 读写状态，0为没有，1读2写
    struct fileb *next;
};
struct fileb *flink = NULL; // 全局变量，系统打开文件链
// 定义文件索引项
struct fileindex
{
    char name[15];
    int number;
    int parent;
    char kind;
    struct fileindex *next;
};
struct fileindex *findex = NULL; // 全局变量，文件索引链
// 定义目录表项
struct directory
{
    char name[25]; // 目录或者文件名称
    int parent;    // 上层目录
    int pride;     // 文件操作权限，0只读，1读写
    int empty;     // 是否是空闲块，0为空闲块，1为非空
    char kind;     // 类型，文件为f，目录为d
};

// 用户创建
bool user_create()
{
    struct user newuser;
    char name[10];
    char pass[10];
    int pride;
    if (usernowpride != 1)
    {
        cout << "当前用户没有新建用户的权限\n";
        return false;
    }
    FILE *fp;
    if ((fp = fopen("user", "ab+")) == NULL)
    {
        cout << "用户表打开失败\n";
        return false;
    }
    else
    {
        cout << "请输入用户名:";
        cin >> name;
        if (strcmp(name, "root") == 0)
        {
            printf("Error:此为超级管理员\n");
            return false;
        }
        rewind(fp);
        while (!feof(fp))
        {
            fread(&newuser, sizeof(struct user), 1, fp);
            if (strcmp(newuser.name, name) == 0)
            {
                cout << "该用户名已经存在\n";
                fclose(fp);
                return false;
            }
        }
        cout << "请输入用户密码:";
        cin >> pass;
        cout << "请输入用户权限(0普通用户，1管理员):";
        cin >> pride;
        strcpy(newuser.name, name);
        strcpy(newuser.pass, pass);
        newuser.pride = pride;
        //     FILE fpuser;//为新建用户建立用户目录文件
        if (!fopen(newuser.name, "ab+"))
        {
            cout << "用户创建失败\n";
            // 如创建失败则把已经建立的用户目录删除
            char cmd[20] = "DEL ";
            strcat(cmd, newuser.name);
            system(cmd);
            fclose(fp);
            return false;
        }
        if (!fwrite(&newuser, sizeof(struct user), 1, fp))
        {
            cout << "创建失败\n";
            fclose(fp);
            return false;
        }
        else
        {
            cout << "用户创建成功\n";
            cout << "请输入要执行的操作：";
            fclose(fp);
            return true;
        }
    }
}

// 用户登陆
bool user_login()
{
    char name[10];
    char pass[10];
    cout << "\n\t\t\t    ○用户名:";
    cin >> name;
    cout << "\t\t\t    ○密  码:";
    cin >> pass;
    if ((strcmp("root", name) == 0) && (strcmp("123456", pass) == 0)) // 管理员
    {
        usernowpride = 1;
        strcpy(usernowname, "root");
        return true;
    }
    FILE *fp = NULL;
    struct user actuser;
    if (!(fp = fopen("user", "ab+")))
    {
        cout << "Error:用户表错误\n";
        return false;
    }
    rewind(fp);
    while (!feof(fp))
    {
        fread(&actuser, sizeof(struct user), 1, fp);
        if ((strcmp(actuser.name, name) == 0) && (strcmp(actuser.pass, pass) == 0))
        {
            usernowpride = actuser.pride;      // 记录当前用户的权限
            strcpy(usernowname, actuser.name); // 记录当前用户的主路径
            nowlevel = -1;                     // 记录当前目录层次
            fclose(fp);
            // 设置路径
            if (strcmp(usernowpath, "\\") != 0) // 不是根目录就添加斜杠
            {
                strcat(usernowpath, "\\");
            }
            strcat(usernowpath, usernowname);
            return true;
        }
    }
    cout << "Error:用户名或密码无效，请核对后再输入\n";
    fclose(fp);
    return false;
}

// 初始化空闲区链表以及文件索引链
void list_init()
{
    // 清空各链表
    findex = NULL;
    fblock = NULL;
    int i = 0;
    struct directory dnow;
    FILE *fp;
    if (!(fp = fopen(usernowname, "rb")))
    {
        cout << "Error:打开用户目录失败\n";
        return;
    }
    else
    {
        int enp;
        int sp;
        fseek(fp, 0, 2);
        enp = ftell(fp);
        fseek(fp, 0, 0);
        sp = ftell(fp);
        if (sp == enp)
            return;
        while (!feof(fp))
        {
            fread(&dnow, sizeof(struct directory), 1, fp);
            if (dnow.empty == 0)
            {
                // 把空闲区连到空闲链表中
                struct freeblock *fb = getb(struct freeblock);
                fb->number = i;
                fb->next = NULL;
                struct freeblock *p = fblock;
                if (p == NULL)
                {
                    fblock = getb(struct freeblock);
                    fblock->next = fb;
                }
                else
                {
                    while (p->next != NULL)
                    {
                        p = p->next;
                    }
                    p->next = fb;
                }
            }
            else
            {
                // 建立索引表
                struct fileindex *fi = (struct fileindex *)malloc(sizeof(struct fileindex));
                strcpy(fi->name, dnow.name);
                fi->number = i;
                fi->kind = dnow.kind;
                fi->parent = dnow.parent;
                fi->next = NULL;
                struct fileindex *pi = findex;
                if (pi == NULL)
                {
                    findex = getb(struct fileindex);
                    findex->next = fi;
                }
                else
                {
                    while (pi->next != NULL)
                    {
                        pi = pi->next;
                    }
                    pi->next = fi;
                }
            }
            i++;
        }
        fclose(fp);
        return;
    }
}

// 目录/文件创建
// 输入参数为要建立的类型，f为文件，d为文件夹
void drec_file_create(char ch)
{
    struct directory dnow;
    char name[15];
    char pride;         // 权限，0只读，1读写
    int i;              // 记录空闲区区号
    bool cancrd = true; // 用于判断是否有重名文件
    cout << "请输入名字:";
    fflush(stdin);
    cin >> name;
    // 判断是否已经存在相同名字的文件或者目录
    struct fileindex *fi = findex;
    while (fi)
    {
        if ((strcmp(fi->name, name) == 0) && (fi->parent == nowlevel) && (fi->kind == ch))
        {
            cout << "Error:此文件或者目录已经存在\n";
            cancrd = false;
            break;
        }
        fi = fi->next;
    }
    if (!cancrd)
        return;
    cout << "请输入权限(0只读,1读写):";
    cin >> pride;
    dnow.empty = 0;
    strcpy(dnow.name, name);
    dnow.parent = nowlevel;
    if (pride == '0')
    {
        dnow.pride = 0;
    }
    else
    {
        dnow.pride = 1;
    }
    dnow.kind = ch;
    dnow.empty = 1;
    FILE *fp;
    if (!(fp = fopen(usernowname, "rb+")))
    {
        cout << "错误";
        return;
    }
    else
    {
        std::string path = "C:\\" + string(usernowpath) + "\\" + string(name);
        if (dnow.kind == 'd')
            fs::create_directories(path);
        else
            std::ofstream file(path);
        if (fblock == NULL || fblock->next == NULL) // 空闲区没有就直接在文件添加
        {
            fseek(fp, 0, 2);
            fwrite(&dnow, sizeof(struct directory), 1, fp);
            cout << "创建成功\n";
            fclose(fp);
            return;
        }
        else
        {
            struct freeblock *p = fblock->next; // 拿第一个空闲区来存放文件
            if (p != NULL)
            {
                // fblock=p->next;
                i = p->number;
                fblock->next = p->next;
            }
            free(p);
            fseek(fp, i * sizeof(struct directory), 0);
            fwrite(&dnow, sizeof(struct directory), 1, fp);
            cout << "创建成功\n";
            fclose(fp);
            return;
        }
    }
}

// 列表当前路径下的文件
void file_list()
{
    long ep;
    FILE *fp;
    struct directory drenow;
    fp = fopen(usernowname, "rb");
    fseek(fp, 0, 2);
    ep = ftell(fp);
    fseek(fp, 0, 0);
    cout << "文件名称\t操作权限\t文件类型\t上层目录\n";
    while (ep != ftell(fp))
    {
        fread(&drenow, sizeof(struct directory), 1, fp);
        if (drenow.parent == nowlevel && drenow.empty != 0)
        {
            cout << drenow.name << "\t\t";
            cout << drenow.pride << "\t\t";
            cout << drenow.kind << "\t\t";
            cout << drenow.parent << "\n";
        }
    }
    cout << "所有文件结束\n";
    fclose(fp);
}

// 退出目录
bool direc_back()
{
    if (nowlevel == -1)
    {
        cout << "Error:当前已为用户根目录,无法再回退\n";
        return false;
    }
    char name[15];
    int i;
    i = nowlevel;
    struct directory drecnow;
    FILE *fp;
    fp = fopen(usernowname, "rb");
    fseek(fp, i * sizeof(struct directory), 0);
    fread(&drecnow, sizeof(struct directory), 1, fp);
    fclose(fp);
    strcpy(name, drecnow.name);
    nowlevel = drecnow.parent;
    char cc;       // 检测'\0'位置
    int ccidx = 0; // 记录最好一个'\'位置
    int j = 0;
    cc = usernowpath[j];
    while (cc != '\0')
    {
        j++;
        cc = usernowpath[j];
        if (cc == '\\')
        {
            ccidx = j;
        }
    }
    if (ccidx != 0)
    {
        usernowpath[ccidx] = '\0';
    }
    return true;
}

// 进入目录
bool direc_enter(char chpath[15])
{
    struct directory drenow;
    char mypath[15];
    int i = 0;
    strcpy(mypath, chpath);
    if (strcmp(mypath, "..") == 0)
    {
        if (direc_back())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    FILE *fp;
    fp = fopen(usernowname, "rb");
    while (!feof(fp))
    {
        fread(&drenow, sizeof(struct directory), 1, fp);
        if ((strcmp(drenow.name, mypath) == 0) && (drenow.kind == 'd') && (drenow.parent == nowlevel))
        {
            nowlevel = i;
            fclose(fp);
            if (strcmp(usernowpath, "\\") != 0) // 不是根目录就添加斜杠
            {
                strcat(usernowpath, "\\");
            }
            strcat(usernowpath, mypath);
            return true;
        }
        i++;
    }
    fclose(fp);
    return false;
}

// 进入路径完整版
void intopath()
{
    int tempnl = nowlevel;
    char tempunp[200];
    strcpy(tempunp, usernowpath);
    char wholepath[100];
    char name[15];
    cout << "输入要进入的文件夹:";
    cin >> wholepath;
    int i = 0;
    char cc = wholepath[i];
    while (cc != '\0')
    {
        int j = 0;
        name[j] = cc;
        while (cc != '\0' && cc != '\\')
        {
            name[j] = cc;
            j++;
            i++;
            cc = wholepath[i];
        }
        if (i != 0)
        {
            name[j] = '\0';
            if (!direc_enter(name))
            {
                cout << "Error:路径输入错误，请核对\n";
                // 如果路径错误，则还原
                nowlevel = tempnl;
                strcpy(usernowpath, tempunp);
                return;
            }
        }
        if (cc == '\0')
            break;
        i++;
        cc = wholepath[i];
    }
}

// 删除文件,递归删除文件夹和文件夹里面的内容
// 删除文件
bool drec_file_remove(int p)
{
    bool isfind = false;
    int i = 0;
    int temp = 0;
    struct directory drecnow;
    FILE *fp;
    fp = fopen(usernowname, "rb+");
    long ep;
    fseek(fp, 0, 2);
    ep = ftell(fp);
    fseek(fp, 0, 0);
    while (ep != ftell(fp))
    {
        fread(&drecnow, sizeof(struct directory), 1, fp);
        if (drecnow.parent == p)
        {
            isfind = true;
            temp = p;
            drec_file_remove(i);
        }
        i++;
    }
    if (!isfind)
    {
        drecnow.empty = 0;
        strcpy(drecnow.name, "    ");
        drecnow.parent = -1;
        fseek(fp, p * sizeof(struct directory), 0);
        fwrite(&drecnow, sizeof(struct directory), 1, fp);
        fclose(fp);
        return true;
    }
    else
    {
        drec_file_remove(temp);
        fclose(fp);
        return false;
    }
}

void drec_file_del()
{
    char name[15];
    cout << "输入要删除的目录/文件:";
    cin >> name;
    std::string path = "C:\\" + string(usernowpath) + "\\" + string(name);
    fs::remove_all(path);
    fileindex *fb = findex; // 用于索引表
    fileindex *fi;
    fileb *fob = flink; // 用于已打开文件链表
    if (fb == NULL || fb->next == NULL)
    { // 搜索索引表
        cout << "Error:没有此目录/文件\n";
        return;
    }
    bool isit = true;
    while (fb != NULL && fb->next != NULL)
    {
        fi = fb;
        fb = fb->next;
        if ((strcmp(fb->name, name) == 0) && (fb->parent == nowlevel))
        {
            isit = false;
            if (drec_file_remove(fb->number))
            {
                fi->next = fb->next;
                free(fb);
                cout << "该目录/文件已被删除\n";
                return;
            }
        }
    }
    if (isit)
    {
        cout << "Error:没有此目录/文件\n";
    }
    else
    {
        cout << "该目录/文件已被删除\n";
    }
}

// 打开文件
void file_open()
{
    long ep;
    int i = 0;
    FILE *fp;
    struct directory drenow;
    char name[15];
    cout << "输入要打开的文件名:";
    cin >> name;
    struct fileb *fb = flink;
    while (fb != NULL && fb->next != NULL)
    {
        fb = fb->next;
        if ((strcmp(fb->name, name) == 0) && (fb->parent == nowlevel))
        {
            cout << "Error:该文件已经打开\n";
            return;
        }
    }
    fp = fopen(usernowname, "rb"); // 可用索引节点
    fseek(fp, 0, 2);
    ep = ftell(fp);
    fseek(fp, 0, 0);
    while (ep != ftell(fp))
    {
        fread(&drenow, sizeof(struct directory), 1, fp);
        if ((strcmp(drenow.name, name) == 0) && (drenow.parent == nowlevel) && (drenow.kind == 'f'))
        {
            // 添加到打开链表
            fileb *pb = getb(struct fileb);
            strcpy(pb->name, name);
            pb->parent = nowlevel;
            pb->next = NULL;
            pb->rex = 0;
            pb->pride = drenow.pride;
            if (flink == NULL)
            {
                flink = getb(struct fileb);
                flink->next = pb;
            }
            else
            {
                pb->next = flink->next;
                flink->next = pb;
            }
            cout << "文件已经打开\n";
            fclose(fp);
            // 这里是否要更新显示已打开链表
            return;
        }
        i++;
    }
    cout << "Error:当前目录下无此文件，请核对\n";
    fclose(fp);
}

// 显示当前打开所有文件
void openfile_queue()
{
    if (flink == NULL || flink->next == NULL)
    {
        cout << "当前打开文件队列空\n";
        return;
    }
    else
    {
        cout << "当前打开文件队列如下:(文件名|父节点|读写权限|读写状态)\n";
        fileb *fb = flink;
        while (fb != NULL && fb->next != NULL)
        {
            fb = fb->next;
            printf("\t\t\t%s  |  %d  |    %d   |    %d", fb->name, fb->parent, fb->pride, fb->rex);
        }
        cout << "\n";
    }
}

// 关闭文件
void file_close()
{
    char name[15];
    cout << "输入要关闭的文件:";
    cin >> name;
    if (flink == NULL || flink->next == NULL)
    {
        cout << "Error:该文件没有打开\n";
        return;
    }
    fileb *fb = flink;
    fileb *ffb = NULL;
    while (fb != NULL && fb->next != NULL)
    {
        ffb = fb;
        fb = fb->next;
        if ((strcmp(fb->name, name) == 0) && (fb->parent == nowlevel))
        {
            ffb->next = fb->next;
            free(fb);
            cout << "文件已经关闭\n";
            return;
        }
    }
    cout << "Error:当前路径下找不到你要关闭的文件，请确定文件名已经路径是否正确\n";
}

// 读写文件，该文件必须先打开后才可以进行读写操作
// 读写文件，输入1为读文件，输入2为写文件
void file_readwrite(int ch)
{
    char name[15];
    cout << "要操作的文件名:";
    cin >> name;
    if (flink == NULL || flink->next == NULL)
    {
        cout << "Error:该文件尚未打开\n"; // 可以调用文件打开函数，以后先修改
        return;
    }
    fileb *fb = flink;
    while (fb != NULL && fb->next != NULL)
    {
        fb = fb->next;
        if ((strcmp(fb->name, name) == 0) && (fb->parent == nowlevel))
        {
            // char ch;
            if (fb->rex != ch && fb->rex != 0)
            {
                if (ch == 2)
                {
                    if (fb->pride != 1)
                    {
                        cout << "Error:该文件只读，没有写权限\n";
                        return;
                    }
                    else
                    {
                        cout << "当前文件正在读进程，是否终止读进程进行写?(y OR n):";
                    }
                }
                else
                {
                    cout << "当前文件正在写进程，是否终止写进程进行读?(y OR n):";
                }
                fflush(stdin);
                cin >> ch;
                if (ch == 'y' || ch == 'Y')
                {
                    if (ch == 2)
                    {
                        fb->rex = 1;
                        cout << "文件正在读...\n";
                    }
                    else
                    {
                        fb->rex = 2;
                        cout << "文件正在写...\n";
                    }
                }
            }
            else
            {
                if (ch == 2 && fb->pride != 1)
                {
                    cout << "Error:该文件只读，没有写权限\n";
                    return;
                }
                fb->rex = ch;
                if (ch == 1)
                {
                    cout << "文件正在读...\n";
                }
                else
                {
                    cout << "文件正在写...\n";
                }
            }
            return;
        }
    }
}

// 目录显示
void direct_display()
{
    cout << usernowpath << endl;
}

// 注销
void exit()
{
    strcpy(usernowname, "  ");
    nowlevel = -1;
    fblock = NULL;
    findex = NULL;
    strcpy(usernowpath, "\\");
    flink = NULL;
}

// 修改用户类型
void user_setpride()
{
    char name[15];
    cout << "输入用户名字:";
    cin >> name;
    if ((strcmp("root", name) == 0)) // 超级用户
    {
        cout << "Error:超级管理员权限不可以更改\n";
        return;
    }
    FILE *fp = NULL;
    struct user actuser;
    if (!(fp = fopen("user", "rb+")))
    {
        cout << "Error:用户表错误\n";
        return;
    }
    rewind(fp);
    long np;
    while (!feof(fp))
    {
        np = ftell(fp);
        fread(&actuser, sizeof(struct user), 1, fp);
        if ((strcmp(actuser.name, name) == 0))
        {
            if (actuser.pride == 1)
            {
                int pp = 1;
                cout << "该用户是管理员\n输入0设置普通用户:";
                cin >> pp;
                if (pp == 0)
                {
                    actuser.pride = 0;
                }
            }
            else if (actuser.pride == 0)
            {
                int pp = 0;
                cout << "该用户是普通用户\n输入1设置为管理员:";
                cin >> pp;
                {
                    if (pp == 1)
                    {
                        actuser.pride = 1;
                    }
                }
            }
            fseek(fp, np, 0);
            fwrite(&actuser, sizeof(struct user), 1, fp);
            fclose(fp);
            return;
        }
    }
}

void os()
{
    printf("\t  ***********************************************\n");
    printf("\t  *　　　　　　　　　　　　　                   * \n");
    printf("\t  *     　　       操作系统课设  　     　  　  *\n");
    printf("\t  *         多用户多级目录文件管理系统  　  　  *\n");
    printf("\t  *　　　　　　　　　　　　　                   * \n");
    printf("\t  ***********************************************\n");
    printf("\t  *　　　　　　　　　　　　　                   * \n");
    printf("\t  *　　　学号:2130110393     姓名:陈晓庆　　 　 * \n");
    printf("\t  *　　　学号:2130110417     姓名:张家铭　　 　 * \n");
    printf("\t  *　　　学号:2130110420     姓名:朱恒宇　　 　 * \n");
    printf("\t  *　　　　　　　　　　　　　　　　　　　　　　 *\n");
    printf("\t  ***********************************************\n");
    cout << "\n\t\t\t   请按\"1\"开机";
}

int main()
{
    int choince;
    while (1)
    {
        os();
        bool enter;
        cin >> enter;
        while (enter)
        {
            system("cls");
            printf("\t  ***********************************************\n");
            printf("\t  *　　　　　　　　　　　　　                   * \n");
            printf("\t  *                 用 户 登 录                 *\n");
            printf("\t  *　　　　　　　　　　　　　                   * \n");
            printf("\t  ***********************************************\n");
            if (user_login())
                break;
        }
        if (strcmp(usernowname, "root") == 0)
        {
            system("cls");
            cout << "*************************************************************************\n";
            cout << "*\t            欢迎超级管理员使用此文件管理系统               \t*\n";
            cout << "*************************************************************************\n";
            cout << "*                                                                       *\n";
            cout << "*\t1.新建用户\t\t2.修改用户\t\t3.退出\t\t*\n";
            cout << "*                                                                       *\n";
            cout << "*************************************************************************\n";
            cout << "请输入要执行的操作：";
            bool lout = false;
            while (1)
            {
                fflush(stdin);
                scanf("%d", &choince);
                switch (choince)
                {
                case 1:
                    user_create();
                    break;
                case 2:
                    user_setpride();
                    break;
                case 3:
                    char out;
                    printf("确定是否退出吗?(y OR n):");
                    fflush(stdin);
                    scanf("%c", &out);
                    if (out == 'y' || out == 'Y')
                    {
                        lout = true;
                        exit();
                        system("cls");
                    }
                    break;
                default:
                    printf("Error:错误命令\n");
                    break;
                }
                if (lout)
                    break;
            }
        }
        else
        {
            list_init();
            system("cls");
            cout
                << "***********************************************************************************************************\n";
            cout << "*\t\t\t\t      欢迎 " << usernowname << " 使用此文件管理系统         \t\t\t\t  *\n";
            cout
                << "***********************************************************************************************************\n";
            cout << "*1.所有文件\t2.目录/文件创建\t\t3.目录/文件删除\t\t4.目录显示\t5.进入目录\t6.退出目录*\n";
            cout << "*7.文件打开\t8.文件关闭\t\t9.读文件\t\t10.写文件\t\t\t\t  *\n";
            cout << "*11.用户创建\t12.清屏\t\t\t13.关机\t\t\t\t\t\t\t\t  *\n";
            cout
                << "***********************************************************************************************************\n";
            cout << "请输入要执行的操作：";
            bool lout = false;
            while (1)
            {
                fflush(stdin);
                scanf("%d", &choince);
                switch (choince)
                {
                case 1:
                    file_list();
                    cout << "请输入要执行的操作：";
                    break;
                case 2:
                    int cc;
                    printf("1.新建文件夹 2.新建文件");
                    scanf("%d", &cc);
                    if (cc == 1)
                    {
                        drec_file_create('d');
                    }
                    if (cc == 2)
                    {
                        drec_file_create('f');
                    }
                    list_init();
                    cout << "请输入要执行的操作：";
                    break;
                case 3:
                    drec_file_del();
                    list_init();
                    cout << "请输入要执行的操作：";
                    break;
                case 4:
                    direct_display();
                    cout << "请输入要执行的操作：";
                    break;
                case 5:
                    intopath();
                    cout << "请输入要执行的操作：";
                    break;
                case 6:
                    direc_back();
                    cout << "请输入要执行的操作：";
                    break;
                case 7:
                    file_open();
                    openfile_queue();
                    cout << "请输入要执行的操作：";
                    break;
                case 8:
                    file_close();
                    openfile_queue();
                    cout << "请输入要执行的操作：";
                    break;
                case 9:
                    file_readwrite(1);
                    openfile_queue();
                    cout << "请输入要执行的操作：";
                    break;
                case 10:
                    file_readwrite(2);
                    openfile_queue();
                    cout << "请输入要执行的操作：";
                    break;
                case 11:
                    user_create();
                    cout << "请输入要执行的操作：";
                    break;
                case 12:
                    system("cls");
                    cout << "***********************************************************************************************************\n";
                    cout << "*\t\t\t\t      欢迎 " << usernowname << " 使用此文件管理系统         \t\t\t\t  *\n";
                    cout << "***********************************************************************************************************\n";
                    cout << "*1.所有文件\t2.目录/文件创建\t\t3.目录/文件删除\t\t4.目录显示\t5.进入目录\t6.退出目录*\n";
                    cout << "*7.文件打开\t8.文件关闭\t\t9.读文件\t\t10.写文件\t\t\t\t  *\n";
                    cout << "*11.用户创建\t12.清屏\t\t\t13.关机\t\t\t\t\t\t\t\t  *\n";
                    cout << "***********************************************************************************************************\n";
                    cout << "请输入要执行的操作：";
                    break;
                case 13:
                    char out;
                    printf("确定是否关机吗?(y OR n):");
                    fflush(stdin);
                    scanf("%c", &out);
                    if (out == 'y' || out == 'Y')
                    {
                        lout = true;
                        exit();
                        system("cls");
                    }
                    break;
                default:
                    printf("Error:错误命令\n");
                    cout << "请输入要执行的操作：";
                    break;
                }
                if (lout)
                    break;
            }
        }
    }
    printf("完成\n");
    return 0;
}
