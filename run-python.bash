#!/bin/bash

export PYTHONPATH=.:sources/assets/bindings/python/src:./libpy
export DYLD_LIBRARY_PATH=./cmake-build/sources/assets/polling:./cmake-build/sources/assets/callback

python $*