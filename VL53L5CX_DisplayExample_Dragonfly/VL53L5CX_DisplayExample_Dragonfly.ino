/*
*  VL53L5CX ULD basic example    
*
*  Copyright (c) 2021 Kris Winer and Simon D. Levy
*
*  MIT License
*/

#include "Wire.h"

#include "Debugger.hpp"
#include "VL53L5cx.h"
#include <Adafruit_GFX.h>    // Core graphics library, install from Arduino IDE board manager
#include <Adafruit_ST7735.h> // Hardware-specific library, install from Arduino IDE board manager
#include "SPI.h"
#include "ColorDisplay.h"

// Ladybug STM32L432 development board connections for display
#define sclk 13  // SCLK can also use pin 14
#define mosi 11  // MOSI can also use pin 7
#define cs   10  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define dc   5   // but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define rst  4   // RST can use any pin
#define sdcs 1   // CS for SD card, can use any pin

static const uint8_t LED_PIN = 25;
static const uint8_t INT_PIN =  8;
static const uint8_t LPN_PIN =  3;

uint8_t  status, isAlive = 0, isReady, error, pixels, resolution;
uint32_t integration_time_ms;
  
volatile bool VL53L5_intFlag = false;

static VL53L5CX_Configuration Dev = {};  // Sensor configuration
static VL53L5CX_ResultsData Results = {};  // Results data from VL53L5CX

// Configure VL53L5 measurement parameters
const uint8_t continuous_mode = 0;
const uint8_t autonomous_mode = 1;
const uint8_t VL53L5_mode = autonomous_mode; // either or

const uint8_t resolution_4x4 = 0;
const uint8_t resolution_8x8 = 1;
const uint8_t VL53L5_resolution = resolution_8x8; // either or

const uint8_t VL53L5_freq = 2;     // Min freq is 1 Hz max is 15 Hz (8 x 8) or 60 Hz (4 x 4)
// Sum of integration time (1x for 4 x 4 and 4x for 8 x 8) must be 1 ms less than 1/freq, otherwise data rate decreased
// so integration time must be > 18 ms at 4x4, 60 Hz, for example
// the smaller the integration time, the less power used, the more noise in the ranging data
const uint8_t VL53L5_intTime = 10; // in milliseconds, settable only when in autonomous mode, otherwise a no op

uint16_t minRange, maxRange, tmpRange;

// Configure color display
uint16_t color;
uint8_t rgb, red, green, blue;

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

