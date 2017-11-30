// ****************************
// uMANIAC-V0.5 PRE_ALPHA!
// uMANIAC - generative sequencer music module for Arduino with a cellular automata core
// NANO + cheap OLED - GATE OUTPUTS VERSION - this version is a simple example 1D CA with 3 or 5 neighbours, that simply assigns 12 cells directly to drive gate outputs via the 
// digital out pins. its aimed at experimenters / DIYers who want to get up and running with complex/chaotic/ordered/cyclic systems with 
// a minumum of spend/circuitry.. 
//
// PLEASE NOTE: THIS IS A PRE_ALPHA RELEASE! THERES NOT A LOT IN THE WAY OF CODE COMMENTING 0R USER MANUAL!
// THIS WILL COME BUT IT TAKES TIME :) ! 
// ITS DESIGNED TO RUN ON A NANO, BUT IT CAN BE EASILY PORTED TO 32u4 e.g. MICRO, MEGA, DUE, TEENSY etc. just remember to sort the i/o asnd clock in pins properly
//
// Dave Burraston 2017
// www.noyzelab.com
// ****************************
// ANYONE WISHING TO CONTRIBUTE FINANCIALLY, OR OTHERWISE COLLABORATE ON THIS PROJECT PLEASE FEEL FREE TO EMAIL ME : noyzelab@gmail.com
// ****************************

#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 12
Adafruit_SSD1306 display(OLED_RESET);
#define SSD1306_LCDHEIGHT                 64
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// PIN ALLOCATIONS
// CLOCK IN = D2
// GATE OUTS = D3 to 11, & D13
// MODE ACTION = A1 [this is currently a switch, but I have allocated this on Analogue for future use i.e. possible to have multiple mode actions ..
// MODE POT = A0
// SUBMODE POT = A2
// k size = A3
// CA LENGTH = A6
// variable pot pin = A7
 
// NOTE : A4 & A5 are used by the OLED I2C bus, and D12 is the OLED reset pin, a possible variant is to instead use an I2C to LCD,
// which would free up D12 for mode action switch, and allow for A1 to be used as another analogue / CV input

// Analogue pins
const byte modepotpin = A0;
const byte modeactionpin = A1;
const byte submodepotpin = A2;
const byte ksizepotpin = A3;
const byte lengthpotpin = A6;
const byte varparam1pin = A7;

// Digital pins
const byte clockpin = 2;
// GATE OUTS = D3 to 11, & D13

byte ksize =5;
unsigned int Lsize = 10;
byte Lmin = 8;
const unsigned int maxLsize = 75;  // maximum number of cells. u will need to keep an eye on this when u compile that u dont go over the RAM size, unless u are running on a DUE..
unsigned long rnum =  0xFF00FF00; // 0xcccccccc; // 0x0f0f0f0f;  0xFF00FF00;
byte elembank = 0;
byte rtabsize = 32; // NEEDS to be 32 for v2k5
byte rtab[32]; // e.g. = { .. 1,1,1,1, 0,0,0,0} NOTE THIS VIEW OF THE ARRAY is NOT in BINARY, it is REVERSED!!!
byte rtablkup = 0;
byte oldcells[maxLsize]; 
byte newcells[maxLsize]; 
unsigned int gens = 0;

byte  mode = 2; // 0 = rand all, 1 = left shift, 2 = right shift,  3 = all off
byte submode = 0;
byte camodereset = 0; // 0 = unpressed , 1 = pressed , based on modeaction pin
// note for digital pullup i subtract 1 when reading switches for postitive logic in code for readability

byte penddir = 0;


unsigned long newseedcntr = 0; 
unsigned long newrandseed;
byte firstpulse = 0;

unsigned int ksizepot, lengthpot, varparam1pot;

unsigned long insomute = 8;

byte gotclk = 0;

byte arbgatesassign = 0;

byte tmp1,tmp2;

