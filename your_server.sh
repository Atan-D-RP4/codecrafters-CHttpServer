#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
echo "gcc -lcurl -lz app/*.c -o $tmpFile"
gcc -lcurl -lz app/*.c -o $tmpFile
exec "$tmpFile" "$@"
