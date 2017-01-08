// For an UNO DAQ tethered to USB. Note that the ADC is set to use the internal 1.1volt reference so do not apply higher voltages to the analog lines.
// Observe the output with the serial monitor enabled, to establish what your
// your sampling "LoopThreshold" should be in order to capture "events" that cause the current to rise

#define analogPin 0
#define ADCcycles 490    //You can output 500 samples to the serial plotter, but I find that using a slightly lower number looks nicer
#define OVRsamples 40     //minumum value=1, The number of ADC samples that get 'averaged' to produce one tick on the serial plotter
//setting the OVRsample value to 1 (no oversampling) means only 60ms of time are displayed on the 500 line plot, values near 17 display around 1 second of time on the plot
#define LoopThreshold 10  //the "trigger" that starts a sampling loop: change this to something just slightly above the normal resting/sleeping current before the event

// you find out what the resting state reading  is by first observing the output with the serial TEXT monitor:

//#define TEXToutput    //un-comment this statement for text-only output AND to get the elapsed time for your reading loop
 
// the serial monitor output also lets you figure out how to set you trigger threshold... or you can just drop a line on the graph

int16_t start;
int16_t elapsed;
uint16_t currentADCreadings[ADCcycles];  //array variable to hold up to 500 samples for serial plotter output
int16_t filteredADCvalue; 
int16_t previousADCvalue; 
int ADCoffset=2;                     // this is a manual fudge factor - my ADC was reading 2 bit too low.
uint16_t tempread[OVRsamples];           //array variable to hold the number of ADC readings you are averaging together
int Peakflag = 0;                    //peak flag is set to 1/TRUE if a brief spike in readings was detected                  

float scalefactor = 1.07421F; // this is the ADC resolution/bit AFTER setting to the internal 1.1vref
float millivolts = 0.0;       // The result of applying the scale factor to the raw ADC value
float shuntResistor =12.2;    // the value of the shunt resistor being used to track the current
uint16_t microamps = 0;            // only relevant to the calculations I am doing
uint16_t milliamps = 0; 
float supplyVoltage = 4.35;
float averageCurrent = 0.0;
float PreviousLoopPower = 0.0;

 // variables used in the leaky integrator filter:
uint32_t filtsum;  
int shift=2;                        // if you increase the shift value, the readings get smoothed more heavily
                                    // but I rarely use a value above 1 or 2

void setup(void)
{
  Serial.begin(250000);  
  analogReference(INTERNAL);  //this sets the ADC aref voltage to 1.1v - do not try to read > 1.1v on the A0 line.

 //=================================================================================================
 //ADC prescaler settings 200kHz is max recommended in datasheet,  16MHz/64=250,  8 MHz/64 = 125 kHz
 //=================================================================================================
 //  from: http://www.gammon.com.au/adc 
 // see http://forum.arduino.cc/index.php?topic=120004.0 for fat16lib tests of reading accuracy at faster speeds
 // at the default ADC speed, your minimum loop time for 500 samples is ~60ms, but you can reduce that significantly at PS32

 /* Prescaler settings : ~# of ADC Conversions/sec @ 16mHz clock:
  2           615,385
  4           307,692
  8           153,846
 16            76,923
 32            38,462
 64            19,231
128             9,615
  */
 
 // the Arduino core initializes those bits. So add a statement to clear them before you set them.  
 ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear prescaler bits 
 
 // TO CHANGE the ADC prescalar uncomment one of the following lines as required 
 //ADCSRA |= bit (ADPS0) | bit (ADPS1);                     //  P8  expect to see significant under-reading at this speed
 //ADCSRA |= bit (ADPS2);                                   //  PS16 see http://forum.arduino.cc/index.php?topic=120004.0 for fat16lib tests
ADCSRA |= bit (ADPS0) | bit (ADPS2);                     //  PS32           probably max you can use with 10K impedance: 8MHz/32 = 250 kHz
 //ADCSRA |= bit (ADPS1) | bit (ADPS2);                     //  PS64          Note: on 8mhz 3.3v boards 8 MHz/64 = 125 kHz = default setting
 //ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);       //  PS128         = 128kHz Default for 16mhz UNO's

}

