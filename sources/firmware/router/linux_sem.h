#ifndef __LINUX_SEM_H_
#define __LINUX_SEM_H_

#ifndef SEM_FULL
#define SEM_FULL 0
#endif
#ifndef SEM_EMPTY
#define SEM_EMPTY 1
#endif

int sem_create(int iInitial);
void sem_delete(int iSem);
void sem_take(int iSem);
void sem_give(int iSem);

#endif /* __LINUX_SEM_H_ */

