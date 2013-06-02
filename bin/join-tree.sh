#!/bin/sh

cd `dirname $0`/..
cat joinGraph.dot | dot -Tpng | display -

