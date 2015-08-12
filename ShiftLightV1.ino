/* ARDUINO RPM TACHOMETER MULTI-DISPLAY 
Written by Jonduino 
10-13-2013 


********************* Version NOTES ******************** 
/* v1.0 BETA 11/17/2013 -- Initial Release 
** v1.1 03/09/2014 - 
** Fixed bug with flasher that didn't correspond to brightness value 
** Improved sleep function, now it shuts off after 5-seconds of engine OFF 
** Other minor improvements. 
** 
** 
*/ 





// declares Digital Pin 6 as the output for the NeoPixel Display 
#define PIN 6 

// Include these libraries 
#include <Wire.h> 
#include "Adafruit_LEDBackpack.h" 
#include "Adafruit_GFX.h" 
#include <Adafruit_NeoPixel.h> 

#include <EEPROM.h> 
#include "EEPROMAnything.h" 

// Create a 15 bit color value from R,G,B 
unsigned int Color(byte r, byte g, byte b) 
{ 
//Take the lowest 5 bits of each value and append them end to end 
return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F); 
} 


// configure the neopixel strip and the matrix 7-seg 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800); 
Adafruit_7segment matrix = Adafruit_7segment(); 


//configuration for the Tachometer variables 
const int sensorPin = 2; 
const int ledPin = 13; 
const int sensorInterrupt = 0; 
const int timeoutValue = 5; 
volatile unsigned long lastPulseTime; 
volatile unsigned long interval = 0; 
volatile int timeoutCounter; 

int rpm; 
int rpmlast = 3000; 
int rpm_interval = 3000; 

int rpmled; 
int activation_rpm; 
int shift_rpm; 
int segment_int; 

int menu_enter = 0; 


//These are stored memory variables for adjusting the (5) colors, activation rpm, shift rpm, brightness 
//Stored in EEPROM Memory 
int c1; 
int c2; 
int c3; 
int c4; 
int c5; 
int r1; 
int r2; 
int brightval; //7-seg brightness 
int sb; //strip brightness 


// COLOR VARIABLES - for use w/ the strips and translated into 255 RGB colors 
uint32_t color1; 
uint32_t color2; 
uint32_t color3; 
uint32_t flclr1; 
uint32_t flclr2; 



// ROTARY ENCODER VARIABLES 
int button_pin = 4; 
int menuvar; 
int val; 
int rotaryval = 0; 




// CONFIGURATION FOR THE ROTARY ENCODER 
// Half-step mode? 
//#define HALF_STEP 
// Arduino pins the encoder is attached to. Attach the center to ground. 
#define ROTARY_PIN1 10 
#define ROTARY_PIN2 11 
// define to enable weak pullups. 
#define ENABLE_PULLUPS 

#ifdef HALF_STEP 
// Use the half-step state table (emits a code at 00 and 11) 
const char ttable[6][4] = { 
{0x3 , 0x2, 0x1, 0x0}, {0x83, 0x0, 0x1, 0x0}, 
{0x43, 0x2, 0x0, 0x0}, {0x3 , 0x5, 0x4, 0x0}, 
{0x3 , 0x3, 0x4, 0x40}, {0x3 , 0x5, 0x3, 0x80} 
}; 
#else 
// Use the full-step state table (emits a code at 00 only) 
const char ttable[7][4] = { 
{0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x40}, 
{0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0}, 
{0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x80}, 
{0x6, 0x5, 0x4, 0x0}, 
}; 
#endif 
volatile char state = 0; 

/* Call this once in setup(). */ 
void rotary_init() { 
pinMode(ROTARY_PIN1, INPUT); 
pinMode(ROTARY_PIN2, INPUT); 
#ifdef ENABLE_PULLUPS 
digitalWrite(ROTARY_PIN1, HIGH); 
digitalWrite(ROTARY_PIN2, HIGH); 
#endif 
} 


