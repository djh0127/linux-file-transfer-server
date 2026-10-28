#include<stdio.h>
#include<sqlite3.h>
int main()
{
	sqlite3 *p;
	if(sqlite3_open("netdisk.db",&p)==0)
	{
		sqlite3_exec(p,"create table user_info(userid varchar(20),passwd varchar(20))",NULL,NULL,NULL);
		sqlite3_exec(p,"create table hist_info(userid varchar(20),history varchar(20),his_time varchar(20))",NULL,NULL,NULL);
		sqlite3_close(p);
	}
}
