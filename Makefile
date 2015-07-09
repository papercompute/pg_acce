
EXTENSION = pg_acce
DATA = pg_acce--1.0.sql

PGFILEDESC = "Postgres Accelerated Engine"
MODULE_big = pg_acce

OBJS = pg_acce.o

REGRESS = pg_acce

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)



