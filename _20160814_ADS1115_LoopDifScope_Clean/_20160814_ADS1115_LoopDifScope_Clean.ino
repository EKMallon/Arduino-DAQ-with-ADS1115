#include <Wire.h>
#include <ADS1115.h> // from https://github.com/jrowberg/i2cdevlib/blob/master/Arduino/ADS1115/ADS1115.h

// this code is simply to demonstrate running an ADS1115 with an UNO tethered to USB.
// You need to observe the output with the serial monitor first, to establish what your
// your sampling "LoopThreshold" should be in order to capture "events" that cause the current to rise

#define ADCcycles 450      //You can output 500 samples to the serial plotter, but the UNO runs low on mem around 450
#define LoopThreshold 80   //change to something just slightly above your normal resting/sleeping current
//#define ECHO_TO_SERIAL   // to print out the elapsed time for your reading loop

ADS1115 adc0(ADS1115_ADDRESS_ADDR_GND); 
float scalefactor = 0.015625F; // change this to match your amplifier settings
float millivolts = 0.0; // The result of applying the scale factor to the raw value
float microamps = 0.0; // The result of applying the scale factor to the raw value

int16_t start;
int16_t elapsed;
uint16_t currentADCreadings[ADCcycles];  

void setup(void)
{
  Serial.begin(115200);  
  Wire.begin();

  //Changing I2C speed  http://www.gammon.com.au/forum/?id=10896
  // also http://electronics.stackexchange.com/questions/29457/how-to-make-arduino-do-high-speed-i2c
  //if (EEPROM_ADDRESS==0x50){  
  // TWBR = 64;  // 400 kHz @ 8MHz CPU speed only // AT24c256 ok @ 400kHz http://www.atmel.com/Images/doc0670.pdf  
  // see http://www.avrfreaks.net/forum/can-8mhz-clock-run-400khz-twi?skey=TWBR%208mhz 8mhz arduinos might not be capable of 400hz
  // TWBR = 12; // 200kHz mode @ 8MHz or 400kHz mode @ 16MHz CPU 
  // TWBR = 32;  // 100 kHz (default) @ 8MHz only 
  // Note that High-speed mode on the ADS115 needs to be enabled
  // send a special address byte of 00001xxx following the START condition, where xxx are bits unique to the
  // Hs-capable master. This byte is called the Hs master code. Upon receiving a master code, the ADS1113/4/5 switch on Hs mode

  
#ifdef ECHO_TO_SERIAL
Serial.println("Initializing I2C devices..."); 
#endif
adc0.initialize(); // initialize ADS1115 16 bit A/D chip

#ifdef ECHO_TO_SERIAL
Serial.println("Testing device connections...");
#endif
Serial.println(adc0.testConnection() ? "ADS1115 connection successful" : "ADS1115 connection failed");  


// To get output from this method, you'll need to turn on the 
//#define ADS1115_SERIAL_DEBUG // in the ADS1115.h file
    adc0.showConfigRegister();
    adc0.setRate(ADS1115_RATE_860);  //860 samples/sec
    adc0.setMode(ADS1115_MODE_CONTINUOUS);  //continuous sampling
    adc0.setGain(ADS1115_PGA_0P512);  // +/- 0.512V  1 bit = 0.015625mV  710kΩ   
// The ADC input range (or gain) can be changed via the following (32767 is largest 15 bit value)
// Never to exceed VDD +0.3V max, NEVER exceed the upper and lower limits of gain adjusted input range
// If you to it may destroy your ADC!
//                                                                 ScaleFactor  D.Input Impedance 
// ads.setGain(ADS1115_PGA_6P144);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV    22MΩ        //start up default
// ads.setGain(ADS1115_PGA_4P096);  // 1x gain   +/- 4.096V  1 bit = 0.125mV     15MΩ
// ads.setGain(ADS1115_PGA_2P048);  // 2x gain   +/- 2.048V  1 bit = 0.0625mV    4.9MΩ       // default for this library
// ads.setGain(ADS1115_PGA_1P024);  // 4x gain   +/- 1.024V  1 bit = 0.03125mV   2.4MΩ
// ads.setGain(ADS1115_PGA_0P512);  // 8x gain   +/- 0.512V  1 bit = 0.015625mV  710kΩ
// ads.setGain(ADS1115_PGA_0P256);  // 16x gain  +/- 0.256V  1 bit = 0.0078125mV 710kΩ

    adc0.setMultiplexer(ADS1115_MUX_P0_N1);   // sets mux to differential
    // Our 5 ohm shunt resisor is on P0/N1 (pins 4/5) [Note: A0&A1 on adafruit board]
}

void loop(void)
{  
int sensorOneCounts=adc0.getConversion();  //note we already set the mux to differential in setup

if(sensorOneCounts >= LoopThreshold){ //rapid sampling loop to capture the event

   start=millis();   
      for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //rapid ADC sampling loop
      currentADCreadings[Cycle]=adc0.getConversion(); //it takes 1.16 ms for the ADC to do a new conversion
      //delayMicroseconds(430); //tweak this delay until your loop takes around 588ms for 500 readings
      }                       //The goal is to get the data requests from the arduino to match 860 samples/second    
   elapsed=millis()-start;// elapsed gives you the timebase for your samples

    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop for the serial text monitor
    Serial.println(currentADCreadings[Cycle]); 
    } 

//Once you get your threshold set properly, you can view your output values on the serial plotter
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop to the serial plotter
    Serial.print(4000); Serial.print(" ");//this constant sets a stable upper value line to keep the plotter from rescaling - change this to suit your situation
    Serial.print(0); Serial.print(" ");  //this constant sets a stable lower value for the plotter 
    //Serial.print(200); Serial.print(" "); //You can put a line anywhere on the graph you want
    //Here I am converting the output to μA before sending it to the serial plotter
    millivolts=(currentADCreadings[Cycle]*scalefactor);
    microamps=(millivolts/5.4)*1000; //our shunt resistor was 5.4 ohms
    //Serial.println(microamps); 
    Serial.println(microamps); 
    } // End of plotter output 

#ifdef ECHO_TO_SERIAL 
    //After seeing the raw readings, you have some idea how set your LoopThreshold value
    Serial.print(F("Time for "));Serial.print(ADCcycles);Serial.print(F(" readings: "));Serial.print(elapsed);Serial.println(F(" milliseconds"));
#endif
    
} // end of the threshold triggered sampling loop

}  //end of main Loop

