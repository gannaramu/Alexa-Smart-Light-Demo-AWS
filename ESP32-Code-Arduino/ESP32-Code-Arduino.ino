/*********
  Gourav Das
  Complete project details at https://hackernoon.com/cloud-home-automation-series-part-2-use-aws-device-shadow-to-control-esp32-with-arduino-code-tq9a37aj
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
#include <AWS_IOT.h>
#include <WiFi.h>
#include <ArduinoJson.h> //Download latest ArduinoJson 6 Version library only by Benoît Blanchon from Manage Libraries
#include <NeoPixelBus.h>
#include <ColorConverterLib.h>
//#include "NeoFade.h"
//NeoFade F;

AWS_IOT shadow;
#define ledPin 2 // LED Pin

#define light_name "WS2812 Hue Strip" //default light name
#define lightsCount 3
#define pixelCount 74
uint16_t pixelStart[lightsCount + 1] = {0, 1, 21, pixelCount};

#define use_hardware_switch false // on/off state and brightness can be controlled with above gpio pins. Is mandatory to connect them to ground with 10K resistors
#define button1_pin 4 // on and bri up
#define button2_pin 5 // off and bri down

#define transitionLeds 6 // must be even number

#define use_hardware_switch false // on/off state and brightness can be controlled with above gpio pins. Is mandatory to connect them to ground with 10K resistors

char WIFI_SSID[] = "UV31C";
char WIFI_PASSWORD[] = "TPSQ255134";
char HOST_ADDRESS[] = "a3mvvasqjcyxts-ats.iot.us-east-1.amazonaws.com"; // a1tp25b40hi9dh-ats.iot.us-east-1.amazonaws.com";  //AWS IoT Custom Endpoint Address
char CLIENT_ID[] = "alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U";
char SHADOW_GET[] = "$aws/things/alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U/shadow/get/accepted";
char SENT_GET[] = "$aws/things/alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U/shadow/get";
char SHADOW_UPDATE[] = "$aws/things/alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U/shadow/update";
char GET_DELTA[] = "$aws/things/alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U/shadow/update/delta";
char UPDATE_ACCEPTED[] = "$aws/things/alexa-smart-home-demo-SmartHomeThing-RV9S38NS5U6U/shadow/update/accepted";



int status = WL_IDLE_STATUS;
int msgReceived = 0;
char payload[1024];
char reportpayload[1024];
char rcvdPayload[1024];
const uint8_t PixelPin = 14;  // make sure to set this to the correct pin, ignored for Esp8266
float transitiontime = 4;
uint8_t rgb[lightsCount][3], bri[lightsCount], sat[lightsCount], color_mode[lightsCount], scene;
bool light_state[lightsCount], in_transition;
int ct[lightsCount], hue[lightsCount];
float step_level[lightsCount][3], current_rgb[lightsCount][3], x[lightsCount], y[lightsCount];
byte mac[6];

byte packetBuffer[64];

RgbColor red = RgbColor(255, 0, 0);
RgbColor green = RgbColor(0, 255, 0);
RgbColor white = RgbColor(255);
RgbColor black = RgbColor(0);

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(pixelCount, PixelPin);


void infoLight(RgbColor color) {
  // Flash the strip in the selected color. White = booted, green = WLAN connected, red = WLAN could not connect
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, color);
    strip.Show();
    delay(10);
    strip.SetPixelColor(i, black);
    strip.Show();
  }
}

void process_lightdata(uint8_t light, float transitiontime) {
  transitiontime *= 17 - (pixelCount / 40); //every extra led add a small delay that need to be counted
//  if (color_mode[light] == 1 && light_state[light] == true) {
//    convert_xy(light);
//  } else if (color_mode[light] == 2 && light_state[light] == true) {
//    convert_ct(light);
//  } 
if (color_mode[light] == 3 && light_state[light] == true) {
    convert_hue(light);
  }
  for (uint8_t i = 0; i < 3; i++) {
    if (light_state[light]) {
      step_level[light][i] = ((float)rgb[light][i] - current_rgb[light][i]) / transitiontime;
    } else {
      step_level[light][i] = current_rgb[light][i] / transitiontime;
    }
  }
}


RgbColor blending(float left[3], float right[3], uint8_t pixel) {
  uint8_t result[3];
  for (uint8_t i = 0; i < 3; i++) {
    float percent = (float) pixel / (float) (transitionLeds + 1);
    result[i] = (left[i] * (1.0f - percent) + right[i] * percent) / 2;
  }
  return RgbColor(result[0], result[1], result[2]);
}

RgbColor convInt(uint8_t color[3]) {
  return RgbColor(color[0], color[1], color[2]);
}

RgbColor convFloat(float color[3]) {
  return RgbColor((int)color[0], (int)color[1], (int)color[2]);
}





void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload, payLoad, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

void Startup (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload, payLoad, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
   Serial.println("******* Startup Function Called");
}
void updateShadowPower (int power)
{
  if (power) {
    sprintf(reportpayload, "{\"state\": {\"reported\": {\"powerState\":\"ON\"}}}");
    for (int light = 0; light < lightsCount; light++) {
      light_state[light] = true;
    }
  } else {
    sprintf(reportpayload, "{\"state\": {\"reported\": {\"powerState\":\"OFF\"}}}");
    for (int light = 0; light < lightsCount; light++) {
      light_state[light] = false;
    }

  }
  delay(500);
  if (shadow.publish(SHADOW_UPDATE, reportpayload) == 0)
  {
    Serial.print("Publish Message:");
    Serial.println(reportpayload);
  }
  else
  {
    Serial.println("Publish failed");
    Serial.println(reportpayload);
  }
}



void updateShadowColor (float h, float s, float b)
{
  sprintf(reportpayload, "{\"state\": {\"reported\": {\"color\": {\"hue\": \"%f\",\"saturation\":  \"%f\",\"brightness\":  \"%f\"}}}}", h, s, b);
  delay(500);
  if (shadow.publish(SHADOW_UPDATE, reportpayload) == 0)
  {
    Serial.print("Publish Message:");
    Serial.println(reportpayload);
  }
  else
  {
    Serial.println("Publish failed");
    Serial.println(reportpayload);
  }
}

void updateBrightness (float b)
{
  sprintf(reportpayload, "{\"state\": {\"reported\": { \"brightness\": \"%f\", \"color\": {\"hue\": \"%f\",\"saturation\":  \"%f\",\"brightness\":  \"%f\"}}}}", b, b);
  delay(500);
  if (shadow.publish(SHADOW_UPDATE, reportpayload) == 0)
  {
    Serial.print("Publish Brightness:");
    Serial.println(reportpayload);
  }
  else
  {
    Serial.println("Publish failed");
    Serial.println(reportpayload);
  }
}



void convert_hue(uint8_t light)
{
  double      hh, p, q, t, ff, s, v;
  int        i;

  s = sat[light]/ 255.0;
  v = bri[light]/ 255.0;

//  if (s <= 0.0) {      // < is bogus, just shuts up warnings
//    rgb[light][0] = v;
//    rgb[light][1] = v;
//    rgb[light][2] = v;
//    return;
//  }

//
//        if (s == 0) {
//          // Achromatic (grey)
//          r = g = b = round(v * 255);
//        }
//        h /= 60; // sector 0 to 5
//        i = floor(h);
//        f = h - i; // factorial part of h
//        p = v * (1 - s);
//        q = v * (1 - s * f);
//        t = v * (1 - s * (1 - f));


  hh = hue[light];
  if (hh > 360.0) hh = 0;
  hh /= 60;
  i = floor(hh);
  ff = hh - i;
  p = v * (1 - s);
  q = v * (1 - (s * ff));
  t = v * (1 - (s * (1 - ff)));

  switch (i) {
    case 0:
      rgb[light][0] = round(v * 255.0);
      rgb[light][1] = round(t * 255.0);
      rgb[light][2] = round(p * 255.0);
      break;
    case 1:
      rgb[light][0] = round(q * 255.0);
      rgb[light][1] = round(v * 255.0);
      rgb[light][2] = round(p * 255.0);
      break;
    case 2:
      rgb[light][0] = round(p * 255.0);
      rgb[light][1] = round(v * 255.0);
      rgb[light][2] = round(t * 255.0);
      break;

    case 3:
      rgb[light][0] = round(p * 255.0);
      rgb[light][1] = round(q * 255.0);
      rgb[light][2] = round(v * 255.0);
      break;
    case 4:
      rgb[light][0] = round(t * 255.0);
      rgb[light][1] = round(p * 255.0);
      rgb[light][2] = round(v * 255.0);
      break;
    case 5:
    default:
      rgb[light][0] = round(v * 255.0);
      rgb[light][1] = round(p * 255.0);
      rgb[light][2] = round(q * 255.0);
      break;
  }
if (s == 0 && hh==0) {
          // Achromatic (grey)
          rgb[light][0] = rgb[light][1] = rgb[light][2] = round(v * 255);
        }
}
void lightEngine() {
  for (int i = 0; i < lightsCount; i++) {
    if (light_state[i]) {
      if (rgb[i][0] != current_rgb[i][0] || rgb[i][1] != current_rgb[i][1] || rgb[i][2] != current_rgb[i][2]) {
        in_transition = true;
        for (uint8_t k = 0; k < 3; k++) {
          if (rgb[i][k] != current_rgb[i][k]) current_rgb[i][k] += step_level[i][k];
          if ((step_level[i][k] > 0.0 && current_rgb[i][k] > rgb[i][k]) || (step_level[i][k] < 0.0 && current_rgb[i][k] < rgb[i][k])) current_rgb[i][k] = rgb[i][k];
        }
        for (int j = 0; j < pixelStart[i + 1] - pixelStart[i]  ; j++) // i will have a loop with the number of pixels in that light
        {
          strip.SetPixelColor(pixelStart[i] + j, RgbColor((int)current_rgb[i][0], (int)current_rgb[i][1], (int)current_rgb[i][2]));
        }
        strip.Show();
      }
    } else {
      if (current_rgb[i][0] != 0 || current_rgb[i][1] != 0 || current_rgb[i][2] != 0) {
        in_transition = true;
        for (uint8_t k = 0; k < 3; k++) {
          if (current_rgb[i][k] != 0) current_rgb[i][k] -= step_level[i][k];
          if (current_rgb[i][k] < 0) current_rgb[i][k] = 0;
        }
        for (int j = 0; j < pixelStart[i + 1] - pixelStart[i]  ; j++) // i will have a loop with the number of pixels in that light
        {
          strip.SetPixelColor(pixelStart[i] + j, RgbColor((int)current_rgb[i][0], (int)current_rgb[i][1], (int)current_rgb[i][2]));
        }
        strip.Show();
      }
    }
  }
  if (in_transition) {
    delay(6);
    in_transition = false;
  } else if (use_hardware_switch == true) {
    if (digitalRead(button1_pin) == HIGH) {
      int i = 0;
      while (digitalRead(button1_pin) == HIGH && i < 30) {
        delay(20);
        i++;
      }
      for (int light = 0; light < lightsCount; light++) {
        if (i < 30) {
          // there was a short press
          light_state[light] = true;
        }
        else {
          // there was a long press
          bri[light] += 56;
          if (bri[light] > 255) {
            // don't increase the brightness more then maximum value
            bri[light] = 255;
          }
        }
      }
    } else if (digitalRead(button2_pin) == HIGH) {
      int i = 0;
      while (digitalRead(button2_pin) == HIGH && i < 30) {
        delay(20);
        i++;
      }
      for (int light = 0; light < lightsCount; light++) {
        if (i < 30) {
          // there was a short press
          light_state[light] = false;
        }
        else {
          // there was a long press
          bri[light] -= 56;
          if (bri[light] < 1) {
            // don't decrease the brightness less than minimum value.
            bri[light] = 1;
          }
        }
      }
    }
  }
}



float fract(float x) {
  return x - int(x);
}

float mix(float a, float b, float t) {
  return a + (b - a) * t;
}

float step(float e, float x) {
  return x < e ? 0.0 : 1.0;
}


void setup() {
  strip.Begin();
  strip.Show();
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
//  F.begin();
//  F.setPeriod(5000);



  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    infoLight(white);
    Serial.println(WIFI_SSID);
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(1000);// wait 5 seconds for connection:
  }

  Serial.println("Connected to wifi");
  infoLight(green);
  if (shadow.connect(HOST_ADDRESS, CLIENT_ID) == 0) //Connect to AWS IoT COre
  {
    Serial.println("Connected to AWS");
    delay(1000);

    if (0 == shadow.subscribe(UPDATE_ACCEPTED, mySubCallBackHandler)) //Subscribe to Accepted GET Shadow Service
    {
      Serial.println("Subscribe to GET_DELTA Successfull");
    }
    else
    {
      Serial.println("Subscribe Failed for GET_DELTA, Check the Thing Name and Certificates");
      while (1);
    }

    if (0 == shadow.subscribe(SHADOW_GET, Startup)) //Subscribe to Accepted GET Shadow Service
    {
      Serial.println("Subscribe to GET_DELTA Successfull");
    }
    else
    {
      Serial.println("Subscribe Failed for GET_DELTA, Check the Thing Name and Certificates");
      while (1);
    }


  }
  else
  {
    Serial.println("AWS connection failed, Check the HOST Address");
    while (1);
  }


  delay(3000); /*Sent Empty string to fetch Shadow desired state*/
  if (shadow.publish(SENT_GET, "{}") == 0)
  {
    Serial.print("Empty String Published\n");
  }
  else
  {
    Serial.println("Empty String Publish failed\n");
  }  /*Sent Empty string to fetch Shadow desired state*/
  uint8_t light;
  float transitiontime = 4;
