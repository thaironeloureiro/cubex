#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>"
unzip -o sonar-scanner-cli-4.0.0.1744-linux
echo "--- export PATH=$PATH:sonar-scanner-4.0.0.1744-linux/bin ---"
export PATH=$PATH:sonar-scanner-4.0.0.1744-linux/bin
echo "------> sonar-scanner ..."
sonar-scanner -Dproject.settings=sonar-project.properties
