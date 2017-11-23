#include <Arduino.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            12

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;


//PINS:

const int in1 = 8;
const int in2 = 4;
const int en = 10;
const int led = 13;
const int cmd = 5;
//VARS:
int del = 3;
int pulses = 0;
boolean gotCMD = false;
int speed =0;
int distance =0;
int run_once = 0;
volatile int pos = 0;

//LED colors:

// Define the color codes by name to make it easier to call later on
uint32_t red = pixels.Color(255, 0, 0);
uint32_t green = pixels.Color(0, 255, 0);
uint32_t yellow = pixels.Color(255, 100, 0);


//heartBeat Vars:
int beat = 0;
boolean hearbeatDown = false;
long heartBeatTimer =0;
int heartBeatMax = 130;

//functions
void  off();
void fram();
void bak();
void run(int steps);
void count_pulses();
void enc();
void heartBeat();


void setup() {

        Serial.begin(115200);
        //init Rotary Encoder
        Serial.println("Init Rotary Encoder...");
        oled.begin(&Adafruit128x32, I2C_ADDRESS);
        oled.setFont(Adafruit5x7);
        pixels.begin(); // This initializes the NeoPixel library.
        pixels.setBrightness(32);


        pinMode(in1, OUTPUT);
        pinMode(in2, OUTPUT);
        pinMode(en, OUTPUT);
        pinMode(cmd, INPUT);
        detachInterrupt(0);
        oled.clear();
        //first row
        oled.println("Dispenser");


                for (size_t i = 0; i < 50; i++) {
                        pixels.setPixelColor(0, pixels.Color(-i,i,-i)); // Moderately bright green color.
                        pixels.show(); // This sends the updated pixel color to the hardware.
                        delay(3);
                }
                for (size_t i = 0; i < 50; i++) {
                        pixels.setPixelColor(0, pixels.Color(-i,-i,i)); // Moderately bright green color.
                        pixels.show(); // This sends the updated pixel color to the hardware.
                        delay(3);
                }
                for (size_t i = 0; i < 50; i++) {
                        pixels.setPixelColor(0, pixels.Color(i,-i,-i)); // Moderately bright green color.
                        pixels.show(); // This sends the updated pixel color to the hardware.
                        delay(3);
                }



}



void loop() {

        if (digitalRead(cmd)  == HIGH) {
                int runonce = 0;
                oled.clear();
                oled.set2X();
                oled.print("CMD");
                oled.setCursor(0, 2);
                //pulses = 0;

                while(digitalRead(cmd)  == HIGH) {
                        if (runonce == 0) {
                                Serial.println("got cmd, attaching interupt");
                                attachInterrupt(0, count_pulses, RISING);
                                runonce =1;
                                pulses = 0;
                                pixels.setPixelColor(0, pixels.Color(0,0,150));
                                pixels.show(); // This sends the updated pixel color to the hardware.
                        }
                }
                oled.print(pulses);
                Serial.println(pulses);

                gotCMD = true;
                detachInterrupt(0);
                Serial.println("interupt detached");
                for (int i = 0; i < pulses; i++) {

                                pixels.setPixelColor(0, pixels.Color(0,0,150));
                                pixels.show(); // This sends the updated pixel color to the hardware.
                                delay(50);
                                pixels.setPixelColor(0, pixels.Color(0,0,0));
                                pixels.show(); // This sends the updated pixel color to the hardware.
                                delay(50);
                              }
                run(pulses);
        }
        heartBeat();
}

void enc(){
        pos++;
}



void heartBeat(){


        if (millis() - heartBeatTimer > 20) {
                pixels.setPixelColor(0, pixels.Color(0,beat,0)); // Moderately bright green color.
                pixels.show(); // This sends the updated pixel color to the hardware.

                if (beat > heartBeatMax && hearbeatDown == false) hearbeatDown = true;
                if (beat < 10 && hearbeatDown == true) hearbeatDown = false;

                if (hearbeatDown == false) {
                        beat=beat+2;
                }
                else beat = beat -2;

                heartBeatTimer = millis();
                //  Serial.print("beat: "); Serial.println(beat);
        }




}
void run(int steps){
        pixels.setPixelColor(0, yellow); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.

        attachInterrupt(1, enc, RISING);

        steps = 3250*1.25;
        fram();
        while ( pos - steps < 0) {

/*
                Serial.print(pos);
                Serial.print(" of ");
                Serial.println(steps);
 */
        }

        off();
        detachInterrupt(1);
        pos =0;
        gotCMD = false;
        distance=distance+steps;
        Serial.print("distance: ");
        Serial.println(distance);
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(50);
        pixels.setPixelColor(0, yellow); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(50);
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(50);
        pixels.setPixelColor(0, yellow); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.
        delay(50);
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show(); // This sends the updated pixel color to the hardware.
}

void count_pulses(){

        pulses++;
}
void fram(){
        digitalWrite(en, HIGH);
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
}
void bak(){
        digitalWrite(en, HIGH);
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
}
void off(){
        digitalWrite(en, HIGH);
        digitalWrite(in1, HIGH);
        digitalWrite(in2, HIGH);
        delay(100);
        digitalWrite(en, LOW);

}
