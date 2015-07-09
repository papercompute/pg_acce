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

PG_MODULE_MAGIC;

/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;

/* GUC variables */
//static int acce_num_workers = 0;
static int acce_master_worker_setup_counter = 0;


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

static void
master_worker_sigterm(SIGNAL_ARGS)
{
	int save_errno = errno;

	got_sigterm = true;
	SetLatch(MyLatch);
	errno = save_errno;
}

static void
master_worker_sighup(SIGNAL_ARGS)
{
	int save_errno = errno;

	got_sighup = true;
	SetLatch(MyLatch);
	errno = save_errno;
}

void
acce_master_worker(Datum main_arg)
{
	elog(LOG,"acce_master_worker enter");

	pqsignal(SIGHUP, master_worker_sighup);
	pqsignal(SIGTERM, master_worker_sigterm);

	BackgroundWorkerUnblockSignals();

	while (!got_sigterm)
	{
		//int			ret;
		int			rc;

		rc = WaitLatch(MyLatch,
					   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
					   1 * 1000L);
		ResetLatch(MyLatch);

		/* emergency bailout if postmaster has died */
		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);

		/*
		 * In case of a SIGHUP, just reload the configuration.
		 */
		if (got_sighup)
		{
			got_sighup = false;
			//ProcessConfigFile(PGC_SIGHUP);
		}
	} // while got_sigterm


	elog(LOG,"acce_master_worker exit");
}

/*
static void
acce_setup_master_worker(void)
{
	BackgroundWorker worker;
	if(acce_master_worker_setup_counter>0)
	{
		elog(LOG,"acce_master_worker_setup_counter  > 0");
		return;
	}
        acce_master_worker_setup_counter++;
	elog(LOG,"acce_setup_master_worker enter");
	memset(&worker, 0, sizeof(BackgroundWorker));
	strcpy(worker.bgw_name, "acce master worker");
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_PostmasterStart;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = acce_master_worker;
	worker.bgw_main_arg = 0;
	RegisterBackgroundWorker(&worker);
	elog(LOG,"acce_setup_master_worker exit");
}

*/
static void
acce_setup_master_worker_dynamic(int a)
{
	BackgroundWorker worker;
	BackgroundWorkerHandle *handle;
	BgwHandleStatus status;
	pid_t		pid;

        if(a!=0)return;

	if(acce_master_worker_setup_counter>0)
	{
		elog(LOG,"acce_master_worker_setup_counter  > 0");
		return;
	}

	acce_master_worker_setup_counter++;

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	sprintf(worker.bgw_library_name, "pg_acce");
	sprintf(worker.bgw_function_name, "acce_master_worker");
	snprintf(worker.bgw_name, BGW_MAXLEN, "acce_master_worker");
	worker.bgw_main_arg = 0;
	// set bgw_notify_pid so that we can use WaitForBackgroundWorkerStartup
	worker.bgw_notify_pid = MyProcPid;

	if (!RegisterDynamicBackgroundWorker(&worker, &handle))
		return;

	status = WaitForBackgroundWorkerStartup(handle, &pid);

	if (status == BGWH_STOPPED)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("could not start background process"),
			   errhint("More details may be available in the server log.")));
	if (status == BGWH_POSTMASTER_DIED)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
			  errmsg("cannot start background processes without postmaster"),
				 errhint("Kill all remaining database processes and restart the database.")));
	Assert(status == BGWH_STARTED);

//	PG_RETURN_INT32(pid);
}


Datum
acce_setup(PG_FUNCTION_ARGS)
{
elog(LOG, "acce_setup");

acce_setup_master_worker_dynamic(0);

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

