#!/bin/bash

tmp=$(mktemp)
readelf -s ./sdleditor | gawk '
{ 
  if($4 == "FUNC" && $2 != 0) { 
    print "# code for " $NF; 
    print "b *0x" $2; 
    print "commands"; 
    print "silent"; 
    print "bt 1"; 
    print "c"; 
    print "end"; 
    print ""; 
  } 
}' > $tmp; 
gdb --command=$tmp ./sdleditor; 
rm -f $tmp
