//server.c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <dirent.h>
struct net_info
{
	int type;	//1 注册 2 登录  3 list  4 get  5 put  6  history 7 exit
	char userid[20];	//注册 和 登录时用
	char passwd[20];  //注册 和 登录时用
	char file_name[100];//文件名，列表，上传，下载
	char info[500];	//内容 ，上传下载时用 ，500 表示每次最多传 500字节(在此处可以做手脚，vip 1000)
	int len;			//包数据长度(发送1次数据长度，文件有多包数据)
	int offset;			//断点位置 (比如 发送到2000字节断了，下次从2000开始)
	int total_len;		//文件总长度
};
//因为注册，需要打开数据库，  登录 ，添加历史，查询历史，最简单的方法 一个数据库全局变量
sqlite3 *db;		//全局变量，在软件启动(main中) 打开
void my_register(int fd, struct net_info *p)		//注册
{	//执行 insert into 语句   然后 要给用户创建一个文件夹
	char *err;
	char sql[100] = { 0 };
	char **resultp;
	int nrow, ncolumn;
	//注册之前要检查 用户是否已经存在
	sprintf(sql, "select * from user_info where userid='%s'", p->userid);
	sqlite3_get_table(db, sql,  &resultp,  &nrow, &ncolumn, &err);
	if(nrow > 0)
	{
		strcpy(p->info, "用户名已存在");
		send(fd, p, sizeof(struct net_info), 0);
		return;
	}
	sprintf(sql, "insert into user_info values('%s', '%s')", p->userid, p->passwd);
	if(sqlite3_exec(db, sql, NULL, NULL, &err) != 0)
	{
		printf("%s\n", err);
	}
	int pid = fork();
	if(pid == 0)
		execlp("mkdir", "mkdir", p->userid, NULL);
	strcpy(p->info, "success");			//success 成功
	send(fd, p, sizeof(struct net_info), 0);
}
void login(int fd, struct net_info *p) //登录
{	//检查 用户名 和 密码是否 与数据库里面的一致
	char *err;
	char sql[100] = { 0 };
	char **resultp;
	int nrow, ncolumn;
	//注册之前要检查 用户是否已经存在
	sprintf(sql, "select * from user_info where userid='%s' and passwd = '%s'", p->userid, p->passwd);
	sqlite3_get_table(db, sql,  &resultp,  &nrow, &ncolumn, &err);
	if(nrow > 0)	//查到一条 数据 与 传入 的 用户名 密码相同
	{
		strcpy(p->info, "success");
	}	
	else
	{
		strcpy(p->info, "用户名或密码错");	
	}
	send(fd, p, sizeof(struct net_info), 0);	
}
void get_list(int fd, struct net_info *p)
{
	DIR *dp = opendir(p->userid);
	if(dp != NULL)
	{
		struct dirent *ep;   //用来保存一个文件
		ep = readdir(dp);    //readdir,执行一次从目录中读出一个文件
		while(ep != NULL)
		{	//读出一个，发一个
			if(ep->d_name[0] != '.')	//以.开头是隐藏文件，不显示
			{
				strcpy(p->file_name, ep->d_name);
				send(fd, p, sizeof(struct net_info), 0);
			}
			ep = readdir(dp);
		}
		closedir(dp);
	}
}
void get_file(int fd, struct net_info *p)
{	//xiaoli/hello.c
	char file_name[200] = { 0 };
	sprintf(file_name, "%s/%s", p->userid, p->file_name);
	FILE *fp = fopen(file_name, "r");
	if(fp != NULL)
	{
		int len = 0;
		while((len = fread(p->info, 1, sizeof(p->info), fp)) > 0)
		{
			p->len = len;
			send(fd, p, sizeof(struct net_info), 0);
			usleep(1000);		//延时1ms
		}
		fclose(fp);
	}
}
void save_file(int fd, struct net_info *p)
{
	char file_name[200] = { 0 };
	sprintf(file_name, "%s/%s", p->userid, p->file_name);
	FILE *fp = fopen(file_name, "a+");
	if(fp != NULL)
	{
		fwrite(p->info, 1, p->len, fp);
		fclose(fp);
	}
}
void *recv_fun(void *p)
{
	struct net_info s;
	int newfd = *(int *)p;
	char buf[100] = { 0 };
	while(1)
	{
		if(recv(newfd, &s, sizeof(s), 0) <= 0)	//接收
			break;			
		printf("type is %d\n", s.type);			
		if(s.type == 1)			
		{	//注册，要把用户名和密码添加到数据库中
			my_register(newfd, &s);
		}
		else if(s.type == 2)
		{
			login(newfd, &s);
		}
		else if(s.type == 3)
		{	//要文件列表
			get_list(newfd, &s);
		}
		else if(s.type == 4)
		{
			get_file(newfd, &s);
		}
		else if(s.type == 5)
		{
			save_file(newfd, &s);
		}
	}
}
int main()
{
	pthread_t id;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(9999);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);	//INADDR_ANY 得到当前计算机的ip地址
	int ret = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
	listen(fd, 5);
	if(sqlite3_open("netdisk.db", &db) != 0)  //数据打开失败
	{
		printf("netdisk.db open failed\n");
		return 0;		//结束程序
	}
	while(1)
	{
		int newfd = accept(fd, NULL, NULL);	//等待客户端连接
		printf("client connect %d\n", newfd);
		pthread_create(&id, NULL, recv_fun, &newfd);
	}
	close(fd);
}
