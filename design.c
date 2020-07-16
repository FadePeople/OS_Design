#define __LIBRARY__
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

int judge(char a, char b);
void usleep(int num);

typedef void sem_t;
#define __NR_sem_open 87
#define __NR_sem_wait 88
#define __NR_sem_post 89
#define __NR_sem_unlink 90
_syscall2(int, sem_open, const char*, name, unsigned int, value)
_syscall1(int, sem_wait, sem_t *, sem)
_syscall1(int, sem_post, sem_t *, sem)
_syscall1(int, sem_unlink, const char *, name)

/*0:石头 1:剪刀 2:布 X:超时*/

int main()
{
	/*游戏回合数和出拳限制时间*/
	int BUF,ti;
    /*文件地址*/
	int fd=0;
	/*两个选手给裁判发出拳信息管道*/
	int fd1[2], fd2[2];
	/*裁判给选手发对方出拳汇总管道*/
	int fd3[2], fd4[2];
	/*存储出拳信息所用数组*/
	char buf1[1000], buf2[1000],str2[1000];
	/*存储对方出拳信息所用数组*/
	char data1[1000], data2[1000];
	/*进程定义*/
	pid_t pid1, pid2, jud;
	/*读取信息的长度和裁判判定结果*/
	int len, tmp;
	int i,j;
	/*胜负和平局的数量*/
	int p1 = 0, p2 = 0, ping = 0;
	/*选手1出拳汇总信息*/
	int num1_0 = 0, num1_1 = 0, num1_2 = 0;
	/*选手2出拳汇总信息*/
	int num2_0 = 0, num2_1 = 0, num2_2 = 0;
	int n1_0 = 0, n1_1 = 0, n1_2 = 0;
	int n2_0 = 0, n2_1 = 0, n2_2 = 0;
	/*出拳总数和超时标志*/
	int num = 0, flag = 0;
	/*输入异常标志*/
	int ff = 1;

	/*定义初始化信号量*/
	sem_t *signal_1, *signal_2, *mutex,*signal_jud;
	signal_1 = (sem_t*)sem_open("signal_1", 1);
	signal_2 = (sem_t*)sem_open("signal_2", 0);
	signal_jud = (sem_t*)sem_open("signal_jud", 0);

	printf("Please input game num and limit time: ");
	printf("\n");
	scanf("%d %d", &BUF,&ti);
	if (BUF < 0) {
		ff = -1;
		printf("Please input right game num!\n");
	}
	if (ti < 0) {
		ff = -2;
		printf("Please input right limit time!\n");
	}
	while (ff<0)
	{
		scanf("%d %d", &BUF, &ti);
		if (BUF < 0) {
			ff = -1;
			printf("Please input right game num!\n");
		}
		if (ti < 0) {
			ff = -2;
			printf("Please input right limit time!\n");
		}
		if (BUF > 0 && ti > 0) ff = 1;
	}

	/*创建管道*/
	if (pipe(fd1) < 0)
	{
		perror("failed to pipe");
		exit(1);
	}
	if (pipe(fd2) < 0)
	{
		perror("failed to pipe");
		exit(1);
	}
	if (pipe(fd3) < 0)
	{
		perror("failed to pipe");
		exit(1);
	}
	if (pipe(fd4) < 0)
	{
		perror("failed to pipe");
		exit(1);
	}
	/*创建选手1进程*/
	pid1 = fork();
	if (pid1 == 0)
	{
		if (BUF != 0) printf("create p1 pid successfully\n\n");
		srand(time(NULL));
		for (i = 1; i <= BUF; i++)
		{
			/*进程被堵塞，等待激活*/
			sem_wait(signal_1);
			/*memset(buf2,0,sizeof(buf2));*/
			/*随机设置延时时长，作为选手的出牌时间*/
			j = (rand()+i) % 30;
			usleep(j+20);
			if (j>ti) {
				flag = 1;
				sprintf(buf2, "%c", 'X');
			}
			if (i > 1&&!flag) {
				len = read(fd3[0], data1, 10);
				/*printf("data1: %s\n",data1);*/
				sscanf(data1,"%d %d %d",&n2_0,&n2_1,&n2_2);
				num = n2_0 + n2_1 + n2_2;
				if (n2_0 > num*3*1.0/8)sprintf(buf2, "%d", 2);
				else if (n2_1 > num*3*1.0/8)sprintf(buf2, "%d", 0);
				else if (n2_2 > num*3*1.0/8)sprintf(buf2, "%d", 1);
				else sprintf(buf2, "%d", rand() % 3);
			}
			else if(i == 1) sprintf(buf2, "%d", rand() % 3);
			printf("i = %d, p1 pid is %d, p1:%c\n", i, getpid(), buf2[0]);
			write(fd2[1], buf2, BUF);
			usleep(50);
			/*激活进程*/
			flag = 0;
			sem_post(signal_2);
		}
		exit(0);
	}
	/*创建选手2进程*/
	pid2 = fork();
	if (pid2 == 0)
	{
		if (BUF != 0) printf("create p2 pid successfully\n");
		srand(time(NULL) + BUF);
		for (i=1; i <= BUF; i++)
		{
			/*进程被堵塞，等待激活*/
			sem_wait(signal_2);
			/*memset(buf1, 0, sizeof(buf1));*/
			/*随机设置延时时长，作为选手的出牌时间*/
			j = (rand() + i) % 30;
			if (j > ti) {
				flag = 1;
				sprintf(buf1, "%c", 'X');
			}
			if (i > 1&&!flag) {
				len = read(fd4[0], data2, 10);
				/*printf("data2: %s\n",data2);*/
				sscanf(data2, "%d %d %d", &n1_0, &n1_1, &n1_2);
				num = n1_0 + n1_1 + n1_2;
				if (n1_0 > num * 3 * 1.0 / 8)sprintf(buf1, "%d", 2);
				else if (n1_1 > num * 3 * 1.0 / 8)sprintf(buf1, "%d", 0);
				else if (n1_2 > num * 3 * 1.0 / 8)sprintf(buf1, "%d", 1);
				else sprintf(buf1, "%d", rand() % 3);
			}
			else if (i == 1) sprintf(buf1, "%d", rand() % 3);
			printf("i = %d, p2 pid is %d, p2:%c\n", i, getpid(), buf1[0]);
			write(fd1[1], buf1, BUF);
			usleep(50);
			/*激活裁判进程*/
			flag = 0;
			sem_post(signal_jud);
		}
		exit(0);
	}
	/*创建裁判进程*/
	jud = fork();
	if (jud == 0) 
	{
		if(BUF!=0) printf("create jud pid successfully\n");
		fd = open("data.txt", O_WRONLY | O_TRUNC | O_CREAT);
		for (i = 1; i <= BUF; i++)
		{
			/*进程被堵塞，等待激活*/
			sem_wait(signal_jud);
			memset(buf1, 0, sizeof(buf1));
			memset(buf2, 0, sizeof(buf2));
			memset(data1, 0, sizeof(data1));
			memset(data2, 0, sizeof(data2));
			printf("i = %d, jud pid is %d ",i, getpid());
			len = read(fd1[0], buf1, BUF);
			usleep(50);
			len = read(fd2[0], buf2, BUF);
			usleep(50);
			tmp = judge(buf2[0], buf1[0]);
			if (buf1[0] == '0') num2_0++;
			else if (buf1[0] == '1') num2_1++;
			else if (buf1[0] == '2') num2_2;
			if (buf2[0] == '0') num1_0++;
			else if (buf2[0] == '1') num1_1++;
			else if (buf2[0] == '2') num1_2++;

			if (buf2[0] == 'X'&&buf1[0]!='X') {
				printf("p1 is time out,so p2 wins!\n\n");
				sprintf(str2, "i: %d p1: %s p2: %c p2 wins!\n",i, "over", buf1[0]);
				write(fd, str2, strlen(str2));
				p2++;

				sprintf(data1, "%d %d %d", num2_0, num2_1, num2_2);
				sprintf(data2, "%d %d %d", num1_0, num1_1, num1_2);
				write(fd3[1], data1, 10);
				write(fd4[1], data2, 10);
				/*printf("num1: %s,num2: %s\n", data1, data2);*/
			}
			else if (buf1[0] == 'X'&&buf2[0]!='X') {
				printf("p2 is time out,so p1 wins!\n\n");
				sprintf(str2, "i: %d p1: %c p2: %s p1 wins!\n", i,buf2[0], "over");
				write(fd, str2, strlen(str2));
				p1++;

				sprintf(data1, "%d %d %d", num2_0, num2_1, num2_2);
				sprintf(data2, "%d %d %d", num1_0, num1_1, num1_2);
				write(fd3[1], data1, 10);
				write(fd4[1], data2, 10);
				/*printf("num1: %s,num2: %s\n", data1, data2);*/
			}
			else if (buf1[0] == 'X'&&buf2[0] == 'X') {
				ping++;
				printf("p1 and p2 are both time out,so no one wins!\n\n");
				sprintf(str2,"i: %d p1: %s p2: %s no one wins!\n",i,"over","over");
				write(fd, str2, strlen(str2));

				sprintf(data1, "%d %d %d", num2_0, num2_1, num2_2);
				sprintf(data2, "%d %d %d", num1_0, num1_1, num1_2);
				write(fd3[1], data1, 10);
				write(fd4[1], data2, 10);
				/*printf("num1: %s,num2: %s\n", data1, data2);*/
			}
			else
			{
				sprintf(data1,"%d %d %d",num2_0,num2_1,num2_2);
				sprintf(data2,"%d %d %d",num1_0,num1_1,num1_2);
				write(fd3[1], data1, 10);
				write(fd4[1], data2, 10);
				/*printf("num1: %s,num2: %s\n",data1,data2);*/

				if (tmp == 0) {
					ping++;
					printf("no one wins!\n\n");
					sprintf(str2, "i: %d p1: %c p2: %c no one wins!\n", i, buf1[0], buf2[0]);
					write(fd, str2, strlen(str2));
				}
				else {
					if (tmp > 0)
					{
						p1++;
						printf("p1 wins!\n\n");
						sprintf(str2, "i: %d p1: %c p2: %c p1 wins!\n", i, buf2[0], buf1[0]);
						write(fd, str2, strlen(str2));
					}
					else
					{
						p2++;
						printf("p2 wins!\n\n");
						sprintf(str2, "i: %d p1: %c p2: %c p2 wins!\n", i, buf2[0], buf1[0]);
						write(fd, str2, strlen(str2));
					}
				}
			}
			/*激活进程*/
			sem_post(signal_1);
		}
		if (BUF != 0) {
			sprintf(str2, "In summary:\np2 wins %d  rounds.\np1 wins %d  rounds.\np1 and p2 are %d draws\n", p1, p2, ping);
			write(fd, str2, strlen(str2));
			printf("In summary:\n");
			printf("p1 wins %d  rounds.\n", p1);
			printf("p2 wins %d  rounds.\n", p2);
			printf("p1 and p2 are %d draws\n", ping);
			if (p1 == p2)
			{
				printf("p1 and p2 rounds are same,so p1 and p2 are draws!\n");
				strcpy(str2, "p1 and p2 rounds are same,so p1 and p2 are draws!\n");
				write(fd, str2, strlen(str2));
			}
			else
			{
				printf("%s wins in the game!\n", (p1 > p2) ? "p1" : "p2");
				sprintf(str2, "%s wins in the game!\n", (p1 > p2) ? "p1" : "p2");
				write(fd, str2, strlen(str2));
			}
			printf("write file successfully!\n");
		}
		else printf("The game has not started\n");
	}
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
	waitpid(jud, NULL, 0);
	exit(0);
}

void usleep(int num)
{
	int i;
	for (i = 1; i <= num; i++);
}

int judge(char a, char b)
{
	int r = 0;
	if (a == '3') return -1;
	else if (b == '3') return 1;
	if (a == b)
		r = 0;
	else
	{
		if (a == '0'&&b == '1')r = 1;
		if (a == '0'&&b == '2')r = -1;
		if (a == '1'&&b == '2')r = 1;
		if (a == '1'&&b == '0')r = -1;
		if (a == '2'&&b == '0')r = 1;
		if (a == '2'&&b == '1')r = -1;
	}
	return r;
}