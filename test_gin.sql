\timing

CREATE TABLE t1 (
    f1 text,
    f2 text
);

INSERT INTO t1 SELECT md5(random()::text), md5(random()::text) FROM
          (SELECT * FROM generate_series(1,1000000) AS id) AS x;


SELECT count(*) FROM t1 where f1 ilike '%aeb%';

SELECT count(*) FROM t1 where f1 ilike '%aeb%' or f2 ilike'%aeb%';

CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE INDEX users_search_idx ON t1 USING gin (f1 gin_trgm_ops, f2 gin_trgm_ops);