// ***************************** SETUP **************************** 
void setup(){

  byte i;
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    // Clear the buffer.
  display.clearDisplay();

      //Serial.begin(9600); // ONLY USE THIS FOR TESTING. TURN SERIAL OFF FOR FASTER CLOCKING VIA THE CLOCK IN ON PIN 2
      //Serial.println(); 
      //Serial.println("uMANIAC in setup... oled version"); 
      //Serial.println(); 
    
  // ALLOCATE PINS  
  pinMode(modeactionpin, INPUT_PULLUP); // mode action input 
       // set up pins 3 to 11 for cell gate outputs   
       for (i = 3; i < 12; i = i + 1) 
        {
         pinMode(i, OUTPUT);     
        }
        pinMode(13,OUTPUT); // cell 9 pin        
        clearca();
        oldcells[9] = 1;           
    ruleinit();    
    gens =0;
      // digital pin 2 = clock in for interrupt!!!!!
      attachInterrupt(0, castep, RISING); // NANO/UNO
     //attachInterrupt(2, castep, RISING); // 2 = Teensy LC
      //attachInterrupt(digitalPinToInterrupt(clockpin), castep, RISING); // 7 = MICRO / LEONARDO


 }
// ***************************** end SETUP **************************** 

// ********* MAIN LOOP *********
void loop(){
    byte i;
     // READING INPUTS ***************************   
      mode = analogRead(modepotpin)/64;   
      submode = analogRead(submodepotpin)/64;        
      camodereset = 1-digitalRead(modeactionpin);
      lengthpot = analogRead(lengthpotpin)/4;
      lengthpot=map(lengthpot,0,255,1,maxLsize);  
      ksizepot = analogRead(ksizepotpin)/256;
      elembank = analogRead(ksizepotpin)/128; // this gets 8 bank numbers for the elementary manual selek
      varparam1pot = analogRead(varparam1pin);   // insomniac mutatation multiplier etc    
 
     // END READING INPUTS ***************************

    processinputs(); // PROCESS INPUTS  ***************************
    
  // ********************SECTION FOR TESTING WITHOUT EXT CLOCK. COMMENT OUT WHEN USING CLOCK IN ****************
    // RUN CA ALGORITHM  
    //delay(250);
    //castep();
 // ******************** END _ SECTION FOR TESTING WITHOUT EXT CLOCK. COMMENT OUT WHEN USING CLOCK IN ****************
  
    // DISPLAY ROUTINES

// SET OUTPUS OR ANY VAR NEED SETTING ???

// incremenmnt or twiddle seed cntr
  newseedcntr = newseedcntr + 1UL;
  if (newseedcntr > 4294000UL) 
  {
    newseedcntr = 0;
    newrandseed = millis();
     randomSeed(newrandseed);      
    }
  // **  DISPLAY INFO TO OLED / serial  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
 
  display.setCursor(32,0); // DISPLAY RULE NUM
  switch (ksize)
  {
    case 3:
      //display.setCursor(96,0);  
      display.print(byte(rnum),HEX);  
      display.print(" : "); 
      display.print(byte(rnum));
    break;
    case 4:
      //display.setCursor(64,0);
      display.print("    "); 
      display.print(word(rnum),HEX);
    break;
    case 5:
      //display.setCursor(32,0);
      display.print(rnum,HEX);
    break;
        default:
        break;    
  } // end switch ksize
 
  display.setCursor(32,16);
  display.print("L");  
  display.print(Lsize);
  display.setCursor(92,16);
  display.print("K");  
  display.print(ksize);
  display.setCursor(80,32);
  display.print("G");  
  display.print(gens%1000); 
 
  display.setCursor(44,48); // position for mode/submode display
  switch (mode)// MODE DISPLAY
  {
    case 0:
        display.print("GM ");  // GLOBAL MUTATE     
        mutatesubmodedisplay();          
    break;
   case 1:
    display.print("IG ");  // INSOMNIAC GLOBAL MUTATOR 
        mutatesubmodedisplay();  
      display.setCursor(32,32);
      display.print("I");  
      display.print(insomute);  
    break;
   case 2:
    display.print("LM ");  // LOCAL MUTATE
    mutatesubmodedisplay();        
    break;    
   case 3:
    display.print("IL ");  // INSOMNIAC LOCAL MUTATOR 
        mutatesubmodedisplay();  
      display.setCursor(32,32);
      display.print("I");  
      display.print(insomute);  
    break;
     case 4: //GLOCAL MUTATE
    display.print("gM ");  // GLOCAL MUTATE
    glocalsubdisp();          
    break; 
  case 5:
    display.print("Ig ");  // INSOMNIAC GLOCAL MUTATOR 
        glocalsubdisp();  
      display.setCursor(32,32);
      display.print("I");  
      display.print(insomute);  
    break;
    case 6: 
       display.print("EL ");  // ELEMENTARY k3
       elemsubmodedisp();
      display.setCursor(32,32);
      display.print("B");  
      display.print(elembank*32);          
    break; 
    case 7: 
       display.print("E4 ");  // 250 K4
       elemsubmodedisp();
      display.setCursor(32,32);
      display.print("B");  
      display.print(elembank*32);          
    break; 
    case 8: 
       display.print("E5 ");  // 250 K4
       elemsubmodedisp();
      display.setCursor(32,32);
      display.print("B");  
      display.print(elembank*32);          
    break;     
    case 9:
    display.print("DS ");  // LEFT/DOWN SHIFT
    break;
    case 10:
    display.print("US ");  // RIGHT/UP SHIFT
    break;
    case 11:
    display.print("PEN");  // PENDULUM
    break; 
    case 12:
    display.print("ON ");  // ALL ON
    break;
    case 13:
    display.print("OFF");  // ALL OFF
    break;      
    case 14:
    display.print("ARB");  // ARBITRARY CELL GATE SETUP 
    arbcelldisp();
    break;      
 
    default:
    break;
  } // end switch case
        
  // do any clock dependant stuff here that can't happen in the ISR
  if   (gotclk = 1)
  {
  cabardisplay();
  //caspacetimedisplay(); // note this is for debugging only 
  gotclk = 0;
  }
  display.display();
}
// ********* end MAIN LOOP *********

