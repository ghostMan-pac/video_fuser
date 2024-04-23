/* ===========================================================================
 * @file sem_util.h
 *
 * @path $apps/components/utils/inc
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

#ifndef __SEM_UTIL_H__
#define __SEM_UTIL_H__

#ifdef __cplusplus
extern "C"{
#endif
/* Semaphores APIs*/
int Sem_init(int key);
int Sem_Getid(int key);
int Sem_delete(int id);
int Sem_release(int id);
int Sem_gain(int id);
int Sem_Pid(int id);
int Sem_GetValue(int id);
#ifdef __cplusplus
}
#endif
#endif
