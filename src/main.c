#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
void *run(void *ptr){
	int i;
	for( i=0;i<3;i++)
	{
		sleep(1);	
		printf("hello %d\n",i);
	}
	return 0;
}
int main()
{
	pthread_t id;
	int ret=0;
	ret=pthread_create(&id,NULL,run,NULL);
	if(ret){
		printf("create thread failed\n");
	}
	pthread_join(id,NULL);
	return 0;
}