void arbcelldisp()
{
switch (submode) // SUBMODE DISPLAY
      {
        case 0:
        display.print("ACBC");  
        break;
        case 1: 
        display.print("ARBC"); 
        break;
        case 2: 
        display.print("ACBR");        
       break;
        case 3:  
        display.print("ARBR");       
       break;
        case 4:  
        display.print("AGBR");         
        break;   
         case 5:  
        display.print("GCTR");         
        break;                         
        default:
        break;
      } // end switch case submode    
}

void mutatesubmodedisplay()
{
switch (submode) // SUBMODE DISPLAY
      {
        case 0:
        display.print("_RCL");  
        break;
        case 1: 
        display.print("_R__"); 
        break;
        case 2: 
        display.print("__C_");        
       break;
        case 3:  
        display.print("___L");       
       break;
        case 4:  
        display.print("_RC_");         
        break;   
        case 5: 
        display.print("_R_L");           
        break;
        case 6:
        display.print("__CL");           
        break;    
        case 7:
        display.print("KRCL");           
        break;        
         case 8: 
        display.print("KR__"); 
        break;
        case 9: 
        display.print("K_C_");        
       break;
        case 10:  
        display.print("K__L");       
       break;
        case 11:  
        display.print("KRC_");         
        break;   
        case 12: 
        display.print("KR_L");           
        break;
        case 13:
        display.print("K_CL");           
        break;   
        case 14:
        display.print("K___");           
        break;   
        case 15:
        display.print("LM/2");      
        break;                          
        default:
        break;
      } // end switch case submode  
}

void glocalsubdisp()
{
switch (submode) // SUBMODE DISPLAY
      {
        case 0:
        display.print("_R__");  
        break;
        case 1: 
        display.print("KRCL"); 
        break;
        case 2: 
        display.print("_r__");        
       break;
        case 3:  
        display.print("Krcl");       
       break;
        case 4:  
        display.print("__CL");         
        break;   
        case 5: 
        display.print("__C_");           
        break;
        case 6:
        display.print("___L");           
        break;    
        case 7:
        display.print("__cl");           
        break;        
         case 8: 
        display.print("__c_"); 
        break;
        case 9: 
        display.print("___l");        
       break;
        case 10:  
        display.print("FST1");       
       break;
       case 11:
        display.print("L+1");           
        break;
        case 12:
        display.print("L-1");           
        break;
       case 13:
        display.print("L+10");           
        break;
        case 14:
        display.print("L-10");           
        break;
        case 15:
        display.print("LM/2");      
        break;          
        default:
        break;
      } // end switch case submode GLOCAL MUTATE  
}

void elemsubmodedisp()
{
switch (submode) // SUBMODE DISPLAY
      {
        case 0:
        display.print("CL");  
        break;
        case 1: 
        display.print("C_"); 
        break;
        case 2: 
        display.print("_L");        
       break;
        case 3:  
        display.print("END1");       
       break;
        case 4:  
        display.print("FST1");         
        break;   
        case 5: 
        display.print("L+1");           
        break;
        case 6:
        display.print("L-1");           
        break;    
        case 7:
        display.print("LMAX");           
        break; 
        case 8:
        display.print("LMIN");           
        break;
        case 9:  
        display.print("L+10");           
        break;
        case 10:
        display.print("L-10");           
        break;    
        case 11:
        display.print("LM/2");      
        break;                             
        default:
        break;
      } // end switch case submode     
}