/* Read input pins and process for events. Call this either from a 
* loop or an interrupt (eg pin change or timer). 
* 
* Returns 0 on no event, otherwise 0x80 or 0x40 depending on the direction. 
*/ 
char rotary_process() { 
char pinstate = (digitalRead(ROTARY_PIN2) << 1) | digitalRead(ROTARY_PIN1); 
state = ttable[state][pinstate]; 
return (state & 0xc0); 
} 



//This subroutine reads the stored variables from memory 
void getEEPROM(){ 

brightval = EEPROM.read(0); 
sb = EEPROM.read(1); 
c1 = EEPROM.read(2); 
c2 = EEPROM.read(3); 
c3 = EEPROM.read(4); 
c4 = EEPROM.read(5); 
c5 = EEPROM.read(6); 
r1 = EEPROM.read(7); 
r2 = EEPROM.read(8); 

} 


//This subroutine writes the stored variables to memory 
void writeEEPROM(){ 

EEPROM.write(0, brightval); 
EEPROM.write(1, sb); 
EEPROM.write(2, c1); 
EEPROM.write(3, c2); 
EEPROM.write(4, c3); 
EEPROM.write(5, c4); 
EEPROM.write(6, c5); 
EEPROM.write(7, r1); 
EEPROM.write(8, r2); 

} 




//SETUP TO CONFIGURE THE ARDUINO AND GET IT READY FOR FIRST RUN 
void setup() { 
// Serial.begin(9600); 
// Serial.println("7 Segment Backpack Test"); 
matrix.begin(0x70); 
strip.begin(); 
strip.show(); // Initialize all pixels to 'off' 

// config for the Tach 
pinMode(sensorPin, INPUT); 
// digitalWrite(sensorPin, HIGH); // enable internal pullup (if Hall sensor needs it) 
pinMode(ledPin, OUTPUT); 
attachInterrupt(sensorInterrupt, sensorIsr, RISING); 
// Serial.begin(9600); 
lastPulseTime = micros(); 
timeoutCounter = 0; 

//ROTARY ENCODER 
pinMode(button_pin, INPUT_PULLUP); 
rotary_init(); 

//get stored variables 
getEEPROM(); 

//translate the stored 255 color variables into meaningful RGB colors 
color1 = load_color(c1); 
delay(10); 
color2 = load_color(c2); 
delay(10); 
color3 = load_color(c3); 
delay(10); 
flclr1 = load_color(c4); 
delay(10); 
flclr2 = load_color(c5); 
delay(10); 
} 


//each time the interrupt receives a rising tach signal, it'll run this subroutine 
void sensorIsr() 
{ 
unsigned long now = micros(); 
interval = now - lastPulseTime; 
lastPulseTime = now; 
timeoutCounter = timeoutValue; 
} 





// Normal Running Operations 
void loop() { 

if (timeoutCounter != 0) 
{ 
--timeoutCounter; 

//This calculation will need to be calibrated for each application 
//float rpm = 60e6/(float)interval; // ORIGINAL 
rpm = 21e6/interval; // CALIBRATED 
// Serial.print(rpm); 
// matrix.println(rpm); 
// matrix.writeDisplay(); 

} 

//remove erroneous results 
if (rpm > rpmlast+500){rpm=rpmlast;} 

rpmlast = rpm; 

//Poll the Button, if pushed, cue animation and enter menu subroutine 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
clearStrip(); 

//Ascend strip 
for (int i=0; i<9; i++){ 
strip.setPixelColor(i, strip.Color(0, 0, 25)); 
strip.setPixelColor(16-i, strip.Color(0, 0, 25)); 
strip.show(); 
delay(35); 
} 
// Descend Strip 
for (int i=0; i<9; i++){ 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.setPixelColor(16-i, strip.Color(0, 0, 0)); 
strip.show(); 
delay(35); 
} 

menuvar=1; 
menu(); 
} 


//Write the BarGraph Display 
// Translate our stored RPM values into meaningful RPM values (incriments of 30 rpm) 
activation_rpm = r1*30; 
shift_rpm = r2*30; 

//Let's keep this RPM value under control, between 0 and 8000 
rpm = constrain (rpm, 0, 8000); 

