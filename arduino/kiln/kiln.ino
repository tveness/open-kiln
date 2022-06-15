
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "max6675.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Bounce2.h>

int thermoDO = D6;
int thermoCS = D8;
int thermoCLK = D5;
char Tbuffer[50];
char set_buffer[50];
bool short_press=false;
bool long_press=false;
bool long_press_action=false;


class pid {
  float duty;
  float kp;
  float ki;
  float kd;
  float e_p;
  float last_time;

  float i_output;
  float i_max;
  float i_offset;
  float last_actual;
  
  
  public:
    float get_duty(void);
    pid (float, float, float, float);
    float update_duty(float, float);
};

pid::pid(float kp_init,float ki_init,float kd_init, float i_offset_init){
  kp=kp_init;
  ki=ki_init;
  kd=kd_init;
  i_output=0;
  i_offset=i_offset_init;
  i_max = 0.1;
  last_actual=0;
  last_time=millis();
  duty=0.0;
  e_p=0.0;
}

float pid::get_duty() {
  return duty;
}

float pid::update_duty(float target, float actual){
  float now=millis();
  float dt = (now - last_time)/1000.0;
  
  float error=target - actual;
  

 
  i_output += error * ki*dt;
  if (i_output > i_max) {
    i_output = i_max;
  }
  if (i_output < -i_max) {
    i_output = -i_max;
  }
  float d_out = kd*(actual-last_actual)/dt;
  Serial.print("p:");
  Serial.print(kp*error);
  Serial.print(",i:");
  Serial.print(i_output);
  Serial.print(",d:");
  Serial.print(d_out);
  float output = kp*error + i_output + d_out;
  

  last_actual = actual;
  last_time=now;

  if (output < 0) {
    output= 0.0;
  }
  else if (output > 1) {
    output= 1.0;
  }
  duty = output;
  return output;
 
  
}



MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

/*************************************/


#define TFT_CS         D8
//#define TFT_RST        RST
#define TFT_RST         -1                                          //And connect to arduino reset
#define TFT_DC         D3
#define TFT_MOSI D7  // Data out
#define TFT_SCLK D5  // Clock out

// For ST7735-based displays, we will use this call
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

/***************************/

/////////////////////////////////////////////////////////////////


#define PIN_IN1 D1
#define PIN_IN2 D2
#define BOUNCE_PIN D4

#define LED_PIN D0

#define T_OFFSET 20
#define T_STEP 5
#define T_MIN 5
#define T_MAX 1200

#define ROTARY_MIN ( (T_MIN-T_OFFSET)/T_STEP )
#define ROTARY_MAX ( (T_MAX-T_OFFSET)/T_STEP )

RotaryEncoder *encoder = nullptr;
Bounce bounce = Bounce();


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
pid pidc(0.01,1e-5,-0.01,0.0);
unsigned long tt;
int pos = -1;
float T = T_OFFSET+10;
unsigned long duty_period = 10;//duty period in seconds
unsigned long duty_cutoff_lower=1.0;
unsigned long duty_cutoff_higher = 59.0;
unsigned long duty_counter = 0.0;
float duty_cycle = 0.0;
void setup() {
  bounce.attach( BOUNCE_PIN, INPUT );
  bounce.interval(5); // interval in ms
  
  
  Serial.begin(9600);

  
  pinMode(LED_PIN, OUTPUT);
  //analogWrite(LED_PIN,0 );
  digitalWrite(LED_PIN,HIGH);
  Serial.println("MAX6675 test");
  //SPI.begin();
  
  tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
  
  Serial.println(F("Initialized"));

  tt = millis();
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);

// wait for MAX chip to stabilize
  delay(500);
   // use TWO03 mode when PIN_IN1, PIN_IN2 signals are both LOW or HIGH in latch position.
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);


  // LED
  duty_counter = millis();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  write_set(0,&pos);
  
}




void loop() {
  // basic readout test, just print the current temp
   //tft.fillScreen(ST77XX_BLACK);


  bounce.update();
  int debouncedState = bounce.read();
  
   if ( debouncedState == LOW && bounce.currentDuration() > 3 ) {
   short_press = true;
   
  } 
  if (debouncedState == LOW && bounce.currentDuration() > 1000 && !long_press){

    long_press = true;
    long_press_action = true;
    short_press = false;
  }

  if (debouncedState == HIGH && long_press){
    long_press=false;
    short_press=false;
  }

  if (long_press_action) {
    Serial.println("LONG PRESS");
    long_press_action=false;
  }
  
 
  if (debouncedState == HIGH && short_press ){
    short_press = false;
    Serial.println("CLICK!");
  }


  encoder->tick(); // just call tick() to check the state.

  unsigned long now = millis();
  unsigned long elapsed = now - tt;

  
  // Reset counter every period
  if ( (now-duty_counter) > duty_period*1000) {
    duty_counter =now;
    digitalWrite(LED_PIN,LOW);
  }
  
  if ( (now - duty_counter) > duty_cycle * duty_period*1000 ){
   digitalWrite(LED_PIN,HIGH); 
  }

  float target=pos*T_STEP+T_OFFSET;
   
   if (elapsed > 500 ){
    //delay(1000);
    T = thermocouple.readCelsius();
    write_T(&tt);
    duty_cycle = pidc.update_duty(target, T);
    Serial.print(",C:");
    Serial.print(T);
    Serial.print(",target:"); 
    Serial.print(target);
   //Serial.println(thermocouple.readCelsius());
   //Serial.print(", F:");
   //Serial.print(thermocouple.readFahrenheit());
   Serial.print(",duty:");
    Serial.println(duty_cycle);

    
    
    }
  

  
  /*
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
  */
 
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
   *tt=millis();
   

}

void write_set(int newPos, int *pos){
if (*pos != newPos) {
/*    Serial.print("pos:");
    Serial.print(newPos);
    Serial.print(" dir:");
    Serial.println((int)(encoder->getDirection()));
   
     */
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
