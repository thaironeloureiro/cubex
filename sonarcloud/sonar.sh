#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>>"
unzip -o sonar-scanner-cli-4.6.2.2472-linux.zip
echo "--- export PATH=$PATH:sonar-scanner-4.6.2.2472-linux/bin ---"
export PATH=$PATH:sonar-scanner-4.6.2.2472-linux/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -Dsonar.verbose=true -X
