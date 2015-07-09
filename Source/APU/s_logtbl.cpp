#include <afx.h>
#include <memory>
#include "../Common.h"
#include "s_logtbl.h"

#if STATIC_TABLES

static void LogTableRelease(void *ctx)
{
}

static KMIF_LOGTABLE log_static_tables = {
	&log_static_tables;
	LogTableRelease,
//#include "s_logt.h"
};


KMIF_LOGTABLE *LogTableAddRef(void)
{
	log_static_tables.release = LogTableRelease;
	return &log_static_tables;
}

#else

#include <math.h>

static volatile uint32 log_tables_mutex = 0;
static uint32 log_tables_refcount = 0;
static KMIF_LOGTABLE *log_tables = 0;

static void LogTableRelease(void *ctx)
{
	++log_tables_mutex;
	while (log_tables_mutex != 1)
	{
		_sleep(0);
	}
	log_tables_refcount--;
	if (!log_tables_refcount)
	{
		free(ctx);
		log_tables = 0;
	}
	--log_tables_mutex;
}

void LogTableCalc(KMIF_LOGTABLE *kmif_lt)
{
	uint32 i;
	double a;
	for (i = 0; i < (1 << LOG_BITS); i++)
	{
		a = (1 << LOG_LIN_BITS) / pow(2, i / (double)(1 << LOG_BITS));
		kmif_lt->logtbl[i] = (uint32)a;
	}
	kmif_lt->lineartbl[0] = LOG_LIN_BITS << LOG_BITS;
	for (i = 1; i < (1 << LIN_BITS) + 1; i++)
	{
		uint32 ua;
		a = i << (LOG_LIN_BITS - LIN_BITS);
		ua = (uint32)((LOG_LIN_BITS - (log(a) / log(2.0))) * (1 << LOG_BITS));
		kmif_lt->lineartbl[i] = ua << 1;
	}
}

KMIF_LOGTABLE *LogTableAddRef(void)
{
	++log_tables_mutex;
	while (log_tables_mutex != 1)
	{
		_sleep(0);
	}
	if (!log_tables_refcount)
	{
		log_tables = static_cast<KMIF_LOGTABLE*>(malloc(sizeof(KMIF_LOGTABLE)));
		if (log_tables)
		{
			memset(log_tables, 0, sizeof(KMIF_LOGTABLE));
			log_tables->ctx = log_tables;
			log_tables->release = LogTableRelease;
			LogTableCalc(log_tables);
		}
	}
	if (log_tables) log_tables_refcount++;
	--log_tables_mutex;
	return log_tables;
}

#endif
