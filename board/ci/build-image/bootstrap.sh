#!/bin/sh

set -e

echo "Updating distro..."
apt-get update

echo "Installing dependencies..."
apt install -y git build-essential cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib python3

echo "===================="
echo "= Install Pico SDK ="
echo "===================="

git clone https://github.com/raspberrypi/pico-sdk.git --branch master
cd pico-sdk
git submodule update --init
cd /

echo "Fininshed."
