#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <math.h>


#define VBATPIN A7
#define BME_CS 6
     
Adafruit_NeoPixel_ZeroDMA pixels(7, 5, NEO_GRBW); //please note, it's actually GRBW >_>)a
Adafruit_BME280 bme(BME_CS);

int interDigitDelay=600;

//bytemasking for base7-coded decimal
//center LED represents 7, outer LEDs all count as 1
//  
//       o             o             o             -             o             o
//   -       -     -       -     -       -     o       o     o       o     o       o
//       -             -             -             -             -             -
//   -       -     -       -     o       o     o       o     o       o     o       o
//       -             o             -             -             -             o
//
//       1             2             3             4             5             6
//
//
//  
//       -             o             o             o             o
//   -       -     -       -     -       -     -       -     o       o
//       o             o             o             o             o
//   -       -     -       -     -       -     o       o     o       o
//       -             -             o             -             o
//
//       7             8             9             0             .
//
//
byte mask = 1;
byte seven1   =   2;  // 0000010
byte seven2   =  18;  // 0010010
byte seven3   =  42;  // 0101010
byte seven4   = 108;  // 1101100
byte seven5   = 110;  // 1101110
byte seven6   = 126;  // 1111110
byte seven7   =   1;  // 0000001
byte seven8   =   3;  // 0000011
byte seven9   =  19;  // 0010011
byte seven0   =  43;  // 0101011
byte sevendot = 127;  // 1111111 



//constants, do not change
float lambdaHalpha = 656.46;

float k1  = 1.16705214528e+03;
float k2  = -7.24213167032e+05;
float k3  = -1.70738469401e+01;
float k4  = 1.20208247025e+04;
float k5  = -3.23255503223e+06;
float k6  = 1.49151086135e+01;
float k7  = -4.82326573616e+03;
float k8  = 4.05113405421e+05;
float k9  = -2.38555575678e-01;
float k10 = 6.50175348448e+02;

float alpha1 = 1.00062;
float beta1 = 3.14e-8;
float gamma1 = 5.60e-7;

float CO2ppm = 420.0;

float w0 = 295.235;
float w1 = 2.6422;
float w2 = -0.03238;
float w3 = 0.004028;
float ck0 = 238.0185;
float ck1 = 5792105.0;
float ck2 = 57.362;
float ck3 = 167917.0;
float a0 = 1.58123e-6;
float a1 = -2.9331e-8;
float a2 = 1.1043e-10;
float b0 = 5.707e-6;
float b1 = -2.051e-8;
float c0 = 1.9898e-4;
float c1 = -2.376e-6;
float d = 1.83e-11;
float e = -0.765e-8;
float pr1 = 101325.0;
float tr1 = 288.15;
float Za = 0.9995922115;
float rhovs = 0.00985938;
float R = 8.314472;
float Mv = 0.018015;

float currentTemp = 0.0;
float currentPres = 0.0;
float currentH2O = 0.0;
float currentMolarFraction = 0.0;
float currentIoR = 0.0;

// air-gap spacing of etalon, in nm.  Actual spacing will be some multiple of this value doubled.
// calibration routine will store this value on SD card or FRAM for future lookup
float etalonFactor = 328.14;

int loopCount = 1;

void setup() {
  pixels.begin();
  pixels.setBrightness(32); // 1/3 brightness
  bme.begin();
}

void loop() {
  displayInt(192, 255, 0,loopCount);
  pixels.clear();
  pixels.show();
  delay(2000);
  
  displayVoltage();
  pixels.clear();
  pixels.show();
  delay(2000);
  
  displayTemperature();
  pixels.clear();
  pixels.show();
  delay(2000);

  displayPressure();
  pixels.clear();
  pixels.show();
  delay(2000);

  displayHRel();
  pixels.clear();
  pixels.show();
  delay(2000);
  
  // Calculate Index of Refraction
  currentTemp = bme.readTemperature();
  Serial.print("Temp: " ); Serial.println(currentTemp,2);
  currentPres = float(bme.readPressure());
  Serial.print("Pres: " ); Serial.println(currentPres,2);
  currentH2O = bme.readHumidity();
  Serial.print("HRel: " ); Serial.println(currentH2O,2);
  currentMolarFraction = relHumidity2MoleFraction(currentH2O, currentPres, currentTemp);
  Serial.print("Xv  : " ); Serial.println(currentMolarFraction,8);
  currentIoR = refractiveIndex(lambdaHalpha, currentTemp, currentPres, currentMolarFraction);
  Serial.print("n   : " ); Serial.println(currentIoR,8);
  displayFloat(255,255,255,currentIoR,8);
  pixels.clear();
  pixels.show();
  delay(2000);


  displayLambda(etalonFactor, currentIoR);
  pixels.clear();
  pixels.show();
  delay(2000);
  
  pixels.clear();
  pixels.show();
  delay(2000);   
  loopCount++;
}


char *ftoa(char *buffer, double d, int precision) {
  long wholePart = (long) d;
  // Deposit the whole part of the number.
  itoa(wholePart,buffer,10);
  // Now work on the faction if we need one.
  if (precision > 0) {
    // We do, so locate the end of the string and insert
    // a decimal point.
    char *endOfString = buffer;
    while (*endOfString != '\0') endOfString++;
    *endOfString++ = '.';
    // Now work on the fraction, be sure to turn any negative
    // values positive.
    if (d < 0) {
      d *= -1;
      wholePart *= -1;
    }
    double fraction = d - wholePart;
    while (precision > 0) {
      // Multipleby ten and pull out the digit.
      fraction *= 10;
      wholePart = (long) fraction;
      *endOfString++ = '0' + wholePart;
      // Update the fraction and move on to the
      // next digit.
      fraction -= wholePart;
      precision--;
    }
    // Terminate the string.
    *endOfString = '\0';
  }
   return buffer;
}


