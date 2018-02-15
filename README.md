# MQTTClient

In this project, we have a NodeMCU firmware which is connected to a DHT11 sensor to constantly read temperature and humidity values of the room. On start, NodeMCU will connect to the local wifi, then search for a Gateway nearby, in this case a NUC android device, then it will find the IP/Name of that device and establishes MQTT connection. This device is registered and connected to our IoT-Ignite cloud platform, it receives datas sent by the NodeMCU and sends them to the cloud system.

## System Setup

* First,  to use the cloud platform you need to create an acoount. Go to this link https://enterprise.iot-ignite.com/v3/access/login to create your own account, and login.

* Then you need to register your gateway to the cloud system, to do that, we start by installing Iot-Ignite agent application. You can download and install latest IoT-Ignite Agent app from Google Play Store https://play.google.com/store/apps/details?id=com.ardic.android.iotigniteagent .

Then register your device from Administration->RegisterGateway.

*	Uploading and signing IoTGate-MQTT app. You can upload and make your app trusted from Application->App Store->Categories->Add Application, don’t forget to select “Trusted App” checkbox.

* You have uploaded your app. Now it’s time to create a mode and push it with the app’s certificate. You can create a new mode from Gateway Modes->Mode Store->Add Mode, or you can use default mode from Gateway Modes->Default Mode and add your appication’s certificate from Applications->Add Aplication section.

To send the mode to your gateway, go to Gateways->All gateways, and select your device, then add to working set by clicking “Add To Working Set”, click on “Modes”, choose your mode, and send it by Change Mode->yes->Send Immidiately.

Your gateway is connected and registered to the Iot-Ignite, and ready to use.

##Setup Nodes and Sensors

It’s time to setup Nodes and Sensors. In this case we use NodeMCU and DHT11 temperature&humidity sensor. Connect them on a breadboard, GND to GND, VCC to 5V, and Data pin to D0 pin of the NodeMCU.

The only thing left is to program our NodeMCU to get it to do what we need. NodeMCU firmware can be programmed using Arduino IDE, you can download and install it from https://www.arduino.cc/en/Main/Software . To be able to programm this frimware on Arduino IDE “esp8266 package” is needed to be installed, to do that, go to Files->Preferences and copy paste this link “ http://arduino.esp8266.com/stable/package_esp8266com_index.json “ to “Additional Boards Manager URLs” text box, and click “OK”. Then go to Tools->Boards->Boards Manager, search for “esp8266” and install it. After installation is done go back to Tools->Boards, select NodeMCU 1.0(ESP-1.2E Module) from the bords list.

There are some additional general libraries needed to be installed, go to Scketch->Include Library->Manage Library, search for and install libraries such as ESP8266WiFi, ESP8266mDNS, WiFiUdp, FS and DHT seperately.

That’s it, your NodeMCU frimware is ready to be programmed. 