void setup(void)
{
    // Start serial debugging
    Serial.begin(115200);
    delay(4000);
    Serial.println("Serial begun!");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // start with led on, active HIGH

    pinMode(INT_PIN, INPUT);     // VL53L5CX interrupt pin

    pinMode(sdcs, INPUT_PULLUP);          // don't touch the SD card

    tft.initR(INITR_BLACKTAB);            // initialize a ST7735S chip, black tab
    tft.setRotation(3);                   // 0, 2 are portrait mode, 1,3 are landscape mode
    Serial.println("initialize display");

    // Print device banner
    for (uint8_t ii = 1; ii < 8; ii++)
    {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(20, tft.height()/2);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setTextColor(setColor[ii]);
    tft.println("VL53L5CX Camera");
    delay(250);
    }
    
    Wire.begin();                // Start I2C
    Wire.setClock(400000);       // Set I2C frequency at 400 kHz  
    delay(1000);

    I2Cscan(); // Should show 0x29 on the I2C bus

    Debugger::printf("starting\n\n");

    delay(1000);

    // Fill the platform structure with customer's implementation. For this
    // example, only the I2C address is used.
    Dev.platform.address = 0x29;

    // Reset the sensor by toggling the LPN pin
    Reset_Sensor(LPN_PIN);

/* (Optional) Set a new I2C address if the wanted address is different
  * from the default one (filled with 0x20 for this example).
  */
  //status = vl53l5cx_set_i2c_address(&Dev, 0x20);

    /*   Initialize the VL53L5CX   */
    // Check if there is a VL53L5CX sensor connected
    error = vl53l5cx_is_alive(&Dev, &isAlive);
    if(!isAlive || error) {
        Debugger::reportForever("VL53L5CX not detected at requested address");
    }
   
    if(isAlive) {
    // (Mandatory) Init VL53L5CX sensor
    error = vl53l5cx_init(&Dev);
    Serial.print("error = 0x"); Serial.println(error, HEX);
    if(error) {
        Debugger::reportForever("VL53L5CX ULD Loading failed");
    }

    Debugger::printf("VL53L5CX ULD ready ! (Version : %s)\n",
            VL53L5CX_API_REVISION);

    digitalWrite(LED_PIN, LOW); // turn off led when initiation successful
    }

   
  /*   Configure the VL53L5CX      */
 
  /* Set resolution. WARNING : As others settings depend to this
   * one, it must come first.
   */
  if(VL53L5_resolution == resolution_4x4){
    pixels = 16;
    status = vl53l5cx_set_resolution(&Dev, VL53L5CX_RESOLUTION_4X4);
    if(status)
    {
      printf("vl53l5cx_set_resolution failed, status %u\n", status);
    }
  }
  else
  {
    pixels = 64;
    status = vl53l5cx_set_resolution(&Dev, VL53L5CX_RESOLUTION_8X8);
    if(status)
    {
      printf("vl53l5cx_set_resolution failed, status %u\n", status);
    }
  }
  
  // Confirm resolution choice
  vl53l5cx_get_resolution(&Dev, &resolution);
  Serial.print("Resolution is "); Serial.println(resolution);

  /* Select operating mode */
  if(VL53L5_mode == autonomous_mode) {
    /* set autonomous ranging mode */
    status = vl53l5cx_set_ranging_mode(&Dev, VL53L5CX_RANGING_MODE_AUTONOMOUS);
    if(status)
    {
    printf("vl53l5cx_set_ranging_mode failed, status %u\n", status);
    }

  /* can set integration time in autonomous mode */
    status = vl53l5cx_set_integration_time_ms(&Dev, VL53L5_intTime); //  
    if(status)
    {
      printf("vl53l5cx_set_integration_time_ms failed, status %u\n", status);
    }
  }
  else 
  { 
    /* set continuous ranging mode, integration time is fixed in continuous mode */
    status = vl53l5cx_set_ranging_mode(&Dev, VL53L5CX_RANGING_MODE_CONTINUOUS);
    if(status)
    {
    printf("vl53l5cx_set_ranging_mode failed, status %u\n", status);
    }
  }

  /* Select data rate */
  status = vl53l5cx_set_ranging_frequency_hz(&Dev, VL53L5_freq);
  if(status)
  {
    printf("vl53l5cx_set_ranging_frequency_hz failed, status %u\n", status);
  }

  /* Select sharpener percent */
  status = vl53l5cx_set_sharpener_percent(&Dev, 10); // default is 5, valid input 0 - 99 (in percent)
  if(status)
  {
    printf("vl53l5cx_set_sharpener_percent failed, status %u\n", status);
  }

  /* Set target order to closest */
  status = vl53l5cx_set_target_order(&Dev, VL53L5CX_TARGET_ORDER_STRONGEST); // default is VL53L5CX_TARGET_ORDER_STRONGEST
  if(status)
  {
    printf("vl53l5cx_set_target_order failed, status %u\n", status);
  }

  /* Get current integration time */
  status = vl53l5cx_get_integration_time_ms(&Dev, &integration_time_ms);
  if(status)
  {
    printf("vl53l5cx_get_integration_time_ms failed, status %u\n", status);
  }
  printf("Current integration time is : %d ms\n", integration_time_ms);


  // Configure the data ready interrupt
  attachInterrupt(INT_PIN, VL53L5_intHandler, FALLING);
  
  // *********
  // tailor functionality to decrease SRAM requirement, etc
//  #define VL53L5CX_DISABLE_AMBIENT_PER_SPAD
//  #define VL53L5CX_DISABLE_NB_SPADS_ENABLED
//  #define VL53L5CX_DISABLE_SIGNAL_PER_SPAD
//  #define VL53L5CX_DISABLE_RANGE_SIGMA_MM
//  #define VL53L5CX_DISABLE_REFLECTANCE_PERCENT
//  #define VL53L5CX_DISABLE_MOTION_INDICATOR
  // *********

  // Put the VL53L5CX to sleep
  status = vl53l5cx_set_power_mode(&Dev, VL53L5CX_POWER_MODE_SLEEP);
  if(status)
  {
    printf("vl53l5cx_set_power_mode failed, status %u\n", status);
  }
  printf("VL53L5CX is now sleeping\n");

  /* We wait 5 seconds, only for the example */
  printf("Waiting 5 seconds for the example...\n");
  WaitMs(&(Dev.platform), 5000);

  /* After 5 seconds, the sensor needs to be restarted */
  status = vl53l5cx_set_power_mode(&Dev, VL53L5CX_POWER_MODE_WAKEUP);
  if(status)
  {
    printf("vl53l5cx_set_power_mode failed, status %u\n", status);
  }
  printf("VL53L5CX is now waking up\n");


  /* Start ranging */
  error = vl53l5cx_start_ranging(&Dev);
  if(error !=0) {Serial.print("start error = "); Serial.println(error);}

  error = vl53l5cx_check_data_ready(&Dev, &isReady); // clear the interrupt
 } /* end of setup */


