/**

KUTDuino_Control

(C) 2015 Andrew Koenig, KE5GDB - Project Engineer, KUTD-FM

Features to be implemented:
> 1-wire thermal sensors (Transmitter, PA, Rack, Ambient)
> Humidity Sensor

Analog Values:
PA Current, PA Voltage, RF Forward, RF Reflected, Compression, VSWR

Commands:
i - Inhibits Transmission (MX-15)
t - Transmits (MX-15)
s - Enable Stereo Generation (Optimod)
l - Disable Stereo; Enable Mono (Left) (Optimod)
r - Disable Stereo; Enable Mono (Right) (Optimod)
c - Clears high VSWR transmit inhibit
p - Programs EEPROM with new values, direct serial access required (prompts)

Connections:
a0 - PA Current
a1 - PA Voltage
a2 - RF Forward
a3 - RF Reflected
a4 - Gain/Reduction (Compression)

[d0/d1 - Serial]
d2 - Transmit Inhibit (HIGH = inhibit, LOW = transmit)
d3 - Enable Stereo Generation (500ms pulse)
d4 - Disable Stereo; Enable Mono (Left) (500ms pulse)
d5 - Disable Stereo; Enable Mono (Right) (500ms pulse)

Circuit notes:
G/R from optimod is [-10v, 0v], so an inverting op-amp circuit
is used to bring this from 0-5v. The negative feedback resistor
is 1.8k; no input resistor is being used. 

All Stereo/Mono toggles are fed 5v constant, but the ground is 
connected to a 2n3904 NPN transistor. The d3-d5 will connect
the negative side of the opto-isolator (internal in the Optimod)
to ground, completing the circuit. The same type of buffer is
utilized on the transmit inhibit (d2), but the MX-15 will inhibit 
transmissions when both pins are grounded. 

Programming notes:
There are a couple of functions programmed locally to mitigate transmitter
problems in the event that the controlling PC or Pi does not respond. The
VSWR check calculates the VSWR, and will inhibit transmission if the reflected
power or VSWR exceed defined amounts. To program these values, run the 'p'
command. The program will check for high SWR several times before turning the 
transmitter off for good. Once the problem is remedied, run the 'c' command.

All constants are stored in EEPROM. You need to have the entire array of 
parameters ready to be entered when running the 'p' command, as there is no
method to only enter some parameters. 
**/

#include <math.h>
#include <EEPROM.h>

char incomingChar;

/**

These arrays bring the raw values to their actual values following this form:
y=mx+b, or x=(y-b)/x

[0] - Actual value
[1] - m
[2] - b
[3] - Raw value

**/

// DEFINE VARIABLES
double paCurrent[] = {0, 0, 0, 0, 0};     // [1, 2] are in EEPROM [0,4] respectively
double paVoltage[] = {0, 0, 0, 0, 0};     // [1, 2] are in EEPROM [8,12] respectively
double rfForward[] = {0, 0, 0, 0, 0};     // [1, 2] are in EEPROM [16,20] respectively
double rfReflected[] = {0, 0, 0, 0, 0};   // [1, 2] are in EEPROM [24,28] respectively
double compression[] = {0, 0, 0, 0, 0};   // [1, 2] are in EEPROM [32,36] respectively
double vswr;

boolean vswrInhibit = 0;
int vswrCount = 0;
int max_vswr_count;        // Number of retrys before transmitter must be checked (recalled from EEPROM in setup())
double max_rfReflected;    // Max reflected RF allowed (recalled from EEPROM in setup())
double max_vswr;           // Max VSWR allowed (recalled from EEPROM in setup())

// DEFINE SOME FUNCTIONS

// Write double to EEPROM
void EEPROMWriteDouble(int address, double value)
{
  byte* p = (byte*)(void*)&value;
    for (int i = 0; i < sizeof(value); i++)
      EEPROM.write(address++, *p++);
}

// Read double from EEPROM
double EEPromReadDouble(int address)
{
  double value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(address++);
  return value;
}

// Enable the reset function
void(* resetFunc) (void) = 0;

