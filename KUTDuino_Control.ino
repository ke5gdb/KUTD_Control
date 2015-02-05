/**

KUTDuino_Control

(C) 2015 Andrew Koenig, KE5GDB - Project Engineer, KUTD-FM

Analog Values:
PA Current, PA Voltage, RF Forward, RF Reflected, Compression

Commands:
i - Inhibits Transmission (MX-15)
t - Transmits (MX-15)
s - Enable Stereo Generation (Optimod)
l - Disable Stereo; Enable Mono (Left) (Optimod)
r - Disable Stereo; Enable Mono (Right) (Optimod)

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


**/

#include <math.h>

char incomingChar;

/**

These arrays bring the raw values to their actual values following this form:
y=mx+b, or x=(y-b)/x

[0] - Actual value
[1] - m
[2] - b
[3] - Raw value

**/
double paCurrent[] = {0, 48.4, 159.4, 0};
double paVoltage[] = {0, 82, 5, 0};
double rfForward[] = {0, 33.125, 285.75, 0};
double rfReflected[] = {0, 217, 131.5, 0};
double compression[] = {0, 18, 424, 0};
double vswr;

boolean vswrInhibit = 0;
int vswrCount = 0;
int max_vswr_count = 3;        // Number of retrys before transmitter must be checked
double max_rfReflected = 1.5;  // Max reflected RF allowed
double max_vswr = 2;           // Max VSWR allowed

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  Serial.println("POWER ON");
  Serial.println("PA I, PA V, RF FWD, RF REF, COMPRESSION");
  Serial.println("COMMANDS:");
  Serial.println("i - inhibit transmission");
  Serial.println("t - enable transmission");
  Serial.println("s - enable stereo generation");
  Serial.println("l - disable stereo; switch to mono left");
  Serial.println("r - disable stereo; switch to mono right");
  Serial.println("c - clears high VSWR transmit inhibit");
  Serial.println("=======================================");
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
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
         
       case 'c':
         vswrCount = -1; 
         Serial.println("VSWR transmit inhibit cleared");
         break;
       
       default:
         Serial.println("INVALID COMMAND!");
       }
  }

  // read the analog in value:
  paCurrent[3] = analogRead(A0);
  paVoltage[3] = analogRead(A1);
  rfForward[3] = analogRead(A2);
  rfReflected[3] = analogRead(A3);
  compression[3] = analogRead(A4);
  
  
  // Calculate real values from raw values
  paCurrent[0] = ((paCurrent[3] - paCurrent[2])/paCurrent[1]);
  paVoltage[0] = ((paVoltage[3] - paVoltage[2])/paVoltage[1]);
  rfForward[0] = ((rfForward[3] - rfForward[2])/rfForward[1]);
  rfReflected[0] = ((rfReflected[3] - rfReflected[2])/rfReflected[1]);
  compression[0] = ((compression[3] - compression[2])/compression[1]);
  
  // Calculate VSWR
  vswr = ((1 + sqrt(rfReflected[0] / rfForward[0])) / (1 - sqrt(rfReflected[0] / rfForward[0])));
  
  // Check for high reflected power OR VSWR
  if((vswr > 2) || (rfReflected[0] > .75))
  {
    // Notify Serial client
    Serial.println("CHECK ANTENNA - HIGH SWR");
    
    // Turn off transmitter
    digitalWrite(2, HIGH);
    
    // Set flag
    vswrInhibit = 1;
    delay(5000);
  }
  
  // Turn transmitter back on if VSWR lowers
  if((vswr < max_vswr) && (rfReflected[0] < max_rfReflected) && (vswrCount < max_vswr_count) && vswrInhibit)
  {
    // Notify Serial client
    Serial.print("VSWR in safe range; transmitting. VSWR Count = ");
    Serial.println(vswrCount);

    // Turn on transmitter
    digitalWrite(2, LOW);
    
    delay(1000);
    
    // Set flag
    vswrInhibit = 0;
    vswrCount++;
  }

  // print the results to the serial monitor:
  Serial.print(paCurrent[3]);
  Serial.print(", ");
  Serial.print(paVoltage[3]);
  Serial.print(", ");
  Serial.print(rfForward[3]);
  Serial.print(", ");
  Serial.print(rfReflected[3]);
  Serial.print(", ");
  Serial.print(compression[3]);
  Serial.print(", VSWR: ");
  Serial.println(vswr);
  delay(100);
}
