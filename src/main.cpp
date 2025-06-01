#include <Arduino.h>
/*#############################################################################
EdgeTX/OpenTX BLE Gamepad  v1.0

Copyright (C) by mosch
License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
GITHUB: https://github.com/moschotto

Description:
Creates a bluetooth LE gamepad which can be used to play simulators with your
RC remote. It decodes SBUS data/frames transmitted by the radio. The SBUS
signal can be grabbed from the external module bay.

The serial port and PINs must be defined accordingly
In this case serial port 1 PINs 20 and 21 are used
The LED is optional...

The DISCONNECTPIN allows you to delete the bond to an other device. In this case you 
will be able to bond to an other device. It will delete the actual connected device to 
your remote.

Check out and download these libraries (thanks to the contributors)

|-- Bolder Flight Systems SBUS @ 7.0.0
|-- ESP32-BLE-Gamepad @ 0.5.0
|   |-- NimBLE-Arduino @ 1.4.0
|-- FastLED @ 3.5.0
|   |-- EspSoftwareSerial @ 6.13.2
|   |-- SPI @ 2.0.0

https://www.arduino.cc/reference/en/libraries/bolder-flight-systems-sbus/
https://github.com/FastLED/FastLED
https://github.com/lemmingDev/ESP32-BLE-Gamepad
https://github.com/h2zero/NimBLE-Arduino
################################################################################*/

#include <Arduino.h>
#include <BleGamepad.h>
#include "sbus.h"
#include <FastLED.h>
// OTA
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

// Pin definitions
#define RX_PIN 20
#define TX_PIN 21
#define LED_PIN 7
#define DISCONNECTPIN 9

boolean debug_console = false;

// OTA
const char* host = "ESP32-C3-EdgeTX-Bluetooth";
const char* ssid = "EdgeTX-BT-OTA";
const char* password = "12345678";
String hostname = "EdgeTX";
WebServer server(80);
unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

////////////////////////////////////////////////////////////////////////////////
//LEd object
CRGB leds[1];

////////////////////////////////////////////////////////////////////////////////
//BLE gamepad settings
BleGamepad bleGamepad("OpenTX EdgeTX GamePad", "Moschotto", 93);
BleGamepadConfiguration bleGamepadConfig;

////////////////////////////////////////////////////////////////////////////////
//Serial and SBUS parameters
bfs::SbusRx sbus_rx(&Serial1);
std::array<int16_t, bfs::SbusRx::NUM_CH()> sbus_data_in;
std::array<int16_t, bfs::SbusRx::NUM_CH()> sbus_data_out;

int channel_max = 16;
int16_t channel_min_val = 172;
int16_t channel_max_val = 1811;
boolean lostframe = false;
boolean failsafe = false;
boolean isconnected = false;
int button_count = 4;

//default channel map EdgeTX AETR -just for debugging purposes
//ROLL = AIL | PITCH = ELE | THR = THR | YAW = RUD
String channel_names[] = { " ROLL ", " PITCH ", " TROTTLE ", " YAW "};

//functions
void LED_blink(int, String, int);
void LED_single_color(String);
void LED_pulse(String);

void setup()
{

    WiFi.mode(WIFI_AP);  
    WiFi.softAP(ssid, password);
    WiFi.softAP(hostname.c_str());
    delay(1000);
    IPAddress myIP = WiFi.softAPIP();
    Serial.println("*WiFi-AP-Mode*");
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    delay(1000);

    // OTA
    server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is EdgeTX Bluetooth module. For updating type ip-address/update !");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");

    //Setup Pins bluetooth
    pinMode(DISCONNECTPIN, INPUT_PULLUP);

    //LED init
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, 1);
    LED_single_color("green");
    delay(1500);

    //Serial init
    Serial.begin(115200);
    LED_blink(1, "green", 100);
    delay(1000);

    //SBUS init
    sbus_rx.Begin(RX_PIN,TX_PIN);
    Serial.println("starting SBUS");
    LED_blink(1, "green", 100);
    delay(1000);

    //BLE init
    Serial.println("starting BLE");
    bleGamepadConfig.setButtonCount(button_count);
    bleGamepadConfig.setAutoReport(false);
    Serial.println("BLE config done");
    bleGamepad.begin(&bleGamepadConfig);
    Serial.println("BLE init done");
    LED_blink(1, "green", 100);
    delay(1000);


    LED_blink(10, "green", 50);
    delay(2000);
}