void loop(void)
{
    if (VL53L5_intFlag) {
        VL53L5_intFlag = false;

        isReady = 0, error = 0;
        while(isReady == 0){
           error = vl53l5cx_check_data_ready(&Dev, &isReady);
        if(error !=0) {Serial.print("ready error = "); Serial.println(error);}
        delay(10);
        }

    if(isReady)
    {
//      status = vl53l5cx_get_resolution(&Dev, &resolution);
      status = vl53l5cx_get_ranging_data(&Dev, &Results);

      for(int i = 0; i < pixels; i++){
        /* Print per zone results */
        printf("Zone : %2d, Nb targets : %2u, Ambient : %4lu Kcps/spads, ",
            i,
            Results.nb_target_detected[i],
            Results.ambient_per_spad[i]);
 
        /* Print per target results */
        if(Results.nb_target_detected[i] > 0){
          printf("Target status : %3u, Distance : %4d mm\n",
              Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE * i],
              Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE * i]);
        }else{
          printf("Target status : 255, Distance : No target\n");
        }
      }
      printf("\n");

    // Get min and max range for display
    minRange = 1000;
    maxRange =    0;
    for(int y=0; y<8; y++){ //go through all the rows
    for(int x=0; x<8; x++){ //go through all the columns
      if(Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*(y+x*8)] > maxRange) maxRange = Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*(y+x*8)];
      if(Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*(y+x*8)] < minRange) minRange = Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*(y+x*8)];
      }
      }

    for(int y=0; y<8; y++){ //go through all the rows
    for(int x=0; x<8; x++){ //go through all the columns
      tmpRange = Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*(y+x*8)];
      Serial.print(tmpRange); Serial.print(","); // use the serial monitor to plot the data 
      if(x == 7) Serial.println(" "); 
      if(y+x*8 == 63) Serial.println(" "); 

      // Make closest pixels brightest
      rgb =(uint8_t) ((1.0f - ((float)(tmpRange - minRange)/(float)((maxRange - minRange)))) * 199); // 0 - 199 = 200 possible rgb color values

      red   = rgb_colors[rgb*3] >> 3;          // keep 5 MS bits
      green = rgb_colors[rgb*3 + 1] >> 2;      // keep 6 MS bits
      blue  = rgb_colors[rgb*3 + 2] >> 3;      // keep 5 MS bits
      color = red << 11 | green << 5 | blue;   // construct rgb565 color for tft display

      tft.fillRect(x*16, y*16, 16, 16, color); // data on 128 x 128 pixels of a 160 x 128 pixel display
    }
    }
    
      tft.fillRect(128, 0, 32, 128, BLACK);    // fill 32 x 128 pixel non-data patch with black

      tft.setRotation(0);                      // 0, 2 are portrait mode, 1,3 are landscape mode
      tft.setTextSize(0);
      tft.setTextColor(WHITE);
      tft.setCursor(32, 4 );                   // write min,max temperature on non-data patch
      tft.print("min R = "); tft.print(minRange); tft.print(" mm");
      tft.setCursor(32, 20 );
      tft.print("max R = "); tft.print(maxRange); tft.print(" mm");
      tft.setRotation(3);                      // 0, 2 are portrait mode, 1,3 are landscape mode

      Serial.print("min R = "); Serial.println(minRange);
      Serial.print("max R = "); Serial.println(maxRange);
  }

      digitalWrite(LED_PIN, HIGH); delay(1); digitalWrite(LED_PIN, LOW);

    } // end of VL53L5CX interrupt handling

 //   STM32.sleep();
} /* end of main loop */


void VL53L5_intHandler(){
  VL53L5_intFlag = true;
}


// I2C scan function
void I2Cscan()
{
// scan for i2c devices
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 //     error = Wire.transfer(address, NULL, 0, NULL, 0);
      
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
    
}
