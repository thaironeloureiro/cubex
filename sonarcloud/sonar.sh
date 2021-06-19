#!/bin/bash
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>"
echo ">>>>>> unzip sonar >>>>>>>>"
unzip -o sonar-scanner-cli-4.4.0.2170-linux.zip
echo "--- export PATH=$PATH:sonar-scanner-4.4.0.2170-linux/bin ---"
export PATH=$PATH:sonar-scanner-4.4.0.2170-linux/bin
echo "------> sonar-scanner ..."
#sonar-scanner -Dproject.settings=sonar-project.properties -Dsonar.sonar.cfamily.compile-commands=build/compile_commands.json -Dsonar.verbose=true -X
sonar-scanner   -Dsonar.organization=thaironeloureiro   -Dsonar.projectKey=thaironeloureiro_cubex   -Dsonar.sources=.   -Dsonar.cfamily.build-wrapper-output=build  -Dsonar.host.url=https://sonarcloud.io
