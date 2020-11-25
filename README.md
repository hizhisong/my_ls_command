## 中文文档 (Chinese Doc)
### 功能实现
实现了ls的 -a -l -r -t -R功能
存在的问题：
对于-l功能，最开始输出的total: block-size num如何计算暂时不清楚

### 函数说明

1. `void diaplay_dir(char* path, int paramFlag)`

   传入目录名和展示规格要求(paramFlag)，对目录下文件使用其绝对路径逐个调用display函数根据paramFlag打印其相关信息

2. `void display(char* path, int paramFlag)`

   对单个文件根据其paramFlag参数打印信息

3. `void display_single(char* name)`

   仅仅打印单个文件名

4. `void display_attribute(char* name, struct stat buf)`

   打印该文件`-l`属性



### 前置知识

系统调用

* **获取文件属性(ls -l)**

  文件的属性信息被保存在 `struct stat` 结构体中，依次读取即可

  ```c
  struct stat {
      dev_t     st_dev;         /* ID of device containing file */
      ino_t     st_ino;         /* Inode number */
      // 文件类型
      mode_t    st_mode;        /* File type and mode */
      nlink_t   st_nlink;       /* Number of hard links */
      uid_t     st_uid;         /* User ID of owner */
      gid_t     st_gid;         /* Group ID of owner */
      dev_t     st_rdev;        /* Device ID (if special file) */
      off_t     st_size;        /* Total size, in bytes */
      blksize_t st_blksize;     /* Block size for filesystem I/O */
      blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */
  
      /* Since Linux 2.6, the kernel supports nanosecond
                    precision for the following timestamp fields.
                    For the details before Linux 2.6, see NOTES. */
      
  	// 时间戳
      struct timespec st_atim;  /* Time of last access */
      struct timespec st_mtim;  /* Time of last modification */
      struct timespec st_ctim;  /* Time of last status change */
  
      #define st_atime st_atim.tv_sec      /* Backward compatibility */
      #define st_mtime st_mtim.tv_sec
      #define st_ctime st_ctim.tv_sec
  };
  
  ```

  ```c
  struct stat buf;
  char path[PATH_MAX+1];
  if(stat(path, &buf) == -1){
      perror("stat");
  }
  
  if(lstat(path, &buf) == -1){
      perror("stat");
  }
  
  // lstat对链接文件返回链接文件本身状态信息，stat返回链接文件所指文件信息
  ```

  **Tip.**

  结构体中`st_mode`成员用来保存文件类型和存取权限，与chmod、open函数的mode参数含义相同，基于此成员特点，可以使用POSIX定义的一系列宏函数传入`st_mode`得到文件类型

  ```c
  if(S_ISREG(buf.st_mode))
          ptr = "regular";
  else if(S_ISDIR(buf.st_mode))
      ptr = "diretory";
  else if(S_ISBLK(buf.st_mode))
      ptr = "block special";
  else if(S_ISCHR(buf.st_mode))
      ptr = "character special";
  else if(S_ISFIFO(buf.st_mode))
      ptr = "fifo";
  else if(S_ISLNK(buf.st_mode))
      ptr = "symbolic link";
  else if(S_ISSOCK(buf.st_mode))
      ptr = "socket";
  else
      ptr = "** unknown mode **";
  printf("%s\n", ptr);
  
  ```

* **获取进程当前工作目录**

  `char* getcwd(char* buf, size_t size)`

  buf指向内存空间为存放当前工作目录字符串的空间，size为buf指向空间的大小

  如果buf为NULL，则getcwd会根据size大小malloc空间返回地址

  如果size**也**为0，getcwd会根据目录字符串大小自动分配大小，返回malloc后空间地址

  * 执行成功则将结果复制到buf指向空间，或返回自动配置的字符串指针，失败返回NULL，错误代码存errno

  ```c
  #include <stdio.h>
  #include <unistd.h>
  #include <stdlib.h>
  
  int main()
  {
      // 情况1
      // char buf[5];
      // printf("%s\n", getcwd(buf, 5));
  
      // 情况2 -- 根据size自动malloc
      // char* buf = NULL;
      // printf("%s\n", getcwd(buf, 100000));
      // free(buf);
  
      // 情况3 -- 根据字符串自动分配
      char* buf = NULL;
      printf("%s\n", getcwd(buf, 0));
      free(buf);
  }
  ```

* **获取目录信息**

  * `opendir`

    `DIR* opendir(const char* path)`

    打开一个目录path(绝对/相对路径)，返回一个类似于文件描述符的 `目录流` `DIR*`

  * `readdir`

    `struct dirent* readdir(DIR* dir) `

    返回目录**中**<u>文件项</u>的结构体信息，多次读取同一目录流，每次顺延返回目录中下一文件项信息于`struct dirent`

    `struct dirent` -- man readdir

    ```c
    struct dirent {
        ino_t          d_ino;       /* Inode number */
        off_t          d_off;       /* Not an offset; see below */
        unsigned short d_reclen;    /* Length of this record */
        unsigned char  d_type;      /* Type of file; not supported
                                                  by all filesystem types */
        char           d_name[256]; /* Null-terminated filename */
    };
    ```

    

  * `closedir`

    `int closedir(DIR* dir)`

    成功返回0，失败返回-1，错误代码存errno中

  

