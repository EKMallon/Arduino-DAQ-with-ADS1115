// For an UNO DAQ tethered to USB. Note that the ADC is set to use the internal 1.1volt reference so do not apply higher voltages to the analog lines.
// Observe the output with the serial monitor enabled, to establish what your
// your sampling "LoopThreshold" should be in order to capture "events" that cause the current to rise

#define analogPin 0
#define ADCcycles 450    //You can output 500 samples to the serial plotter, but I find that using a slightly lower number looks nicer
#define OVRsamples 5     //The number of samples that get 'averaged' to produce one tick on the serial plotter
//setting the OVRsample value to 1 (no oversampling) means only 60ms of time are displayed on the plot, values near 17 display around 1 second of time on the plot
#define LoopThreshold 3  //the "trigger" that starts a sampling loop: change this to something just slightly above the normal resting/sleeping current before the event
//#define TEXToutput     //un-comment this define statement for text-only RAW output with the elapsed time for your reading loop
                         // the RAW reading output lets you figure out how to set you trigger threshold

int16_t start;
int16_t elapsed;
uint16_t currentADCreadings[ADCcycles];  
int16_t filteredADCvalue; 
int16_t previousADCvalue; 
int ADCoffset=2;  // this is a manual fudge factor - my ADC was reading 2 bit too low.
uint16_t tempread[OVRsamples];  
int Peakflag = 0; 
 
static uint32_t filtsum;  // used in the leaky integrator filter
int shift=1;              // if you increase the shift value, the readings get smoothed more heavily
                          // but I rarely use a value above 1 or 2

float scalefactor = 1.07421F; // this is the ADC resolution/bit AFTER setting to the internal 1.1vref
float millivolts = 0.0;       // The result of applying the scale factor to the raw ADC value
int microamps = 0;            // only relevant to this particular application

void setup(void)
{
  Serial.begin(115200);  
  Wire.begin();
  analogReference(INTERNAL);  //this sets the ADC aref voltage to 1.1v - do not apply higher voltages to the A0 line.
  
}

void loop(void)
{  

if(analogRead(analogPin) >= LoopThreshold){    //the trigger that starts the rapid sampling loop

   start=millis();   
      for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //rapid ADC sampling loop

             Peakflag=0; currentADCreadings[Cycle]=previousADCvalue;
             
             for (int i = 0; i < OVRsamples; i++) {   // over sampling loop
              tempread[i] = analogRead(analogPin);                   // the raw reading
              filteredADCvalue= filter(tempread[i]);                 // the raw reading added to the running average
               if(tempread[i]>currentADCreadings[Cycle]){            // a higher reading always becomes the selected reading
                currentADCreadings[Cycle]=tempread[i];Peakflag=1;    // sets a flag so we know a peak event occured
                }         
               }
              
              if(Peakflag==0){                                      //this only happens if no peaks were detected during the interval
                //currentADCreadings[Cycle]=tempread[OVRsamples-1]; //choose this if you only want the last reading in a series to become the new reading
                currentADCreadings[Cycle]=filteredADCvalue;         //choose this if you want the leaky average of the over sampled readings to become the new reading
                }
              previousADCvalue=currentADCreadings[Cycle];  // saves this interval reading as a starting point for the next one
              
      }  // end of rapid sampling loop  
   elapsed=millis()-start;// elapsed gives you the timebase for your samples

#ifdef TEXToutput
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the RAW values from the sample loop to the serial text monitor
    Serial.println(currentADCreadings[Cycle]); 
    }  
    //After seeing the raw readings, you have some idea how set your LoopThreshold value
    Serial.print(F(",Time for "));Serial.print(ADCcycles);Serial.print(F(" readings: "));Serial.print(elapsed);Serial.println(F(" milliseconds"));
#endif


#ifndef TEXToutput  //for serial plotter only output
//Once you get your threshold set properly, you can view your output values on the serial plotter
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop to the serial plotter
    Serial.print(4000); Serial.print(" ");//this constant sets a stable upper value line to keep the plotter from rescaling - change this to suit your situation
    Serial.print(0); Serial.print(" ");  //this constant sets a stable lower value for the plotter
    Serial.print(200); Serial.print(" "); //You can put a line anywhere on the graph you want 
    Serial.print(400); Serial.print(" "); //You can put a line anywhere on the graph you want

    //Here I am doing a calculation to convertthe output to μA before sending it to the serial plotter
    //Your application would be doing different calculations, suitable to your situation.
    millivolts=((currentADCreadings[Cycle]+ADCoffset)*scalefactor);
    microamps=int((millivolts/10.5)*1000); //our shunt resistor was 10.5 ohms
    //microamps= filter(microamps); 
    Serial.println(microamps); 
    } // End of plotter output 

#endif
   
} // end of the threshold triggered sampling loop

}  //end of main Loop

// Leaky integrator filter
// see http://www.bot-thoughts.com/2013/07/oversampling-decimation-filtering.html for more info on the leaky integrator filter
uint16_t filter(uint16_t value) {
        filtsum += (value - (filtsum >> shift));
        return (filtsum >> shift);
}

