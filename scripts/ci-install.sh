#!/bin/bash
set -e

echo "Downloading ux.stackexchange.com.7z ..."
mkdir -p $HOME/.downloads
( cd $HOME/.downloads && wget --no-check-certificate --timestamping https://archive.org/download/stackexchange/ux.stackexchange.com.7z )
cp $HOME/.downloads/ux.stackexchange.com.7z tests/data/ux.stackexchange.com.7z
