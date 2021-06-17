#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-3.1.0.1141-linux
echo "--- export PATH=$PATH:sonar-scanner-3.1.0.1141-linux/bin ---"
export PATH=$PATH:sonar-scanner-3.1.0.1141-linux/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -X
