/*   
 *   File: lock_if.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: 
 *   lock_if.h is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LOCK_IF_H_
#define _LOCK_IF_H_

#include "utils.h"
#include "latency.h"

#ifdef USE_TSX
#include <immintrin.h>
#endif

#define PREFETCHW_LOCK(lock)                            PREFETCHW(lock)

#if defined(MUTEX)
typedef pthread_mutex_t ptlock_t;
#  define PTLOCK_SIZE sizeof(ptlock_t)
#  define INIT_LOCK(lock)				pthread_mutex_init((pthread_mutex_t *) lock, NULL);
#  define DESTROY_LOCK(lock)			        pthread_mutex_destroy((pthread_mutex_t *) lock)
#  define LOCK(lock)					pthread_mutex_lock((pthread_mutex_t *) lock)
#  define UNLOCK(lock)					pthread_mutex_unlock((pthread_mutex_t *) lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				pthread_mutex_init((pthread_mutex_t *) lock, NULL);
#  define GL_DESTROY_LOCK(lock)			        pthread_mutex_destroy((pthread_mutex_t *) lock)
#  define GL_LOCK(lock)					pthread_mutex_lock((pthread_mutex_t *) lock)
#  define GL_UNLOCK(lock)				pthread_mutex_unlock((pthread_mutex_t *) lock)
#elif defined(SPIN)		/* pthread spinlock */
typedef pthread_spinlock_t ptlock_t;
#  define PTLOCK_SIZE sizeof(ptlock_t)
#  define INIT_LOCK(lock)				pthread_spin_init((pthread_spinlock_t *) lock, PTHREAD_PROCESS_PRIVATE);
#  define DESTROY_LOCK(lock)			        pthread_spin_destroy((pthread_spinlock_t *) lock)
#  define LOCK(lock)					pthread_spin_lock((pthread_spinlock_t *) lock)
#  define UNLOCK(lock)					pthread_spin_unlock((pthread_spinlock_t *) lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				pthread_spin_init((pthread_spinlock_t *) lock, PTHREAD_PROCESS_PRIVATE);
#  define GL_DESTROY_LOCK(lock)			        pthread_spin_destroy((pthread_spinlock_t *) lock)
#  define GL_LOCK(lock)					pthread_spin_lock((pthread_spinlock_t *) lock)
#  define GL_UNLOCK(lock)				pthread_spin_unlock((pthread_spinlock_t *) lock)
#elif defined(TAS)			/* TAS */
#  if RETRY_STATS == 1
extern RETRY_STATS_VARS;
#    define PTLOCK_SIZE 32		/* choose 8, 16, 32, 64 */
typedef struct tticket
{
  union
  {
    volatile uint32_t whole;
    struct
    {
      volatile uint16_t tick;
      volatile uint16_t curr;
    };
  };
} tticket_t;
#elif (WAIT_LOCK_STATS == 1)
extern RETRY_STATS_VARS;
#    define PTLOCK_SIZE 32		/* choose 8, 16, 32, 64 */
typedef struct tticket
{
  union
  {
    volatile uint32_t whole;
    struct
    {
      volatile uint16_t tick;
      volatile uint16_t curr;
    };
  };
} tticket_t;

#  else
#    define PTLOCK_SIZE 32		/* choose 8, 16, 32, 64 */
#  endif
#  define PASTER(x, y, z) x ## y ## z
#  define EVALUATE(sz) PASTER(uint, sz, _t)
#  define UTYPE  EVALUATE(PTLOCK_SIZE)
#  define PASTER2(x, y) x ## y
#  define EVALUATE2(sz) PASTER2(CAS_U, sz)
#  define CAS_UTYPE EVALUATE2(PTLOCK_SIZE)
typedef volatile UTYPE ptlock_t;
#  define INIT_LOCK(lock)				tas_init(lock)
#  define DESTROY_LOCK(lock)			
#ifdef USE_TSX
#  define LOCK(lock)					tas_lock_tsx(lock)
#  define UNLOCK(lock)					tas_unlock_tsx(lock)
#  define TRYLOCK(lock)					tas_trylock_tsx(lock)
#else
#  define LOCK(lock)					tas_lock(lock)
#  define UNLOCK(lock)					tas_unlock(lock)
#  define TRYLOCK(lock)					tas_trylock(lock)
#endif
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				tas_init(lock)
#  define GL_DESTROY_LOCK(lock)			
#ifdef USE_TSX
#  define GL_LOCK(lock)					tas_lock_tsx(lock)
#  define GL_TRYLOCK(lock)				tas_trylock_tsx(lock)
#  define GL_UNLOCK(lock)              			tas_unlock_tsx(lock)
#else 
#  define GL_LOCK(lock)					tas_lock(lock)
#  define GL_TRYLOCK(lock)				tas_trylock(lock)
#  define GL_UNLOCK(lock)              			tas_unlock(lock)
#endif