// given the nature of the RPM interrupt reader, a zero reading will produce a max result 
// this fixes this quirk 
if (rpm==8000){rpm=0;} 



// if the engine is running, print the rpm value to the 7-seg display 
//if not, shut her down 

if ((micros() - lastPulseTime) < 5e6 ) { 
// Serial.print(rpm); 
matrix.println(rpm); 
matrix.setBrightness(brightval); 
matrix.writeDisplay(); 
delay(10); 
} 
else { 
matrix.clear(); 
matrix.writeDisplay(); 
} 





// divide the LED strip into segments based on our start-point (act rpm) and end point (shift rpm) 
segment_int = (shift_rpm - activation_rpm) / 8; 
// Serial.print(" RPMLED: "); 
// Serial.println(segment_int); 


if (rpm > activation_rpm){ 
strip.setPixelColor(7, color1); 
strip.setPixelColor(8, color1); 
} 
else{ 
strip.setPixelColor(7, strip.Color(0, 0, 0)); 
strip.setPixelColor(8, strip.Color(0, 0, 0)); 
} 



if ((rpm-activation_rpm) > (segment_int)) { 
strip.setPixelColor(6, color1); 
strip.setPixelColor(9, color1); 
} 
else { 
strip.setPixelColor(6, strip.Color(0, 0, 0)); 
strip.setPixelColor(9, strip.Color(0, 0, 0)); 
} 



if ((rpm-activation_rpm) > (segment_int * 2)) { 
strip.setPixelColor(5, color1); 
strip.setPixelColor(10, color1); 
} 
else { 
strip.setPixelColor(5, strip.Color(0, 0, 0)); 
strip.setPixelColor(10, strip.Color(0, 0, 0)); 
} 



if ((rpm-activation_rpm) > (segment_int * 3)) { 
strip.setPixelColor(4, color1); 
strip.setPixelColor(11, color1); 
} 
else { 
strip.setPixelColor(4, strip.Color(0, 0, 0)); 
strip.setPixelColor(11, strip.Color(0, 0, 0)); 
} 


if ((rpm-activation_rpm) > (segment_int * 4)) { 
strip.setPixelColor(3, color2); 
strip.setPixelColor(12, color2); 
} 
else { 
strip.setPixelColor(3, strip.Color(0, 0, 0)); 
strip.setPixelColor(12, strip.Color(0, 0, 0)); 
} 


if ((rpm-activation_rpm) > (segment_int * 5)) { 
strip.setPixelColor(2, color2); 
strip.setPixelColor(13, color2); 
} 
else { 
strip.setPixelColor(2, strip.Color(0, 0, 0)); 
strip.setPixelColor(13, strip.Color(0, 0, 0)); 
} 


if ((rpm-activation_rpm) > (segment_int * 6)) { 
strip.setPixelColor(1, color3); 
strip.setPixelColor(14, color3); 
} 
else { 
strip.setPixelColor(1, strip.Color(0, 0, 0)); 
strip.setPixelColor(14, strip.Color(0, 0, 0)); 
} 



if ((rpm-activation_rpm) > (segment_int * 7)) { 
strip.setPixelColor(0, color3); 
strip.setPixelColor(15, color3); 
} 
else { 
strip.setPixelColor(0, strip.Color(0, 0, 0)); 
strip.setPixelColor(15, strip.Color(0, 0, 0)); 
} 


// SHIFT!!!!! Flasher Function 
if (rpm >= shift_rpm) { 


for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr1); 
} 
strip.show(); 
delay(50); 


for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr2); 
} 
strip.show(); 
delay(50); 

} 

strip.show(); 


} 





