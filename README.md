# uMANIAC

****************************
uMANIAC-V0.5 PRE_ALPHA!
uMANIAC - generative sequencer music module for Arduino with a cellular automata core
NANO + cheap OLED - GATE OUTPUTS VERSION - this version is a simple example 1D CA with 3 neighbours (elementary) or 5 neighbours with a variable number of cells. it assigns 12 of these cells to drive gate outputs via the digital out pins. 

its aimed at experimenters / DIYers who want to get up and running with complex/chaotic/ordered/cyclic systems with a minimum of spend/circuitry.. there is also a cv/gate version in progress using SPI DACs, but i intend to keep both versions separate so there is always this 'minimal system'. u can easily add a simple R2R ladder DAC to this module for a few cents => 
http://www.instructables.com/id/Arduino-Audio-Output/

PLEASE NOTE: THIS IS A PRE_ALPHA RELEASE! THERES NOT A LOT IN THE WAY OF CODE COMMENTING, CIRCUIT DIAGRAM 0R USER MANUAL! 
THIS WILL COME BUT IT TAKES TIME :) ! JUST LOOK UP THE STANDARD ARDUINO I/O & OLED examples and you'll be ok. for the cell gate outs i use a simple current limiting resistor e.g. 470ohm to 1k ish. so its completely unbuffered for the current experimental version. for the analogue ins on my eurorack prototype i wire the jack thru the pot and with no jack plug this sends 5v direct to the pot. this is pretty standard method for eurorack.. theres a pic of my prototype in this repository, but note this has a 5pin DIN MIDI out on the panel which is attached to the TX pin. Arduino serial I/O pins 0 & 1 are planned to be left unassigned in this minimal uMANAIC version, in order to obtain the maximum clock speed performance.

ITS DESIGNED TO RUN ON A NANO, BUT IT CAN BE EASILY PORTED TO 32u4 e.g. MICRO, MEGA, DUE, TEENSY etc. just remember to sort the i/o and clock in pins properly

the CA core runs from pin2 via an interrupt service routine ISR, and the UI and general housekeeping etc. is done in the main loop(). this might seem a bit strange to lot of coder types..  

this line in the code can be adjusted to make the maximum system size [i.e. the max number of cells] bigger depending on the processor u are burning it on 
=>

const unsigned int maxLsize = 75;  // maximum number of cells. u will need to keep an eye on this when u compile that u dont go over the RAM size, unless u are running on a DUE..

<= 
note that if u are modding the code, 75 cells should be considered pretty small in order to obtain truly 'mathematical' complex patterns from complex CA rules, and 150 cells is more closer to a scientific minimum :) this is the current build =>
Sketch uses 20,476 bytes (66%) of program storage space. Maximum is 30,720 bytes.
Global variables use 1,786 bytes (87%) of dynamic memory, leaving 262 bytes for local variables. Maximum is 2,048 bytes.


these lines of code i uncomment for testing / hardware build, which calls the ISR to step the CA, which would normally be coming in from pin2. so u might find these handy..  

=>
    
// ********************SECTION FOR TESTING WITHOUT EXT CLOCK. COMMENT OUT WHEN USING CLOCK IN ****************
// RUN CA ALGORITHM  
//delay(250);
//castep();
// ******************** END _ SECTION FOR TESTING WITHOUT EXT CLOCK. COMMENT OUT WHEN USING CLOCK IN ****************

<=

for more info on generative music & cellular automata please read my papers and refs here =>
http://noyzelab.com/research/research.html
 
i also HIGHLY RECOMMEND Andy Wuensche's publications and program DDLab =>
http://ddlab.org/

some of my thoughts on patenting ended up online here =>
https://noise-admiration.blogspot.com.au/2016/10/the-45sis-no2-2016-david-burraston.html


Dave Burraston 2017

www.noyzelab.com

****************************

ANYONE WISHING TO CONTRIBUTE FINANCIALLY, OR OTHERWISE COLLABORATE ON THIS PROJECT PLEASE FEEL FREE TO EMAIL ME : noyzelab@gmail.com

****************************

