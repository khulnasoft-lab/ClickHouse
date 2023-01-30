-- { echoOn }
SYSTEM STOP MERGES tbl;

-- simple test case
create table if not exists replacing_mt (x String) engine=ReplacingMergeTree() ORDER BY x;

insert into replacing_mt values ('abc');
insert into replacing_mt values ('abc');

-- expected output is 2 because force_select_final is turned off
select count() from replacing_mt;

set force_select_final = 1;
-- expected output is 1 because force_select_final is turned on
select count() from replacing_mt;

-- JOIN test cases
create table if not exists lhs (x String) engine=ReplacingMergeTree() ORDER BY x;
create table if not exists rhs (x String) engine=ReplacingMergeTree() ORDER BY x;

insert into lhs values ('abc');
insert into lhs values ('abc');

insert into rhs values ('abc');
insert into rhs values ('abc');

set force_select_final = 0;
-- expected output is 4 because select_final == 0
select count() from lhs inner join rhs on lhs.x = rhs.x;

set force_select_final = 1;
-- expected output is 1 because force_select_final == 1
select count() from lhs inner join rhs on lhs.x = rhs.x;

-- regular non final table
set force_select_final = 1;
create table if not exists regular_mt_table (x String) engine=MergeTree() ORDER BY x;
insert into regular_mt_table values ('abc');
insert into regular_mt_table values ('abc');
-- expected output is 1, it should silently ignore final modifier
select count() from regular_mt_table;

-- view test
create materialized VIEW mv_regular_mt_table TO regular_mt_table AS SELECT * FROM regular_mt_table;
create view nv_regular_mt_table AS SELECT * FROM mv_regular_mt_table;

set force_select_final=1;
select count() from nv_regular_mt_table;

-- join on mix of tables that support / do not support select final
create table if not exists left_table (x String) engine=ReplacingMergeTree() ORDER BY x;
create table if not exists middle_table (x String) engine=MergeTree() ORDER BY x;
create table if not exists right_table (x String) engine=ReplacingMergeTree() ORDER BY x;

insert into left_table values ('abc');
insert into left_table values ('abc');
insert into left_table values ('abc');

insert into middle_table values ('abc');
insert into middle_table values ('abc');

insert into right_table values ('abc');
insert into right_table values ('abc');
insert into right_table values ('abc');

-- Expected output is 2 because middle table does not support final
select count() from left_table
    inner join middle_table on left_table.x = middle_table.x
    inner join right_table on middle_table.x = right_table.x;