void displayFloat(uint8_t red, uint8_t green, uint8_t blue, float value, int precision) {
  char number[16];
  ftoa(number,value,precision);
  int i;
  char digit;
  for (i=0;i<strlen(number);i++) {
    digit = number[i] - 48;
    displayDigit(red, green, blue, digit);
  }
}

void displayInt(uint8_t red, uint8_t green, uint8_t blue, int value) {
  char number[16];
  itoa(value,number,10);
  int i;
  char digit;
  for (i=0;i<strlen(number);i++) {
    digit = number[i] - 48;
    displayDigit(red, green, blue, digit);
  }
}

void displayDigit(uint8_t red, uint8_t green, uint8_t blue, uint8_t digit) {
  byte glyph = sevendot;
  uint8_t i = 0;
  switch(digit) {
    case 0: 
      glyph = seven0;
      break;
    case 1:
      glyph = seven1;
      break;
    case 2:
      glyph = seven2;
      break;
    case 3:
      glyph = seven3;
      break;            
    case 4:
      glyph = seven4;
      break;
    case 5:
      glyph = seven5;
      break;
    case 6:
      glyph = seven6;
      break;  
    case 7:
      glyph = seven7;
      break;  
    case 8:
      glyph = seven8;
      break;  
    case 9:
      glyph = seven9;
      break;
    default:
      glyph = sevendot;
      break;
  }
  pixels.clear();
  pixels.show();
  delay(interDigitDelay/10);
  for (mask = 00000001; mask>0; mask <<= 1) {
    if (glyph & mask){
      pixels.setPixelColor(i, red, green, blue, 0);      
    }
    else {
      pixels.setPixelColor(i, 0, 0, 0, 0);
    }
    i++;
  }
  pixels.show();
  delay(interDigitDelay);
}

void displayVoltage() {
  float voltage = analogRead(VBATPIN);
  voltage += analogRead(VBATPIN);
  voltage += analogRead(VBATPIN);
  voltage += analogRead(VBATPIN);
  voltage /= 2.0;
  voltage *= 3.3;
  voltage /= 1024;
  displayFloat(0,255,0,voltage,2);
}

void displayTemperature() {
  float temperature = bme.readTemperature();
  displayFloat(255,0,0,temperature,2);
}

void displayPressure() {
  float atmospheres = float(bme.readPressure()) / 101325.0;
  displayFloat(128,0,255,atmospheres,4);
}

void displayHRel() {
  float HRel = bme.readHumidity();
  displayFloat(0,255,128,HRel,2);
}

void displayLambda(float etalonFactor, float indexOfRefraction) {
  float lambdaVac = 2 * etalonFactor * indexOfRefraction;
  displayFloat(255,96,0,lambdaVac,3);
}

float psv_water (float temperature) {
  // saturation pressure over water at temp
  float T = temperature + 273.15;
  float omega = T + k9 / (T - k10);
  float A = sq(omega) + k1 * omega + k2;
  float B = k3 * sq(omega) + k4 * omega + k5;
  float C = k6 * sq(omega) + k7 * omega + k8;
  float X = -B + sqrt(sq(B) - 4*A*C);
  float psv = pow(2.0 * C / X, 4) * 1.0e6;
  return psv;
}

float enh_factor(float p, float t) {
  return alpha1 + beta1 * p + gamma1 * sq(t);
}

float relHumidity2MoleFraction(float RH, float pressure, float temperature) {
  return (RH/100.0) * enh_factor(pressure, temperature) * psv_water(temperature)/pressure;
}

float refractiveIndex(float lambda, float temperature, float pressure, float moleFraction) {
  float wavelength = lambda * 1.0e-3;
  float S = 1.0 / sq(wavelength);

  float ras = 1e-8 * ((ck1 / (ck0 - S)) + (ck3 / (ck2 - S)));
  float rvs = 1.022e-8 * (w0 + w1 * S + w2 * sq(S) + w3 * pow(S,3));
  float Ma = 0.0289635 + 1.2011e-8 * (CO2ppm - 400);
  float raxs = ras * (1 + 5.34e-7 * (CO2ppm - 450));
  float T = temperature + 273.15;

  float Zm = 1 - (pressure / T) * (a0 + a1 * temperature + a2 * sq(temperature) + (b0 + b1 * temperature) * moleFraction + (c0 + c1 * temperature) * sq(moleFraction));
  Zm += sq(pressure / T) * (d + e * sq(moleFraction));
  float rhoaxs = pr1 * Ma / (Za * R * tr1);
  float rhov = moleFraction * pressure * Mv / (Zm * R * T);
  float rhoa = (1 - moleFraction) * pressure * Ma / (Zm * R * T);
  float n = 1.0 + (rhoa / rhoaxs) * raxs + (rhov / rhovs) * rvs;
  return n;
}


void rainbow(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}
 
void rainbowCycle(uint8_t wait) {
  uint16_t r, j;
 
  for(j=0; j<256*6; j++) { // 6 cycles of all colors on wheel
    for(r=0; r< pixels.numPixels(); r++) {
      pixels.setPixelColor(r, Wheel(((r * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}
void rainbowCycleslow(uint8_t wait) {
  uint16_t r, j;
 
  for(j=0; j<256*3; j++) { // 3 cycles of all colors on wheel
    for(r=0; r< pixels.numPixels(); r++) {
      pixels.setPixelColor(r, Wheel(((r * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}
void rainbowHold(uint8_t wait) {
  uint16_t r, j;
 
  for(j=0; j<256*1; j++) { // 3 cycles of all colors on wheel
    for(r=0; r< pixels.numPixels(); r++) {
      pixels.setPixelColor(r, Wheel(((r * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}
 
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
