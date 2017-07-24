#!/bin/bash -e

usage() {
	echo 'usage: ./run_setup.sh path_to_cd_rom' 1>&2
	exit 1
}

if [[ $# < 1 ]]; then
	usage
fi

dir="$PWD"

make setup
cd "$1"
"$dir"/setup
