#!/usr/bin/env bash

export LD_LIBRARY_PATH=${HOME}/Applications/anaconda2/lib:./lib

./bin/testpsf ${1}