void processinputs()
{  
  switch (mode)
  {
    case 0:
    globalmutate();
    break;
   case 1:
    insomniacglobalmutate();
    break;
   case 2:
    localmutate();
    break;
   case 3:
    insomniaclocalmutate();
    break;
    case 4:
    glocalmutate();
    break;
     case 5:
    insomniacglocalmutate();
    break;     
    case 6:
    ruleselekk3();
    break; 
    case 7:
    ruleselekk4();
    break; 
    case 8:
    ruleselekk5();
    break;           
    case 9:
    leftshift();
    break;
    case 10:
    rightshift();
    break;
    case 11:
    pendulum();
    break;     
    case 12:
    allon();
    break;
    case 13:
    alloff();
    break;   
    case 14:
    arbgates();
    break;           
    default:
    break;
  } // end switch case
// END PROCESS INPUTS  ***************************
}

// GLOBAL MUTATE
void globalmutate()
{
      if (camodereset == 1) 
    {  
      gens = 0;
      globalmutatesubmode();      
      camodereset = 0; // reset the switch status        
    } // end if camodereset == 1
} // END GLOBAL MUTATE 

void insomniacglobalmutate() //  INSOMIAC MUTATOR
{
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        insomute = random(1,varparam1pot/4);
        gens=0;
         }      
  if (gens >insomute-1) // do insomniac mutation submode
        {
        gens=0;     
        globalmutatesubmode();              
        }  // do insomniac mutation submode          
} // END INSOMIAC MUTATOR 

void globalmutatesubmode()
{
switch (submode)
      {
        case 0:  // rand rule, cells, size 
        ksize=map(ksizepot,0,3,3,5); 
        randR();
        randC();
        randL(); 
        break;
        case 1:
        ksize=map(ksizepot,0,3,3,5);      
        randR();   // rand rule
        break;
        case 2:
        ksize=map(ksizepot,0,3,3,5);      
        randC(); // rand cell
        break;
        case 3:
        ksize=map(ksizepot,0,3,3,5);     
        randL(); // rand size
        break;
        case 4:
        ksize=map(ksizepot,0,3,3,5);      
        randR();
        randC();     
        break;   
        case 5:
        ksize=map(ksizepot,0,3,3,5);      
        randR();
        randL();    
        break;
        case 6:
        ksize=map(ksizepot,0,3,3,5);      
        randC();
        randL(); 
        break;  
        case 7:    // rand K, rule, cells, size 
        ksize=random(3,6);            
        randR();
        randC();
        randL();    
        break;
        case 8:
        ksize=random(3,6);      
        randR();   // rand rule
        break;
        case 9:
        ksize=random(3,6);      
        randC(); // rand cell
        break;
        case 10:
        ksize=random(3,6);     
        randL(); // rand size
        break;
        case 11:
        ksize=random(3,6);      
        randR();
        randC();     
        break;   
        case 12:
        ksize=random(3,6);      
        randR();
        randL();    
        break;
        case 13:
        ksize=random(3,6);       
        randC();
        randL(); 
        break; 
        case 14:
        ksize=random(3,6);        
        break;   
        case 15:      
        Lsize = maxLsize/2;
        break;                                   
        default:
        break;
      } // end switch case submode  
}

// LOCAL MUTATE
void localmutate()
{
      if (camodereset == 1) 
    {  
      gens = 0;
        
      localmutatesubmode();
        camodereset = 0; // reset the switch status        
    } // end if camodereset == 1
} // END LOCAL MUTATE 

void insomniaclocalmutate() //  INSOMIAC MUTATOR
{
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        insomute = random(1,varparam1pot/4);
        gens=0;
         }      
  if (gens >insomute-1) // do insomniac mutation submode
        {
        gens=0;     
        localmutatesubmode();              
        }  // do insomniac mutation submode          
} // END INSOMIAC MUTATOR 


