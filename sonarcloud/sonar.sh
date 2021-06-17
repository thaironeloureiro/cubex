#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-3.3.0.1492-linux_new_java
echo "--- export PATH=$PATH:sonar-scanner-cli-3.3.0.1492-linux_new_java/bin ---"
export PATH=$PATH:sonar-scanner-cli-3.3.0.1492-linux_new_java/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties -X