CREATE FUNCTION acce_info()
 RETURNS void
 AS 'MODULE_PATHNAME'
 LANGUAGE C STRICT;

CREATE TYPE __acce_mem_info AS (
  zone    int4,
  size    text,
  active  int8,
  free    int8
);

CREATE FUNCTION acce_mem_info()
  RETURNS SETOF __acce_mem_info
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT;

CREATE TYPE __acce_ocl_info AS (
  id		int4,
  value		text
);

CREATE FUNCTION acce_ocl_info()
  RETURNS SETOF __acce_ocl_info
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT;


