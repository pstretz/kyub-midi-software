/* Teensy 2.0 Based Kyub 9/2/2013
 *
 * Background Info:
 * ADC 8-Bit Mode 
 * Analog input 0 (ADC0) is used to sample the accelerometer signal
 * 
 * Rev History:
 * - Initial Release -
 * Keith Baxter 2-Sep-2013
 *
 * - Rev 3 -  201403111009
 * Petyr Stretz 11-Mar-2014
 * changed note define to an array for ease of coding
 * removed all the pallets, users should just paste in what they want kyub to do currently
 * 
 */

//*******Global Variables

//-------INSERT YOUR CONFIGURATION CODE HERE---------- 

int modeChannel[]={4,5,6,7};
int noteIdx[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,20,23,27,30,36,39,43,48,52,53,54,55,56,57,58,59,60,61,62};
int note[]={49,46,51,36,41,42,38,45,48,36,39,44,38,47,40,41,43,43,48,55,48,53,55,48,52,55,57,48,52,55,48,52,55,59,62,69,48,50,55,48,52,55,57,48,52,55,57,62,48,52,55,59,48,51,56,50,59,52,53,53,55};
int notevolume=0;
int volume=0;
int pitch=0;
long int min_note_duration=100000;
//long int holdoff[11]; //still must be implemented

//----------------END CONFUGRUATION CODE---------------

int channel;
int consolemidimode=1; //consolemode=0 midimode=1 accelerometer=3
int chord1=0;  //determined by pads 9 and 10
int chord2=0;

//pin assignments
int driverpin = 13; //common pad drive pin
int zpin=2; //acelerometer axes
int xpin=1;
int ypin=0;
int pad[]={0,1,2,3,4,5,6,7,8,9,10};  //array holding sensing pad numbers
int cap_calibration[11];  //calibration value for each pad
long int chargetime[11];  //sensed charge time for each pad
int padstate[11]={0,0,0,0,0,0,0,0,0,0,0};  //state of pad as touched (1) or not (0)
int padmode[11]={0,0,0,0,0,0,0,0,0,0,0};  //state of pad touch as it is processed
//   0 = ready for new pad touch
//   1 = have touch, waiting for volume
//   2 = have volume, waiting to be played (note on)
//   3 = have enrolled in buffer
//   4 = played, waiting to be turned off (note off)
//   5 = disabled pad 

long int padlasttime[11];  //last time pad was triggered
int padvolume[11];  //current note volume
int pnum=0; //index for pads through each loop
int overflow=0;
unsigned long starttime=0;
unsigned long grabtime;
int hysteresishigh=25; //turn on threshold for touch
int hysteresislow=20; //turn off threshold for touch

//debug printout variable
long int next=0;

//accelerometer variables
//int once=0; //controls sample acquisitions
int circularaccbufferx[101]; //holds samples of A/D taken before and after pad hit
int circularaccbuffery[101];
int circularaccbufferz[101];
int circbuffpointer=0;
int triggerpoint=0; //time of pad hit
long acc_calibrationx=0; //A/D calibration values (may not be needed)
long acc_calibrationy=0;
long acc_calibrationz=0;
int xaxispeak=0; //peaks and valleys of acceleration waveforms
int xaxisvalley=0;
int yaxispeak=0;
int yaxisvalley=0;
int zaxispeak=0;
int zaxisvalley=0;
int firsttime=0;  //trigger for calibration

//********Setup*************************************************

void setup() 
{
  pinMode(driverpin, OUTPUT); //set the driver as output to charge/discharge pads
  for (int x=0; x<11; x++){ //set other pins to input and turn off pull up resistors
    pinMode(pad[x], INPUT);
    digitalWrite (pad[x], LOW);
  }
}//end setup

