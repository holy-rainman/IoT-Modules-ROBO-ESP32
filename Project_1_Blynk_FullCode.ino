#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define ledR    21
#define ledY    22
#define ledG    26
#define pinPB   34

#define BLYNK_GREEN   "#23C48E"
#define BLYNK_YELLOW  "#FFFF00"
#define BLYNK_RED     "#D3435C"

char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PW";

BlynkTimer timer;
WidgetLED led1(V4);

int bypass = 0;
int ledStatus = 0;   // 0 = Green, 1 = Yellow, 2 = Red

bool blinkV1 = false;
bool blinkV2 = false;
bool blinkV3 = false;

int blinkCount1 = 0;
int blinkCount2 = 0;
int blinkCount3 = 0;

void setLED(int greenState, int yellowState, int redState)
{ digitalWrite(ledG, greenState);
  digitalWrite(ledY, yellowState);
  digitalWrite(ledR, redState);
}

void updateLEDbyStatus()
{ if (ledStatus == 0)
  { setLED(HIGH, LOW, LOW);
      led1.setColor(BLYNK_GREEN);
  }
  else if (ledStatus == 1)
  { setLED(LOW, HIGH, LOW);
    led1.setColor(BLYNK_YELLOW);
  }
  else if (ledStatus == 2)
  { setLED(LOW, LOW, HIGH);
    led1.setColor(BLYNK_RED);
  }
}

BLYNK_WRITE(V0)   // Bypass
{ bypass = param.asInt();

  if (bypass == 0)
  { Blynk.virtualWrite(V1, LOW);
    Blynk.virtualWrite(V2, LOW);
    Blynk.virtualWrite(V3, LOW);
  }
  else
    updateLEDbyStatus();
}

BLYNK_WRITE(V1)   // Green
{ if (bypass == 1)
  { if (param.asInt() == 1)
    { ledStatus = 0;
      Blynk.virtualWrite(V2, LOW);
      Blynk.virtualWrite(V3, LOW);
      updateLEDbyStatus();
    }
  }
  else
  { blinkV1 = true;
    blinkCount1 = 0;
  }
}

BLYNK_WRITE(V2)   // Yellow
{ if (bypass == 1)
  { if (param.asInt() == 1)
    { ledStatus = 1;
      Blynk.virtualWrite(V1, LOW);
      Blynk.virtualWrite(V3, LOW);
      updateLEDbyStatus();
    }
  }
  else
  { blinkV2 = true;
    blinkCount2 = 0;
  }
}

BLYNK_WRITE(V3)   // Red
{ if (bypass == 1)
  { if (param.asInt() == 1)
    { ledStatus = 2;
      Blynk.virtualWrite(V1, LOW);
      Blynk.virtualWrite(V2, LOW);
      updateLEDbyStatus();
    }
  }
  else
  { blinkV3 = true;
    blinkCount3 = 0;
  }
}

void blinkWidget()
{ if (blinkV1)
  { if (blinkCount1 < 6)
    { Blynk.virtualWrite(V1, blinkCount1 % 2);
      blinkCount1++;
    }
    else
    { Blynk.virtualWrite(V1, LOW);
      blinkV1 = false;
    }
  }

  if (blinkV2)
  { if (blinkCount2 < 6)
    { Blynk.virtualWrite(V2, blinkCount2 % 2);
      blinkCount2++;
    }
    else
    { Blynk.virtualWrite(V2, LOW);
      blinkV2 = false;
    }
  }

  if (blinkV3)
  { if (blinkCount3 < 6)
    { Blynk.virtualWrite(V3, blinkCount3 % 2);
      blinkCount3++;
    }
    else
    { Blynk.virtualWrite(V3, LOW);
      blinkV3 = false;
    }
  }
}

int cnt=0;
void checkPB()
{ if (bypass == 0)
  { if (digitalRead(pinPB) == LOW)
    { cnt++;
      if (cnt>2) cnt=0;
      delay(200); //debounce
    }
    if(cnt==0) { ledStatus = 0; updateLEDbyStatus(); }
    if(cnt==1) { ledStatus = 1; updateLEDbyStatus(); }
    if(cnt==2) { ledStatus = 2; updateLEDbyStatus(); }
  }
}

BLYNK_CONNECTED()
{ led1.on();
  Blynk.virtualWrite(V0, LOW);
  Blynk.virtualWrite(V1, LOW);
  Blynk.virtualWrite(V2, LOW);
  Blynk.virtualWrite(V3, LOW);
}
void setup()
{ Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(ledR, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ledG, OUTPUT);

  pinMode(pinPB, INPUT_PULLUP);

  ledStatus = 0;
  updateLEDbyStatus();

  timer.setInterval(500L, blinkWidget);
  timer.setInterval(50L, checkPB);
}

void loop()
{ Blynk.run();
  timer.run();
}
