#include <Wire.h>

// UNO tethered to USB. Note that the ADC is set to use the internal 1.1volt reference so do not apply higher voltages to the analog lines.
// You need to observe the output with the serial monitor first, to establish what your
// your sampling "LoopThreshold" should be in order to capture "events" that cause the current to rise

#define analogPin 0
#define ADCcycles 450      //You can output 500 samples to the serial plotter, but the UNO runs low on mem around 450
#define OVRsamples 5
#define LoopThreshold 3  //change to something just slightly above your normal resting/sleeping current
//#define TEXToutput   // to print out the elapsed time for your reading loop

int16_t start;
int16_t elapsed;
uint16_t currentADCreadings[ADCcycles];  
int16_t filteredADCvalue; 
int16_t previousADCvalue; 
int ADCoffset=2;  // this is a manual fudge factor - my ADC was reading 2 bit too low.
uint16_t tempread[OVRsamples];  
int Peakflag = 0; 
 
// see http://www.bot-thoughts.com/2013/07/oversampling-decimation-filtering.html for more info on leaky integrator filter
static uint32_t filtsum;
int shift=1;

float scalefactor = 1.07421F; // this is the ADC resolution/bit AFTER setting to the internal 1.1vref
float millivolts = 0.0; // The result of applying the scale factor to the raw value
int microamps = 0; 


void setup(void)
{
  Serial.begin(115200);  
  Wire.begin();
  analogReference(INTERNAL);  //this sets aref to 1.1v - do not apply higher voltages to the A0 line.
  
}

void loop(void)
{  

if(analogRead(analogPin) >= LoopThreshold){ //rapid sampling loop to capture the event

   start=millis();   
      for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //rapid ADC sampling loop

             Peakflag=0; currentADCreadings[Cycle]=previousADCvalue;
             
             for (int i = 0; i < OVRsamples; i++) {                          //over sampling loop
              tempread[i] = analogRead(analogPin); 
              filteredADCvalue= filter(tempread[i]);
               if(tempread[i]>currentADCreadings[Cycle]){         // always gives preference to a higher reading
                currentADCreadings[Cycle]=tempread[i];Peakflag=1;  
                }         
               }
              
              if(Peakflag==0){  //if no short peaks were detected
                //currentADCreadings[Cycle]=tempread[OVRsamples-1]; //choose this if you just want the last reading to become the new reading
                currentADCreadings[Cycle]=filteredADCvalue;         //choose this if you want the leaky average of the over sampled readings to become the new reading
                }
              previousADCvalue=currentADCreadings[Cycle];
              //delayMicroseconds(10);  //no longer any need for delays, just set your oversampling until you have the time base you want
              
      }  // end of rapid sampling loop  
   elapsed=millis()-start;// elapsed gives you the timebase for your samples

#ifdef TEXTouput
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop for the serial text monitor
    Serial.println(currentADCreadings[Cycle]); 
    }  
    //After seeing the raw readings, you have some idea how set your LoopThreshold value
    Serial.print(F(",Time for "));Serial.print(ADCcycles);Serial.print(F(" readings: "));Serial.print(elapsed);Serial.println(F(" milliseconds"));
#endif


#ifndef TEXToutput  //plotter only output
//Once you get your threshold set properly, you can view your output values on the serial plotter
    for (int Cycle = 0; Cycle < ADCcycles; Cycle++) {  //output of the sample loop to the serial plotter
    Serial.print(4000); Serial.print(" ");//this constant sets a stable upper value line to keep the plotter from rescaling - change this to suit your situation
    Serial.print(0); Serial.print(" ");  //this constant sets a stable lower value for the plotter
    Serial.print(200); Serial.print(" "); //You can put a line anywhere on the graph you want 
    Serial.print(400); Serial.print(" "); //You can put a line anywhere on the graph you want

    //Here I am converting the output to μA before sending it to the serial plotter
    millivolts=((currentADCreadings[Cycle]+ADCoffset)*scalefactor);
    microamps=int((millivolts/10.5)*1000); //our shunt resistor was 10.5 ohms
    //microamps= filter(microamps); 
    Serial.println(microamps); 
    } // End of plotter output 

#endif
   
} // end of the threshold triggered sampling loop

}  //end of main Loop

/** Leaky integrator filter */
uint16_t filter(uint16_t value) {
        filtsum += (value - (filtsum >> shift));
        return (filtsum >> shift);
}