void loop()
{

  if (sbus_rx.Read())
    {
        sbus_data_in = sbus_rx.ch();

        lostframe = sbus_rx.lost_frame();
        failsafe = sbus_rx.failsafe();

        ////////////////////////////////////////////////////////////////////////
        //send data
        //default channel map EdgeTX AETR
        //ROLL = AIL | PITCH = ELE | THR = THR | YAW = RUD
        ////////////////////////////////////////////////////////////////////////
        if (bleGamepad.isConnected() == true && lostframe == false)
        {
          for(int i=0;i < channel_max; i++)
          {
              sbus_data_out[i] = map(sbus_data_in[i], channel_min_val, channel_max_val, 0, 32767);
          }

          //workaround for parsing errors. channel 13-16 are not configured in the radio but sometimes all channels are set to strange random numbers:
          /*check output of channel 13-16 below
          CH1: ROLL 993 | CH2: PITCH 324 | CH3: TROTTLE 717 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 993 | CH2: PITCH 324 | CH3: TROTTLE 717 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 993 | CH2: PITCH 324 | CH3: TROTTLE 717 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 993 | CH2: PITCH 324 | CH3: TROTTLE 717 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 1476 | CH2: PITCH 1825 | CH3: TROTTLE 154 | CH4: YAW 1391 | CH5:AUX1 1384 | CH6:AUX2 1384 | CH7:AUX3 1384 | CH8:AUX4 1384 | CH9:AUX5 1384 | CH10:AUX6 1400 | CH11:AUX7 1384 | CH12:AUX8 1792 | CH13:AUX9 1795 | CH14:AUX10 1795 | CH15:AUX11 1795 | CH16:AUX12 3 |  lost frames: 0
          CH1: ROLL 1727 | CH2: PITCH 964 | CH3: TROTTLE 555 | CH4: YAW 555 | CH5:AUX1 555 | CH6:AUX2 555 | CH7:AUX3 555 | CH8:AUX4 1579 | CH9:AUX5 555 | CH10:AUX6 43 | CH11:AUX7 248 | CH12:AUX8 248 | CH13:AUX9 248 | CH14:AUX10 248 | CH15:AUX11 0 | CH16:AUX12 120 |  lost frames: 0
          CH1: ROLL 1101 | CH2: PITCH 590 | CH3: TROTTLE 828 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 1182 | CH2: PITCH 684 | CH3: TROTTLE 912 | CH4: YAW 1811 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          CH1: ROLL 1299 | CH2: PITCH 766 | CH3: TROTTLE 1034 | CH4: YAW 1752 | CH5:AUX1 173 | CH6:AUX2 173 | CH7:AUX3 173 | CH8:AUX4 173 | CH9:AUX5 173 | CH10:AUX6 173 | CH11:AUX7 175 | CH12:AUX8 173 | CH13:AUX9 992 | CH14:AUX10 992 | CH15:AUX11 992 | CH16:AUX12 992 |  lost frames: 0
          */

          if(sbus_data_in[14] > 890 && sbus_data_in[14] < 1000 && sbus_data_in[15] > 890 && sbus_data_in[15] < 1000)
          {
            // bleGamepad.setAxes sets all axes at once. There are a few:
            // (x axis, y axis, z axis, rx axis, ry axis, rz axis, slider 1, slider 2)
            bleGamepad.setAxes(sbus_data_out[3],sbus_data_out[2],sbus_data_out[4],sbus_data_out[5],sbus_data_out[0],sbus_data_out[1],sbus_data_out[10],sbus_data_out[11]);

            if(sbus_data_out[8] > 1000){bleGamepad.press(BUTTON_1);}else{bleGamepad.release(BUTTON_1);}
            if(sbus_data_out[9] > 1000){bleGamepad.press(BUTTON_2);}else{bleGamepad.release(BUTTON_2);}
            if(sbus_data_out[6] > 1000){bleGamepad.press(BUTTON_3);}else{bleGamepad.release(BUTTON_3);}
            if(sbus_data_out[7] > 1000){bleGamepad.press(BUTTON_4);}else{bleGamepad.release(BUTTON_4);}

            bleGamepad.sendReport();
          }

        }

        ////////////////////////////////////////////////////////////////////////
        //print channel output if debug is enabled
        ////////////////////////////////////////////////////////////////////////

        if(debug_console)
        {
          for(int i=0;i < channel_max; i++)
          {
            Serial.print("CH"); Serial.print(i+1); Serial.print(":");
            if(i<=3) {Serial.print(channel_names[i]);}
            else { Serial.print("AUX"); Serial.print(i - 3); Serial.print(" "); }
            Serial.print(sbus_data_in[i]);
            Serial.print(" | ");
          }

          Serial.print(" lost frames: ");
          Serial.println(sbus_rx.lost_frame());
        }


      }

    if (bleGamepad.isConnected())
    {
      //do only only once
      if(isconnected == false)
      {
         LED_blink(5, "blue", 250);
         isconnected = true;
         if(debug_console)
         {
          Serial.println("BLE is now connected");
         }
         LED_single_color("blue");
      }
      // Force disconnecting bluetooth
      if (digitalRead(DISCONNECTPIN) == LOW) // On my board, pin 0 is LOW for pressed
        {   
            bool pairingResult = bleGamepad.enterPairingMode();

            if(pairingResult)
            {
              Serial.println("New device paired successfully"); 
            }
            else
            {
              Serial.println("No new device paired");
            }
        }
    }
    else
    {
      if(debug_console)
      {
        Serial.println("BLE is NOT connected");
      }
      LED_pulse("blue");
      //LED_blink(1, "blue", 500);
      isconnected = false;
    }
  
  // OTA handle
  server.handleClient();
  ElegantOTA.loop();

}//end main

