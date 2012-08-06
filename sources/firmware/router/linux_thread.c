/*! \file linux_thread.c \brief Synchronous threads library. */
#include "os_linux.h"
#include "linux_thread.h"

void thread_main_init(main_thread_t *pMain)
{
	if(pMain)
		bzero(pMain, sizeof(main_thread_t));
}

static void thread_list_destroy(thread_t *pThread)
{
	thread_t *pCurrent = pThread, *pDel;
	while(pCurrent)
	{
		pDel = pCurrent;
		pCurrent = pCurrent->pNext;
		free(pDel);
	}
}

void thread_main_destroy(main_thread_t *pMain)
{
	if(NULL == pMain)
		return;
	thread_list_destroy(pMain->pReadList);
	thread_list_destroy(pMain->pWriteList);
	thread_list_destroy(pMain->pTimerList);
	thread_list_destroy(pMain->pEventList);
	thread_list_destroy(pMain->pReadyList);
	thread_list_destroy(pMain->pUnusedList);
}

static thread_t *thread_get(main_thread_t *pMain)
{
	thread_t *pThread=NULL;
	if(NULL == pMain)
		return NULL;
	pThread = pMain->pUnusedList;
	if(NULL == pThread)
	{
		pThread = malloc(sizeof(thread_t));
	}
	else
	{	
		pMain->pUnusedList = pThread->pNext;
	}
	if(pThread)
	{
		bzero(pThread, sizeof(thread_t));
		pThread->pMain = pMain;
	}
	return pThread;
}

thread_t *thread_read_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, int32_t iFd)
{
	thread_t *pThread = thread_get(pMain);
	
	if(NULL == pThread)
		return NULL;
	pThread->pNext = pMain->pReadList;
	pMain->pReadList = pThread;
	pThread->iType = THREAD_READ;
	pThread->fCallback = fCallback;
	pThread->pArg = pArg;
	pThread->iFd = iFd;
    if (!FD_ISSET(pThread->iFd, &(pMain->readfd)))
    	FD_SET (pThread->iFd, &(pMain->readfd));	
    return pThread;
}

thread_t *thread_write_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, int32_t iFd)
{
	thread_t *pThread = thread_get(pMain);
	
	if(NULL == pThread)
		return NULL;

	pThread->pNext = pMain->pWriteList;
	pMain->pWriteList = pThread;
	pThread->iType = THREAD_WRITE;
	pThread->fCallback = fCallback;
	pThread->pArg = pArg;
	pThread->iFd = iFd;
    if (!FD_ISSET(pThread->iFd, &(pMain->writefd)))
    	FD_SET (pThread->iFd, &(pMain->writefd));	
    return pThread;	
}

thread_t *thread_event_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg)
{
	thread_t *pThread = thread_get(pMain);
	
	if(NULL == pThread)
		return NULL;
	pThread->pNext = pMain->pEventList;
	pMain->pEventList = pThread;
	pThread->iType = THREAD_EVENT;
	pThread->fCallback = fCallback;
	pThread->pArg = pArg;
    return pThread;	
}


int time_compare ( struct timeval *t1, struct timeval *t2)
{
	if(NULL == t1 && NULL == t2)
		return 0;
	if(NULL == t1)
		return -1;
	if(NULL == t2)
		return 1;
	if(t1->tv_sec > t2->tv_sec)
		return 1;
	if(t1->tv_sec < t2->tv_sec)
		return -1;
	if(t1->tv_usec > t2->tv_usec)
		return 1;
	if(t1->tv_usec < t2->tv_usec)
		return -1;
	return 0;
}

thread_t *thread_timer_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, struct timeval *pTime)
{
	thread_t *pThread = thread_get(pMain);
	thread_t *pCurrent, *pPrev=NULL;
	struct timeval stTime;
	
	if(NULL == pThread || NULL == pTime)
		return NULL;
	pThread->iType = THREAD_TIMER;
	pThread->fCallback = fCallback;
	pThread->pArg = pArg;
	gettimeofday(&stTime, NULL);
	pThread->stTime.tv_sec = stTime.tv_sec + pTime->tv_sec;
	pThread->stTime.tv_usec = stTime.tv_usec + pTime->tv_usec;
	if(pThread->stTime.tv_usec >= 1000000)
	{
		pThread->stTime.tv_sec++;
		pThread->stTime.tv_usec -= 1000000;
	}
	if(NULL == pMain->pTimerList)
	{
		pMain->pTimerList = pThread;
	}
	else
	{
		pCurrent = pMain->pTimerList;
		while(pCurrent)
		{
			if(time_compare(&(pThread->stTime), &(pCurrent->stTime)) < 0)
			{
				if(pPrev)
				{
					pPrev->pNext = pThread;
					pThread->pNext = pCurrent;
				}
				else
				{
					pMain->pTimerList=pThread;
					pThread->pNext = pCurrent;
				}
				break;
			}
			pPrev = pCurrent;
			pCurrent =  pCurrent->pNext;
		}
		if(NULL == pCurrent)
		{
			pPrev->pNext = pThread;
			pThread->pNext = NULL;
		}
	}
	
    return pThread;		
}

