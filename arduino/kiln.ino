// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/

#include "max6675.h"

int thermoDO = D6;
int thermoCS = D8;
int thermoCLK = D5;
char Tbuffer[50];
char set_buffer[50];

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

/*************************************/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>


#define TFT_CS         D8
//#define TFT_RST        16  
#define TFT_RST         -1                                          //And connect to arduino reset
#define TFT_DC         D3
#define TFT_MOSI D7  // Data out
#define TFT_SCLK D5  // Clock out

// For ST7735-based displays, we will use this call
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

/***************************/

/////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <RotaryEncoder.h>

#define PIN_IN1 D1
#define PIN_IN2 D2

#define LED_PIN TX

#define T_OFFSET 20
#define T_STEP 5
#define T_MIN 5
#define T_MAX 1200

#define ROTARY_MIN ( (T_MIN-T_OFFSET)/T_STEP )
#define ROTARY_MAX ( (T_MAX-T_OFFSET)/T_STEP )

RotaryEncoder *encoder = nullptr;

IRAM_ATTR void checkPosition()
{
  encoder->tick(); // just call tick() to check the state.

  if ( encoder->getPosition() < ROTARY_MIN){
    encoder->setPosition(ROTARY_MIN);
  }
  if ( encoder->getPosition() > ROTARY_MAX){
    encoder->setPosition(ROTARY_MAX);
  }
}



/////////////////////////////////////////////////////////////////

unsigned long tt;
int pos = -1;
float T = T_OFFSET+10;
void setup() {
  
  Serial.begin(9600);

  
  pinMode(LED_PIN, OUTPUT);
  //analogWrite(LED_PIN,0 );
  digitalWrite(LED_PIN,HIGH);
  Serial.println("MAX6675 test");
  
  
  tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
  tft.setRotation(3);
  Serial.println(F("Initialized"));

  tt = millis();
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);

// wait for MAX chip to stabilize
  delay(500);
   // use TWO03 mode when PIN_IN1, PIN_IN2 signals are both LOW or HIGH in latch position.
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);


  // LED

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  write_set(0,&pos);
}




void loop() {
  // basic readout test, just print the current temp
   //tft.fillScreen(ST77XX_BLACK);

   

  encoder->tick(); // just call tick() to check the state.

  
   
   
   unsigned long now = millis();
   unsigned long elapsed = now - tt;
   
   if (elapsed > 1000 ){
    //delay(1000);
    T = thermocouple.readCelsius();
    write_T(&tt);
    
    }
  float target=pos*T_STEP+T_OFFSET;
  
  if (target > T) {
    //int mag = 255*(target-T)*(target-T)/T /T;
    //mag = mag>0?mag:0;
    //analogWrite(LED_PIN,(mag>255)?255:mag );
    digitalWrite(LED_PIN,LOW);
    
  }
  else {
    //analogWrite(LED_PIN,0)
    digitalWrite(LED_PIN,HIGH);
  }
 
 encoder->tick(); // just call tick() to check the state.

 int newPos = encoder->getPosition();
 write_set(newPos,&pos);
  
  

   // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!
   //delay(1000);
}

void write_T(unsigned long *tt){
   
   tft.setTextColor(ST77XX_GREEN,ST77XX_BLACK);
   tft.setTextSize(3);  
   tft.setCursor(60, 10);
   tft.print(" ");
   //tft.print(T,2);
   tft.setCursor(60,10);
   char t_buf[10];
   sprintf(t_buf,"%.2f",T);
   tft.print(t_buf);
   //tft.print(T,2);
    Serial.print("C = "); 
   Serial.println(T);
   //Serial.println(thermocouple.readCelsius());
   Serial.print("F = ");
   Serial.println(thermocouple.readFahrenheit());
   *tt=millis();
   

}

void write_set(int newPos, int *pos){
if (*pos != newPos) {
    Serial.print("pos:");
    Serial.print(newPos);
    Serial.print(" dir:");
    Serial.println((int)(encoder->getDirection()));
    *pos = newPos;
 
   
  tft.setTextColor(ST77XX_RED,ST77XX_BLACK);
   tft.setTextSize(3);  
   tft.setCursor(20,40);
   tft.print(" ");
   tft.setCursor(20,40);
   int p=*pos*5+20;
   //tft.print((p>0)?p:0,DEC);
  sprintf(set_buffer,"%d ",p);
   tft.print((char *) set_buffer);
    } // if
}
