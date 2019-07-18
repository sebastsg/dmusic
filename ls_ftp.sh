#!/bin/bash
sftp $1@$2 <<EOF
cd files
ls -t
quit
EOF
