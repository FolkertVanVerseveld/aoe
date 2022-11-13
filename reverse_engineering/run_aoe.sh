#!/bin/bash -e

usage() {
	echo 'usage: ./run_aoe.sh path_to_cd_rom' 1>&2
	exit 1
}

if [[ $# < 1 ]]; then
	usage
fi

dir="$PWD"

make aoe
cd "$1"
wine "$dir"/aoe
