#!/bin/bash 
 COUNTER=0
 while [  $COUNTER -lt 10000 ]; do
	 echo The counter is $COUNTER
         let COUNTER=COUNTER+1
	 snmpget -v 2c -c public 192.168.1.56 1.3.6.1.4.1.54021.1.5.0
	 sleep 10
  done
