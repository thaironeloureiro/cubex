#!/bin/bash
arduino-cli compile -b arduino:avr:mega --build-path $PWD/build sniffer_cube/sniffer_cube
#arduino-cli compile --build-path $PWD/build --fqbn arduino:avr:mega --verbose sniffer_cube/sniffer_cube -e src/software/firmware/output/sniffer_cube
