#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-3.4.0.1729-linux.zip
echo "--- export PATH=$PATH:sonar-scanner-3.4.0.1729-linux/bin ---"
export PATH=$PATH:sonar-scanner-3.4.0.1729-linux/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -X