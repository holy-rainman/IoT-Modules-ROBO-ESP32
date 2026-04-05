#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
 
// ======== RGB LED ========
const int RGBPin    = 15; // RGB pin for Robo ESP32
const int numPixels = 2;  //Number of onboard RGB pixels
Adafruit_NeoPixel pixels(numPixels, RGBPin, NEO_GRB + NEO_KHZ800);

// ======== HiveMQ Cloud (TLS) ========
const char* mqtt_server = "YOUR_HIVEMQ_URL_.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;          // TLS port
const char* mqtt_user   = "YOUR_HIVEMQ_CREDENTIAL_USERNAME";
const char* mqtt_pass   = "YOUR_HIVEMQ_CREDENTIAL_PW";

// ======== Topics ========
const char* sub_topic = "aim/bypass";   // expects "bypass_1" / "bypass_0"

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

// ---------- MQTT callback ----------
void callback(char* topic, byte* payload, unsigned int length) 
{ String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("Message ["); Serial.print(topic); Serial.print("] ");
  Serial.println(msg);

  // Control LED by payload
  if (msg == "bypass_1")  pixels.setPixelColor(0, pixels.Color(200, 0, 0));
  if (msg == "bypass_0")  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

// ---------- MQTT reconnect ----------
void reconnect() 
{ while (!client.connected()) 
  { Serial.print("Attempting MQTT connection...");
    String clientId = "ESPClient-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) 
    { Serial.println("connected");
      client.subscribe(sub_topic);
      Serial.print("Subscribed to: "); Serial.println(sub_topic);
    } 
    else 
    { Serial.print("failed, rc="); 	Serial.print(client.state());
      Serial.println("  retry in 5s");	delay(5000);
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

void loop() 
{ if (!client.connected()) reconnect();
  client.loop();
}
