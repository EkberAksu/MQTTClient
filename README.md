# MQTTClient

  In this project, we have a NodeMCU firmware which is connected to a DHT11 sensor to constantly read temperature and humidity values of the room. On start, NodeMCU will connect to the local wifi, then search for a Gateway nearby, in this case a NUC android device, then it will find the IP/Name of that device and establishes MQTT connection. This device is registered and connected to our IoT-Ignite cloud platform, it receives datas sent by the NodeMCU and sends them to the cloud system.