#  define TAS_FREE 0
#  define TAS_LCKD 1

#if WAIT_LOCK_STATS == 1
extern __thread volatile ticks wait_lock;
extern __thread ticks gt_correction;
#endif


static inline void
tas_init(ptlock_t* l)
{
  *l = TAS_FREE;
#  if defined(__tile__)
  MEM_BARRIER;
#  endif
}

static inline uint32_t
tas_lock(ptlock_t* l)
{
  LOCK_TRY();
#if WAIT_LOCK_STATS == 1
  volatile tticket_t* t = (volatile tticket_t*) l;
  volatile uint16_t tick = FAI_U16(&t->tick);

  uint16_t cur = t->curr;
  int16_t distance = tick - cur;
  if (distance < 0 ) { printf("distatnce is %d / %u\n", distance, distance); }
  if (t->curr == tick) return 0;

  volatile ticks w_start = getticks(); 
  while (t->curr != tick)
    {
      PAUSE;
    }
  volatile ticks w_end = getticks(); 
  wait_lock += (w_end - w_start - gt_correction);
#if RETRY_STATS == 1
#define FACTOR 2800
    ticks tot = wait_lock/FACTOR; 
    if (tot >= HIST_SIZE) {
        tot=HIST_SIZE-1;
    }
    lat[tot]++;
#endif
#else
#if RETRY_STATS == 1
  volatile tticket_t* t = (volatile tticket_t*) l;
  volatile uint16_t tick = FAI_U16(&t->tick);

  uint16_t cur = t->curr;
  int16_t distance = tick - cur;
  if (distance < 0 ) { printf("distatnce is %d / %u\n", distance, distance); }

  LOCK_QUEUE(distance);

  while (t->curr != tick)
    {
      PAUSE;
    }

#else
  while (CAS_UTYPE(l, TAS_FREE, TAS_LCKD) == TAS_LCKD)
    {
      PAUSE;
    }
#endif
#endif
  return 0;
}

static inline uint32_t
tas_trylock(ptlock_t* l)
{
#if RETRY_STATS == 1
  LOCK_TRY_ONCE();
  volatile uint32_t tc = *(volatile uint32_t*) l;
  volatile tticket_t* tp = (volatile tticket_t*) &tc;

  if (tp->curr == tp->tick)
    {
      COMPILER_NO_REORDER(uint64_t tc_old = tc;);
      tp->tick++;
      int res = CAS_U32((uint32_t*) l, tc_old, tc) == tc_old;
      if (!res) 
	{
	  LOCK_QUEUE_ONCE(tp->tick - tp->curr);
	}
      return res;
    }
  else
    {
      LOCK_QUEUE_ONCE(tp->tick - tp->curr);
      return 0;
    }
#else
  return (CAS_UTYPE(l, TAS_FREE, TAS_LCKD) == TAS_FREE);
#endif

}

static inline uint32_t
tas_unlock(ptlock_t* l)
{
#  if defined(__tile__) 
  MEM_BARRIER;
#  endif

#if RETRY_STATS == 1
  volatile tticket_t* t = (volatile tticket_t*) l;
  PREFETCHW(t);
  COMPILER_NO_REORDER(t->curr++;);
#elif WAIT_LOCK_STATS == 1
  volatile tticket_t* t = (volatile tticket_t*) l;
  PREFETCHW(t);
  COMPILER_NO_REORDER(t->curr++;);
#else
  COMPILER_NO_REORDER(*l = TAS_FREE;);
#endif
  return 0;
}


#ifdef USE_TSX

#define TSX_NUM_RETRIES 10

