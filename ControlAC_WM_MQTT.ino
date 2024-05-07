/* Copyright 2019 David Conran
*
* Modifyed by Dionne Monteiro, May 07 2024.
*
* This example code demonstrates how to use the "Common" IRac class to control
* various air conditions. The IRac class does not support all the features
* for every protocol. Some have more detailed support that what the "Common"
* interface offers, and some only have a limited subset of the "Common" options.
*
* This example code will:
* o Try to turn on, then off every fully supported A/C protocol we know of.
* o It will try to put the A/C unit into Cooling mode at 25C, with a medium
*   fan speed, and no fan swinging.
* Note: Some protocols support multiple models, only the first model is tried.
*
*
*/

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>

#define MQTTBroker   "broker.emqx.io"
#define TOPIC_OUT    "UFPA/ControlAC/Out"
#define TOPIC_IN     "UFPA/ControlAC/In"
#define USERBroker   ""
#define PASSWDBroker ""

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>                 //https://github.com/esp8266/Arduino
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

// Configuração do controlador de AC
const uint16_t kIrLed = D5;  // The ESP GPIO pin to use that controls the IR LED.
IRac ac(kIrLed);  // Create a A/C object using GPIO to sending messages with.

int estadoAC = 0;

const char* mqtt_server = MQTTBroker;
const char* topicoIn = TOPIC_IN;
const char* topicoOut = TOPIC_OUT;

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

unsigned long lastMsg = 0;

void configWiFi(){
  //Define o auto connect e o SSID do modo AP
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect("Control AC");
  
  //Log na serial para ver conexão
  Serial.println("Conectado");
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.localIP());
}

void ligarAC() {
  //for (int i = 1; i < kLastDecodeType; i++) {
    int i = 24;
    decode_type_t protocol = (decode_type_t)i;
    // If the protocol is supported by the IRac class ...
    if (ac.isProtocolSupported(protocol)) {
      Serial.println("Protocolo " + String(protocol) + " / " +
                    typeToString(protocol) + " é suportado.");
      ac.next.protocol = decode_type_t::GREE;  // Set a protocol to use.
      ac.next.model = 1;  // Some A/Cs have different models. Try just the first.
      ac.next.mode = stdAc::opmode_t::kCool;  // Run in cool mode initially.
      ac.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
      ac.next.celsius = true;
      ac.next.degrees = 22;
      ac.next.fanspeed = stdAc::fanspeed_t::kMax;  // Start the fan at high.
      ac.next.swingv = stdAc::swingv_t::kAuto;  // Don't swing the fan up or down.
      ac.next.swingh = stdAc::swingh_t::kAuto;  // Don't swing the fan left or right.
      ac.next.light = true;  // Turn on any LED/Lights/Display that we can.
      ac.next.beep = true;  // Turn on any beep from the A/C if we can.
      ac.next.econo = false;  // Turn off any economy modes if we can.
      ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
      ac.next.turbo = false;  // Don't use any turbo/powerful/etc modes.
      ac.next.quiet = false;  // Don't use any quiet/silent/etc modes.
      ac.next.sleep = -1;  // Don't set any sleep time or modes.
      ac.next.clean = false;  // Turn off any Cleaning options if we can.
      ac.next.clock = -1;  // Don't set any current time if we can avoid it.
      
      ac.next.power = true;  // We want to turn on the A/C unit.
      Serial.println("Enviando mensagem para LIGAR AC.");
      ac.sendAc();  // Have the IRac class create and send a message.
      delay(1000);  // Wait 1 second.
    }
  //}
}

