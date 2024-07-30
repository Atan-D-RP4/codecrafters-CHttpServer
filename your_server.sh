#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
echo "Compiling..."
gcc -o app/nob app/nob.c
echo "$tmpFile"
make app/nob
./app/nob build "$tmpFile"
exec "$tmpFile" "$@"