void programEEPROM()
{
  // Explain how this will work, and request a character to proceed
  Serial.println("Enter the appropriate value and press enter.");
  Serial.println();  
  Serial.println("Remeber: y=ax^2+bx+c, and enter .0001 for 0");
  Serial.println();
  Serial.println("Enter 'p' when ready");
  while(Serial.read() != 'p');
  
  // Setup the variable
  float serialValue = 0;
  
  Serial.println("PA Current (I) a: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(0, (double)serialValue);
  serialValue = 0;
  
  Serial.println("PA Current (I) b: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(4, (double)serialValue);
  serialValue = 0;
  
  Serial.println("PA Current (I) c: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(8, (double)serialValue);
  serialValue = 0;
  
  Serial.println("PA Voltage (V) a: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(12, (double)serialValue);
  serialValue = 0;
  
  Serial.println("PA Voltage (V) b: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(16, (double)serialValue);
  serialValue = 0;
  
  Serial.println("PA Voltage (V) c: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(20, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Forward a: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(24, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Forward b: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(28, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Forward c: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(32, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Reflected a: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(36, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Reflected b: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(40, (double)serialValue);
  serialValue = 0;
  
  Serial.println("RF Reflected c: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(44, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Compression a: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(48, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Compression b: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(52, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Compression c: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(56, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Max VSWR retries: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(60, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Max reflected power (watts): ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(64, (double)serialValue);
  serialValue = 0;
  
  Serial.println("Max VSWR: ");
  while (serialValue == 0)
      serialValue = Serial.parseFloat();
  EEPROMWriteDouble(68, (double)serialValue);
  serialValue = 0;
  
  // Reset the ATMega
  resetFunc();
  
}

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  Serial.println("POWER ON");
  Serial.println("PA I, PA V, RF FWD, RF REF, COMPRESSION, VSWR");
  Serial.println("COMMANDS:");
  Serial.println("i - inhibit transmission");
  Serial.println("t - enable transmission");
  Serial.println("s - enable stereo generation");
  Serial.println("l - disable stereo; switch to mono left");
  Serial.println("r - disable stereo; switch to mono right");
  Serial.println("c - clears high VSWR transmit inhibit");
  Serial.println("p - reprogram raw to actual values");
  Serial.println("=======================================");
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  paCurrent[1] = EEPromReadDouble(0);
  paCurrent[2] = EEPromReadDouble(4);
  paCurrent[3] = EEPromReadDouble(8);
  paVoltage[1] = EEPromReadDouble(12);
  paVoltage[2] = EEPromReadDouble(16);
  paVoltage[3] = EEPromReadDouble(20);
  rfForward[1] = EEPromReadDouble(24);
  rfForward[2] = EEPromReadDouble(28);
  rfForward[3] = EEPromReadDouble(32);
  rfReflected[1] = EEPromReadDouble(36);
  rfReflected[2] = EEPromReadDouble(40);
  rfReflected[3] = EEPromReadDouble(44);
  compression[1] = EEPromReadDouble(48);
  compression[2] = EEPromReadDouble(52);
  compression[3] = EEPromReadDouble(56);
  max_vswr_count = (int)EEPromReadDouble(60);
  max_rfReflected = EEPromReadDouble(64);
  max_vswr = EEPromReadDouble(68);
  delay(2500);
}

void loop() {
  if (Serial.available() > 0) {

     incomingChar = Serial.read();
     
     switch (incomingChar) {
       case 'i': // Inhibit Transmission
         digitalWrite(2, HIGH);
         break;
       
       case 't': // Transmit
         digitalWrite(2, LOW);
         break;
         
       case 's': // Enable Stereo Generation
         digitalWrite(3, HIGH);
         delay(500);
         digitalWrite(3, LOW);
         break;
       
       case 'l': // Disable Stereo, Enable Mono (Left)
         digitalWrite(4, HIGH);
         delay(500);
         digitalWrite(4, LOW);
         break;
       
       case 'r': // Disable Stereo, Enable Mono (Left)
         digitalWrite(5, HIGH);
         delay(500);
         digitalWrite(5, LOW);
         break;
         
       case 'c': // Clear VSWR Transmit inhibit
         vswrCount = -1; 
         Serial.println("VSWR transmit inhibit cleared");
         break;
         
       case 'p': // Run the EEPROM programming routine
         programEEPROM();
         break;
       
       default:
         Serial.println("INVALID COMMAND!");
       }
  }

  // read the analog in value:
  paCurrent[4] = analogRead(A0);
  paVoltage[4] = analogRead(A1);
  rfForward[4] = analogRead(A2);
  rfReflected[4] = analogRead(A3);
  compression[4] = analogRead(A4);
  
  
  // Calculate real values from raw values
  paCurrent[0] = ((paCurrent[4] - paCurrent[3])/paCurrent[2]);
  paVoltage[0] = ((paVoltage[4] - paVoltage[3])/paVoltage[2]);
  rfForward[0] = ((rfForward[4] - rfForward[3])/rfForward[2]);
  rfReflected[0] = ((rfReflected[4] - rfReflected[3])/rfReflected[2]);
  compression[0] = ((compression[4] - compression[3])/compression[2]);
  
  // Eliminate non-real results
  if(rfForward[0] < 0)
    rfForward[0] = 0.0;
    
  if(rfReflected[0] < 0)
    rfReflected[0] = 0.0;
  
  // Calculate VSWR
  vswr = ((1 + sqrt(rfReflected[0] / rfForward[0])) / (1 - sqrt(rfReflected[0] / rfForward[0])));
  
  // Check for high reflected power OR VSWR
  if((vswr > max_vswr) || (rfReflected[0] > max_rfReflected))
  {
    // Notify Serial client
    Serial.println("ALERT: HIGH SWR - CHECK ANTENNA!");
    
    // Turn off transmitter
    digitalWrite(2, HIGH);
    
    // Set flag
    vswrInhibit = 1;
    delay(5000);
  }
  
  // Turn transmitter back on if VSWR lowers
  if((vswr < max_vswr || isnan(vswr)) && (rfReflected[0] < max_rfReflected) && (vswrCount < max_vswr_count) && vswrInhibit)
  {
    // Notify Serial client
    Serial.print("ALERT: VSWR in safe range; transmitting. VSWR Count = ");
    Serial.println(vswrCount);

    // Turn on transmitter
    digitalWrite(2, LOW);
    
    delay(1000);
    
    // Set flag
    vswrInhibit = 0;
    vswrCount++;
  }

  // print the results to the serial monitor:
  Serial.print(paCurrent[0]);
  Serial.print(", ");
  Serial.print(paVoltage[0]);
  Serial.print(", ");
  Serial.print(rfForward[0]);
  Serial.print(", ");
  Serial.print(rfReflected[0]);
  Serial.print(", ");
  Serial.print(compression[0]);
  Serial.print(", ");
  Serial.println(vswr);
  delay(100);
}
