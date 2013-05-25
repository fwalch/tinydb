#!/bin/sh

cd `dirname $0`/..
bin/query "$1" "$2" | circo -Tpng | display -

