#include <Arduino.h>
#include <WiFi.h>
#include <ESPUI.h>
#include <ESPmDNS.h>

#define HUMIDITY_GPIO 33
#define METER_OUT 26

int dac_val = 0;
bool use_slider = false;

void generalCallback(Control *sender, int type) {
	Serial.print("CB: id(");
	Serial.print(sender->id);
	Serial.print(") Type(");
	Serial.print(type);
	Serial.print(") '");
	Serial.print(sender->label);
	Serial.print("' = ");
	Serial.println(sender->value);
  if (sender->id == 1) {
    use_slider = sender->value.toInt();
  } else if (sender->id == 2) {
    dac_val = sender->value.toInt();
  }
}

void setup() {
  Serial.begin(115200);

  /* Setup GPIO pins */
  pinMode(HUMIDITY_GPIO, INPUT);
  pinMode(METER_OUT, OUTPUT);

  /* Setup Wifi as an AP */
  WiFi.softAP("ESP32-AP", "1234567890");
	#if defined(ESP32)
		WiFi.setSleep(false); //For the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
	#endif

  /* Setup ESPUI */
	ESPUI.sliderContinuous = true;
  ESPUI.switcher("use_slider", generalCallback, ControlColor::Alizarin, "Use slider");
  ESPUI.slider("value", generalCallback, ControlColor::Alizarin, 30, 0, 255);
  ESPUI.begin("ESP32-AP");

  Serial.println("Setup completed.");
}

void loop() {
  int val = analogRead(HUMIDITY_GPIO);
  Serial.print("ADC value: ");
  Serial.print(val);

  if (!use_slider) {
    int mappedVal = map(val, 1600, 3600, 0, 255);
    Serial.print(" ouput value: ");
    Serial.println(mappedVal);
    analogWrite(METER_OUT, mappedVal);
  } else {
    Serial.print(" ouput value: ");
    Serial.println(dac_val);
    analogWrite(METER_OUT, dac_val);
  }
}