void localmutatesubmode()
{
switch (submode)
      {
        case 0:  // local rand rule, cell, size 
        ksize=map(ksizepot,0,3,3,5); 
        localrandR();
        localrandC();
        localrandL(); 
        break;
        case 1: // local rand rule
        ksize=map(ksizepot,0,3,3,5);      
        localrandR();   
        break;
        case 2: // local rand cell
        ksize=map(ksizepot,0,3,3,5);      
        localrandC(); 
        break;
        case 3: // local rand size
        ksize=map(ksizepot,0,3,3,5);     
        localrandL();
        break;
        case 4:  // local rand rule, cell
        ksize=map(ksizepot,0,3,3,5);      
        localrandR();
        localrandC();     
        break;   
        case 5:  // local rand rule, size
        ksize=map(ksizepot,0,3,3,5);      
        localrandR();
        localrandL();    
        break;
        case 6: // local rand cell, size
        ksize=map(ksizepot,0,3,3,5);      
        localrandC();
        localrandL(); 
        break;  
        case 7:    // rand K, rule, cell\, size 
        ksize=random(3,6);            
        localrandR();
        localrandC();
        localrandL();    
        break;   
        case 8:    // rand K, rule, 
        ksize=random(3,6);            
        localrandR();   
        break; 
         case 9:    // rand K, cell 
        ksize=random(3,6);            
        localrandC();   
        break;  
         case 10:    // rand K, size 
        ksize=random(3,6);            
        localrandL();   
        break;    
        case 11:    // rand K, rule, cell 
        ksize=random(3,6);            
        localrandR();
        localrandC();   
        break;    
        case 12:    // rand K, rule, size 
        ksize=random(3,6);            
        localrandR();
        localrandL();   
        break;          
        case 13:    // rand K, cell, size 
        ksize=random(3,6);            
        localrandC();
        localrandL();  
        break;
       case 14:
        ksize=random(3,6);        
        break;   
        case 15:      
        Lsize = maxLsize/2;
        break;                                        
        default:
        break;
      } // end switch case submode  
}


// GLOCAL MUTATE
void glocalmutate()
{
      if (camodereset == 1) 
    {  
      gens = 0;
      glocalmutatesubmode();      
        camodereset = 0; // reset the switch status        
    } // end if camodereset == 1
} // END GLOCAL MUTATE 


void insomniacglocalmutate() //  INSOMIAC MUTATOR
{
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        insomute = random(1,varparam1pot/4);
        gens=0;
         }      
  if (gens >insomute-1) // do insomniac mutation submode
        {
        gens=0;     
        glocalmutatesubmode();              
        }  // do insomniac mutation submode          
} // END INSOMIAC MUTATOR 

void glocalmutatesubmode()
{
        switch (submode)
      {
        case 0:  // global rand rule 
        ksize=map(ksizepot,0,3,3,5); 
        randR();
        break;
        case 1:
        ksize=random(3,6); // global rand ksize, rule, cells, size           
        randR();
        randC();
        randL();    
        break;
        case 2:
        ksize=map(ksizepot,0,3,3,5);      
        localrandR();   // local rand rule
        break;
        case 3:    // local rand K, rule, cells, size 
        ksize=random(3,6);            
        localrandR();
        localrandC();
        localrandL();    
        break;         
        case 4: //global rand cells, size
        ksize=map(ksizepot,0,3,3,5);      
        randC();
        randL(); 
        break;
        case 5:// global rand cell
        ksize=map(ksizepot,0,3,3,5);      
        randC(); 
        break;
        case 6: // global rand size
        ksize=map(ksizepot,0,3,3,5);     
        randL(); 
        break;
        case 7: // local rand cell, size
        ksize=map(ksizepot,0,3,3,5);      
        localrandC();
        localrandL(); 
        break; 
        case 8: // local rand cell
        ksize=map(ksizepot,0,3,3,5);      
        localrandC(); 
        break;
        case 9: // local rand size
        ksize=map(ksizepot,0,3,3,5);      
        localrandL(); 
        break; 
        case 10:  
        clearca();   
        firstcellset();   
        break;
        case 11:      
        Lsizeplus1();
        break;  
        case 12:    
        Lsizeminus1();
        break;
       case 13:      
        Lsizeplus10();
        break;  
        case 14:    
        Lsizeminus10();
        break;
        case 15:      
        Lsize = maxLsize/2;
        break;            
        default:
        break;
      } // end switch case submode  
}

void ruleselekk3() // 0 to 255 rule selek  with a fixed ksize of 3, uses kpot to select banks of 32 rules
{
    ksize=3;    
    rnum = varparam1pot/32 + (elembank*32);    
    ruleinit();
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        elemsubmode();
        gens=0;   
    } // end camode action switch      
}


