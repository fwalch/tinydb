#!/bin/sh

cd `dirname $0`/..

bin/query data/tpch/tpch "explain * from lineitem l, orders o, customer c where l.l_orderkey=o.o_orderkey and o.o_custkey=c.c_custkey and c.c_name='Customer#000014993'"

