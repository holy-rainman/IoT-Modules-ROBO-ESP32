#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <HCSR04.h>
#include "CytronMotorDriver.h"
 
CytronMD motor1(PWM_PWM, 12, 13);   // PWM 1A = Pin 12, PWM 1B = Pin 13.
CytronMD motor2(PWM_PWM, 14, 27);   // PWM 2A = Pin 14, PWM 2B = Pin 27.

UltraSonicDistanceSensor distanceSensor(22, 21); 
// ======== RGB LED ========
const int RGBPin    = 15; // RGB pin for Robo ESP32
const int numPixels = 2;  //Number of onboard RGB pixels
Adafruit_NeoPixel pixels(numPixels, RGBPin, NEO_GRB + NEO_KHZ800);

// ======== HiveMQ Cloud (TLS) ========
const char* mqtt_server = "YOUR_HIVEMQ_URL_.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;          // TLS port
const char* mqtt_user   = "YOUR_HIVEMQ_CREDENTIAL_USERNAME";
const char* mqtt_pass   = "YOUR_HIVEMQ_CREDENTIAL_PW";

WiFiClientSecure espClient;
PubSubClient client(espClient);
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PW";

void setup_wifi() 
{ Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  { delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  espClient.setInsecure();
}

int flag=0;
// ---------- MQTT callback ----------
void callback(char* topic, byte* payload, unsigned int length) 
{ String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("Message ["); Serial.print(topic); Serial.print("] ");
  Serial.println(msg);

  // ===== BYPASS CONTROL =====
  if (strcmp(topic, "aim/bypass") == 0)
  { if (msg == "bypass_1")  
    { pixels.setPixelColor(0, pixels.Color(200, 0, 0)); flag=1; }
    if (msg == "bypass_0")  
    { pixels.setPixelColor(0, pixels.Color(0, 0, 0));   flag=0; }
    pixels.show();
  }

  // ===== MOTOR SPEED CONTROL =====
  if(flag==1)
  { motor1.setSpeed(0);
    if (strcmp(topic, "aim/speed") == 0)
    { int pwmValue = msg.toInt();   // slider 0–255

      // Safety limit
      if (pwmValue < 0)   pwmValue = 0;
      if (pwmValue > 255) pwmValue = 255;

      motor1.setSpeed(pwmValue);

      Serial.print("PWM Value = ");
      Serial.println(pwmValue);
    }
  }
}

// ---------- MQTT reconnect ----------
void reconnect() 
{ while (!client.connected()) 
  { Serial.print("Attempting MQTT connection...");
    String clientId = "ESPClient-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) 
    { Serial.println("connected");
      client.subscribe("aim/bypass");
      client.subscribe("aim/speed");
      Serial.print("Subscribed to: "); 
      Serial.print(" aim/bypass,");
      Serial.println(" aim/speed,");
    } 
    else 
    { Serial.print("failed, rc=");  Serial.print(client.state());
      Serial.println("  retry in 5s");  delay(5000);
    }
  }
}

void setup() 
{ Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pixels.begin();  pixels.clear(); pixels.show();   
}

float distance;
unsigned long lastPublish = 0;
void loop() 
{ if (!client.connected()) reconnect();
  client.loop();

  if(flag==0)
  { distance = distanceSensor.measureDistanceCm();
    if(distance<0)  distance = 0;
    if(distance>20) distance = 20;

    int motorSpeed = map(distance,0,20,0,255);
    motor1.setSpeed(motorSpeed);
    delay(200);

    if (millis() - lastPublish >= 300)
    { String distanceStr = String(distance);
      client.publish("aim/distance", distanceStr.c_str());
      lastPublish = millis();
    }
  }
}