void ruleselekk4() // 0 to 255 rule selek  with a fixed ksize of 4, uses kpot to select banks of 32 rules
{
    ksize=4;    
    rnum = varparam1pot/32 + (elembank*32);    
    ruleinit();
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        elemsubmode();
        gens=0;   
    } // end camode action switch      
}

void ruleselekk5() // 0 to 255 rule selek  with a fixed ksize of 5, uses kpot to select banks of 32 rules
{
    ksize=4;    
    rnum = varparam1pot/32 + (elembank*32);    
    ruleinit();
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        elemsubmode();
        gens=0;   
    } // end camode action switch      
}

void elemsubmode()
{
        switch (submode)
      {
        case 0:  // rand  cells, size 
        randC();
        randL(); 
        break;
        case 1:      
        randC(); // rand cell
        break;
        case 2:    
        randL(); // rand size
        break;
        case 3:    
        clearca(); 
        endcellset();
        break;   
        case 4:  
        clearca();   
        firstcellset();   
        break;
        case 5:      
        Lsizeplus1();
        break;  
        case 6:    
        Lsizeminus1();
        break;
        case 7:      
        Lsize = maxLsize;
        break;  
        case 8:    
        Lsize = Lmin;
        break;
       case 9:      
        Lsizeplus10();
        break;  
        case 10:    
        Lsizeminus10();
        break;
        case 11:      
        Lsize = maxLsize/2;
        break;     
        default:
        break;
      } // end switch case submode     
}

void leftshift()
{
      ksize=map(ksizepot,0,3,3,5); 
      if (ksize == 3)
      {
        rnum = 240;
      }
      else
      {
      rnum =  0xFF00FF00;   
      }       
      ruleinit();
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        Lsize = 10;        
        clearca();
        oldcells[9] = 1; 
        gens = 0;
       } 
}
     
void rightshift()
{
      ksize=map(ksizepot,0,3,3,5);   
        if (ksize == 3)
      {
        rnum = 170;
      }
      else
      {
      rnum =  0xcccccccc;  
      }  
      ruleinit();
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        Lsize = 10;        
        clearca();
        oldcells[0] = 1; 
        gens = 0;
       }        
}

void pendulum()
{
      if (camodereset == 1)
      {
        camodereset = 0; // reset the switch status
        ksize=3;  
        rnum = 240;  
        ruleinit();            
        Lsize = 10;     
        penddir = 0;   
        clearca();
        oldcells[9] = 1; 
        gens = 0;
       } 
 if (gens >(Lsize-2))  
        {
         if (penddir == 0)
          {
         rnum = 170;  
         ruleinit();  
         penddir = 1;
         clearca();
         oldcells[0] = 1; 
          }          
         else
          {
         rnum = 240;  
         ruleinit();  
         penddir = 0;
         clearca();         
         oldcells[Lsize-1] = 1; 
          }
        gens = 0;
        }        
}
     
void allon()
{
      rnum = 0xffffffff ;
      Lsize = 10;      
      setca();
      ruleinit();
      gens = 0;
}

void alloff()
{
      rnum = 0xf0f0f0f0;
      Lsize = 10;      
      clearca();
      ruleinit();
      gens = 0;
}

void cabardisplay() // this gets called from the main arduino loop
{
    byte i,j;
    /*
    j = 0; //index into newcells
    for (i=0; i<(display.width() - 3); i=i+3) {     
        if (newcells[j])
        {
          display.fillRect(i, 48, 2, 47,WHITE); 
          //display.fillRect(i, 16, 2, 15,WHITE); 
        }
        j++; // next cell
      }    
*/
/*          j = 0; //index into newcells
    for (i=0; i<100; i=i+10) {     
        if (newcells[j])
        {
          display.fillRect(i, 16, 8, 15,WHITE); // horizontal layout - all 12 cell gates
          //display.fillRect(i, 16, 2, 15,WHITE); 
        }
        j++; // next cell
      }  
*/
        j = 0; //index into newcells
    for (i=0; i<57; i=i+8) {     
        if (newcells[j])
        {
          display.fillRect(0, i, 15, 7,WHITE); // vertical layout - 8 cell gates
          //display.fillRect(i, 16, 2, 15,WHITE); 
        }
        j++; // next cell
      }  
      
 /*         j = 8; //index into newcells
    for (i=48; i<57; i=i+8) {     
        if (newcells[j])
        {
          display.fillRect(16, i, 15, 7,WHITE); // vertical layout - arb gates
          //display.fillRect(i, 16, 2, 15,WHITE); 
        }
        j++; // next cell
      } */
      
       if (digitalRead(11))
        {
          display.fillRect(16, 48, 15, 7,WHITE); // vertical layout - arb gates
        }
       if (digitalRead(13))
        {
          display.fillRect(16, 56, 15, 7,WHITE); // vertical layout - arb gates
        }                    
}