void loop(void)
{  

// ADC RAPID SAMPLING LOOP:
//-------------------------

if(analogRead(analogPin) >= LoopThreshold){    //the trigger that starts the rapid sampling loop

   start=millis();   
      for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //rapid ADC sampling loop

             //Peakflag=0; currentADCreadings[Cycle]=previousADCvalue;
             Peakflag=0; currentADCreadings[Cycle]=previousADCvalue;
             
             for (int i = 0; i < OVRsamples; i++) {   // over sampling loop
              tempread[i] = analogRead(analogPin);                   // the raw reading
              filteredADCvalue= filter(tempread[i]);                 // current raw reading added to the running average
               if(tempread[i]>=currentADCreadings[Cycle]){            // a higher reading always becomes the selected reading
                currentADCreadings[Cycle]=tempread[i];Peakflag=1;     // sets a flag so we know a peak event occured so not using runing average
                //filtsum=tempread[i];
               }                       
               }
              
              if(Peakflag==0){   //this only happens if no peaks were detected during the interval
                //currentADCreadings[Cycle]=tempread[OVRsamples-1]; //choose this if you only want the last reading in a series to become the new reading
                currentADCreadings[Cycle]=filteredADCvalue;         //choose this if you want the leaky average of the over sampled readings to become the new reading
                }
              previousADCvalue=currentADCreadings[Cycle];  // saves the last interval reading as a starting point for the next one
              
      }  // end of rapid sampling loop  
   elapsed=millis()-start;// elapsed gives you the timebase for your samples

// OUTPUT for()LOOPS:
//--------------------
//This program has two separate output loops, depending on wether you want to view the output in the serial monitor, or the serial plotter

#ifdef TEXToutput  //if TEXToutput IS defined at the beginning of the program , then output includes the SAMPLING elapsed time info
    averageCurrent =0.0;
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the RAW values from the sample loop to the serial text monitor
    Serial.println(currentADCreadings[Cycle]); 
    
    millivolts=((currentADCreadings[Cycle]+ADCoffset)*scalefactor);
    microamps=int((millivolts/shuntResistor)*1000);     //milivolts is divided by the value of your shunt resistor in ohms
    averageCurrent += microamps;
    
    }  
    //After seeing the raw readings, you have some idea how set your LoopThreshold value
    Serial.print(F(",Time for "));Serial.print(ADCcycles);Serial.print(F(" readings: "));Serial.print(elapsed);Serial.println(F(" milliseconds"));
    Serial.println(F("Note:the Serial Plotter always displays 500 readings, no matter how many you send"));
    Serial.print(F(",Average current for the event: "));Serial.print(averageCurrent/ADCcycles);Serial.println(F(" microamps"));
    
#endif  // End of Serial Text Monitor output

#ifndef TEXToutput  //if TEXToutput is NOT defined at the beginning of the program , then output is for serial plotter ONLY
//Once you get your threshold set properly, you can view your output values on the serial plotter

    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) { 
          //Here I am doing a calculation to convert the output to μA before sending it to the serial plotter
    //Your application WOULD BE DOING DIFFERENT CALCULATIONS, suitable to your situation.
    millivolts=((currentADCreadings[Cycle]+ADCoffset)*scalefactor);
    microamps=int((millivolts/shuntResistor)*1000);     //milivolts is divided by the value of your shunt resistor in ohms
    averageCurrent += microamps;
    }
    averageCurrent = (averageCurrent/ADCcycles);
    
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop to the serial plotter

    //repeating sequence of 4 colors of indicator lines on the plotter is Dk.Blue - Orange - Red - Lt.Blue - Dk.Blue - Orange - etc
     
    //Serial.print(7000); Serial.print(" "); // the largest constant sets a stable upper value line to keep the plotter from rescaling
    Serial.print(60000); Serial.print(" "); // my expected peak current with a pro-mini: ~5.5mA
    Serial.print(30000); Serial.print(" "); // my expected peak current with a pro-mini: ~5.5mA
    //Serial.print(averageCurrent); Serial.print(" "); // places a CONSTANT line on the graph at the average current drawn for the loop event
    //Here I am doing a calculation to convert the output to μA before sending it to the serial plotter
    millivolts=((currentADCreadings[Cycle]+ADCoffset)*scalefactor);
    microamps=int((millivolts/shuntResistor)*1000);     //milivolts is divided by the value of your shunt resistor in ohms
    Serial.print(microamps); Serial.print(" ");  
    Serial.print(5000); Serial.print(" ");  // You can put a line anywhere to intersect the graph output
    Serial.println(0);  // the lowest constant sets a stable lower value for the plotter
    } 
    averageCurrent =0.0;
    
#endif // End of Serial Plotter output 
   
} // end of the threshold triggered sampling loop:   if(analogRead(analogPin) >= LoopThreshold){ 

}  //end of MAIN  void loop(void)


// Leaky integrator filter - chosen because of low compuation overhead
// see http://www.bot-thoughts.com/2013/07/oversampling-decimation-filtering.html for more info on the leaky integrator filter
uint16_t filter(uint16_t value) {
        filtsum += (value - (filtsum >> shift));
        return (filtsum >> shift);
}

// or you could try other moving average filters
// http://electronics.stackexchange.com/questions/42734/fastest-moving-averaging-techniques-with-minimum-footprint
// but they usually involve more computational overhead, increasing the changes of missing a brief current peak...
