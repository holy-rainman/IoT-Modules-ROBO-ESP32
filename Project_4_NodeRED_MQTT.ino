#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include "time.h"

// ===== PINS =====
#define SERVO_PIN   19
#define IR_PIN      34
#define BUTTON_PIN  35

#define OPEN_DURATION 10000

Servo servo;
Adafruit_NeoPixel led(1, 15, NEO_GRB + NEO_KHZ800);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// WIFI
const char* ssid = "YOUR_SSID";
const char* pass = "YOUR_PASSWORD";

// MQTT
const char* server = "YOUR_MQTT_SERVER.s1.eu.hivemq.cloud";
const char* user   = "YOUR_MQTT_USERNAME";
const char* pwd    = "YOUR_MQTT_PASSWORD";

const char* topic_servo = "gate/state";

// SYSTEM
String openTime = "";
int durationSec = 0;

bool armed = false, executed = false;
bool gateOpen = false, closing = false;
bool manual = false, manualState = false;

unsigned long tOpen = 0, tClose = 0;
int retry = 0, lastBtn = HIGH;
String lastTriggeredTime = "";

// ===== LED =====
void setLED(int r,int g,int b)
{ led.setPixelColor(0, led.Color(r,g,b));
  led.show();
}

// ===== WIFI =====
void wifi()
{ Serial.print("Connecting WiFi");
  WiFi.begin(ssid, pass);

  while(WiFi.status()!=WL_CONNECTED)
  { delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
  espClient.setInsecure();
}

// ===== TIME MATCH (ROBUST) =====
bool matchTime()
{ struct tm t;
  if(!getLocalTime(&t)) return false;

  char now[6];
  sprintf(now,"%02d:%02d",t.tm_hour,t.tm_min);
  Serial.print("NOW: ");
  Serial.print(now);
  Serial.print(" | TARGET: ");
  Serial.print(openTime);
  Serial.print(" | DURATION: ");
  Serial.println(durationSec);

  return openTime == String(now);
}
// ===== MQTT CALLBACK =====
void cb(char* topic, byte* payload, unsigned int len)
{ String msg="";
  for(int i=0;i<len;i++) msg+=(char)payload[i];

  Serial.print("MQTT: ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(msg);

  if(!strcmp(topic,"gate/time"))     openTime=msg;
  if(!strcmp(topic,"gate/duration")) durationSec=msg.toInt();

  if(openTime!="" && durationSec>0)
  { armed=true;
    executed=false;

    setLED(0,0,255);
    Serial.println("SYSTEM ARMED");
  }
}

// ===== MQTT RECONNECT =====
void reconnect(){
  while(!client.connected())
  { Serial.print("MQTT connecting...");

    if(client.connect("ESP32",user,pwd))
    { Serial.println("connected");

      client.subscribe("gate/time");
      client.subscribe("gate/duration");

      client.publish(topic_servo, "Door is closed");
    }
    else
    { Serial.print("failed rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ===== SETUP =====
void setup()
{ Serial.begin(9600);
  wifi();

  client.setServer(server,8883);
  client.setCallback(cb);

  configTime(8*3600,0,"pool.ntp.org");

  pinMode(IR_PIN,INPUT);
  pinMode(BUTTON_PIN,INPUT);

  servo.attach(SERVO_PIN);
  servo.write(90);

  led.begin();
  setLED(0,0,0);
}

// ===== LOOP =====
void loop()
{ if(!client.connected()) reconnect();
  client.loop();

  // ===== BUTTON =====
  int btn=digitalRead(BUTTON_PIN);

  if(lastBtn==HIGH && btn==LOW)
  { delay(50);

    manual=true;
    manualState=!manualState;

    if(manualState)
    { Serial.println("MANUAL OPEN");
      servo.write(0);
      setLED(0,255,0);
      client.publish(topic_servo, "Door is open");
    }
    else
    { Serial.println("MANUAL CLOSE");
      servo.write(90);
      client.publish(topic_servo, "Door is closed");

      if(armed) setLED(0,0,255);
      else      setLED(0,0,0);
      manual=false;
    }
  }

  lastBtn = btn;

  if(manual) return;

  // ===== AUTO OPEN =====
  if(armed && matchTime() && openTime != lastTriggeredTime)
  { Serial.println(">>> AUTO TRIGGERED <<<");
    servo.write(0);
    setLED(0,255,0);
    client.publish(topic_servo, "Door is open");

    tOpen = millis();
    gateOpen = true;

    lastTriggeredTime = openTime;  
    retry = 0;
  }

  // ===== AUTO CLOSE =====
  if(gateOpen && millis()-tOpen >= OPEN_DURATION)
  { Serial.println("AUTO CLOSE");
    servo.write(90);
    tClose = millis();

    gateOpen = false;
    closing = true;
  }

  // ===== VERIFY =====
  if(closing)
  { if(digitalRead(IR_PIN)==LOW)
    { Serial.println("CLOSED OK");
      client.publish(topic_servo, "Door is closed");

      if(armed) setLED(0,0,255);
      else      setLED(0,0,0);

      closing = false;
      retry = 0;
    }

    if(millis()-tClose > durationSec * 1000)
    { retry++;

      if(retry < 3)
      { Serial.println("RETRY");
        servo.write(0);
        delay(2000);

        servo.write(90);
        tClose = millis();
      }
      else
      { Serial.println("ERROR");
        setLED(255,0,0);
        client.publish(topic_servo, "Door Error");

        closing = false;
      }
    }
  }
  delay(100);
}