void caspacetimedisplay() // this gets called from the main arduino loop
{
    byte i,j;

    for(i=0; i<16; i++){
      if (newcells[i])
      {
       //display.fillRect(gens%128, 48, 2, 47,WHITE); 
       display.drawPixel(gens%128, 48+i, WHITE);
      } 
    }  
}

void randL()
{
  Lsize = 1+random(lengthpot);
  if (Lsize > maxLsize)
  {
    Lsize = maxLsize; 
  }
  if (Lsize < Lmin)
  {
    Lsize = Lmin; 
  }  
}

void randK()
{
  ksize=map(ksizepot,0,3,3,5);   
}

// ********* RULE INIT *********
// this inits a rule directly form a 32 bit unsigned long
// note that the newer processors e.g. Due have different sized ints and longs see arduino website
void ruleinit()
{
 byte ri;  
  for (ri = 0; ri < rtabsize; ri = ri + 1) 
    {
      if bitRead(rnum, ri)
      {
        rtab[ri]=1;
      }
      else
      {
       rtab[ri]=0;
      } 
    }      
}
// ********* end RULE INIT *********

// ********* RANDOM RULE INIT *********
void randR()
{
   rnum = random();
    ruleinit();
}

void localrandR()
{
  byte mutantbitnum, mutantbit;
  
   //mutantbitnum = random(pow(ksize,2));
   mutantbitnum = random(pow(2,ksize));  
   mutantbit = bitRead(rnum,mutantbitnum);
   bitWrite(rnum,mutantbitnum,1-mutantbit); //flip the random bit 
    ruleinit();
}

void localrandC()
{
  byte mutantbitnum, mutantbit;
  
   //mutantbitnum = random(pow(ksize,2));
   mutantbitnum = random(Lsize);  
   mutantbit = oldcells[mutantbitnum];
   oldcells[mutantbitnum] = 1-mutantbit;
}

void localrandL()
{
  byte mutantbitnum, mutantbit;
  
   //mutantbitnum = random(pow(ksize,2));
   mutantbitnum = random(3);  
  switch (mutantbitnum)
  {
  case 0:
  Lsizeminus1();
  break;
  case 1 : // leave L the same
  break;
  case 2:
  Lsizeplus1();
  break;
  default:
  break;  
  }
   mutantbit = oldcells[mutantbitnum];
   oldcells[mutantbitnum] = 1-mutantbit;
}

void endcellset()
{
   oldcells[Lsize-1] = 1;   
}

void firstcellset()
{
   oldcells[0] = 1;   
}

void Lsizeplus1()
{
  Lsize = Lsize+1;
  if (Lsize > maxLsize)
  {
    Lsize = maxLsize; 
  }
}

void Lsizeminus1()
{
  Lsize = Lsize-1;
  if (Lsize < Lmin)
  {
    Lsize = Lmin; 
  }
}

void Lsizeplus10()
{
  Lsize = Lsize+10;
  if (Lsize > maxLsize)
  {
    Lsize = maxLsize; 
  }
}

void Lsizeminus10()
{
  Lsize = Lsize-10;
  if (Lsize < Lmin)
  {
    Lsize = Lmin; 
  }
}

// ********* CLEAR CA  *********
void clearca()
{
  unsigned int si;  
  for (si = 0; si < Lsize; si = si + 1) 
    {
      oldcells[si] = 0;  
    };  
}
// ********* end CLEAR CA *********

// ********* SET CA  *********
void setca()
{
  unsigned int si; 
  for (si = 0; si < Lsize; si = si + 1) 
    {
      oldcells[si] = 1;  
    };  
}
// ********* end SET CA *********


// ********* RAND CELLS *********
void randC()
{
  unsigned int si;
  //gens = 0;  
  for (si = 0; si < Lsize; si = si + 1) 
    {
      oldcells[si] = random(2);  
    }  
}
// ********* end RAND CELLS *********

