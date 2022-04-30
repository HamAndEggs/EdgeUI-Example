#!/bin/bash
function ShowHelp() {
	echo "usage: makeit.sh [options]"
	echo "  -r rebuild"
	echo "  --help This help"
	echo ""
}

# Process the params
while [ "$1" != "" ];
do
    if [ "$1" == "-r" ]; then
        REBUILD_SOMETHING="TRUE"
    elif [ "$1" == "--help" ]; then
        ShowHelp
        exit 0
    fi
    # Got to next param. shift is a shell builtin that operates on the positional parameters.
    # Each time you invoke shift,
    shift 
done

# Remove exec so if build fails, we don't run old version.
rm -f ./build/EdgeUIExample

if [ -n "$REBUILD_SOMETHING" ]; then
    echo "Cleaning folders"
    rm -drf ./build
    mkdir -p ./build
    cd ./build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    cd ..
fi

cd ./build
make -j12
