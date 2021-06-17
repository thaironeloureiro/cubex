#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-4.6.2.2472-linux.zip
$install_directory=sonar-scanner-4.6.2.2472-linux
#unzip -o sonar-scanner-cli-4.2.0.1873-linux.zip
#$install_directory=sonar-scanner-cli-4.2.0.1873-linux
export PATH=$PATH:$install_directory/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties
