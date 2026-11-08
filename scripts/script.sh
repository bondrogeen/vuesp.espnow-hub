#!/bin/bash

echo "Start build"
while getopts t:p:o: flag
do
    case "${flag}" in
        t) target=${OPTARG};;   # Target platform
        p) path=${OPTARG};;     # Project path
        o) outdir=${OPTARG};;   # Output directory
    esac
done
echo "target: $target"
echo "path: $path"
echo "outdir: $outdir"

if [ "$1" = "build" ]
then
    echo 'Building...'
    
    # Run PlatformIO build and filesystem build
    ~/.platformio/penv/bin/pio run && \
    ~/.platformio/penv/bin/pio run --target buildfs
    
    echo "Copying firmware files..."
    
    # Check if build directory exists
    if [ ! -d "./build" ]; then
        echo "⚠️  Build directory not found!"
        exit 1
    fi
    
    # Clean firmware directory before copying
    echo "Cleaning firmware directory..."
    if [ -d "./firmware" ]; then
        # Count files before deletion
        file_count=$(find ./firmware -type f 2>/dev/null | wc -l)
        if [ "$file_count" -gt 0 ]; then
            echo "Removing $file_count file(s) from firmware directory..."
            rm -rf ./firmware/*
            echo "✅ Firmware directory cleaned ($file_count files removed)"
        else
            echo "✅ Firmware directory already empty"
        fi
    else
        mkdir -p ./firmware
        echo "✅ Firmware directory created"
    fi
    
    # Counter for copied platforms
    copied_count=0
    
    # Find all platform directories in build that contain firmware.bin and littlefs.bin
    for platform_dir in ./build/*/; do
        # Get platform name from directory path
        platform=$(basename "$platform_dir")
        
        # Check if both firmware and littlefs files exist
        if [ -f "$platform_dir/firmware.bin" ] && [ -f "$platform_dir/littlefs.bin" ]; then
            # Create destination directory
            mkdir -p "./firmware/$platform"
            
            # Copy files with verbose output
            cp -v "$platform_dir/firmware.bin" "./firmware/$platform/"
            cp -v "$platform_dir/littlefs.bin" "./firmware/$platform/"
            
            echo "✅ $platform copied successfully"
            ((copied_count++))
        else
            echo "⚠️  $platform files not found, skipping..."
        fi
    done
    
    echo "========================================="
    echo "Build complete!"
    echo "✅ Copied $copied_count platform(s)"
    
    if [ "$copied_count" -gt 0 ]; then
        echo "Copied platforms:"
        ls -1 ./firmware/ 2>/dev/null | while read -r dir; do
            echo "  - $dir"
        done
    else
        echo "⚠️  No platforms were copied!"
        echo "Available directories in ./build:"
        ls -la ./build/ 2>/dev/null || echo "  ./build is empty or not accessible"
    fi
fi