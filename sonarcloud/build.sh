#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
ls $HOME/bin/arduino-cli
echo "build.sh"
#wget http://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz
#tar xf arduino-cli_latest_Linux_64bit.tar.gz
#mkdir -p $HOME/bin
#mv arduino-cli $HOME/bin/arduino-cli
#export PATH=$PATH:$HOME/bin
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
pwd
arduino-cli compile -b arduino:avr:mega --build-path $PWD/build sniffer_cube/sniffer_cube
#arduino-cli compile --build-path $PWD/build --fqbn arduino:avr:mega --verbose sniffer_cube/sniffer_cube -e src/software/firmware/output/sniffer_cube
echo "#########################"
echo "ls $PWD/build"
ls $PWD/build
echo "----"
#cat $PWD/build/compile_commands.json
echo "----"
echo "ls sonarcloud/bo"
#ls sonarcloud/bo
#cat sonarcloud/bo/build-wrapper-dump.json
#echo "--------"
#cat sonarcloud/bo/build-wrapper.log
echo "--------"
python convert-compile-commands.py $PWD/build/compile_commands.json
ls cfamily-compilation-database
#cat cfamily-compilation-database/build-wrapper-dump.json
#echo "ls $PWD/cfamily-compilation-database"
#cat /home/runner/work/cubex/cubex/cfamily-compilation-database/build-wrapper-dump.json
echo "#########################"
