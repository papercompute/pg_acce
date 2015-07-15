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
#include "miscadmin.h"
#include "storage/procsignal.h"
#include "storage/shm_toc.h"

#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/planner.h"


#include <limits.h>
#include <unistd.h>
#include "pg_acce.h"

#define MAX_WORKERS 128

PG_MODULE_MAGIC;

static int	num_workers;

//static planner_hook_type	planner_hook_next;
//static bool _enabled = true;
#define		PG_TEST_SHM_MQ_MAGIC		0xa89bc209


#define ACCE_LOG(...) \
	fprintf(stderr, __VA_ARGS__); \
	fflush(stderr);


void _PG_finit(void);
void _PG_init(void);

void
acce_worker(Datum args);


// Task uname dbname msg

typedef struct WorkerData {
	int id;
	int uname_sz;
	int dbname_sz;
	int msg_sz;
} WorkerData;




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



static void
handle_sigterm(SIGNAL_ARGS)
{
	int save_errno = errno;

	SetLatch(MyLatch);

	if (!proc_exit_inprogress)
	{
		InterruptPending = true;
		ProcDiePending = true;
	}

	errno = save_errno;
}






void
acce_worker(Datum args)
{
//	Size		len;
//	void	   *data;
//	shm_mq_result res;
///	static Latch signalLatch;

	uint32 seg_arg = DatumGetUInt32( args );
	WorkerData	*wd;
	char* uname;
	char* dbname;
	char* msg;
	int j;

	dsm_segment *segment=NULL;
	ResourceOwner oldowner;

	oldowner = CurrentResourceOwner;
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pg_acce");

	segment = dsm_attach( seg_arg );
	if (segment == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                 errmsg("Could not attach to DSM")));

	CurrentResourceOwner = oldowner;


	wd = (WorkerData*) palloc ( sizeof(WorkerData) );
	j=sizeof(WorkerData);
	memcpy(wd, dsm_segment_address(segment), j);

	uname = (char*) palloc(sizeof(char)*wd->uname_sz);
	dbname = (char*) palloc(sizeof(char)*wd->dbname_sz);
	msg = (char*) palloc(sizeof(char)*wd->msg_sz);

	memcpy(uname, 	(char*)dsm_segment_address(segment)+j,wd->uname_sz);
	j+=wd->uname_sz;
	memcpy(dbname,  (char*)dsm_segment_address(segment)+j,wd->dbname_sz);
	j+=wd->dbname_sz;
	memcpy(msg,  	(char*)dsm_segment_address(segment)+j,wd->msg_sz);

	elog(LOG,"acce_worker %d enter (%s,%s):%s",wd->id, uname, dbname,msg);

	pqsignal(SIGTERM, handle_sigterm);
	BackgroundWorkerUnblockSignals();

//	while (1)
//	{
//		CHECK_FOR_INTERRUPTS();
//
//	}
	pg_usleep(100000L); // 100 msec


	elog(LOG,"acce_worker %d exit",wd->id);
	pfree(wd);
	pfree(uname);
	pfree(dbname);
	pfree(msg);
}

static pid_t
acce_add_worker_dynamic(char* msg)
{
	BackgroundWorker worker;
	BackgroundWorkerHandle *handle;
	BgwHandleStatus status;
	WorkerData	wd;
	ResourceOwner oldowner;
	dsm_segment *segment=NULL;
	pid_t		pid;
	int j;
	char* uname;
	char* dbname;


	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = NULL;
	sprintf(worker.bgw_library_name, "pg_acce");
	sprintf(worker.bgw_function_name, "acce_worker");
	worker.bgw_notify_pid = MyProcPid;

	uname = GetUserNameFromId( GetUserId(),true  );
	dbname = DatumGetCString(current_database(NULL)) ;

	wd.id	= num_workers++;

	wd.uname_sz	= strlen(uname)+1;
	wd.dbname_sz	= strlen(dbname)+1;
	wd.msg_sz	= strlen(msg)+1;

	oldowner = CurrentResourceOwner;
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "pg_acce");

	segment = dsm_create(sizeof(WorkerData)+wd.uname_sz +wd.dbname_sz +wd.msg_sz, 0);

	memcpy( dsm_segment_address(segment), &wd,  sizeof(WorkerData));
	j=sizeof(WorkerData);
	memcpy( (char*)dsm_segment_address(segment)+j, uname,  wd.uname_sz);	
	j+=wd.uname_sz;
	memcpy( (char*)dsm_segment_address(segment)+j, dbname,  wd.dbname_sz);	
	j+=wd.dbname_sz;
	memcpy( (char*)dsm_segment_address(segment)+j, msg,  wd.msg_sz);	

	CurrentResourceOwner = oldowner;

	snprintf(worker.bgw_name, BGW_MAXLEN, "acce_worker %d", wd.id);
	worker.bgw_main_arg = UInt32GetDatum(dsm_segment_handle(segment));

	if (!RegisterDynamicBackgroundWorker(&worker, &handle))
		return 0;

	status = WaitForBackgroundWorkerStartup(handle, &pid);
	if(status != BGWH_STARTED){
		ACCE_LOG("Error: WaitForBackgroundWorkerStartup status(%d)!=BGWH_STARTED\n",status);
		proc_exit(1);
	}

	return pid;
	
}

Datum
acce_setup(PG_FUNCTION_ARGS)
{
	int i=0;
	int32		nworkers = PG_GETARG_INT32(0);
	if(nworkers<=0 || nworkers>128)
		PG_RETURN_VOID();

	ACCE_LOG("acce_setup %d\n",nworkers);

	//acce_add_more_workers_dynamic(nworkers);
	//acce_add_worker_dynamic("worker1",128,acce_worker);
	//setup_dynamic_shared_memory(128);
	for(i=0;i<nworkers;i++)
	{
		acce_add_worker_dynamic("hello");
	}

	//num_workers++;

	PG_RETURN_VOID();
}
PG_FUNCTION_INFO_V1(acce_setup);




void
_PG_finit(void)
{
	ACCE_LOG("acce finit\n");
}

void
_PG_init(void)
{

 	if (!process_shared_preload_libraries_in_progress)
		return;

// 	if (!process_shared_preload_libraries_in_progress){
//		ereport(ERROR,
//		(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
//		errmsg("not process_shared_preload_libraries_in_progress"))); 
//		return;
//	}

	ACCE_LOG("acce init\n");

	
//	planner_hook_next = planner_hook;
//	planner_hook = acce_planner;


	ACCE_LOG("acce init done\n");
}






