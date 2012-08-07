#include "os_linux.h"
#include "linux_sem.h"

int sem_create(int iInitial)
{
 int iSem;
 
	iSem = semget(IPC_PRIVATE, 1, 0644);
	if(iSem >= 0)
	{
		union{
			int val;
			struct semid_ds *buf;
			uint16_t *array;
			struct seminfo *pbuf;		
		}semun;
		semun.val = iInitial;
		semctl(iSem, 0, SETVAL, semun);
	}
	return iSem;	
}

void sem_delete(int iSem)
{
	semctl(iSem, 0, IPC_RMID, 0);
}

void sem_take(int iSem)
{
 struct sembuf sb;
 
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	semop(iSem, &sb, 1);
}

void sem_give(int iSem)
{
 struct sembuf sb;
 
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	semop(iSem, &sb, 1);
}

