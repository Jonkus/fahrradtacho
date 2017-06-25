/* 
Fahrradtacho Version 1.0, 25.06.2017

© Daniel und Jonathan Kuhse

 */
 
#include <OneWire.h>            // Temp-Sensor-Libs
#include <DallasTemperature.h>

#include <SPI.h>                // Display-Libs
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#include <EEPROM.h>             // saving data in eeprom
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>


// ###################################################
// #            variables and constants              #
// ###################################################

// Temp-Sensor-Port
#define ONE_WIRE_BUS 14
         
// Configuration for display
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

// config for fahrradtacho
#define ClockPin                  2                       // dynamo input-pin
#define Radumfang                 2.2                     // wheel circumference: meters per turn of dynamo
#define Memoryaddress             42                      // Address where data is stored persistently. TODO: change address dynamicly


volatile unsigned long    DynImpulse;                     // impulses of dynamoimpulse since last read
volatile unsigned long    DynTurns;                       // turns of dynamo, persistent (TODO: save meters instead of Turns
volatile unsigned long    timeOfLastTurn;                 // Time in millis of the last complete cycle
volatile double           velo;                           // current velocity


// Initiate OneWire
OneWire ourWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&ourWire);

// Connect Disply with software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);



void setup() {

  // Initialize display
  display.begin();
  display.setContrast(40);

  // Initialize Temp_sensor
  sensors.begin();

  // Initialize dynamo-scanner TODO: Do we still need this?
  pinMode(ClockPin, INPUT);

  // set the interrupt according to your Arduino
  attachInterrupt(0, InputFilter, CHANGE);  // INT0 for Nano and INT1 for Micro! 

  // Read saved data
  ReadEEprom();
}

void loop()
{
  // print Display with current data
  output(velo, (DynTurns * Radumfang) / 1000 );

// delay(1000);

  // TODO: This would not work very well, as loop can take longer than one DynTurn. Thus it is totally random if writing occurs
  if ((DynTurns % 10) == 0) {
    WriteEEprom();
  }

}


// ###################################################
// # interrupt-func for sampling input pin of dynamo #
// ###################################################
void InputFilter(){
  
  // Increment to next impulse
  DynImpulse++;
  
  // 28 Increments make one Turn: e.g. Shimano-dynamo has 14 poles with 28 changing flanks
  if (DynImpulse >= 28) {
    
    // every turn, get the elapsed time since last turn...
    unsigned long elapsedTime;
    elapsedTime = (millis() - timeOfLastTurn);
    
    // ... and compute the velocity
    velo = (Radumfang / elapsedTime)*3600;

    // save Time, reset Impulses and increment Turns
    timeOfLastTurn = millis();
    DynImpulse = 0;
    DynTurns++;
    
  }
  
}

// ###################################################
// #        function for displaying the data         #
// ###################################################

void output(int v, double s) {

  sensors.requestTemperatures();

  display.clearDisplay();
  // text display tests
  display.setTextColor(BLACK);
  display.setCursor(0,0);

  // print velocity
  display.setTextSize(2);
  display.print(v);
  display.setTextSize(1);
  display.println(" Km/h");
  display.print("\n");

  // print distance
  display.setTextSize(1);
  display.print(s);
  display.println(" Km");

  // print temperature
  display.setTextSize(1);
  display.print("Temp: ");  
  display.print(sensors.getTempCByIndex(0) );
  display.println(" C");
  
  display.display();
}

// ###################################################
// #             writing data to eeprom              #
// ###################################################
void WriteEEprom(){

      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four =  (DynTurns & 0xFF);
      byte three = ((DynTurns >> 8) & 0xFF);
      byte two =   ((DynTurns >> 16) & 0xFF);
      byte one =   ((DynTurns >> 24) & 0xFF);

      int address = Memoryaddress;
      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
  
}
// ###################################################
// #            reading data from eeprom             #
// ###################################################
void ReadEEprom(){
  int address = Memoryaddress;
  
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  //Return the recomposed long by using bitshift.
  DynTurns = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