// MENU SYSTEM 
void menu(){ 

//this keeps us in the menu 
while (menuvar == 1){ 


// This little bit calls the rotary encoder 

int result = rotary_process(); 

if (result == -128){ 
rotaryval--; 
} 

if (result == 64){ 
rotaryval++; 
} 


//matrix.println(rotaryval); 
//matrix.writeDisplay(); 

rotaryval = constrain(rotaryval, 0, 8); 


switch (rotaryval){ 

case 0: //Menu Screen. Exiting saves variables to EEPROM 
matrix.writeDigitRaw(0,0x39); //e 
matrix.writeDigitRaw(1,0x9); //x 
matrix.writeDigitRaw(3,0x9); //i 
matrix.writeDigitRaw(4,0xF); //t 

//Poll the Button to exit 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
rotaryval = 0; 
menuvar=0; 
writeEEPROM(); 
getEEPROM(); 

//Ascend strip 
for (int i=0; i<9; i++){ 
strip.setPixelColor(i, strip.Color(25, 25, 25)); 
strip.setPixelColor(16-i, strip.Color(25, 25, 25)); 
strip.show(); 
delay(35); 
} 

for (int i=0; i<9; i++){ 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.setPixelColor(16-i, strip.Color(0, 0, 0)); 
strip.show(); 
delay(35); 
} 




} 

break; 


case 1: //Adjust the global brightness 
matrix.writeDigitRaw(0,0x0); // 
matrix.writeDigitRaw(1,0x7C); //b 
matrix.writeDigitRaw(3,0x50); //r 
matrix.writeDigitRaw(4,0x78); //t 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 


while (menu_enter == 1){ 

int bright = rotary_process(); 

if (bright == -128){ 
brightval--; 
sb++; 
} 
if (bright == 64){ 
brightval++; 
sb--; 
} 
brightval = constrain (brightval, 0, 15); 
sb = constrain(sb, 1, 15); 

color1 = load_color(c1); 
color2 = load_color(c2); 
color3 = load_color(c3); 
flclr1 = load_color(c4); 
flclr2 = load_color(c5); 

matrix.setBrightness(brightval); 
matrix.println(brightval); 
matrix.writeDisplay(); 

strip.setPixelColor(0, color3); 
strip.setPixelColor(1, color3); 
strip.setPixelColor(2, color2); 
strip.setPixelColor(3, color2); 
strip.setPixelColor(4, color1); 
strip.setPixelColor(5, color1); 
strip.setPixelColor(6, color1); 
strip.setPixelColor(7, color1); 
strip.setPixelColor(8, color1); 
strip.setPixelColor(9, color1); 
strip.setPixelColor(10, color1); 
strip.setPixelColor(11, color1); 
strip.setPixelColor(12, color2); 
strip.setPixelColor(13, color2); 
strip.setPixelColor(14, color3); 
strip.setPixelColor(15, color3); 
strip.show(); 





if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, color1); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 


} 

break; 


case 2: // ACTIVATION RPM 
matrix.writeDigitRaw(0,0x0); // 
matrix.writeDigitRaw(1,0x77); //A 
matrix.writeDigitRaw(3,0x39); //C 
matrix.writeDigitRaw(4,0x78); //t 

if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 


while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
r1--; 
} 
if (coloradjust1 == 64){ 
r1++; 
} 

r1 = constrain(r1, 0, 255); 

matrix.println(r1*30); 
matrix.writeDisplay(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, strip.Color(50, 50, 50)); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 

break; 


case 3: // SHIFT RPM 
matrix.writeDigitRaw(0,0x6D); //S 
matrix.writeDigitRaw(1,0x76); //H 
matrix.writeDigitRaw(3,0x71); //F 
matrix.writeDigitRaw(4,0x78); //t 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 


while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
r2--; 
} 
if (coloradjust1 == 64){ 
r2++; 
} 

r2 = constrain(r2, 0, 255); 

matrix.println(r2*30); 
matrix.writeDisplay(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, strip.Color(50, 50, 50)); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 


break; 



case 4: //Adjust Color #1 

matrix.writeDigitRaw(0,0x39); //C 
matrix.writeDigitRaw(1,0x38); //L 
matrix.writeDigitRaw(3,0x33); //R 
matrix.writeDigitRaw(4,0x6); //1 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 

while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
c1--; 
} 
if (coloradjust1 == 64){ 
c1++; 
} 

c1 = constrain(c1, 0, 255); 

color1 = load_color(c1); 


