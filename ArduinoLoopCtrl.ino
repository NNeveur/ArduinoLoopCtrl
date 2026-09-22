/*
| Commande | Paramètres | Description |
| :--- | :--- | :--- |
| `$SINIT<ant>;` | `<ant>` (0-2) | Initialisation du contrôleur d'antenne RS485 pour l'antenne cible. |
| `$SON<ant>;` | `<ant>` (0-2) | Activation de l'alimentation moteur de l'antenne spécifiée. |
| `$SOF<ant>;` | `<ant>` (0-2) | Coupure d'alimentation moteur pour économie d'énergie. |
| `$SINC<ant><res>;` | `<ant> <res>` | Déplacement sens horaire pour l'antenne cible avec résolution `res` (0 = 1/16 micropas, 4 = pas entier). |
| `$SDEC<ant><res>;` | `<ant> <res>` | Déplacement sens anti-horaire pour l'antenne cible avec résolution `res`. |
| `$SMOV<ant>;` | `<ant>` (0-2) | Exécution du déplacement d'un pas sur l'antenne spécifiée. |
| `$SANT<ant>;` | `<ant>` (0-2) | Sélection / Commutation de l'antenne active sur le bus RS485. |
*/
//
const int ctrl_ant   = 0;     // Controller antenna assing
//
// SparkFun Big easy driver Allegro A4983 or A4988 stepper driver chip
//
const int stepctrl_dir   = 3;     // Direction pin
const int stepctrl_step  = 4;     // Step pin (positive pulse of +1us for each step)
const int stepctrl_enable= 5;     // Enable pin
const int stepctrl_ms3   = 6;     // Microstepping pin MS2
const int stepctrl_ms2   = 7;     // Microstepping pin MS2
const int stepctrl_ms1   = 8;     // Microstepping pin MS1
//
// Increment Stepper
//
void stepctrl_Incr(uint8_t res)
{
  res = 4 - res;                       // Reversed: 0 for no microsteps
                                       // 1 for half step (2 microsteps)
                                       // 2 for quarter step (4 microsteps)
                                       // 3 for eighth step (8 microsteps)
                                       // 4 for sixteenth step (16 microsteps)

  stepctrl_PwrOn();                     // Ensure Power On state
  digitalWrite(stepctrl_dir, LOW);      // Clockwise
  digitalWrite(stepctrl_ms1, (res&0x01)?HIGH:LOW);  // ... Microstep resolution
  digitalWrite(stepctrl_ms2, (res&0x02)?HIGH:LOW);
  digitalWrite(stepctrl_ms3, (res&0x04)?HIGH:LOW);
  digitalWrite(stepctrl_step, HIGH);    // Prime for Movement, turn Step pulse on
//Serial.print("$SINC");
//Serial.println(res);
}
//
// Decrement Stepper
//
void stepctrl_Decr(uint8_t res)
{
  res = 4 - res;                       // Reversed: 0 for no microsteps
                                       // 1 for half step (2 microsteps)
                                       // 2 for quarter step (4 microsteps)
                                       // 3 for eighth step (8 microsteps)
									   // 4 for sixteenth step (16 microsteps)

  stepctrl_PwrOn();                     // Ensure Power On state
  digitalWrite(stepctrl_dir, HIGH);     // Counterclockwise
  digitalWrite(stepctrl_ms1, (res&0x01)?HIGH:LOW);  // ... Microstep resolution
  digitalWrite(stepctrl_ms2, (res&0x02)?HIGH:LOW);
  digitalWrite(stepctrl_ms3, (res&0x04)?HIGH:LOW);
  digitalWrite(stepctrl_step, HIGH);    // Prime for Movement, turn Step pulse on
}

//
// Move Stepper (neends >1+ microsecond delay from positive edge)
//
void stepctrl_Move(void)
{
  digitalWrite(stepctrl_step, LOW);     // Move, turn Step pulse off  
	delay(50);
  digitalWrite(stepctrl_step, HIGH);     // Move, turn Step pulse on  
}

//
// Turn the Stepper On
//
void stepctrl_PwrOn(void)
{
  //digitalWrite(stepctrl_reset, HIGH); // Release Reset, turn Stepper Motor On
  digitalWrite(stepctrl_enable, LOW);   // Enable Stepper
}

//
// Turn the Stepper Off
//
void stepctrl_PwrOff(void)
{
  //digitalWrite(stepctrl_reset, LOW);  // Reset and turn Stepper Motor Off
  digitalWrite(stepctrl_enable, HIGH);  // Disable Stepper, retain last state
}

//
// Init Stepper Outputs
//
void stepctrl_Init(void)
{
  pinMode(stepctrl_dir, OUTPUT);        // Direction Pin    
  pinMode(stepctrl_step, OUTPUT);       // Step Pin
  pinMode(stepctrl_ms3, OUTPUT);        // MS3 pin
  pinMode(stepctrl_ms2, OUTPUT);        // MS2 pin
  pinMode(stepctrl_ms1, OUTPUT);        // MS1 pin
  pinMode(stepctrl_enable, OUTPUT);     // Enable Pin    
  stepctrl_PwrOff();                    // Ensure Power Off state
}

