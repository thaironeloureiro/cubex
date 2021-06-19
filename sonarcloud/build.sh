#!/bin/bash
#echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
export PATH=$PATH:$HOME/bin
#arduino-cli compile -b arduino:avr:mega --build-path $PWD/build sniffer_cube/sniffer_cube
#linha interessante para editar a versão do 
#sed -Ei 's/#define HARDWARE_VERSION [0-9]+/#define HARDWARE_VERSION 2/' src/software/firmware/includes/config.h
#sed -Ei 's/#define MODE .+/#define MODE MODE_PROD/' src/software/firmware/includes/config.h

arduino-cli compile --build-path $PWD/build -b arduino:avr:mega --verbose sniffer_cube  --clean

#echo "######################### ls"
#echo "ls -l $PWD/build"
#ls -l $PWD/build
#echo "ls -l $PWD/ouput"
#ls -l $PWD/output
#echo "---- cat $PWD/build/compile_commands.json "
#cat $PWD/build/compile_commands.json

#echo "----"
#echo "ls sonarcloud/bo"
#ls -l sonarcloud/bo
#cat sonarcloud/bo/build-wrapper-dump.json
#echo "--------"
#cat sonarcloud/bo/build-wrapper.log
#echo ">>>> cat $PWD/build/compile_commands.json"
#cat $PWD/build/compile_commands.json


#echo "ls -l $HOME/work/cubex/cubex"
#ls -l $HOME/work/cubex/cubex
#echo "du $HOME/work/cubex"
#du $HOME/work/cubex
#/home/runner/work/cubex/cubex
#cat cfamily-compilation-database/build-wrapper-dump.json
#echo "ls $PWD/cfamily-compilation-database"
#echo "#########################"
