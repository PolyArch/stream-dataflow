#!/bin/bash

args=""
#args="no run"
#args="perf"

for i in *[0-9]p *[0-9]sb; do
  echo -n "./$i $args "
  ./$i $args
#   ticks=`echo $out | cut -d: -f2`
#   echo $ticks
done
