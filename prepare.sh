#!/bin/sh -ev
cp -r /bin/ls ras/bin
cp -r /bin/cat ras/bin
g++ commands/src/noop.cpp -o ras/bin/noop
g++ commands/src/number.cpp -o ras/bin/number
g++ commands/src/removetag.cpp -o ras/bin/removetag
g++ commands/src/removetag0.cpp -o ras/bin/removetag0
gcc client.c -o client
