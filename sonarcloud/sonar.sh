#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-3.4.0.1729-linux_new_java.zip
echo "--- export PATH=$PATH:sonar-scanner-cli-3.4.0.1729-linux_new_java/bin ---"
export PATH=$PATH:sonar-scanner-cli-3.4.0.1729-linux_new_java/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -X