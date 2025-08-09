#!/bin/bash

sudo cp ./cmake-build/sources/assets/callback/libassets.dylib /usr/local/lib/hakoniwa/libassets.dylib
sudo cp ./cmake-build/sources/conductor/libconductor.dylib /usr/local/lib/hakoniwa/libconductor.dylib
sudo cp ./cmake-build/sources/assets/polling/libshakoc.dylib /usr/local/lib/hakoniwa/libshakoc.dylib 
sudo cp ./cmake-build/sources/assets/bindings/python/libhako_asset_python.dylib /usr/local/lib/hakoniwa/py/hakopy.so
sudo cp ./cmake-build/sources/command/hako-cmd /usr/local/bin/hakoniwa/hako-cmd
sudo cp sources/assets/bindings/python/src/*.py /usr/local/lib/hakoniwa/py/
sudo cp -rp sources/assets/bindings/python/src/hako_binary /usr/local/lib/hakoniwa/py/

# for temporary use
#DEST_UNITY_DIR=../../oss/hakoniwa-unity-drone/simulation/Library/PackageCache/com.hakoniwa-lab.hakoniwa-sim@f3739a3363aa/Plugins/arm64/
#cp /usr/local/lib/hakoniwa/libconductor.dylib ${DEST_UNITY_DIR}
#cp /usr/local/lib/hakoniwa/libshakoc.dylib ${DEST_UNITY_DIR}