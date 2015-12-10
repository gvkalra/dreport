#!/bin/bash

echo "Current Directory: "$(pwd)
echo ""
echo "PATH: "$PATH
echo ""
echo "Building.."
ninja -C ./build all
echo "Static Analysis.."
ninja -C ./build sa