static void thread_unlink(thread_t **ppThreadHead, thread_t *pThread)
{
	thread_t *pCurrent = *ppThreadHead, *pPrev=NULL;
	
	while(pCurrent)
	{
		if(pThread == pCurrent)
		{
			if(pPrev)
				pPrev->pNext = pThread->pNext;
			else
				(*ppThreadHead) = pThread->pNext;
			pThread->pNext = NULL;
			break;
		}
		pPrev = pCurrent;
		pCurrent = pCurrent->pNext;
	}
}

static void thread_add_unused(thread_t *pThread)
{
	if(NULL == pThread || NULL == pThread->pMain)
		return;
	pThread->iType = THREAD_UNUSED;
	pThread->pNext = pThread->pMain->pUnusedList;
	pThread->pMain->pUnusedList = pThread;	
}

void thread_cancel(thread_t *pThread)
{
	if(NULL == pThread || NULL == pThread->pMain)
		return;
	switch(pThread->iType)
	{
		case THREAD_READ:
			thread_unlink(&(pThread->pMain->pReadList), pThread);
			FD_CLR (pThread->iFd, &(pThread->pMain->readfd));
			break;
		case THREAD_WRITE:
			thread_unlink(&(pThread->pMain->pWriteList), pThread);
			FD_CLR (pThread->iFd, &(pThread->pMain->writefd));
			break;
		case THREAD_TIMER:
			thread_unlink(&(pThread->pMain->pTimerList), pThread);
			break;
		case THREAD_EVENT:
			thread_unlink(&(pThread->pMain->pEventList), pThread);
			break;
		default:
			break;
	}
	thread_add_unused(pThread);
}

thread_t *thread_fetch(main_thread_t *pMainThread, thread_t **ppReady)
{
	thread_t *pThread, *pCurrent;
	fd_set readfd;
	fd_set writefd;
	int iRet;
	struct timeval stTimeNow, *pTwait=NULL;
	
	if(NULL == pMainThread)
		return NULL;
	while(1)
	{
		if(NULL != pMainThread->pEventList)
		{
			pThread = pMainThread->pEventList;
			pMainThread->pEventList = pThread->pNext;
			thread_add_unused(pThread);
			*ppReady = pThread;
			return pThread;
		}
		gettimeofday(&stTimeNow, NULL);
		for(pThread = pMainThread->pTimerList; pThread;)	
		{
			pCurrent = pThread;
			pThread = pThread->pNext;
			if(time_compare((&stTimeNow), &(pCurrent->stTime)) >= 0)
			{
				thread_unlink(&(pMainThread->pTimerList), pCurrent);
				pCurrent->pNext = pMainThread->pReadyList;
				pMainThread->pReadyList = pCurrent;
			}
		}

		if(NULL != pMainThread->pReadyList)
		{
			pThread = pMainThread->pReadyList;
			pMainThread->pReadyList = pThread->pNext;
			thread_add_unused(pThread);
			*ppReady = pThread;
			return pThread;
		}
	
		readfd = pMainThread->readfd;
		writefd = pMainThread->writefd;
		if(pMainThread->pTimerList)
		{
			stTimeNow.tv_sec = pMainThread->pTimerList->stTime.tv_sec - stTimeNow.tv_sec;
			stTimeNow.tv_usec = pMainThread->pTimerList->stTime.tv_usec - stTimeNow.tv_usec;
			if(stTimeNow.tv_usec < 0)
			{
				stTimeNow.tv_sec--;
				stTimeNow.tv_usec = (1000000 - (-stTimeNow.tv_usec));
			}
			pTwait = &stTimeNow;
		}
		
		iRet = select (FD_SETSIZE, &readfd, &writefd, NULL, pTwait);
		
		if(iRet > 0)
		{
			pThread = pMainThread->pReadList;
			while(pThread)
			{
				pCurrent = pThread;
				pThread = pThread->pNext;
				if (FD_ISSET (pCurrent->iFd, &readfd))
				{
					thread_unlink(&(pMainThread->pReadList), pCurrent);
					FD_CLR(pCurrent->iFd, &pMainThread->readfd);
					pCurrent->pNext = pMainThread->pReadyList;
					pMainThread->pReadyList = pCurrent;
				}
			}
			pThread = pMainThread->pWriteList;
			while(pThread)
			{
				pCurrent = pThread;
				pThread = pThread->pNext;
				if (FD_ISSET (pCurrent->iFd, &writefd))
				{
					thread_unlink(&(pMainThread->pWriteList), pCurrent);
					FD_CLR(pCurrent->iFd, &pMainThread->writefd);
					pCurrent->pNext = pMainThread->pReadyList;
					pMainThread->pReadyList = pCurrent;
				}
			}			
		}
	}
	*ppReady = NULL;
	return NULL;
}

void thread_main_run(main_thread_t *pMainThread)
{
	thread_t *pThread=NULL;
	while(thread_fetch(pMainThread, &pThread))
		pThread->fCallback(pThread);
}
