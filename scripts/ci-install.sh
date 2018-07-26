#!/bin/bash
set -e

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	if [[ $PYVER == '2' ]]; then
		formula="python${PYVER}"
	else
		formula="python"
	fi
	# Update Python if already installed, otherwise install.
	echo "Installing / updating ${formula} ..."
	if [[ $PYVER == '3' ]]; then
		brew list "${formula}" &>/dev/null && brew upgrade "${formula}"
	fi
	brew list "${formula}" &>/dev/null || brew install "${formula}"
	echo -n "Using: "
	"python$PYVER" --version
fi

echo "Downloading ux.stackexchange.com.7z ..."
mkdir -p $HOME/.downloads
( cd $HOME/.downloads && wget --no-check-certificate --timestamping https://archive.org/download/stackexchange/ux.stackexchange.com.7z )
cp $HOME/.downloads/ux.stackexchange.com.7z tests/data/ux.stackexchange.com.7z