// the loop routine runs over and over again forever:
void loop() 
{
  //*********************************************************************
  //*******Main loop--read capacitive pads and accelerometer ************
  //*********************************************************************
 
  //pad calibration early after boot
  if (firsttime<20) firsttime++; 
  if (firsttime==19) for (int x=0; x<12; x++) cap_calibration[x]=chargetime[x];    

  //******************************************************************************************************
  //loop through each of 11 pads according to pnum
  //******************************************************************************************************
  if (pnum<10) pnum++; 
  else pnum=0;   
  overflow=0;

  //***********************start cap sensing and accel sensing ****************************
  //for high speed, read A/D for x, y, and z interleaved at times of necessary delay

  if (circbuffpointer<99) circbuffpointer++; 
  else circbuffpointer=0; //acell. axis circular buffer pointer

  //first measure charge up time, then measure fall time to cut sensitivity to gate threshold level

  //CHARGEUP
  //set driver pin high and measure rise time of selected pad
  digitalWrite(driverpin, HIGH);   // common driver pin high
//  once=0;
  starttime = micros();
  while ((digitalRead(pad[pnum])==LOW) && (overflow==0))  //digital read is pretty slow it seems
  { //charge while loop
    if (micros()-starttime>500) 
    {
      overflow=1;
      if (consolemidimode==0) //debug output to console
      {
        Serial.print ("overflow up on pin:"); 
        Serial.print(pnum);
        Serial.println(""); 
      } 
    }
  } //end charge while loop

  grabtime= micros()-starttime;
  //finish charging of input pin to full voltage
  digitalWrite(pad[pnum],HIGH);  //set pullup resistor to on

  //*********************interleaved x-axis accelerometer read **************************************

  //get x axis accel

  circularaccbufferx[circbuffpointer]= analogRead(xpin);
  //Serial.println(analogRead(xpin));
  //delay(1000);
  delayMicroseconds(30);  //a/d conversion time about 26us?

  //*********************interleave x-axis end****************************************************** 

  delayMicroseconds(100);//not needed if have delay from A/D
  digitalWrite(pad[pnum],LOW); //turn off pull up resistor

  //*********************interleaved y-axis accelerometer read **************************************

  //get y axis accel
//  once=1;
  circularaccbuffery[circbuffpointer]= analogRead(ypin);
  delayMicroseconds(30);  


  //*********************interleave y-axis end ****************************************************** 

  //set driver pin low and measure fall time of selected pad
  //CHARGE DOWN
  digitalWrite(driverpin, LOW);   
  starttime = micros();
//  once=0;
  while ((digitalRead(pad[pnum])==HIGH) && (overflow==0)){ //discharging while loop
    if (micros()-starttime>500) 
    {
      overflow=1;
      if (consolemidimode==0) //debug mode console output
      {
        Serial.print ("overflow down on pin:"); 
        Serial.print(pnum);
        Serial.println("");
      } 
    }
  } //end discharging while loops

  grabtime= grabtime+ micros()-starttime;  //add rise and fall times together
  
//*********************interleaved z-axis accelerometer read **************************************

  //get z axis accel
  circularaccbufferz[circbuffpointer] = analogRead(zpin);
//  delayMicroseconds(30);


  //*********************interleave #3 ******************************************************  

  delayMicroseconds(100); //to obtain the benefit of rise and fall measurements, must hit zero volta here
  chargetime[pnum]=grabtime;
  //*************************end of cap and accell sensing *****************************
  //****************************************************************************************
  //****************************************************************************************

  //touch detected for the first time****************************
  if (chargetime[pnum]-cap_calibration[pnum]>hysteresishigh) padstate[pnum]=1; 
  else if (chargetime[pnum]-cap_calibration[pnum]<hysteresislow) padstate[pnum]=0;

  //special non playing pads--chord selectors
  if (padstate[9]==1) chord1=1;
  else chord1=0;
  if (padstate[10]==1) chord2=1; 
  else chord2=0;

  //deactivate these pins for all other functions
  padmode[9]=5;
  padmode[10]=5;

  if ((padmode[pnum]==0) && (padstate[pnum]==1)) //ready for new note
  {
    padlasttime[pnum]=micros();  //keep this low as long as pad is held
    triggerpoint=0; 
    padmode[pnum]=1;
//    holdoff[pnum]=micros();  //?? needed ??hold off stops rapid second trigger "bounce"
  }

  if (triggerpoint<99) triggerpoint++; //let buffer run a bit then find max and load it into pending notes
  if (triggerpoint==25) { //half of buffer
    yaxispeak=circularaccbuffery[0];
    yaxisvalley=circularaccbuffery[0];
    xaxispeak=circularaccbufferx[0];
    xaxisvalley=circularaccbufferx[0];
    zaxispeak=circularaccbufferz[0];
    zaxisvalley=circularaccbufferz[0];
    acc_calibrationx=0; //calibration values will be average value of circular buffer
    acc_calibrationy=0;
    acc_calibrationz=0;

//?? check if there is a faster way to min-max an array  ??
    for (int x=0; x<99; x++)  //grab peaks and valles of 100 samples of accelerometer
    {
      acc_calibrationx+=circularaccbufferx[x]; //running total of accelerations used to derive base or average
      acc_calibrationy+=circularaccbuffery[x];
      acc_calibrationz+=circularaccbufferz[x];

      if (circularaccbufferx[x]>xaxispeak)  xaxispeak=circularaccbufferx[x];
      if (circularaccbufferx[x]<xaxisvalley) xaxisvalley=circularaccbufferx[x];
      if (circularaccbuffery[x]>yaxispeak) yaxispeak=circularaccbuffery[x];
      if (circularaccbuffery[x]<yaxisvalley) yaxisvalley=circularaccbuffery[x];
      if (circularaccbufferz[x]>zaxispeak) zaxispeak=circularaccbufferz[x];
      if (circularaccbufferz[x]<zaxisvalley)  zaxisvalley=circularaccbufferz[x]; 
    } 

    xaxispeak=xaxispeak-int(acc_calibrationx/100);
    yaxispeak=yaxispeak-int(acc_calibrationy/100);
    zaxispeak=zaxispeak-int(acc_calibrationz/100);

    xaxisvalley=xaxisvalley-int(acc_calibrationx/100);
    yaxisvalley=yaxisvalley-int(acc_calibrationy/100);
    zaxisvalley=zaxisvalley-int(acc_calibrationz/100);

    if (consolemidimode==3)  //debug console outputs
    {
      int indexer=0; 
      for (int p=0; p<99; p++)
      {
        if (p==50) 
        {
          Serial.println("");
          Serial.println("hit point");
        }
        Serial.println("");
        Serial.print(p);
        Serial.print(" x:");
        Serial.print(circularaccbufferx[circbuffpointer+indexer]-int(acc_calibrationx/100));
        Serial.print(" raw:  ");
        Serial.print(circularaccbufferx[circbuffpointer+indexer]);
        Serial.print(" ~y:");
        Serial.print(circularaccbuffery[circbuffpointer+indexer]-int(acc_calibrationy/100));
        Serial.print(" z:");
        Serial.print(circularaccbufferz[circbuffpointer+indexer]-int(acc_calibrationz/100));
        indexer++;
        if (indexer+circbuffpointer>99) indexer=-circbuffpointer;  
      } 


      Serial.println ("");   
      Serial.print (" zaxis peak:");
      Serial.print (zaxispeak);
      Serial.print (" xaxis peak:");
      Serial.print (xaxispeak);
      Serial.print (" yaxis peak:");
      Serial.println (yaxispeak);

      Serial.print (" zaxis valley:");
      Serial.print (zaxisvalley);
      Serial.print (" xaxis valley:");
      Serial.print (xaxisvalley);
      Serial.print (" yaxis valley:");
      Serial.println (yaxisvalley);

      Serial.print (" x cal raw:");
      Serial.println (acc_calibrationx);
      Serial.print (" x cal:  ");
      Serial.println (int(acc_calibrationx/100));

    } 
    //temp override of accel sensing
    /*xaxisvalley=60;
     xaxisvalley=60;
     zaxisvalley=60;
     xaxispeak=60;
     yaxispeak=60;
     zaxispeak=60;*/


    for (int x=0; x<9; x++) { //load up pending all notes with volume numbers
      if (padmode[x]==1) { 
        
        if ((x==6) || (x==5)|| (x==2)|| (x==1)) { //top of Kyub
          padvolume[x]=-zaxisvalley;
          padmode[x]=2;    
        }
        if ((x==4)) { //side of Kyub
          padvolume[x]=xaxispeak; 
          padmode[x]=2;
        }
        if ((x==3) ) {
          padvolume[x]=-xaxisvalley;
          padmode[x]=2;
        }
        if ((x==0) ) {
          padvolume[x]=-yaxisvalley;
          padmode[x]=2;
        }
        if ((x==7)|| (x==8) ) {
          padvolume[x]=yaxispeak;
          padmode[x]=2;
        }
      }
    }
  }//end of triggerpoint


//play notes *****************************************************************
  for (int x=0; x<9; x++){
    if (padmode[x]==2){
      notevolume=int((padvolume[x])*10);  //??room for improviment--mapping of accel to volume
      if (notevolume>127) notevolume=127;
      if (notevolume<10) notevolume=10;
      switch (x){
        case 0:
          playNote(x);
          break;
        case 1:
          playNote(x);
          break;
        case 2:
          playNote(x);
          break;
        case 3:
          playNote(x);
          break;
        case 4:
          playNote(x);
          break;
        case 5:
          playNote(x);
          break;
        case 6:
          playNote(x);
          break;
        case 7:
          playNote(x);
          break;
        case 8:
          playNote(x);
          break;
      }
      padmode[x]=4;
    }
  }


//turn off notes **************************************************
  for (int x=0; x<9; x++){
    if ((padstate[x]==0) && (padmode[x]==4)&& (micros()-padlasttime[x]>min_note_duration)){ //need reset
      switch (x){
        case 0:
          playNote(x);
          break;
        case 1:
          playNote(x);
          break;
        case 2:
          playNote(x);
          break;
        case 3:
          playNote(x);
          break;
        case 4:
          playNote(x);
          break;
        case 5:
          playNote(x);
          break;
        case 6:
          playNote(x);
          break;
        case 7:
          playNote(x);
          break;
        case 8:
          playNote(x);
          break;
      }
      padmode[x]=0;
    }
  }

  if ((micros()/1000000>next) &&  (consolemidimode==0)){ // wait a second so as not to send massive amounts of data
    next++;
    for (int x=0; x<11; x++){  //prints out all charge times
      Serial.print(" pin:");
      Serial.print(x);
      Serial.print("=");
      Serial.print(chargetime[x]-cap_calibration[x]);
    }
    Serial.println("");
//    Serial.print ("padvol#1:  ");
//    Serial.print (padmode[1]);
//    Serial.print ("-xvalley: ");
//    Serial.print(-xaxisvalley); 
//    Serial.println(""); 
  }

}//end main loop

//chooses pitch and channel from the array based on pin9 and pin10 settings then turns notes on or off
void playNote(int n){
  int y;
  int z;
  if (chord1==0 && chord2==0){
    channel=0x90+modeChannel[0];
  }
  if (chord1==1 && chord2==0){
    channel=0x90+modeChannel[1];
  }
  if (chord1==0 && chord2==1){
    channel=0x90+modeChannel[2];
  }
  if (chord1==1 && chord2==1){
    channel=0x90+modeChannel[3];
  }
  if (chord1==0 && chord2==0){
    y=noteIdx[n];
    z=noteIdx[n+1];
  }
  if (chord1==1 && chord2==0){
    y=noteIdx[n+9];
    z=noteIdx[n+10];
  }
  if (chord1==0 && chord2==1){
    y=noteIdx[n+18];
    z=noteIdx[n+19];
  }
  if (chord1==1 && chord2==1){
    y=noteIdx[n+27];
    z=noteIdx[n+28];
  }
  for (int x=y; x<z; x++){
    pitch=note[x];
    if (padmode[n]==2) usbMIDI.sendNoteOn(pitch, notevolume, channel);
    if (padmode[n]==4) usbMIDI.sendNoteOff(pitch, notevolume, channel);
  }
}
