#!/bin/bash
OPENCV_VERSION='3.4.0'
if [ -n "$(ls -A opencv/build)" ];
then
    # We're using a cached version of our OpenCV build
    echo "Cache found - using that."
    cd opencv
    git init
    git remote add origin https://github.com/opencv/opencv.git
    git fetch origin --tags
    git checkout tags/${OPENCV_VERSION}
else
    # No OpenCV cache – clone and make the files
    echo "No cache found - cloning and making files."
    rm -r opencv
    git clone https://github.com/opencv/opencv.git
    cd opencv
    git fetch origin --tags
    git checkout tags/${OPENCV_VERSION}
    mkdir build
    cd build
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
          -D CMAKE_INSTALL_PREFIX=/usr/local \
          -D INSTALL_C_EXAMPLES=OFF \
          ..
    make -j8
fi