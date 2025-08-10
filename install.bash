#!/bin/bash
OS_TYPE="posix"
OS=`uname`
INSTALL_DIR=""
HAKONIWA_DIR=""
if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	SUDO=
	which sudo > /dev/null
	if [ $? -eq 0 ]
	then
		SUDO=sudo
	fi
	INSTALL_DIR="/usr/local"
	HAKONIWA_DIR="/etc/hakoniwa"
else
    OS_TYPE="win"
	SUDO=
	INSTALL_DIR="../local"
	HAKONIWA_DIR="../hakoniwa"
fi

${SUDO} mkdir -p ${INSTALL_DIR}/bin/hakoniwa
${SUDO} mkdir -p ${INSTALL_DIR}/lib/hakoniwa
${SUDO} mkdir -p ${INSTALL_DIR}/include/hakoniwa
${SUDO} mkdir -p ${HAKONIWA_DIR}

if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	${SUDO} mkdir -p /var/lib/hakoniwa/mmap
fi

${SUDO} cp hakoniwa-core-cpp/cpp_core_config.json ${HAKONIWA_DIR}

if [ "$OS" = "Darwin"  ]
then
    ${SUDO} cp ./cmake-build/sources/assets/callback/libassets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/libconductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/libhako_asset_python.* ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.so
	${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.dylib ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd ${INSTALL_DIR}/bin/hakoniwa/
    ${SUDO} cp sources/assets/bindings/python/src/*.py ${INSTALL_DIR}/lib/hakoniwa/py/
    ${SUDO} cp -rp sources/assets/bindings/python/src/hako_binary ${INSTALL_DIR}/lib/hakoniwa/py/
elif [ "$OS" = "Linux"  ]
then
    ${SUDO} cp ./cmake-build/sources/assets/callback/libassets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/libconductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/libhako_asset_python.* ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.so
	${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.so ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd ${INSTALL_DIR}/bin/hakoniwa/
    ${SUDO} cp sources/assets/bindings/python/src/*.py ${INSTALL_DIR}/lib/hakoniwa/py/
    ${SUDO} cp -rp sources/assets/bindings/python/src/hako_binary ${INSTALL_DIR}/lib/hakoniwa/py/
else
    ${SUDO} cp ./cmake-build/sources/assets/callback/assets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/conductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/shakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/hako_asset_python.* ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.pyd
	${SUDO} cp ./cmake-build/sources/assets/polling/shakoc.so ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd.exe ${INSTALL_DIR}/bin/hakoniwa/
    ${SUDO} cp sources/assets/bindings/python/src/*.py ${INSTALL_DIR}/lib/hakoniwa/py/
    ${SUDO} cp -rp sources/assets/bindings/python/src/hako_binary ${INSTALL_DIR}/lib/hakoniwa/py/
fi

${SUDO} cp include/* ${INSTALL_DIR}/include/hakoniwa/

if [ -z $USER ]
then
	:
else
	# ディレクトリの所有者をインストールユーザーに変更
	${SUDO} chown -R $USER /var/lib/hakoniwa
fi

# ディレクトリのパーミッションを適切に設定
if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	${SUDO} chmod -R 755 /var/lib/hakoniwa
fi


# for temporary use
#DEST_UNITY_DIR=../../oss/hakoniwa-unity-drone/simulation/Library/PackageCache/com.hakoniwa-lab.hakoniwa-sim@f3739a3363aa/Plugins/arm64/
#cp /usr/local/lib/hakoniwa/libconductor.dylib ${DEST_UNITY_DIR}
#cp /usr/local/lib/hakoniwa/libshakoc.dylib ${DEST_UNITY_DIR}