#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-3.2.0.1227-linux.zip
echo "--- export PATH=$PATH:sonar-scanner-3.2.0.1227-linux/bin ---"
export PATH=$PATH:sonar-scanner-3.2.0.1227-linux/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -X