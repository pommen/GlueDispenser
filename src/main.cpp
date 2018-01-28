/*
 ##############################################################################

   Servo dispenser for glue dispesing
   screw pich:4
   enc pulses per rot = 4062










 ############################################################################
 */



#include <Arduino.h>
//#include "SSD1306Ascii.h"
//#include "SSD1306AsciiAvrI2c.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <Bounce2.h>



Bounce debouncer = Bounce();
#define I2C_ADDRESS 0x3C
//SSD1306AsciiAvrI2c oled;

/*

   Version 1 pins:

   const int phase = 6;
   const int en = 5;
   const int Break = 7;
   const int btn = 8;
   const int cmd = A2;
   const int out = A0;
   const int endStop1 = 10;
   const int endStop2 = 9;



 */
//PINS:
const uint8_t PixelPin = A1;  // make sure to set this to the correct pin, ignored for Esp8266
const int phase = 6;
const int en = 7;
const int Break = 5;
const int btn = 8;
const int cmd = 4;
const int out = A0;
const int endStop1 = 12;
const int endStop2 = 13;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PixelPin, NEO_GRB + NEO_KHZ800);




//*************  Config Params**************


const int step_multiplier = 130; //encoder pulses per commanded step
const int motorSpeed = 90; //pwm for motor speed  - lägre är snabbare
const int deadline_multiplier = 4500; //är denna för liten så kommer dispesern gå i timeout näar vi kör långsamt
int BackUpPulses = 1000;
int waitTimeUntilRetract = 0;
int backUpExtra = 1500;



//*****************************************


//VARS:
//int del = 3;
volatile int pulses = 0;
boolean gotCMD = false;
int speed =0;
int distance =0;
int run_once = 0;
volatile int pos = 0;
int runstate = 0;
boolean manuelDone = false;
boolean has_run_once = false;



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
void run(int steps, int dir, boolean no_BackUp);
void count_pulses();
void enc();
void heartBeat();
void blink_run_finnished();
void refill();
void out_done();
void error_blink(int error);
void backa(int distance);

void setup() {

        Serial.begin(115200);
        //init Rotary Encoder
        Serial.println("Setup..");
        //  oled.begin(&Adafruit128x32, I2C_ADDRESS);
        //oled.setFont(Adafruit5x7);
        pixels.begin(); // This initializes the NeoPixel library.
        pixels.setBrightness(36);


        pinMode(phase, OUTPUT);
        pinMode(Break, OUTPUT);
        pinMode(en, OUTPUT);
        pinMode(cmd, INPUT);
        pinMode(endStop1, INPUT_PULLUP);
        pinMode(endStop2, INPUT_PULLUP);
        pinMode(out,OUTPUT);
        pinMode(btn, INPUT_PULLUP);
        detachInterrupt(0);
        debouncer.attach(btn);
        debouncer.interval(5);  // interval in ms


        //oled.clear();
        //first row
        //oled.println("Dispenser");

        Serial.println("flashing");
        for (size_t i = 0; i < 50; i++) {
                pixels.setPixelColor(0, pixels.Color(-i,i,-i));
                pixels.show();         // This sends the updated pixel color to the hardware.
                delay(3);
        }
        for (size_t i = 0; i < 50; i++) {
                pixels.setPixelColor(0, pixels.Color(-i,-i,i));
                pixels.show();         // This sends the updated pixel color to the hardware.
                delay(3);
        }
        for (size_t i = 0; i < 50; i++) {
                pixels.setPixelColor(0, pixels.Color(i,-i,-i));
                pixels.show();         // This sends the updated pixel color to the hardware.
                delay(3);
        }
        //endstopper();
        Serial.println("flashed");


        Serial.println("..setup Done!");

        //backa();
}


void loop() {

        debouncer.update();
        // Get the updated value :
        int BTNvalue = debouncer.read();

        if (BTNvalue == LOW) {
                int endstop_timer = millis();
                boolean end2 = true;
                pixels.setPixelColor(0, yellow);
                pixels.show();
                Serial.println("Feeding. ");
                fram();
                debouncer.update();
                BTNvalue = debouncer.read();

                while ( BTNvalue == LOW) {
                        debouncer.update();
                        BTNvalue = debouncer.read();

                        if (millis() - endstop_timer > 10) { //samplar var 10'de sekund
                                //      end1 = digitalRead(endStop1);
                                end2 = digitalRead(endStop2);
                                endstop_timer = millis();
                        }


                        if ( end2 == false) {
                                Serial.println("endStop2");
                                runstate = 3;
                                refill();
                                error_blink(5);
                        }
                }

                manuelDone = true;
                off();
        }

        if (manuelDone == true) {
                backa(50);
                manuelDone = false;
                has_run_once = true;
        }

        if (digitalRead(cmd)  == HIGH) {
                int runonce = 0;
                /*
                   oled.clear();
                   oled.set2X();
                   oled.print("CMD");
                   oled.setCursor(0, 2);
                 */
                //pulses = 0;

                while(digitalRead(cmd)  == HIGH) {
                        if (runonce == 0) {
                                Serial.println("got cmd, attaching interupt");
                                attachInterrupt(1, count_pulses, RISING);
                                runonce =1;
                                pulses = 0;
                                pixels.setPixelColor(0, pixels.Color(0,0,150));
                                pixels.show(); // This sends the updated pixel color to the hardware.
                        }
                }
                //pulses = 10;
                //oled.print(pulses);
                Serial.println(pulses);

                gotCMD = true;
                detachInterrupt(1);
                Serial.println("interupt detached");

                if (pulses == 0 ) {
                        error_blink(1);
                }


                else  {

                        /*
                              for (int i = 0; i < pulses; i++) {

                                      pixels.setPixelColor(0, pixels.Color(0,0,150));
                                      pixels.show();   // This sends the updated pixel color to the hardware.
                                      delay(30);
                                      pixels.setPixelColor(0, pixels.Color(0,0,0));
                                      pixels.show();   // This sends the updated pixel color to the hardware.
                                      delay(30);
                              }
                         */
                        out_done();
                        run(pulses, 1, 0);
                }

                switch (runstate) {
                case 0:
                        blink_run_finnished();
                        out_done();
                        break;
                case 1:
                        // statements
                        break;
                case 2:
                        Serial.println("Run State 2, too long to finnish");
                        error_blink(3);
                        break;
                case 3:
                        Serial.println("Run State 3, Refill");

                        refill();
                        error_blink(5);

                        break;
                }

        }
        heartBeat();
//Serial.println("restart loop");

}

void backa(int distance){
        Serial.print("Backing up ");
        Serial.print(distance);
        Serial.println("steps");

        attachInterrupt(0, enc, RISING);
        bak();
        pos=0;
        while (pos < distance) {

        }
        off();
        detachInterrupt(0);
        BackUpPulses = distance;
}


void refill(){
        int endstop_timer = 0;
        Serial.println("Refilling");
        bak();
        boolean endstop = true;

        //kollar var 10'de sekund om vi har kommit till utgångläget
        while (endstop == true) {
                if (millis() - endstop_timer > 10) {
                        endstop = digitalRead(endStop1);
                        endstop_timer = millis();
                }
        }
        off();
        delay(200);
        run(10, 1, 1); //kör fram så vi inte aktivererar lägesbytaren efter vi har kört färdigt
        runstate = 3;

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
                        beat=beat+4;
                }
                else beat = beat -4;

                heartBeatTimer = millis();
        }




}
void run(int steps, int dir, boolean no_BackUp){
        boolean end2 = true;
        pixels.setPixelColor(0, yellow);
        pixels.show();
        attachInterrupt(0, enc, RISING);

        //steps = 3250*1.25; ett helt varv
        steps = steps* step_multiplier;
        int BackUpPulses_temp = steps;
        if (has_run_once == true) {
                Serial.print("har backat förr. före kompensering : ");
                Serial.println(steps);
                steps = steps+BackUpPulses;
                Serial.print("Efter kompensering : ");
                Serial.println(steps);

        }
        int deadline = steps * deadline_multiplier;  // hur lång tid borde det ta att komma i mål. gått den över denna tiden genererad ett fel.

        Serial.print("running ");
        Serial.print(steps);
        Serial.println(" steps.");

        if (dir == 1) fram();
        if (dir== 0) bak();
        delay(5);
        long endstop_timer =millis();
        long deadline_timer = millis();
        //här kör vi motorn tills den är framme, når ett endstop eller kör för länge utan encoder.
        int diff = (steps +backUpExtra) - pos;

        while ( diff > 0) {
                diff = steps - pos;


//********************************* FEL GENERATOR under körning **************************

                if (millis() - endstop_timer > 10) {
                        //      end1 = digitalRead(endStop1);
                        end2 = digitalRead(endStop2);
                        endstop_timer = millis();
                }

                if ( end2 == false) {
                        Serial.println("endStop2");
                        runstate = 3;
                        break;
                }



                if (millis() - deadline_timer > deadline) {

                        runstate = 2;         //motorn kommer inte i mål
                        break;
                }


        }

        off();
        pos = 0;
        delay(BackUpPulses+waitTimeUntilRetract); //låter limmet tyta ut innan vi backar
        if (no_BackUp == 0) {
                backa(BackUpPulses_temp+backUpExtra); //+1000 pga att jag inte vill scalera 10000 varje gång vi kör loopen
        }
        detachInterrupt(0);
        has_run_once = true;
        pos =0;
        gotCMD = false;
        distance=distance+steps;
        Serial.print("distance: ");
        Serial.println(distance);
}
void blink_run_finnished(){
        for (size_t i = 0; i < 2; i++) {
                /* code */

                pixels.setPixelColor(0, pixels.Color(0,0,0));
                pixels.show(); // This sends the updated pixel color to the hardware.
                delay(50);
                pixels.setPixelColor(0, yellow); // Moderately bright green color.
                pixels.show(); // This sends the updated pixel color to the hardware.
                delay(50);
        }
}
void count_pulses(){

        pulses++;
}
void fram(){
        digitalWrite(phase, LOW);
        analogWrite(en, motorSpeed);
        digitalWrite(Break, HIGH);

}
void bak(){
        digitalWrite(phase, HIGH);
        digitalWrite(en, LOW); //max hastigghet
        digitalWrite(Break, HIGH);
}
void off(){
        //  digitalWrite(en, HIGH);
        digitalWrite(en, HIGH);
        digitalWrite(Break, LOW);
        delay(100);
        digitalWrite(en, LOW);

}
void out_done() {
        digitalWrite(out, HIGH);
        delay(300);
        digitalWrite(out, LOW);
        Serial.println("blinking cmd sent");
}
void error_blink(int error) {

        pixels.setPixelColor(0, red);
        pixels.show();                   // This sends the updated pixel color to the hardware.
        digitalWrite(out, HIGH);

        while (1==1) {


                for (size_t i = 0; i != error; i++) {
                        /* code */

                        pixels.setPixelColor(0, red);
                        pixels.show();
                        delay(350);
                        pixels.setPixelColor(0,0,0,0);
                        pixels.show();
                        delay(350);

                }


                delay(3000);
        }
}



/*
   runstate's:

   0: all good
   1: cmd but no step
   2:takes to long to finnish run - 3 blink
   3:tube emty. - 5 blink

 */


/*kladd


                Serial.print(pos);
                Serial.print(" of ");
                Serial.println(steps);

 */
