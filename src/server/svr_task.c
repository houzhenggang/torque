/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/

/*
 * @file svr_task.c
 *
 * Contains functions to deal with the server's task list
 *
 * A task list is a set of pending functions usually associated with
 * processing a reply message.
 *
 * Functions included are:
 */

#include <pbs_config.h>   /* the master config generated by configure */
#include <list>

#include "portability.h"
#include <stdlib.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include "server_limits.h"
#include "list_link.h"
#include "work_task.h"
#include "utils.h"
#include "threadpool.h"
#include "../lib/Liblog/pbs_log.h"

#define AFTER_IS_BEING_RECYCLED -240
extern void check_nodes(struct work_task *);

/* Global Data Items: */

std::list<timed_task>  *task_list_timed;
extern pthread_mutex_t  task_list_timed_mutex;
extern task_recycler    tr;



void insert_timed_task(

  work_task *wt)

  {
  std::list<timed_task>::iterator it;
  bool                            inserted = false;
  timed_task                      tt;

  tt.wt = wt;
  tt.task_time = wt->wt_event;

  pthread_mutex_lock(&task_list_timed_mutex);

  for (it = task_list_timed->begin(); it != task_list_timed->end(); it++)
    {
    if (it->task_time >= tt.task_time)
      {
      task_list_timed->insert(it, tt);
      inserted = true;
      break;
      }
    }

  if (inserted == false)
    task_list_timed->push_back(tt);

  pthread_mutex_unlock(&task_list_timed_mutex);
  } /* END insert_timed_task() */



/*
 * pop_timed_task - return task from list of timed tasks.
 *
 * Returns NULL or pointer to a timed task with its mutex locked.
 */

work_task *pop_timed_task(

  time_t  time_now)

  {
  struct work_task                *wt = NULL;
  std::list<timed_task>::iterator  it;

  // lock the mutex for the timed task list
  pthread_mutex_lock(&task_list_timed_mutex);

  if (task_list_timed->begin() != task_list_timed->end())
    {
    it = task_list_timed->begin();

    if (it->task_time <= time_now)
      {
      wt = it->wt;
      task_list_timed->pop_front();
      }
    }

  // lock the mutex for the task
  if (wt != NULL)
    pthread_mutex_lock(wt->wt_mutex);
  
  // unlock the mutex for the timed task list
  pthread_mutex_unlock(&task_list_timed_mutex);

  return(wt);
  } /* END pop_timed_task() */



/*
 * set_task - add the job entry to the task list
 *
 * Task time depends on the type of task.  The list is time ordered.
 */

struct work_task *set_task(

  enum work_type   type,
  long             event_id,  /* I - based on type can be time of event */
  void           (*func)(struct work_task *),
  void            *parm,
  int              get_lock)

  {
  work_task *pnew;

  if ((pnew = (struct work_task *)calloc(1, sizeof(struct work_task))) == NULL)
    {
    return(NULL);
    }

  pnew->wt_event    = event_id;
  pnew->wt_type     = type;
  pnew->wt_func     = func;
  pnew->wt_parm1    = parm;

  if (type == WORK_Immed)
    {
    enqueue_threadpool_request((void *(*)(void *))func, pnew, task_pool);
    }
  else
    {
    if ((pnew->wt_mutex = (pthread_mutex_t*)calloc(1, sizeof(pthread_mutex_t))) == NULL)
      {
      free(pnew);
      return(NULL);
      }
    
    pthread_mutex_init(pnew->wt_mutex,NULL);
    pthread_mutex_lock(pnew->wt_mutex);
   
    insert_timed_task(pnew);

    /* only keep the lock if they want it */
    if (get_lock == FALSE)
      pthread_mutex_unlock(pnew->wt_mutex);
    }

  return(pnew);
  }  /* END set_task() */




/* 
 * checks if the task has been queued into a threadpool
 * right now, check_nodes is the only one done that way, but all
 * future ones should be added here
 */

int task_is_in_threadpool(

  struct work_task *ptask)

  {
  if (ptask->wt_func == check_nodes)
    return(TRUE);

  return(FALSE);
  } /* END task_is_in_threadpool() */



bool can_dispatch_task()

  {
  /* The difference between the maximun number of threads and the current number of threads */
  int  max_n_thread_difference;
  bool can_dispatch = true;
  
  pthread_mutex_lock(&request_pool->tp_mutex);
  max_n_thread_difference = request_pool->tp_max_threads - request_pool->tp_nthreads;
  if ((request_pool->tp_idle_threads + max_n_thread_difference) <= (request_pool->tp_max_threads * 0.1))
    {
    can_dispatch = false;
    }
  pthread_mutex_unlock(&request_pool->tp_mutex);

  return(can_dispatch);
  } /* END can_dispatch_task() */



/*
 * dispatch_task - dispatch a work task found on a work list
 *
 * delinks the work task entry, calls the associated function with
 * the parameters from the work task entry, and then frees the entry.
 */

int dispatch_task(

  struct work_task *ptask) /* M */

  {
  int rc = PBSE_NONE;

  if (can_dispatch_task() == false)
    return(PBSE_SERVER_BUSY);
  
  if (ptask->wt_tasklist)
    remove_task(ptask->wt_tasklist, ptask);

  /* mark the task as being recycled - it gets freed later */
  ptask->wt_being_recycled = TRUE;
  pthread_mutex_unlock(ptask->wt_mutex);

  if (ptask->wt_func != NULL)
    enqueue_threadpool_request((void *(*)(void *))ptask->wt_func, ptask, task_pool);

  return(rc);
  }  /* END dispatch_task() */


