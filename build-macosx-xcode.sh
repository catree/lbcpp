#!/bin/sh
mkdir build
cd build
mkdir macosx-xcode
cd macosx-xcode
ccmake -G"Xcode" ../..
# xcodebuild -alltargets
open LBCpp.xcodeproj
cd ../..
