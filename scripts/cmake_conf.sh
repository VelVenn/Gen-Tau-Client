#! /bin/bash

SRC_DIR=.
BUILD_DIR="build"
GENERATOR="Ninja"
BUILD_TYPE="Debug"

DO_BUILD=0
JOBS=""

DO_CLEAN=0

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -S|--src-dir)   SRC_DIR="$2";    shift ;;
        -B|--build-dir) BUILD_DIR="$2";  shift ;; # 移走 "-B"
        -G|--generator) GENERATOR="$2";  shift ;; # 移走 "-G"
        -t|--type)      BUILD_TYPE="$2"; shift ;;
        -j|--jobs)      JOBS="$2";       shift ;;

        -b|--build)            DO_BUILD=1 ;;       
        -C|--clean-build)      DO_CLEAN=1 ;;
        
        -h|--help)      
            echo "Usage: $0 [options]"
            echo ""
            echo "Configuration Options:"
            echo "  -s, --src-dir DIR      Set source directory (default: .)"
            echo "  -b, --build-dir DIR    Set build directory (default: build)"
            echo "  -g, --generator NAME   Set CMake generator (default: Ninja)"
            echo "  -t, --type TYPE        Set build type (default: Debug)"
            echo ""
            echo "Build Actions:"
            echo "  -b, --build            Run compilation after configuration"
            echo "  -j, --jobs N           Specifies the number of jobs to run simultaneously"
            echo "  -C, --clean-build      Remove build directory before configure"
            echo ""
            echo "General:"
            echo "  -h, --help             Show this help message"
            exit 0 
            ;;

        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift # 移走参数的值
done

# Do clean at here
if [[ $DO_CLEAN -eq 1 ]]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Warning: Cleaning build directory '$BUILD_DIR'..."
        rm -rf "$BUILD_DIR"
    fi
fi

echo "========================================"
echo "Source Dir : $SRC_DIR"
echo "Build Dir  : $BUILD_DIR"
echo "Generator  : $GENERATOR"
echo "Build Type : $BUILD_TYPE"
if [[ -n "$JOBS" ]]; then
    echo "Build Jobs : $JOBS"
fi
echo "========================================"

cmake -S "$SRC_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_DIR" -B "$BUILD_DIR"

if [ $? -ne 0 ]; then
    echo "Error: CMake Configuration failed."
    exit 1
fi

if [[ $DO_BUILD -eq 1 ]]; then
    echo "========================================"
    echo ">>> Starting Build..."
    echo "========================================"
    
    if [[ -n "$JOBS" ]]; then
        cmake --build "$BUILD_DIR" -j "$JOBS"
    else
        cmake --build "$BUILD_DIR"
    fi
fi