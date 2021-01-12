#!/bin/bash
# Usage: ./startservice.sh <numclients> <startport>
NUMCLIENTS=$1
STARTPORT=$2
GROUP=$3
URL=$4
test -z "${NUMCLIENTS}" && NUMCLIENTS=1
test -z "${STARTPORT}" && STARTPORT=4366
test -z "${URL}" && URL="http://oldbox.orlandoviols.com"
echo "starting ${NUMCLIENTS} services starting at port ${STARTPORT}"
hst=$(hostname)
k=1
while test $k -le ${NUMCLIENTS}; do
    let p=$k+${STARTPORT}
    let p=$p-1
    ./build/ov-server -l ${URL}  -p $p -n ${hst}${k} --group=${GROUP} &
    sleep 4
    let k=$k+1
done
