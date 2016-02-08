#! /bin/sh

PATH=$1
shift
COOKIE=$1
shift
ARGS=$*

echo "Events for \"$PATH\" ($COOKIE):"

while [ $# -ne 0 ]; do
	echo -e "\t $1"
	shift
done