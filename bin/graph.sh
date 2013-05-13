#!/bin/sh

cd `dirname $0`/..
bin/graph "$1" | circo -Tpng | display -

