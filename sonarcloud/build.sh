#!/bin/bash
#echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
#ls $HOME/bin/arduino-cli
#ls -l $HOME/bin
#echo "build.sh"
#wget http://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz
#tar xf arduino-cli_latest_Linux_64bit.tar.gz
#mkdir -p $HOME/bin
#mv arduino-cli $HOME/bin/arduino-cli
export PATH=$PATH:$HOME/bin
#arduino-cli config add board_manager.additional-urls https://downloads.arduino.cc/packages/package_staging_index.json
#arduino-cli config set library.enable_unsafe_install true
#arduino-cli config dump
#arduino-cli core update-index
#arduino-cli core install arduino:megaavr
#arduino-cli lib install Servo
#arduino-cli lib install --zip-path HMC5883L.zip
#arduino-cli lib install --zip-path NewPing.zip
#mkdir -p $HOME/Arduino/libraries
#ln -s $PWD $HOME/Arduino/libraries/.

#arduino-cli compile -b arduino:avr:mega --build-path $PWD/build sniffer_cube/sniffer_cube
arduino-cli cache clean
arduino-cli compile --build-path $PWD/build -b arduino:avr:mega --verbose sniffer_cube/sniffer_cube --output-dir $PWD/output
echo "#########################"
echo "ls -l $PWD/build"
ls -l $PWD/build
#echo "ls -l $PWD/oupu"
#ls -l $PWD/output
#echo "---- cat $PWD/build/compile_commands.json "
#cat $PWD/build/compile_commands.json
#echo "----"
echo "ls sonarcloud/bo"
ls -l sonarcloud/bo
#cat sonarcloud/bo/build-wrapper-dump.json
echo "--------"
#cat sonarcloud/bo/build-wrapper.log
#echo ">>>> cat $PWD/build/compile_commands.json"
#cat $PWD/build/compile_commands.json

echo "-------- usando convert-compile-commands.py ------"
python convert-compile-commands.py $PWD/build/compile_commands.json
ls -l cfamily-compilation-database
rm sonarcloud/bo/build-wrapper-dump.json
cp cfamily-compilation-database/build-wrapper-dump.json sonarcloud/bo/build-wrapper-dump.json
echo "--------------------------------------------------"
echo "du $HOME"
ls $HOME/work/cubex
#/home/runner/work/cubex/cubex
#cat cfamily-compilation-database/build-wrapper-dump.json
#echo "ls $PWD/cfamily-compilation-database"
#echo "#########################"
