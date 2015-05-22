/*   
 *   File: optik.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:  
 *   optik.c is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the teroptik of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "queue-optik.h"
#include "utils.h"

RETRY_STATS_VARS;

#include "latency.h"
#if LATENCY_PARSING == 1
__thread size_t lat_parsing_get = 0;
__thread size_t lat_parsing_put = 0;
__thread size_t lat_parsing_rem = 0;
#endif	/* LATENCY_PARSING == 1 */

sval_t
queue_optik_find(queue_t* qu, skey_t key)
{ 
  return 1;
}

      /* cur = qu->overflow; */
      /* while (cur != NULL) */
      /* 	{ */
      /* 	  printf("[%zu] -> ", cur->key); */
      /* 	  cur = cur->next; */
      /* 	} */
      /* printf("NULL\n"); */

int
queue_optik_insert(queue_t* qu, skey_t key, sval_t val)
{
  queue_node_t* node = queue_new_node(key, val, NULL);

  if (optik_num_queued(qu->tail_lock) > 3)
    {
      optik_lock(&qu->overflow_lock);
      queue_node_t* head = qu->overflow_head;
      if (head == NULL)
	{
	  qu->overflow_head = qu->overflow_tail = node;
	  optik_unlock(&qu->overflow_lock);

	  cpause(1024);

	  optik_lock(&qu->overflow_lock);
	  queue_node_t* tail = qu->overflow_tail;
	  qu->overflow_head = qu->overflow_tail = NULL;
	  optik_unlock(&qu->overflow_lock);

	  optik_lock(&qu->tail_lock);
	  qu->tail->next = node;
	  qu->tail = tail;
	  optik_unlock(&qu->tail_lock);
	}
      else
	{
	  qu->overflow_tail->next = node;
	  qu->overflow_tail = node;
	  optik_unlock(&qu->overflow_lock);
	}
      return 1;
    }

  optik_lock(&qu->tail_lock);
  qu->tail->next = node;
  qu->tail = node; 
  optik_unlock(&qu->tail_lock);
  return 1;
}


sval_t
queue_optik_delete(queue_t* qu)
{
  sval_t val = 0;
  optik_lock(&qu->head_lock);
  queue_node_t* node = qu->head;
  queue_node_t* head_new = node->next;
  if (head_new == NULL)
    {
      optik_unlock(&qu->head_lock);
      return 0;
    }
  val = head_new->val;
  qu->head = head_new;
  optik_unlock(&qu->head_lock);

#if GC == 1
  ssmem_free(alloc, (void*) node);
#endif
  return val;
}
