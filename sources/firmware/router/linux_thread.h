/*! \file linux_thread.h \brief Synchronous threads library. */
//*****************************************************************************
//
// File Name	: 'linux_thread.h'
// Title		: Synchronous threads for POSIX systems Library
// Author		: Tzvetomir Stoyanov
// Created		: 26/08/2009
// Revised		: 
// Version		: 
// Editor Tabs	: 4
//
///	\ingroup sim
///	\defgroup SynThreads Synchronous threads library (linux_thread.c)
///	\par Description
///		Synchronous threads library provides support for read, write, timer 
///		and event threads. It works on every POSIX compliant OS. It is a wrapper 
///		arround a select and is designed to run the application's main loop.
///		Note: when a thread is executed, it is not scheduled again. It is up to
///		application to re-schedule it.
//
//	This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//*****************************************************************************
//@{
#ifndef __LINUX_THREAD_H_
#define __LINUX_THREAD_H_

/// Thread type enum
typedef enum		
{
	THREAD_READ,	///< Read thread
	THREAD_WRITE,	///< Write thread
	THREAD_TIMER,	///< Timer thread
	THREAD_EVENT,	///< Event thread
	THREAD_UNUSED,	///< Unused thread
	THREAD_MAX		///< MAX thread id
}thread_type_t; 	

typedef struct _thread_t thread_t;
typedef int32_t (*thread_callback_t)(thread_t *pThread);	///< Thread callback prototype

/// Main thread structure
typedef struct		
{
	struct _thread_t	*pReadList;		///< List of read threads
	struct _thread_t	*pWriteList;	///< List of write threads
	struct _thread_t	*pTimerList;	///< List of timer threads
	struct _thread_t	*pEventList;	///< List of event threads
	struct _thread_t	*pReadyList;	///< List of ready threads
	struct _thread_t	*pUnusedList;	///< List of unused threads	

	fd_set readfd;	///< Read fdset
	fd_set writefd;	///< Write fdset
}main_thread_t;	

/// Thread structure
struct _thread_t
{
	struct _thread_t	*pNext;		///< Next thread in the list
	main_thread_t	*pMain;			///< Pointer to main thread
	thread_type_t iType;			///< Thread type
	thread_callback_t fCallback;	///< Thread callback
	void *pArg;						///< Thread argument
	int32_t iFd;					///< Thread file descriptor, in case of read / write thread
	struct timeval stTime;			///< Thread time, in case of timer thread
};

//! Init main thread structure
/// \param pMain pointer to main thread, already allocated
void thread_main_init(main_thread_t *pMain);

//! Destroy main thread structure
/// \param pMain pointer to main thread
void thread_main_destroy(main_thread_t *pMain);


//! Add read thread
/// Thread monitors given file descriptor and when it is ready to be read,
///	calls the callback. A custom argument can passed to callback
/// \param pMain pointer to main thread
/// \param fCallback Callback functon
/// \param pArg custom callback argument 
/// \param iFd file descriptor, which will be monitored
thread_t *thread_read_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, int32_t iFd);

//! Add write thread
/// Thread monitors given file descriptor and when it is ready to be written,
///	calls the callback. A custom argument can passed to callback
/// \param pMain pointer to main thread
/// \param fCallback Callback functon
/// \param pArg custom callback argument 
/// \param iFd file descriptor, which will be monitored
thread_t *thread_write_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, int32_t iFd);

//! Add event thread
/// This thread will be executed immediately after current thread, 
/// and before other threads
/// \param pMain pointer to main thread
/// \param fCallback Callback functon
/// \param pArg custom callback argument 
thread_t *thread_event_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg);

//! Add timer thread
/// This thread will be executed after given time period
/// \param pMain pointer to main thread, already allocated
/// \param fCallback Callback functon
/// \param pArg custom callback argument 
/// \param pTime time period
thread_t *thread_timer_add(main_thread_t *pMain, thread_callback_t fCallback, void *pArg, struct timeval *pTime);

//! Cancel thread
/// Cancel executoin of given thread
/// \param pThread thread, which is already scheduled 
void thread_cancel(thread_t *pThread);

//! Run main thread
/// Application main loop. This function will loop until 
/// there is at least one active thread. 
/// \param pMainThread main thread
void thread_main_run(main_thread_t *pMainThread);

#endif /* __LINUX_THREAD_H_ */
//@}