int dispatch_timed_task(

  work_task *ptask)

  {
  int rc = PBSE_NONE;

  if (can_dispatch_task() == false)
    {
    insert_timed_task(ptask);
    rc = PBSE_SERVER_BUSY;
    }
  else
    {
    /* mark the task as being recycled - it gets freed later */
    ptask->wt_being_recycled = TRUE;
    pthread_mutex_unlock(ptask->wt_mutex);

    if (ptask->wt_func != NULL)
      enqueue_threadpool_request((void *(*)(void *))ptask->wt_func, ptask, task_pool);
    }

  return(rc);
  } /* END dispatch_timed_task() */



/*
 * delete_task - unlink and free a work_task structure.
 */

void delete_task(
    
  struct work_task *ptask) /* M */

  {
  if (ptask->wt_tasklist)
    remove_task(ptask->wt_tasklist,ptask);

  /* put the task in the recycler */
  insert_task_into_recycler(ptask);

  pthread_mutex_unlock(ptask->wt_mutex);
  } /* END delete_task() */




/* 
 * initialize_all_tasks_array
 *
 * initializes the all_tasks object
 */


/*
 * adds a task to the specified array
 */

int insert_task(

  all_tasks *at,
  work_task *wt)

  {
  pthread_mutex_lock(at->alltasks_mutex);
  at->tasks.push_back(wt);
  wt->wt_tasklist = at;
  pthread_mutex_unlock(at->alltasks_mutex);

  return(PBSE_NONE);
  } /* END insert_task() */




/*
 * checks if this all_tasks array has any tasks 
 *
 * @param at - the all_tasks array to check
 * @return TRUE is there are any tasks, FALSE otherwise
 */

int has_task(

  all_tasks *at) /* I */

  {
  int rc = FALSE;

  pthread_mutex_lock(at->alltasks_mutex);

  if (at->tasks.size() > 0)
    rc = TRUE;

  pthread_mutex_unlock(at->alltasks_mutex);

  return(rc);
  } /* END has_task() */




/*
 * removes a specific tasks from this array
 */
int remove_task(

  all_tasks *at,
  work_task *wt)

  {
  int rc = PBSE_NONE;

  if (pthread_mutex_trylock(at->alltasks_mutex))
    {
    pthread_mutex_unlock(wt->wt_mutex);
    pthread_mutex_lock(at->alltasks_mutex);
    pthread_mutex_lock(wt->wt_mutex);
    }

  if (wt->wt_being_recycled == FALSE)
    {
    for(std::vector<work_task *>::iterator it = at->tasks.begin();it != at->tasks.end();it++)
      {
      if(wt == *it)
        {
        at->tasks.erase(it);
        break;
        }
      }
    }
    
  pthread_mutex_unlock(at->alltasks_mutex);

  return(rc);
  } /* END remove_task() */




void initialize_task_recycler()

  {
  tr.iter = tr.tasks.tasks.end();

  tr.mutex = (pthread_mutex_t*)calloc(1, sizeof(pthread_mutex_t));
  pthread_mutex_init(tr.mutex, NULL);
  } /* END initialize_task_recycler() */




work_task *next_task_from_recycler(

  all_tasks *at,
  std::vector<work_task *>::iterator& iter)

  {
  work_task *wt = NULL;

  pthread_mutex_lock(at->alltasks_mutex);
  if(at->tasks.size() != 0)
    {
    if(iter == at->tasks.end())
      {
      iter = at->tasks.begin();
      }
    else
      {
      iter++;
      }
    wt = *iter;
    }
  if (wt != NULL)
    pthread_mutex_lock(wt->wt_mutex);
  pthread_mutex_unlock(at->alltasks_mutex);

  return(wt);
  } /* END next_task_from_recycler() */



void *remove_some_recycle_tasks(

  void *vp)

  {
  int        i;
  std::vector<work_task *>::iterator iter = tr.tasks.tasks.end();
  work_task *ptask;

  pthread_mutex_lock(tr.mutex);

  for (i = 0; i < TASKS_TO_REMOVE; i++)
    {
    ptask = next_task_from_recycler(&tr.tasks, iter);

    if (ptask == NULL)
      break;

    iter = tr.tasks.tasks.erase(iter);
    pthread_mutex_unlock(ptask->wt_mutex);
    free(ptask->wt_mutex);
    free(ptask);
    }

  pthread_mutex_unlock(tr.mutex);

  return(NULL);
  } /* END remove_some_recycle_tasks() */




int insert_task_into_recycler(

  struct work_task *ptask)

  {
  pthread_mutex_t *tmp = ptask->wt_mutex;
  int              rc;

  memset(ptask, 0, sizeof(struct work_task));
  ptask->wt_mutex = tmp;

  pthread_mutex_lock(tr.mutex);

  ptask->wt_being_recycled = TRUE;

  if (tr.tasks.tasks.size() >= MAX_TASKS_IN_RECYCLER)
    enqueue_threadpool_request(remove_some_recycle_tasks, NULL, task_pool);

  rc = insert_task(&tr.tasks, ptask);

  pthread_mutex_unlock(tr.mutex);

  return(rc);
  } /* END insert_task_into_recycler() */



