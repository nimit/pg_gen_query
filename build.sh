#!/bin/bash

cd ai-sdk-cpp
./build.sh
cd ..

make clean
make
sudo make install