void desligarAC() {
  //for (int i = 1; i < kLastDecodeType; i++) {
    int i = 24;
    decode_type_t protocol = (decode_type_t)i;
    // If the protocol is supported by the IRac class ...
    if (ac.isProtocolSupported(protocol)) {
      Serial.println("Protocolo " + String(protocol) + " / " +
                    typeToString(protocol) + " é suportado.");
      ac.next.protocol = decode_type_t::GREE;  // Set a protocol to use.
      ac.next.model = 1;  // Some A/Cs have different models. Try just the first.
      ac.next.mode = stdAc::opmode_t::kCool;  // Run in cool mode initially.
      ac.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
      ac.next.celsius = true;
      ac.next.degrees = 22;
      ac.next.fanspeed = stdAc::fanspeed_t::kHigh;  // Start the fan at high.
      ac.next.swingv = stdAc::swingv_t::kAuto;  // Don't swing the fan up or down.
      ac.next.swingh = stdAc::swingh_t::kAuto;  // Don't swing the fan left or right.
      ac.next.light = true;  // Turn on any LED/Lights/Display that we can.
      ac.next.beep = true;  // Turn on any beep from the A/C if we can.
      ac.next.econo = false;  // Turn off any economy modes if we can.
      ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
      ac.next.turbo = false;  // Don't use any turbo/powerful/etc modes.
      ac.next.quiet = false;  // Don't use any quiet/silent/etc modes.
      ac.next.sleep = -1;  // Don't set any sleep time or modes.
      ac.next.clean = false;  // Turn off any Cleaning options if we can.
      ac.next.clock = -1;  // Don't set any current time if we can avoid it.
      
      ac.next.power = false;  // Now we want to turn the A/C off.
      Serial.println("Enviando messagem para DESLIGAR AC.");
      ac.sendAc();  // Send the message.
      delay(1000);  // Wait 1 second.
    }
  //}
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Função callback acionada.");
  char ch = (char) payload[0];

  if(ch == '0') {
    desligarAC();
    estadoAC = 0;
  }
  else if(ch == '1') {
    ligarAC();
    estadoAC = 1;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");  
    // Create a random client ID
    String clientId = "ESP8266-ControlAC-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),USERBroker,PASSWDBroker)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topicoOut, "Retornando aos trabalhos");
      client.subscribe(topicoIn);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("Tentando novamente em 5 segundos.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  configWiFi();
  
  // Inicializa o controle do AC
  ac.next.protocol = decode_type_t::GREE;  // Set a protocol to use.
  ac.next.model = 1;  // Some A/Cs have different models. Try just the first.
  ac.next.mode = stdAc::opmode_t::kCool;  // Run in cool mode initially.
  ac.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
  ac.next.degrees = 22;  // 25 degrees.
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium;  // Start the fan at medium.
  ac.next.swingv = stdAc::swingv_t::kOff;  // Don't swing the fan up or down.
  ac.next.swingh = stdAc::swingh_t::kOff;  // Don't swing the fan left or right.
  ac.next.light = false;  // Turn off any LED/Lights/Display that we can.
  ac.next.beep = false;  // Turn off any beep from the A/C if we can.
  ac.next.econo = false;  // Turn off any economy modes if we can.
  ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
  ac.next.turbo = false;  // Don't use any turbo/powerful/etc modes.
  ac.next.quiet = false;  // Don't use any quiet/silent/etc modes.
  ac.next.sleep = -1;  // Don't set any sleep time or modes.
  ac.next.clean = false;  // Turn off any Cleaning options if we can.
  ac.next.clock = -1;  // Don't set any current time if we can avoid it.
  ac.next.power = false;  // Initially start with the unit off.
  
  // Inicializa a conexão com o servidor MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.subscribe(topicoIn);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    // Preparando os dados para envio
    char message[5];
    if(estadoAC == 0) {
      strcpy(message, "0");
    }
    else if(estadoAC == 1) {
      strcpy(message, "1");
    }
    
    //char MSG[] = "{\"AC\":%d, \"Temperatura\":%.2f}";                   // NodeRED
    //char MSG[] = "{\n\"Sensor\": {\"Temperatura\":%.2f,\"Unit\":\"ºC\"}\n}";  // PRTG
    
    //sprintf( message, MSG, temp);
    Serial.print("Publicando a mensagem: ");
    Serial.println(message);
    client.publish(topicoOut, message);
  }
}