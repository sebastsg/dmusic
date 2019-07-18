#!/bin/bash
sftp $1@$2 <<EOF
lcd "$3"
cd files
get -r "$4"
quit
EOF
