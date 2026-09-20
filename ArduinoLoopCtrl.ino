/*
| Commande | Paramètres | Description |
| :--- | :--- | :--- |
| `$SINIT <ant>` | `<ant>` (0-2) | Initialisation du contrôleur d'antenne RS485 pour l'antenne cible. |
| `$SON <ant>` | `<ant>` (0-2) | Activation de l'alimentation moteur de l'antenne spécifiée. |
| `$SOF <ant>` | `<ant>` (0-2) | Coupure d'alimentation moteur pour économie d'énergie. |
| `$SINC <ant> <res>` | `<ant> <res>` | Déplacement sens horaire pour l'antenne cible avec résolution `res` (0 = 1/8 micropas, 3 = pas entier). |
| `$SDEC <ant> <res>` | `<ant> <res>` | Déplacement sens anti-horaire pour l'antenne cible avec résolution `res`. |
| `$SMOV <ant>` | `<ant>` (0-2) | Exécution du déplacement d'un pas sur l'antenne spécifiée. |
| `$SANT <ant>` | `<ant>` (0-2) | Sélection / Commutation de l'antenne active sur le bus RS485. |
*/
//
const int ctrl_ant   = 0;     // Controller antenna assing
//
const int drv8825_dir   = 3;     // Direction pin
const int drv8825_step  = 4;     // Step pin (positive pulse of +1us for each step)
const int drv8825_ms2   = 5;     // Microstepping pin MS2
const int drv8825_ms1   = 6;     // Microstepping pin MS1
const int drv8825_enable= 7;     // Enable pin
//
// Increment Stepper
//
void drv8825_Incr(uint8_t res)
{
  res = 3 - res;                       // Reversed: 0 for no microsteps
                                       // 1 for half step (2 microsteps)
                                       // 2 for quarter step (4 microsteps)
                                       // 3 for eighth step (8 microsteps)

  drv8825_PwrOn();                     // Ensure Power On state
  digitalWrite(drv8825_dir, LOW);      // Clockwise
  digitalWrite(drv8825_ms1, (res&0x01)?HIGH:LOW);  // ... Microstep resolution
  digitalWrite(drv8825_ms2, (res&0x02)?HIGH:LOW);
  digitalWrite(drv8825_step, HIGH);    // Prime for Movement, turn Step pulse on
}
//
// Decrement Stepper
//
void drv8825_Decr(uint8_t res)
{
  res = 3 - res;                       // Reversed: 0 for no microsteps
                                       // 1 for half step (2 microsteps)
                                       // 2 for quarter step (4 microsteps)
                                       // 3 for eighth step (8 microsteps)

  drv8825_PwrOn();                     // Ensure Power On state
  digitalWrite(drv8825_dir, HIGH);     // Counterclockwise
  digitalWrite(drv8825_ms1, (res&0x01)?HIGH:LOW);  // ... Microstep resolution
  digitalWrite(drv8825_ms2, (res&0x02)?HIGH:LOW);
  digitalWrite(drv8825_step, HIGH);    // Prime for Movement, turn Step pulse on
}

//
// Move Stepper (neends >1+ microsecond delay from positive edge)
//
void drv8825_Move(void)
{
  digitalWrite(drv8825_step, LOW);     // Move, turn Step pulse off  
	delay(50);
  digitalWrite(drv8825_step, HIGH);     // Move, turn Step pulse off  
}

//
// Turn the Stepper On
//
void drv8825_PwrOn(void)
{
  //digitalWrite(drv8825_reset, HIGH); // Release Reset, turn Stepper Motor On
  digitalWrite(drv8825_enable, LOW);   // Enable Stepper
}

//
// Turn the Stepper Off
//
void drv8825_PwrOff(void)
{
  //digitalWrite(drv8825_reset, LOW);  // Reset and turn Stepper Motor Off
  digitalWrite(drv8825_enable, HIGH);  // Disable Stepper, retain last state
}

//
// Init Stepper Outputs
//
void drv8825_Init(void)
{
  pinMode(drv8825_dir, OUTPUT);        // Direction Pin    
  pinMode(drv8825_step, OUTPUT);       // Step Pin
  pinMode(drv8825_ms2, OUTPUT);        // MS2 pin
  pinMode(drv8825_ms1, OUTPUT);        // MS1 pin
  pinMode(drv8825_enable, OUTPUT);     // Enable Pin    
  drv8825_PwrOff();                    // Ensure Power Off state
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
char incoming_command_string[50];                                // Input from USB Serial

void rs485_parse_incoming(void)
{
  uint8_t x;
  char *pEnd;
  uint8_t res;
  uint8_t ant;
  int32_t  frq_in;

  if (!strncasecmp("sinc",incoming_command_string,4))           // Increment Stepper
  {
    ant = strtol(incoming_command_string+4,&pEnd,0);
    res = strtol(incoming_command_string+5,&pEnd,0);
	if (ant == ctrl_ant) drv8825_Incr(res);
  //Serial.print("$m0stinc ");
  //Serial.print(res);
  //Serial.println("");
  }
  else if (!strncasecmp("sdec",incoming_command_string,4))    // Decrement Stepper
  {
    ant = strtol(incoming_command_string+4,&pEnd,0);
    res = strtol(incoming_command_string+5,&pEnd,0);
	if (ant == ctrl_ant)     drv8825_Decr(res);
  //Serial.print("$m0stdec ");
  //Serial.print(res);
  //Serial.println("");
  }
  else if (!strncasecmp("smov",incoming_command_string,4))    // Move Stepper (neends >1+ microsecond delay from positive edge)
  {
    ant = strtol(incoming_command_string+4,&pEnd,0);
	if (ant == ctrl_ant)     drv8825_Move();
  //Serial.println("$m0stmov");
  }
  else if (!strncasecmp("son",incoming_command_string,3))     // Turn the Stepper On
  {
    ant = strtol(incoming_command_string+3,&pEnd,0);
	if (ant == ctrl_ant)     drv8825_PwrOn();
  //Serial.println("$m0ston");
  }
  else if (!strncasecmp("sof",incoming_command_string,3))     // Turn the Stepper Off
  {
    ant = strtol(incoming_command_string+3,&pEnd,0);
	if (ant == ctrl_ant)     drv8825_PwrOff();
  //Serial.println("$m0stoff");
  }
  else if (!strncasecmp("sinit",incoming_command_string, 5))     // Init Stepper Outputs
  {
    ant = strtol(incoming_command_string+5,&pEnd,0);
	if (ant == ctrl_ant)     drv8825_Init();
//    Serial.println("$m0stini");
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
  
  drv8825_Init();
  Serial.begin(38400);                                   // initialize USB virtual serial serial port
  
//  Serial.println("$m0Ready");

}
