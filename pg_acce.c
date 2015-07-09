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
#include <limits.h>
#include <unistd.h>
#include "pg_acce.h"

PG_MODULE_MAGIC;

/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;

/* GUC variables */
static int acce_num_workers = 0;


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

static void
acce_master_worker(Datum main_arg)
{
 pqsignal(SIGHUP, master_worker_sighup);
 pqsignal(SIGTERM, master_worker_sigterm);
}
void
acce_setup_master_worker(void)
{
 BackgroundWorker worker;
 memset(&worker, 0, sizeof(BackgroundWorker));
 strcpy(worker.bgw_name, "acce master worker");
 worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
 worker.bgw_start_time = BgWorkerStart_PostmasterStart;
 worker.bgw_restart_time = BGW_NEVER_RESTART;
 worker.bgw_main = acce_master_worker;
 worker.bgw_main_arg = 0;
 RegisterBackgroundWorker(&worker);
 elog(LOG,acce_setup_master_worker);
}

/*
void
acce_setup(PG_FUNCTION_ARGS)
{
elog(LOG, "acce_setup");

acce_setup_bgworker(PG_GETARG_INT32(0));
}
PG_FUNCTION_INFO_V1(acce_setup);
*/

void
_PG_init(void)
{

	elog(LOG, "acce init");

 	if (!process_shared_preload_libraries_in_progress){
		ereport(ERROR,
		(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
		errmsg("not process_shared_preload_libraries_in_progress"))); 
		return;
	}
	acce_setup_master_worker();
}


