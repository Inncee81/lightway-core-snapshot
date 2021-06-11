#!/bin/bash -e

# Build Platform specific Frameworks
# iOS Device
xcodebuild archive \
    -scheme Lightway \
    -archivePath "./build/ios.xcarchive" \
    -sdk iphoneos \
    SKIP_INSTALL=NO CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO PLATFORM=universal

# iOS Sim
xcodebuild archive \
    -scheme Lightway \
    -archivePath "./build/ios_sim.xcarchive" \
    -sdk iphonesimulator \
    SKIP_INSTALL=NO CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO PLATFORM=universal

# macOS
xcodebuild archive \
    -scheme Lightway \
    -archivePath "./build/macos.xcarchive" \
    -sdk macosx \
    SKIP_INSTALL=NO CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO PLATFORM=macos

# Package XC Framework
xcodebuild -create-xcframework \
    -framework "./Build/ios.xcarchive/Products/Library/Frameworks/Lightway.framework" \
    -framework "./Build/ios_sim.xcarchive/Products/Library/Frameworks/Lightway.framework" \
    -framework "./Build/macos.xcarchive/Products/Library/Frameworks/Lightway.framework" \
    -output "./Build/Lightway.xcframework"
