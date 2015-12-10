#!/bin/bash

echo "Current Directory: "$(pwd)
echo ""
echo "PATH: "$PATH
echo ""
echo "Now Building.."
ninja -C ./build all
