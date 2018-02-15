#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "dht.h"
#include <stdio.h>
#include <time.h>
#include <ESP8266mDNS.h>
#include "mdns.h"
#include <stdlib.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "ARDIC_GUEST"
#define WLAN_PASS       "w1Cm2ardC"

/************************* Adafruit.io Setup *********************************/

char AIO_SERVER[100] =  {0};
//#define AIO_SERVER      "192.168.2.54"
#define AIO_SERVERPORT  1883        
#define AIO_USERNAME    "ardic.smart.office"
#define AIO_CLIENTID    "ardic.smart.office"

#define AIO_KEY         "ardic12345"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_CLIENTID, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

#define NODE_ID "DHT11NODE"

#define PUBLISH_TEMPERATURE_DATA   "things/" NODE_ID "/Temperature/data"
#define PUBLISH_HUMIDITY_DATA   "things/" NODE_ID "/Humidity/data"

#define SUBSCRIBE_HUMIDITY_DATA   "things/" NODE_ID "/testthing/config"

Adafruit_MQTT_Publish publishHumidity = Adafruit_MQTT_Publish(&mqtt, PUBLISH_HUMIDITY_DATA, MQTT_QOS_1);
Adafruit_MQTT_Publish publishTemperature = Adafruit_MQTT_Publish(&mqtt, PUBLISH_TEMPERATURE_DATA, MQTT_QOS_1);

Adafruit_MQTT_Subscribe configData = Adafruit_MQTT_Subscribe(&mqtt, SUBSCRIBE_HUMIDITY_DATA, MQTT_QOS_1);

/****************************************************************************/

#define QUESTION_SERVICE "_iotgate._tcp.local"

// Make this value as large as available ram allows.
#define MAX_MDNS_PACKET_SIZE 1024

#define MAX_HOSTS 1
#define HOSTS_SERVICE_NAME 0
#define HOSTS_PORT 1
#define HOSTS_HOST_NAME 2
#define HOSTS_ADDRESS 3
String hosts[MAX_HOSTS][4];  // Array containing information about hosts received over mDNS.


// When an mDNS packet gets parsed this callback gets called once per Query.

void answerCallback(const mdns::Answer* answer) {

  // A typical PTR record matches service to a human readable name.

  if (answer->rrtype == MDNS_TYPE_PTR and strstr(answer->name_buffer, QUESTION_SERVICE) != 0) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->rdata_buffer) {
        // Already in hosts[][].
        break;
      }
      if (hosts[i][HOSTS_SERVICE_NAME] == "") {
        // This hosts[][] entry is still empty.
        hosts[i][HOSTS_SERVICE_NAME] = answer->rdata_buffer;
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" ** ERROR ** No space in buffer for ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.print("  :  ");
      Serial.print('"');
      Serial.println(answer->rdata_buffer);
      Serial.print('"');
    }
  }

  // A typical SRV record matches a human readable name to port and FQDN info.

  if (answer->rrtype == MDNS_TYPE_SRV) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->name_buffer) {
        // This hosts entry matches the name of the host we are looking for
        // so parse data for port and hostname.
        char* port_start = strstr(answer->rdata_buffer, "port=");
        if (port_start) {
          port_start += 5;
          char* port_end = strchr(port_start, ';');
          char port[1 + port_end - port_start];
          strncpy(port, port_start, port_end - port_start);
          port[port_end - port_start] = '\0';

          if (port_end) {
            char* host_start = strstr(port_end, "host=");
            if (host_start) {
              host_start += 5;
              hosts[i][HOSTS_PORT] = port;
              hosts[i][HOSTS_HOST_NAME] = host_start;
            }
          }
        }
        break;
      }
    }
  }

  // A typical A record matches an FQDN to network ipv4 address.

  if (answer->rrtype == MDNS_TYPE_A) {
    for (int i = 0; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_HOST_NAME] == answer->name_buffer) {
        hosts[i][HOSTS_ADDRESS] = answer->rdata_buffer;
        Serial.println(hosts[i][HOSTS_ADDRESS]);
        break;
      }
    }
  }

  for (int i = 0; i < MAX_HOSTS; ++i) {
    if (hosts[i][HOSTS_SERVICE_NAME] != "") {
      Serial.print(">  ");
      Serial.print(hosts[i][HOSTS_SERVICE_NAME]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_PORT]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_HOST_NAME]);
      Serial.print("    ");
      Serial.println(hosts[i][HOSTS_ADDRESS]);
    }
  }
}


// buffer can be used bu other processes that need a large chunk of memory.
byte buffer[MAX_MDNS_PACKET_SIZE];
mdns::MDns my_mdns(NULL, NULL, answerCallback, buffer, MAX_MDNS_PACKET_SIZE);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).

#define dht_pin D0

void MQTT_connect();
void setTimer(long delayTime);

dht DHT;
char hostString[16] = {0};

void setup() {
  Serial.begin(9600);
  delay(10);

  Serial.println(F("Adafruit MQTT Test"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Query for all host information for a paticular service.
  my_mdns.Clear();
  struct mdns::Query query_mqtt;
  strncpy(query_mqtt.qname_buffer, QUESTION_SERVICE, MAX_MDNS_NAME_LEN);
  query_mqtt.qtype = MDNS_TYPE_PTR;
  query_mqtt.qclass = 1;    // "INternet"
  query_mqtt.unicast_response = 0;
  my_mdns.AddQuery(query_mqtt);
  my_mdns.Send();

  delay(1000);
}

unsigned int last_packet_count = 0;
long delayTime = 5000;
char input[20] = {0};

void loop() {
  my_mdns.loop();
  
  #ifdef DEBUG_STATISTICS
  
    // Give feedback on the percentage of incoming mDNS packets that fitted in buffer.
    if (last_packet_count != my_mdns.packet_count && my_mdns.packet_count != 0) {
      last_packet_count = my_mdns.packet_count;
      Serial.print("mDNS decode success rate: ");
      Serial.print(100 - (100 * my_mdns.buffer_size_fail / my_mdns.packet_count));
      Serial.print("%\nLargest packet size: ");
      Serial.println(my_mdns.largest_packet_seen);
      sprintf(AIO_SERVER, "%s", hosts[0][HOSTS_SERVICE_NAME].c_str());
      Serial.println(AIO_SERVER);
    }
	
  #endif
  
  MQTT_connect();

  mqtt.subscribe(&configData);
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &configData) {
      Serial.print(F("Got: "));
      Serial.println((char *)configData.lastread);
    }
  }
  
  if(delayTime <= 0){
    Serial.println("Please enter delay time :");
    
    while (!Serial.available());
    for(int i = 0; Serial.available() > 0; ++i){
      input[i] = (long)Serial.read();
    }

    delayTime = atoi(input);
    Serial.print("delay time: ");
    Serial.println(delayTime);
  }
 
  setTimer(delayTime);
  
  DHT.read11(dht_pin);

  Serial.print("Current humidity = ");

  Serial.print(DHT.humidity);

  Serial.print("%  ");

  Serial.print("temperature = ");

  Serial.print(DHT.temperature);

  Serial.println("C  ");
/*
  char data[100];
  sprintf(data, "Humidity: %.2f, Tempereture: %.2f", DHT.humidity, DHT.temperature);
  
  publishData.publish(data);
*/
  publishTemperature.publish(DHT.temperature);
  publishHumidity.publish(DHT.humidity);

  

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
  */

}

void setTimer(long delayTime) {
  long msec = 0;
  long before = millis();
  long current = 0;
  long difference = 0;
  do {
    delay(1);
    current = millis();
    difference = current - before;

  } while ( difference < delayTime );
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 5;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

