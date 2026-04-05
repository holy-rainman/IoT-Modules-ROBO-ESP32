#include <Arduino.h>
#include "DHT_Async.h"

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 25;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

void setup() 
{ Serial.begin(9600);
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

void loop() 
{ float temperature;
  float humidity;

  // Read DHT sensor every 4 seconds
  if (measure_environment(&temperature, &humidity)) 
  { Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" deg. C, H = ");
    Serial.print(humidity, 1);
    Serial.print("%  ");

    int an = analogRead(33);
    Serial.print("Analog = ");
    Serial.println(an);
  }
}
