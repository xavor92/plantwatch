#include <Arduino.h>
#include <WiFi.h>
#include <ESPUI.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#define HUMIDITY_GPIO 33
#define METER_OUT 26
#define PIXEL_OUT 23
#define NUMPIXELS 5
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_OUT, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500


int slider_value = 0;
bool use_slider = true;
bool demo_mode = true;
bool led_updated = true;
int input_id_slider_meter = 0;
int input_id_slider_red = 0;
int input_id_slider_green = 0;
int input_id_slider_blue = 0;
int red, green, blue;
int input_id_useslider_switch = 0;
int demo_mode_id = 0;
int input_id_ssid = 0;
int input_id_password = 0;
Preferences preferences;

void generalCallback(Control *sender, int type) {
	Serial.print("CB: id(");
	Serial.print(sender->id);
	Serial.print(") Type(");
	Serial.print(type);
	Serial.print(") '");
	Serial.print(sender->label);
	Serial.print("' = ");
	Serial.println(sender->value);
  if (input_id_useslider_switch && sender->id == input_id_useslider_switch) {
    use_slider = sender->value.toInt();
  } else if (input_id_slider_meter && sender->id == input_id_slider_meter) {
    slider_value = sender->value.toInt();
  } else if (input_id_slider_red && sender->id == input_id_slider_red) {
    red = sender->value.toInt();
    led_updated = true;
  } else if (input_id_slider_green && sender->id == input_id_slider_green) {
    green = sender->value.toInt();
    led_updated = true;
  } else if (input_id_slider_blue && sender->id == input_id_slider_blue) {
    blue = sender->value.toInt();
    led_updated = true;
  } else if (demo_mode_id && sender->id == demo_mode_id) {
    demo_mode = sender->value.toInt();
  }

}

void textCall(Control* sender, int type)
{
    Serial.print("Text: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);

    if (input_id_ssid != 0 && sender->id == input_id_ssid) { 
      preferences.begin("wifi", false);
      preferences.putString("ssid", sender->value);
      preferences.end();
      Serial.println("SSID saved");
    } else if (input_id_password != 0 && sender->id == input_id_password) {
      preferences.begin("wifi", false);
      preferences.putString("password", sender->value);
      preferences.end();
      Serial.println("Password saved");
    }
}

/* try to join a wifi defined by AP and PASSWORD, if that fails, start wifi in softAP mode with SSID ESP32-AP and password 1234567890 */
void wifiSetup() 
{
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "default_ssid"); // Replace "default_ssid" with your default SSID
  String password = preferences.getString("password", "default_password"); // Replace "default_password" with your default password
  preferences.end();

  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
    if (i++ > 10) {
      Serial.println("Failed to connect to WiFi, starting softAP..");
      break;
    }
  }
  // if connected, print IP address
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAP("ESP32-AP", "1234567890");
  }
}

void setup() {
  Serial.begin(115200);

  /* Setup GPIO pins */
  pinMode(HUMIDITY_GPIO, INPUT);
  pinMode(METER_OUT, OUTPUT);

  /* Setup WiFi */
  wifiSetup();
	#if defined(ESP32)
		WiFi.setSleep(false); //For the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
	#endif

  /* Setup ESPUI */
	ESPUI.sliderContinuous = true;
  input_id_useslider_switch = ESPUI.switcher("use_slider", generalCallback, ControlColor::Alizarin, "Use slider");
  demo_mode_id = ESPUI.switcher("demo_mode", generalCallback, ControlColor::Alizarin, "Demo Mode");
  input_id_slider_meter = ESPUI.slider("Meter", generalCallback, ControlColor::Alizarin, 30, 0, 255);
  input_id_slider_red = ESPUI.slider("red", generalCallback, ControlColor::Alizarin, 30, 0, 255);
  input_id_slider_green = ESPUI.slider("green", generalCallback, ControlColor::Alizarin, 30, 0, 255);
  input_id_slider_blue = ESPUI.slider("blue", generalCallback, ControlColor::Alizarin, 30, 0, 255);
  input_id_ssid = ESPUI.text("SSID", textCall, ControlColor::Dark, "SSID");
  Serial.print("input_id_ssid:");
  Serial.println(input_id_ssid);
  input_id_password = ESPUI.text("Password", textCall, ControlColor::Dark, "Password");
  Serial.print("input_id_password:");
  Serial.println(input_id_password);
  ESPUI.setInputType(input_id_password, "password");

  ESPUI.begin("ESP32-AP");

  pixels.begin();

  Serial.println("Setup completed.");
}

void sweepRGBColorSpace(int* red, int* green, int* blue) {
  static int colorIndex = 0;
  int colorStep = 1;
  
  switch(colorIndex) {
    case 0:
      *red += colorStep;
      if (*red >= 255) {
        *red = 0;
        colorIndex++;
      }
      break;
    case 1:
      *green += colorStep;
      if (*green >= 255) {
        *green = 0;
        colorIndex++;
      }
      break;
    case 2:
      *blue += colorStep;
      if (*blue >= 255) {
        *blue = 0;
        colorIndex = 0;
      }
      break;
  }
}

void loop() {
  int val = analogRead(HUMIDITY_GPIO);
  int output_value;

  if (!demo_mode) {
    if (!use_slider) {
      output_value = map(val, 1600, 3600, 0, 255);
    } else {
      output_value = slider_value;
    }
  } else {
    // iterate through the colors and make output value sweep from 0 to 255
    sweepRGBColorSpace(&red, &green, &blue);
    led_updated = true;
    output_value = max(red, green);
    output_value = max(output_value, blue);
    Serial.print("Demo mode: ");
    Serial.print(red);
    Serial.print(", ");
    Serial.print(green);
    Serial.print(", ");
    Serial.println(blue);
    delay(10);
  }

  analogWrite(METER_OUT, output_value);
  if (led_updated) {
    for(int i=0; i<NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(red, green, blue));
    }
    pixels.show();
  }
}