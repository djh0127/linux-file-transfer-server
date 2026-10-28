#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
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
int login_flag = 0;		//0 未登录  1 登录成功
int fd = 0;
char userid[100] = { 0 };	//用于保存 登陆成功的用户名
void *recv_fun(void *p)
{
	struct net_info s;
	while(1)
	{
		if(recv(fd, &s, sizeof(s), 0) <= 0)
			break;
		if(s.type == 1)
		{
			printf("%s\n", s.info);
		}
		else if(s.type == 2)
		{
			printf("%s\n", s.info);
			if(strcmp(s.info, "success") == 0)
			{
				login_flag = 1;
				strcpy(userid, s.userid);	//把登陆的用户id赋值给全局变量
			}			
		}
		else if(s.type == 3)
		{
			printf("%s\n", s.file_name);
		}
		else if(s.type == 4)
		{
			FILE *fp = fopen(s.file_name, "a+");	//a+不存在就新建，存在追加写入
			if(fp != NULL)
			{
				fwrite(s.info, 1, s.len, fp);
				fclose(fp);
			}
		}
	}
}
void send_file(struct net_info *p)
{
	FILE *fp = fopen(p->file_name, "r");
	if(fp != NULL)
	{
		int len;
		while( (len = fread(p->info, 1, sizeof(p->info), fp)) > 0)
		{
			p->len = len;
			send(fd, p, sizeof(struct net_info), 0);
			usleep(1000);
		}
		fclose(fp);
	}
}
int main(int argc, char *argv[])
{
	pthread_t id;
	if(argc < 3)
	{
		printf("参数错误: ./client 127.0.0.1 9999\n");
		return 0;
	}
	fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in youaddr;	
	youaddr.sin_family = AF_INET;
	youaddr.sin_port = htons(atoi(argv[2]));
	youaddr.sin_addr.s_addr = inet_addr(argv[1]);
	connect(fd, (struct sockaddr *)&youaddr, sizeof(youaddr));
	pthread_create(&id, NULL, recv_fun, NULL);
	while(1)
	{	
		struct net_info s;
		printf("1:register 2:login 3:list 4:get file 5:put file 6:history 7:exit\n");
		scanf("%d", &(s.type));		//输入类型
		if(s.type == 1 || s.type == 2)
		{	//1: 注册
			printf("please input userid passwd\n");
			scanf("%s%s", s.userid, s.passwd);	
		}
		else if(s.type == 3 || s.type == 4 || s.type == 5 || s.type == 6)
		{
			if(login_flag == 0)
			{
				printf("请先登录!\n");
				continue;
			}
			if(s.type == 3)
			{	//获取文件列表, 结构体填  type = 3, 用户名
				strcpy(s.userid, userid);
			}
			if(s.type == 4)
			{
				printf("please input filename:\n");
				strcpy(s.userid, userid);
				scanf("%s", s.file_name);
				FILE *fp = fopen(s.file_name, "w");	//第一次下载，要清空
				fclose(fp);
			}
			if(s.type == 5)
			{	
				printf("please input filename:\n");
				strcpy(s.userid, userid);
				scanf("%s", s.file_name);
				send_file(&s);	//这里有send
				sleep(1);
				continue;		//跨过下面的 send
			}
		}
		send(fd, &s, sizeof(s), 0);	
        sleep(1);
	}
	close(fd);
}