static inline uint32_t tas_lock_tsx(ptlock_t * lock) {
    int num_retries = TSX_NUM_RETRIES;
    while (num_retries >= 0) {
        num_retries--;
        if (likely(_xbegin() == _XBEGIN_STARTED)) {
#if WAIT_LOCK_STATS == 1
            volatile tticket_t* t = (volatile tticket_t*) lock;
            if (t->curr == t->tick) {
                return 1;
            }
#elif RETRY_STATS == 1
            volatile tticket_t* t = (volatile tticket_t*) lock;
            if (t->curr == t->tick) {
                return 1;
            }
#else 
            if (likely(*lock == TAS_FREE)) {
                return 1;
            }
#endif
        _xabort(0xff);
        }
        PAUSE;
    PAUSE;
    PAUSE;
    PAUSE;
    PAUSE;
    }
    PAUSE;
    PAUSE;
    PAUSE;
    return tas_lock(lock);
}

static inline uint32_t tas_trylock_tsx(ptlock_t * lock) {
    int num_retries = TSX_NUM_RETRIES;
    while (num_retries >= 0) {
        num_retries--;
        if (likely(_xbegin() == _XBEGIN_STARTED)) {
#if WAIT_LOCK_STATS == 1
            volatile tticket_t* t = (volatile tticket_t*) lock;
            if (t->curr == t->tick) {
                return 1;
            }
#elif RETRY_STATS == 1
            volatile tticket_t* t = (volatile tticket_t*) lock;
            if (t->curr == t->tick) {
                return 1;
            }
#else 
            if (likely(*lock == TAS_FREE)) {
                return 1;
            }
#endif
        }
        _xabort(0xff);
    }
    return tas_trylock(lock);
}

static inline int tas_unlock_tsx(ptlock_t* lock) {
#if WAIT_LOCK_STATS == 1
    volatile tticket_t* t = (volatile tticket_t*) lock;
    if (t->curr == t->tick) {
        return 0;
    } else {
        return tas_unlock(lock);
    }
#elif RETRY_STATS == 1
    volatile tticket_t* t = (volatile tticket_t*) lock;
    if (t->curr == t->tick) {
        return 0;
    } else {
        return tas_unlock(lock);
    }
#else
    if (likely(*lock == TAS_FREE)) {
        return 0;
    } else {
        return tas_unlock(lock);
    }
#endif
}

#endif
#elif defined(TICKET)			/* ticket lock */

struct ticket_st
{
  uint32_t ticket;
  uint32_t curr;
  /* char padding[40]; */
};

typedef struct ticket_st ptlock_t;
#  define INIT_LOCK(lock)				ticket_init((volatile ptlock_t*) lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					ticket_lock((volatile ptlock_t*) lock)
#  define TRYLOCK(lock)					ticket_trylock((volatile ptlock_t*) lock)
#  define UNLOCK(lock)					ticket_unlock((volatile ptlock_t*) lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				ticket_init((volatile ptlock_t*) lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)					ticket_lock((volatile ptlock_t*) lock)
#  define GL_TRYLOCK(lock)				ticket_trylock((volatile ptlock_t*) lock)
#  define GL_UNLOCK(lock)				ticket_unlock((volatile ptlock_t*) lock)

static inline void
ticket_init(volatile ptlock_t* l)
{
  l->ticket = l->curr = 0;
#  if defined(__tile__)
  MEM_BARRIER;
#  endif
}

#  define TICKET_BASE_WAIT 512
#  define TICKET_MAX_WAIT  4095
#  define TICKET_WAIT_NEXT 128

static inline uint32_t
ticket_lock(volatile ptlock_t* l)
{
  uint32_t ticket = FAI_U32(&l->ticket);

#  if defined(OPTERON_OPTIMIZE)
  uint32_t wait = TICKET_BASE_WAIT;
  uint32_t distance_prev = 1;
  while (1)
    {
      PREFETCHW(l);
      uint32_t cur = l->curr;
      if (cur == ticket)
	{
	  break;
	}
      uint32_t distance = (ticket > cur) ? (ticket - cur) : (cur - ticket);

      if (distance > 1)
      	{
	  if (distance != distance_prev)
	    {
	      distance_prev = distance;
	      wait = TICKET_BASE_WAIT;
	    }

	  nop_rep(distance * wait);
	  wait = (wait + TICKET_BASE_WAIT) & TICKET_MAX_WAIT;
      	}
      else
	{
	  nop_rep(TICKET_WAIT_NEXT);
	}

      if (distance > 20)
      	{
      	  sched_yield();
      	}
    }

#  else  /* !OPTERON_OPTIMIZE */
  PREFETCHW(l);
  while (ticket != l->curr)
    {
      PREFETCHW(l);
      PAUSE;
    }

#  endif
  return 0;
}

