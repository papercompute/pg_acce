/*--------------------------------------------------------------------------
 *
 * pg_acce.h
 *
 * Header file of pg_acce module
 *
 * Copyright (C) 2015 Alexey Shmatok <papercompute@gmail.com>
 *
 *
 */

#ifndef PG_ACCE_
#define PG_ACCE


#include "storage/dsm.h"
#include "storage/shm_mq.h"
#include "storage/spin.h"
#include "commands/explain.h"
#include "miscadmin.h"
#include "utils/resowner.h"
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>


extern Datum acce_info(PG_FUNCTION_ARGS);
extern Datum acce_mem_info(PG_FUNCTION_ARGS);
extern Datum acce_ocl_info(PG_FUNCTION_ARGS);


extern void acce_master_worker(Datum main_arg);

extern Datum acce_setup(PG_FUNCTION_ARGS);

extern void _PG_init(void);

#endif

