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

extern Datum acce_setup(PG_FUNCTION_ARGS);

extern Datum acce_info(PG_FUNCTION_ARGS);
extern Datum acce_mem_info(PG_FUNCTION_ARGS);
extern Datum acce_ocl_info(PG_FUNCTION_ARGS);


#define ACCE_SHM_MQ_MAGIC		0x3e270d92

typedef struct
{
	int		id;
	slock_t		mutex;
	int 		cmd;
	char		param[NAMEDATALEN];
} acce_worker_params;

typedef struct
{
	char		 *name;
	dsm_segment	 *segment;
	shm_toc		 *toc;
	BackgroundWorkerHandle	*handle;
	PGPROC			*parent;
	shm_mq_handle		*in_mq;
	shm_mq_handle		*out_mq;
	acce_worker_params	*params;
} acce_worker_data;

#endif

