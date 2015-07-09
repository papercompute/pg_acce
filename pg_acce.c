/*
 * pg_acce.c
 *
 * Copyright 2015 (C) Alexey Shmatok <papercompute@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "postgres.h"
#include "postmaster/bgworker.h"
#include "postmaster/syslogger.h"
#include "storage/bufmgr.h"
#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "lib/ilist.h"
#include "storage/barrier.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "storage/spin.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "storage/latch.h"
#include "access/xact.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "pgstat.h"
#include "utils/builtins.h"
#include "utils/snapmgr.h"
#include "tcop/utility.h"


#include <limits.h>
#include <unistd.h>
#include "pg_acce.h"

#define MAX_WORKERS 128

PG_MODULE_MAGIC;

typedef struct
{
  
} acce_worker_t;

static int num_workers = 0;



Datum
acce_info(PG_FUNCTION_ARGS)
{

	elog(LOG, "acce_info");

	PG_RETURN_VOID();
}
PG_FUNCTION_INFO_V1(acce_info);


Datum
acce_mem_info(PG_FUNCTION_ARGS)
{
	elog(LOG, "acce_mem_info");
	if (SRF_IS_FIRSTCALL())
	{
		elog(LOG,"first call");
	}
	PG_RETURN_VOID();
}
PG_FUNCTION_INFO_V1(acce_mem_info);

Datum
acce_ocl_info(PG_FUNCTION_ARGS)
{
	elog(LOG, "acce_ocl_info");

	PG_RETURN_VOID();
}
PG_FUNCTION_INFO_V1(acce_ocl_info);

/* Internal functions */


void
acce_worker(Datum main_arg)
{
	int id = DatumGetInt32(main_arg);
	elog(LOG,"acce_master_worker %d enter",id);


	BackgroundWorkerUnblockSignals();

	while (1)
	{
		CHECK_FOR_INTERRUPTS();
	}


	elog(LOG,"acce_master_worker %d exit",id);
}

static void
acce_add_more_workers_dynamic(int n)
{
	BackgroundWorker worker;
	BackgroundWorkerHandle *handle;
//	BgwHandleStatus status;
//	pid_t		pid;
	int i;

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	sprintf(worker.bgw_library_name, "pg_acce");
	sprintf(worker.bgw_function_name, "acce_worker");
//	snprintf(worker.bgw_name, BGW_MAXLEN, "acce_worker %d", n);
//	worker.bgw_main_arg = 0;
	worker.bgw_notify_pid = MyProcPid;

	for (i = 1; i <= n; i++)
	{
		snprintf(worker.bgw_name, BGW_MAXLEN, "acce_worker %d", num_workers+i);
		worker.bgw_main_arg = Int32GetDatum(num_workers+i);

		if (!RegisterDynamicBackgroundWorker(&worker, &handle))
			return;

		num_workers++;
	}
}


Datum
acce_setup(PG_FUNCTION_ARGS)
{
elog(LOG, "acce_setup");

acce_add_more_workers_dynamic(3);

PG_RETURN_VOID();

}
PG_FUNCTION_INFO_V1(acce_setup);


/*
void
_PG_init(void)
{

	elog(LOG, "acce init");

// 	if (!process_shared_preload_libraries_in_progress){
//		ereport(ERROR,
//		(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
//		errmsg("not process_shared_preload_libraries_in_progress"))); 
//		return;
//	}

	acce_setup_master_worker_dynamic(1);
	elog(LOG, "acce init done");
}
*/