//*********************************************************************************
//**
//** 
//**
//*********************************************************************************

#include <stdio.h>
#include <string.h>


//-----------------------------------------------------------------------------------------
//
char incoming_command_string[50];                                // Input from Serial

void rs485_parse_incoming(void)
{
  char *pEnd;
  uint8_t res;
  uint8_t ant;
//Serial.println(incoming_command_string);
// $SINC <ant> <res>
  if (!strncasecmp("SINC",incoming_command_string,4))           // Increment Stepper
  {
    res = strtol(incoming_command_string+5,&pEnd,0);
    ant = strtol(incoming_command_string+6,&pEnd,0);
	if (ant == ctrl_ant) stepctrl_Incr(res);
  }
// $SDEC <ant> <res>
  else if (!strncasecmp("SDEC",incoming_command_string,4))    // Decrement Stepper
  {
    res = strtol(incoming_command_string+5,&pEnd,0);
    ant = strtol(incoming_command_string+6,&pEnd,0);
	if (ant == ctrl_ant)     stepctrl_Decr(res);
  }
// $SMOV <ant>
  else if (!strncasecmp("SMOV",incoming_command_string,4))    // Move Stepper (neends >1+ microsecond delay from positive edge)
  {
    ant = strtol(incoming_command_string+5,&pEnd,0);
	if (ant == ctrl_ant)     stepctrl_Move();
  }
// $SON <ant>
  else if (!strncasecmp("SON",incoming_command_string,3))     // Turn the Stepper On
  {
    ant = strtol(incoming_command_string+4,&pEnd,0);
	if (ant == ctrl_ant)     stepctrl_PwrOn();
  }
// $SOF <ant>
  else if (!strncasecmp("SOF",incoming_command_string,3))     // Turn the Stepper Off
  {
    ant = strtol(incoming_command_string+4,&pEnd,0);
	if (ant == ctrl_ant)     stepctrl_PwrOff();
  }
// $INIT <ant>
  else if (!strncasecmp("SINIT",incoming_command_string, 5))     // Init Stepper Outputs
  {
Serial.print("$SINIT");
    ant = strtol(incoming_command_string+6,&pEnd,0);
Serial.println(ant);
	if (ant == ctrl_ant)     stepctrl_Init();
  }
  
} 

//
//-----------------------------------------------------------------------------------------
//      Monitor rs485 Serial port for an incoming command
//-----------------------------------------------------------------------------------------
//
void rs485_read_and_parse(void)
{
  static uint8_t a;                     // Indicates number of chars received in an incoming command
  static bool Incoming;
  uint8_t ReceivedChar;
  uint8_t waiting;                      // Number of chars waiting in receive buffer

  waiting = Serial.available();         // Find out how many characters are waiting to be read.

    // Scan for a command attention symbol -> '$'
    if (waiting && !Incoming)
    {
      ReceivedChar = Serial.read();
      // A valid incoming message starts with an "attention" symbol = '$'.
      // in other words, any random garbage received on USB port is ignored.
      if (ReceivedChar == '$')          // Start command symbol was received,
      {                                 // we can begin processing input command
        Incoming = true;
        a = 0;
        waiting--;
      }
      //else ***********************ADD UART Receive here
    } 
    // Input command is on its way.  One or more characters are waiting to be read
    // and Incoming flag has been set. Read any available bytes from the USB OUT endpoint
    while (waiting && Incoming)
    {
      ReceivedChar = Serial.read();
      waiting--;
      if (a == sizeof(incoming_command_string)-1)  // Line is too long, discard input
      {
        Incoming = false;
        a = 0;
      }
      // Check for End of line
      else if ((ReceivedChar=='\r') || (ReceivedChar=='\n') || (ReceivedChar==';'))
      {
        incoming_command_string[a] = 0; // Terminate line
        rs485_parse_incoming();           // Parse the command
        Incoming = false;
        a = 0;
      }
      else                              // Receive message, char by char
      {
        incoming_command_string[a] = ReceivedChar;
      }
      a++;                                         // String length count++
    }
  }


//
//---------------------------------------------------------------------------------
// Here there be all the heavy lifting
//---------------------------------------------------------------------------------
//
void loop()
{
                                        
  //-------------------------------------------------------------------------------
  // Here we do routines which are to be run through as often as possible
  //-------------------------------------------------------------------------------
  
  //-------------------------------------------------------------------
  // Asynchronous management of USB and Serial ports
  //
  rs485_read_and_parse();                     // Read and parse anything on the USB serial port
 
}

void setup()
{
  uint8_t coldstart;
  
  stepctrl_Init();
  Serial.begin(9600);                                   // initialize USB virtual serial serial port
  
//  Serial.println("$m0Ready");

}
