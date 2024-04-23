/* ===========================================================================
 * @file sem_util.c
 *
 * @path $apps/components/utils/src
 *
 * @desc This is the communication interface of semaphore manager.
 *
 * AUTHOR: abeesh.mp@dicortech.com
 *
 * The code contained herein is licensed under the GNU General Public  License.
 * You may obtain a copy of the GNU General Public License Version 2 or later
 * at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 * ==========================================================================*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "sem_util.h"
//#include <error_type.h>
#define LOGE(fmt, args...)
#ifdef SEM_DEBUG
#define __D(fmt, args...) fprintf(stdout,  fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) fprintf(stderr, "Error: " fmt, ## args)

#define ERR_SEM_GET -1

#define STD_FAIL_RETURN -1
#define ERR_SEM_GAIN -3
#define ERR_SEM_REL -4
#define SUCCESS 5
#define ERR_SEM_DLT -6


/* union for IPC semaphore arguments
*/
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
} argument;

/* To get and initatilize semaphore
 * Arguments
 * key --> key for semaphore
 */
int Sem_init(int key)
{
	int id; 

	argument.val = 1;
	id = semget(key, 1, 0666 | IPC_CREAT);
	if (id < 0) {
		printf( "Unable to obtain semaphore.\n");
		return ERR_SEM_GET;
	}
	if ( semctl(id, 0, SETVAL, argument) < 0) {
		fprintf(stderr, "Cannot set semaphore value.\n");
		return ERR_SEM_GET;
	}
	return id;
}

/* To get semaphore ID
 * Arguments
 * id --> semaphore id
 * */
int Sem_Getid(int key)
{
	int id; 

	id = semget(key, 1, 0666);
	if (id < 0) {
		fprintf(stderr, "Unable to obtain semaphore.\n");
		return ERR_SEM_GET;
	}
	return id;
}

/* To gain semaphore
 * Arguments
 * id --> semaphore id
 * */
int Sem_gain(int id)
{
	struct sembuf operations;
	int ret;

	operations.sem_num = 0;
	operations.sem_op = -1;
	operations.sem_flg = 0;
	ret = semop(id, &operations, 1);
	if(ret < STD_FAIL_RETURN) {
		return ERR_SEM_GAIN;
	}
	__D("---------------------------> sem id =%d GAin value  %d\n",id,semctl(id, 0, GETVAL, NULL)) ;
	return 0;

}
/* Get Value of Semaphore*/
int Sem_GetValue(int id)
{
	__D("---------------------------> GET Value call sem id = %d value =%d \n",id, semctl(id, 0, GETVAL, NULL)); 
	return semctl(id, 0, GETVAL, NULL); 
}
int Sem_Pid(int id)
{
	__D("---------------------------> GET PID call sem id=%d   PID %d\n",id, semctl(id, 0, GETPID, NULL)); 
	return semctl(id, 0, GETPID, NULL); 
}
/* To release semaphore
 * Arguments
 * id --> semaphore id
 * */
int Sem_release(int id)
{
	struct sembuf operations;
	int ret;
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short int *array;
	} argument;
	
	argument.val = 1;
	operations.sem_num = 0;
	operations.sem_op =  1;
	operations.sem_flg = 0;
	ret = semop(id, &operations, 1);
	if(ret < STD_FAIL_RETURN) {
		__E("Sem release failed\n");
		return ERR_SEM_REL;
	}
	__D("\n---------------------------> sem id=%d release by val %d\n",id, semctl(id, 0, GETVAL, NULL)); 
	if(semctl(id, 0, GETVAL, NULL) > 1)
	{
		argument.val = 1;
		semctl(id, 0, SETVAL, argument); 
		__D("\n---------- SET Value call sem id = %d value =%d \n",id, semctl(id, 0, GETVAL, NULL)); 
	}

	return SUCCESS;

}

int Sem_delete(int id)
{
	int ret;

	ret = semctl(id, 0, IPC_RMID, argument);
	if(ret < STD_FAIL_RETURN)
		return ERR_SEM_DLT;
	return SUCCESS;
}
