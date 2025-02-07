#!/bin/sh

BUILD_DIR="build"
DIST_DIR="dist"
GCC="/usr/bin/arm-none-eabi-gcc"
GPP="/usr/bin/arm-none-eabi-g++"

# Force the elf and uf2 binary files to always be regenerated on build
# (this is so old uf2 files don't pile up in dist directory)
rm ${BUILD_DIR}/src/*/*/*.elf
rm ${BUILD_DIR}/src/*/*/*.uf2

/usr/bin/cmake \
    --no-warn-unused-cli \
    -DCMAKE_MESSAGE_LOG_LEVEL=DEBUG \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DCMAKE_C_COMPILER:FILEPATH=${GCC} \
    -DCMAKE_CXX_COMPILER:FILEPATH=${GPP} \
    -DDREAMCAST_CONTROLLER_USB_PICO_TEST:BOOL=FALSE \
    -DPICO_BOARD=pico2 \
    -S. \
    -B./${BUILD_DIR} \
    -G "Unix Makefiles" \

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "CMake returned error exit code: ${STATUS}"
    echo "Exiting"
    exit $STATUS
fi

/usr/bin/cmake \
    --build ${BUILD_DIR} \
    --config Debug \
    --target all \
    -j 10 \

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "CMake returned error exit code: ${STATUS}"
    echo "Exiting"
    exit $STATUS
fi

mkdir -p ${DIST_DIR}
rm -rf ${DIST_DIR}/*
cp ${BUILD_DIR}/src/*/*/*.uf2 ${DIST_DIR}
cp ${BUILD_DIR}/src/*/*/*.elf ${DIST_DIR}