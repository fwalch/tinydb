#!/bin/sh

cd `dirname $0`/..
cat queryGraph.dot | circo -Tpng | display -