//  process_lightdata(light, transitiontime);

}
int temp = 0;
void loop() {
  if (msgReceived == 1)
  {
    msgReceived = 0;
    Serial.print("Received Message:");
    Serial.println(rcvdPayload);
    StaticJsonDocument<256> doc;
    deserializeJson(doc, rcvdPayload);

    if (doc.isNull()) { /* Test if parsing succeeds. */
      Serial.println("parseObject() failed");
      return;
    } /* Test if parsing succeeds. */
    if (doc["state"]["desired"]["color"]) {
      float h = doc["state"]["desired"]["color"]["hue"];
      float s = doc["state"]["desired"]["color"]["saturation"];
      float b = doc["state"]["desired"]["color"]["brightness"];
      Serial.print("hue is set to:        "); Serial.println(h);
      Serial.print("saturation is set to: "); Serial.println(s);
      Serial.print("brightness is set to: "); Serial.println(b);
      
      for (int light;light<3;light++)
      {
        color_mode[light] = 3;
        hue[light]=h;
        int ss = (int)(s*100);
        int bb = (int)(b*100);
        Serial.print("ss:  ");
        Serial.println(ss);
        Serial.print("bb:  ");
        Serial.println(bb);
        Serial.print("bri:  ");
        Serial.println(map(bb,0,100,0,255));
        Serial.print("sat:  ");
        Serial.println(map(ss,0,100,0,255));
         bri[light]=map(bb,0,100,0,255);
         sat[light]=map(ss,0,100,0,255);
        Serial.print("HSV light:"); Serial.println(light);
        Serial.print(hue[light]); Serial.print("  ");
        Serial.print(sat[light]); Serial.print("  ");
        Serial.println(bri[light]);
//         convert_hue(light);
          process_lightdata(light, transitiontime);

        Serial.print("RGB light:"); Serial.println(light);
        Serial.print(rgb[light][0]); Serial.print("  ");
        Serial.print(rgb[light][1]); Serial.print("  ");
        Serial.println(rgb[light][2]);
      }
         

        updateShadowColor(h, s, b);
    

    }    
    else if (doc["state"]["desired"]["powerState"]) {
      String power = doc["state"]["desired"]["powerState"];
      Serial.print(power);
      if (power.equals("ON")) {
        updateShadowPower(1);
      }
      else {
        updateShadowPower(0);
      }
    }
    else if(doc["state"]["desired"]["brightness"]) {
      float b = doc["state"]["desired"]["brightness"];

      for(int light;light<3;light++){
        int bb = (int)(b*100);
         bri[light]=map(bb,0,100,0,255);
      }
      updateBrightness(b);
    }
  }
  
  lightEngine();
}
