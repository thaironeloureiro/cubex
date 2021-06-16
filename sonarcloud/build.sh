#!/bin/bash
arduino-cli compile --build-path $PWD/build --fqbn arduino:avr:mega --verbose sniffer_cube/sniffer_cube --output src/software/firmware/output/sniffer_cube