void LED_blink(int cnt, String color, int delay_ms)
{
  FastLED.setBrightness(255);

  for(int i=0;i<cnt;i++)
  {

    if(color == "red") {leds[0] = CRGB::Red;}
    if(color == "green") {leds[0] = CRGB::Green;}
    if(color == "blue") {leds[0] = CRGB::Blue;}

    FastLED.show();
    delay(delay_ms);

    leds[0] = CRGB::Black;
    FastLED.show();
    delay(delay_ms);
  }
}

void LED_single_color(String color)
{
    FastLED.setBrightness(255);

    if(color == "red") {leds[0] = CRGB::Red;}
    if(color == "green") {leds[0] = CRGB::Green;}
    if(color == "blue") {leds[0] = CRGB::Blue;}
    FastLED.show();
}

void LED_pulse(String color)
{
  for(int i=0;i<200;i++)
  {
    if(color == "red") {leds[0] = CRGB::Red;}
    if(color == "green") {leds[0] = CRGB::Green;}
    if(color == "blue") {leds[0] = CRGB::Blue;}
    FastLED.setBrightness(i);
    FastLED.show();
    delay(5);
  }
  for(int i=200;i>0;i--)
  {
    if(color == "red") {leds[0] = CRGB::Red;}
    if(color == "green") {leds[0] = CRGB::Green;}
    if(color == "blue") {leds[0] = CRGB::Blue;}
    FastLED.setBrightness(i);
    FastLED.show();
    delay(5);
  }
}
