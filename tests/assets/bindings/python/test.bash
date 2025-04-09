#!/bin/bash


export PYTHONPATH=sources/assets/bindings/python/src/:/usr/local/lib/hakoniwa/py/pro
export DYLD_LIBRARY_PATH=./cmake-build/sources/conductor:/Users/tmori/project/private/hakoniwa-core-pro/cmake-build/sources/assets/callback
python tests/assets/bindings/python/test.py &
pid=$!
sleep 1
hako-cmd start
wait $pid