void arbgates()
{
      if (camodereset == 1) 
    {  
      gens=0;
         arbgatesassign = submode; // setup the arb gate assign code
        camodereset = 0; // reset the switch status        
    } // end if camodereset == 1
  
}

// ********* CA STEP ********* INTERRUPT SERVICE ROUTINE 
void castep()
{
 unsigned int i;
// this checks for the first ever clock pulse, and reseeds the randon num gen with the num of microseconds it 
// took from reset to the first clock pulse 
    if (!firstpulse)     
    {
      firstpulse = 1;
      newrandseed = micros();
      randomSeed(newrandseed);             
    } 
    
   // compute ca step   
  switch (ksize)
  {
    case 3:
     for (i = 0; i < Lsize; i = i + 1) // do all cells
      { // v2k3 neighbourhood lookups
        rtablkup = 4*oldcells[(i-1+Lsize)%Lsize] + 2*oldcells[i] +  oldcells[(i+1)%Lsize]; // v2k3    
        newcells[i] = rtab[rtablkup];// do table lookups       
      }// end do all cells     
    break;
    case 4:
     for (i = 0; i < Lsize; i = i + 1) // do all cells
      { // v2k4 neighbourhood lookups
       rtablkup = 8*oldcells[(i-1+Lsize)%Lsize] + 4*oldcells[i] + 2 * oldcells[(i+1)%Lsize] + oldcells[(i+2)%Lsize]; 
        newcells[i] = rtab[rtablkup];// do table lookups        
      }// end do all cells     
    break;
    case 5:
     for (i = 0; i < Lsize; i = i + 1) // do all cells
      { // v2k5 neighbourhood lookups
       rtablkup = 16*oldcells[(i-2+Lsize)%Lsize] + 8*oldcells[(i-1+Lsize)%Lsize] + 4*oldcells[i] + 2 * oldcells[(i+1)%Lsize] + oldcells[(i+2)%Lsize]; 
        newcells[i] = rtab[rtablkup];// do table lookups        
      }// end do all cells        
    break;    
    default:
    break;
  } // end switch case
   

     
// ********* CA DISPLAY *********
 /*     for (i = 0; i < 10; i = i + 1) 
    {
       Serial.print( newcells[i]);
    }
      Serial.println();
      */
   for (i = 3; i < 11; i = i + 1) // cells 0 to 7 rto cell gates 0 to 7
    {
       digitalWrite(i,newcells[i-3]);  
    }


    switch (arbgatesassign)
      {
        case 0:   // contiguous cells 8 & 9
        digitalWrite(11,newcells[8]); // cell 8 to arbitrary cell gate A   
        digitalWrite(13,newcells[9]); // cell 9 to arbitrary cell gate B       
        break;
        case 1:      
        digitalWrite(11,newcells[8]); // cell 8 to arbitrary cell gate A   
        digitalWrite(13,newcells[random(Lsize)]); // random cell to arbitrary cell gate B            
         break;
        case 2:    
        digitalWrite(11,newcells[random(Lsize)]); // random cell to arbitrary cell gate A   
        digitalWrite(13,newcells[9]); // cell 8 to arbitrary cell gate B   
        break;
        case 3:    
        digitalWrite(11,newcells[random(Lsize)]); // random cell to arbitrary cell gate A   
        digitalWrite(13,newcells[random(Lsize)]); // random cell to arbitrary cell gate B            
        break;   
        case 4:  
        digitalWrite(11,newcells[gens%Lsize]); // generation counter index into cells to arbitrary cell gate A   
        digitalWrite(13,newcells[random(Lsize)]); // random cell to arbitrary cell gate B      
        break;
        case 5:
        digitalWrite(11,bitRead(gens,0)); // generation counter bit 0  to arbitrary cell gate A   
        digitalWrite(13,bitRead(gens,1)); // generation counter bit 1  to arbitrary cell gate A      
        break;            
        default:
        break;
      } // end switch case submode  


  // copy current generation to old cells  
  //NOTE THIS MUST HAPPEN AFTER   any NOTE/event ON/OFF TESTING HAPPENS
  for (i = 0; i < Lsize; i = i + 1) 
    {
       oldcells[i] = newcells[i];
    }  
    gens = gens +1;
   gotclk = 1; // set this flag for extra display that happens on receipt of clock to indicate a gen has passed   
}
// ********* end CA STEP ********* END ********* INTERRUPT SERVICE ROUTINE 
