#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
echo "$tmpFile"
echo "$@"
gcc -fPIC -shared -o ./app/libserver.so ./app/server.c ./app/plug.c -Wall -Wextra -pedantic -O3 -ggdb -lz -lpthread
gcc -o $tmpFile ./app/main.c ./app/libserver.so -Wall -Wextra -pedantic -O3 -ggdb
exec "$tmpFile" "$@"