static inline uint32_t
ticket_trylock(volatile ptlock_t* l)
{
  volatile uint64_t tc = *(volatile uint64_t*) l;
  volatile struct ticket_st* tp = (volatile struct ticket_st*) &tc;

  if (tp->curr == tp->ticket)
    {
      COMPILER_NO_REORDER(uint64_t tc_old = tc;);
      tp->ticket++;
      return CAS_U64((uint64_t*) l, tc_old, tc) == tc_old;
    }
  else
    {
      return 0;
    }
}

static inline uint32_t
ticket_unlock(volatile ptlock_t* l)
{
#  if defined(__tile__)
  MEM_BARRIER;
#  endif
  PREFETCHW(l);
  COMPILER_NO_REORDER(l->curr++;);
  return 0;
}

#elif defined(HTICKET)		/* Hierarchical ticket lock */

#  include "htlock.h"

#  define INIT_LOCK(lock)				init_alloc_htlock((htlock_t*) lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					htlock_lock((htlock_t*) lock)
#  define UNLOCK(lock)					htlock_release((htlock_t*) lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				init_alloc_htlock((htlock_t*) lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)					htlock_lock((htlock_t*) lock)
#  define GL_UNLOCK(lock)				htlock_release((htlock_t*) lock)

#elif defined(CLH)		/* CLH lock */

#  include "clh.h"

#  define INIT_LOCK(lock)				init_alloc_clh((clh_lock_t*) lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					clh_local_p.my_pred = \
    clh_acquire((volatile struct clh_qnode **) lock, clh_local_p.my_qnode);
#  define UNLOCK(lock)					clh_local_p.my_qnode = \
    clh_release(clh_local_p.my_qnode, clh_local_p.my_pred);
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				init_alloc_clh((clh_lock_t*) lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)					clh_local_p.my_pred = \
    clh_acquire((volatile struct clh_qnode **) lock, clh_local_p.my_qnode);
#  define GL_UNLOCK(lock)				clh_local_p.my_qnode = \
    clh_release(clh_local_p.my_qnode, clh_local_p.my_pred);
#endif



/* --------------------------------------------------------------------------------------------------- */
/* GLOBAL LOCK --------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */


#if defined(LL_GLOBAL_LOCK)
#  define ND_GET_LOCK(nd)                 nd /* LOCK / UNLOCK are not defined in any case ;-) */

#  undef INIT_LOCK
#  undef DESTROY_LOCK
#  undef LOCK
#  undef UNLOCK
#  undef PREFETCHW_LOCK

#  define INIT_LOCK(lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)
#  define UNLOCK(lock)
#  define PREFETCHW_LOCK(lock)

#  define INIT_LOCK_A(lock)       GL_INIT_LOCK(lock)
#  define DESTROY_LOCK_A(lock)    GL_DESTROY_LOCK(lock)			
#  define LOCK_A(lock)            GL_LOCK(lock)
#  define TRYLOCK_A(lock)         GL_TRYLOCK(lock)
#  define UNLOCK_A(lock)          GL_UNLOCK(lock)
#  define PREFETCHW_LOCK_A(lock)  

#else  /* !LL_GLOBAL_LOCK */
#  define ND_GET_LOCK(nd)                 &nd->lock

#  undef GL_INIT_LOCK
#  undef GL_DESTROY_LOCK
#  undef GL_LOCK
#  undef GL_UNLOCK

#  define GL_INIT_LOCK(lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)
#  define GL_UNLOCK(lock)

#  define INIT_LOCK_A(lock)       INIT_LOCK(lock)
#  define DESTROY_LOCK_A(lock)    DESTROY_LOCK(lock)			
#  define LOCK_A(lock)            LOCK(lock)
#  define TRYLOCK_A(lock)         TRYLOCK(lock)
#  define UNLOCK_A(lock)          UNLOCK(lock)
#  define PREFETCHW_LOCK_A(lock)  PREFETCHW_LOCK(lock)

#endif

#endif	/* _LOCK_IF_H_ */
