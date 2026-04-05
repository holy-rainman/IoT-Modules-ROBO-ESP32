#include <Arduino.h>
#include "DHT_Async.h"
#include <Adafruit_NeoPixel.h>

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 25;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const int RGBPin    = 15; // RGB pin for Robo ESP32
const int numPixels = 2;  // Number of onboard RGB pixels
Adafruit_NeoPixel pixels(numPixels, RGBPin, NEO_GRB + NEO_KHZ800);

void setup() 
{ Serial.begin(9600);
  pixels.begin();  // Initialize NeoPixel library
  pixels.clear();  // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  pinMode(23,OUTPUT);
}

static bool measure_environment(float *temperature, float *humidity) 
{ static unsigned long measurement_timestamp = millis();

  /* Measure once every four seconds. */
  if (millis() - measurement_timestamp > 4000ul) 
  { if (dht_sensor.measure(temperature, humidity)) 
    { measurement_timestamp = millis();
      return (true);
    }
  }
  return (false);
}

float temperature, humidity;
int statusCondition;
void loop() 
{ if (measure_environment(&temperature, &humidity)) 
  { Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.print("%  ");

    int an = analogRead(33);
    Serial.print("Analog = ");
    Serial.print(an);

    statusCondition = checkCondition();
  }
  
  if(statusCondition==2)
  { tone(23,500); delay(100);
    noTone(23);   delay(100);
  }
}
int checkCondition()
{ // ================= Decision Making =================
  // Critical Condition (highest priority)
  if ((temperature > 30) ||
      (humidity < 20) ||
      (humidity > 80))
  { pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red
    pixels.show();
    Serial.println("  Critical");
    return 2;
  }

  // Normal Condition
  else if ((temperature >= 23 && temperature <= 26) &&
          (humidity >= 40 && humidity <= 60))
  { pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green
    pixels.show();
    Serial.println("  Normal");
    return 0;
  }
  // Warning Condition
  else if ((temperature > 26 && temperature <= 30) ||
          (humidity >= 20 && humidity < 40) ||
          (humidity > 60 && humidity <= 80))
  { pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Yellow
    pixels.show();
    Serial.println("  Warning");
    return 1;
  }
  // Fallback (safe default)
  else
  { pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Yellow
    pixels.show();
    return 1;
  }
}
