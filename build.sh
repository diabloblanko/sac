#!/bin/sh
username=$(whoami)
if [ "$username" = "root"]; then
    echo "build.sh started"
else
    echo "ERR: Restart this script as root."
    exit 1
fi
echo "Compiling sac-make.."
clang ./sac-make.c -o ./sac-make
echo "Compiling sac-unpack.."
clang ./sac-unpack.c -o ./sac-unpack
echo "Installing.."
cp ./sac-unpack ./sac-make /usr/bin
echo "SAC installed succsesfully."
exit 0