#!/usr/bin/env bash

# Last version: git clone https://github.com/bblanchon/pdfium-binaries.git

cp dependencies/pdfium-binaries.tar.gz ~
tar -xzvf ~/pdfium-binaries.tar.gz -C ~
rm -rf ~/pdfium-binaries.tar.gz
cp parameters/linux.args.gn ~/pdfium-binaries/args/linux.args.gn
cp parameters/build.sh ~/pdfium-binaries
cd ~/pdfium-binaries || exit
export PDFium_BRANCH=chromium/4033 && export PDFium_V8=disabled && export CONFIGURATION=Debug && ./build.sh