for(int i=4; i<12; i++) { 
strip.setPixelColor(i, color1); 
} 

strip.show(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, color1); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 
break; 




case 5: //Adjust Color #2 
matrix.writeDigitRaw(0,0x39); //C 
matrix.writeDigitRaw(1,0x38); //L 
matrix.writeDigitRaw(3,0x33); //R 
matrix.writeDigitRaw(4,0x5B); //2 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 

while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
c2--; 
} 
if (coloradjust1 == 64){ 
c2++; 
} 

c2 = constrain(c2, 0, 255); 

color2 = load_color(c2); 

strip.setPixelColor(2, color2); 
strip.setPixelColor(3, color2); 
strip.setPixelColor(12, color2); 
strip.setPixelColor(13, color2); 
strip.show(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, color2); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 


break; 

case 6: //Adjust Color #3 
matrix.writeDigitRaw(0,0x39); //C 
matrix.writeDigitRaw(1,0x38); //L 
matrix.writeDigitRaw(3,0x33); //R 
matrix.writeDigitRaw(4,0x4F); //3 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 

while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
c3--; 
} 
if (coloradjust1 == 64){ 
c3++; 
} 

c3 = constrain(c3, 0, 255); 

color3 = load_color(c3); 

strip.setPixelColor(0, color3); 
strip.setPixelColor(1, color3); 
strip.setPixelColor(14, color3); 
strip.setPixelColor(15, color3); 
strip.show(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, color3); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 

break; 

case 7: //Adjust Color #4 
matrix.writeDigitRaw(0,0x6D); //S 
matrix.writeDigitRaw(1,0x39); //C 
matrix.writeDigitRaw(3,0x0); // 
matrix.writeDigitRaw(4,0x6); //1 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 

while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
c4--; 
} 
if (coloradjust1 == 64){ 
c4++; 
} 

c4 = constrain(c4, 0, 255); 

flclr1 = load_color(c4); 


for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr1); 
} 

strip.show(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr1); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 


break; 

case 8: //Adjust Color #5 
matrix.writeDigitRaw(0,0x6D); //S 
matrix.writeDigitRaw(1,0x39); //C 
matrix.writeDigitRaw(3,0x0); // 
matrix.writeDigitRaw(4,0x5B); //2 

//Poll the Button to Enter 
if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 1; 
} 

while (menu_enter == 1){ 

int coloradjust1 = rotary_process(); 

if (coloradjust1 == -128){ 
c5--; 
} 
if (coloradjust1 == 64){ 
c5++; 
} 

c5 = constrain(c5, 0, 255); 

flclr2 = load_color(c5); 


for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr2); 
} 

strip.show(); 


if (digitalRead(button_pin) == LOW){ 
delay(250); 
menu_enter = 0; 
clearStrip(); 
strip.show(); 
for(int i=0; i<strip.numPixels(); i++) { 
strip.setPixelColor(i, flclr2); 
strip.show(); 
delay(15); 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 

} 
} 

break; 



} 


matrix.writeDisplay(); 


} 


} 




//This sub clears the strip to all OFF 

void clearStrip() { 
for( int i = 0; i<strip.numPixels(); i++){ 
strip.setPixelColor(i, strip.Color(0, 0, 0)); 
strip.show(); 
} 
} 


uint32_t load_color(int cx){ 

unsigned int r,g,b; 

if (cx == 0){ 
r = 0; 
g = 0; 
b = 0; 
} 


if (cx>0 && cx<=85){ 
r = 255-(cx*3); 
g = cx*3; 
b=0; 
} 

if (cx>85 && cx < 170){ 
r = 0; 
g = 255 - ((cx-85)*3); 
b = (cx-85)*3; 
} 

if (cx >= 170 && cx<255){ 
r = (cx-170)*3; 
g = 0; 
b = 255 - ((cx-170)*3); 
} 

if (cx == 255){ 
r=255; 
g=255; 
b=255; 
} 


r = (r/sb); 
g = (g/sb); 
b = (b/sb); 


return strip.Color(r,g,b); 

}
