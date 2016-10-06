#include <SoftwareSerial.h>                           //we have to include the SoftwareSerial library, or else we can't use it
#include <TimerOne.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <Wire.h>                //enable I2C.




#define rx 2                                          //define what pin rx is going to be
#define tx 3                                          //define what pin tx is going to be
#define LED_PIN 13
#define rx_GSM 4
#define tx_GSM 5
#define valvePin 6
#define ledPin 7

SoftwareSerial myserialEC(rx, tx);                      //define how the soft serial port is going to work
SoftwareSerial myserialGSM(rx_GSM,tx_GSM);

String inputstring = "";                              //a string to hold incoming data from the PC
String sensorstring = "";                             //a string to hold the data from the Atlas Scientific product
boolean input_string_complete = false;                //have we received all the data from the PC
boolean sensor_string_complete = false;               //have we received all the data from the Atlas Scientific product

#define address 99               //default I2C ID number for EZO pH Circuit.



char computerdata[20];           //we make a 20 byte character array to hold incoming data from a pc/mac/other.
byte received_from_computer = 0; //we need to know how many characters have been received.
byte code = 0;                   //used to hold the I2C response code.
char ph_data[20];                //we make a 20 byte character array to hold incoming data from the pH circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the pH Circuit.
byte i = 0;                      //counter used for ph_data array.
int time_ = 1800;                //used to change the delay needed depending on the command sent to the EZO Class pH Circuit.
float ph_float;                  //float var used to hold the float value of the pH.
float f_ec;                     //float var used to hold the float value of the conductivity
  char *EC;                                           //char pointer used in string parsing

volatile int f_wdt=1;
int ledState = 0;

/***************************************************
 *  Name:        ISR(WDT_vect)
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 *
 ***************************************************/
ISR(WDT_vect)
{
  if(f_wdt < 10)
  {
    f_wdt= f_wdt+1;
  }
  else if(f_wdt == 10){
    myserialGSM.begin(9600);
    delay(100);
    /*DO GSM STUFF HERE SINCE IT's an INTERRUPT
     * 
     * 
     * 
     */
     myserialGSM.end();
     f_wdt = f_wdt+1;
     myserialEC.begin(9600);
     delay(100);

  }
  else
  {
    //Serial.println("WDT Overrun!!!");
    f_wdt = 0;
  }
}



/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);                                 //set baud rate for the hardware serial port_0 to 9600
  myserialEC.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(10);                            //set aside some bytes for receiving data from the PC
  sensorstring.reserve(30);                           //set aside some bytes for receiving data from Atlas Scientific product
  Wire.begin();                 //enable I2C port.
// Timer1.initialize(1000000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
//  Timer1.attachInterrupt( timerIsr ); // attach the service routine here
  Serial.println("Initialising...");
  delay(100); //Allow for serial print to complete.

  pinMode(LED_PIN,OUTPUT);
  pinMode(valvePin,OUTPUT);

  /*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
  
  Serial.println("Initialisation complete.");
  delay(100); //Allow for serial print to complete.
}

void serialEvent() {                                  //if the hardware serial port_0 receives a char
  inputstring = Serial.readStringUntil(13);           //read the string until we see a <CR>
  input_string_complete = true;                       //set the flag used to tell if we have received a completed string from the PC
}


void loop() {
  if(f_wdt>3 && f_wdt<6){ 
    conductivityCode();
    getPHData();

    //LED Control Code
    if (ph_float > 8 || ph_float < 6) ledState = 1;   //MODIFY THIS CHECK BASED ON BIO TEAM DESCRIPTION!
    else if (f_ec > 100) ledState = 1; //MODIFY!!
    else ledState = 0;
    digitalWrite(ledPin, ledState);
  }
  else if(f_wdt>=6 && f_wdt<10){
    myserialEC.end();
    /*Do GSM code here
     * 
     * 
     * 
     */
    // Serial.println("EC:");
     //Serial.println(EC);
     //Serial.println(f_ec);
     //Serial.println(ph_float);
     digitalWrite(valvePin,HIGH);
      
  }
  else{
    enterSleep();
    digitalWrite(valvePin,LOW);
  }
}

void conductivityCode(void){
  if (input_string_complete) {                        //if a string from the PC has been received in its entirety
    myserialEC.print(inputstring);                      //send that string to the Atlas Scientific product
    myserialEC.print('\r');                             //add a <CR> to the end of the string
    inputstring = "";                                 //clear the string
    input_string_complete = false;                    //reset the flag used to tell if we have received a completed string from the PC
  }
//  myserialEC.print('r');
//  myserialEC.print('\r');
  while(myserialEC.available()<=0){

    
  }
  /*if*/while (myserialEC.available() > 0) {                     //if we see that the Atlas Scientific product has sent a character
    char inchar = (char)myserialEC.read();              //get the char we just received
    sensorstring += inchar;                           //add the char to the var called sensorstring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      sensor_string_complete = true;                  //set the flag
    }
  }

  while(!sensor_string_complete);
  if (sensor_string_complete == true) {               //if a string from the Atlas Scientific product has been received in its entirety
    if (isdigit(sensorstring[0]) == false) {          //if the first character in the string is a digit
      Serial.println(sensorstring);                   //send that string to the PC's serial monitor
    }
    else                                              //if the first character in the string is NOT a digit
    {
      print_EC_data();                                //then call this function 
    }
    sensorstring = "";                                //clear the string
    sensor_string_complete = false;                   //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
  }  
}





void print_EC_data(void) {                            //this function will pars the string  

  char sensorstring_array[30];                        //we make a char array
  char *TDS;                                          //char pointer used in string parsing
  char *SAL;                                          //char pointer used in string parsing
  char *GRAV;                                         //char pointer used in string parsing
  //float f_ec;                                         //used to hold a floating point number that is the EC
  
  sensorstring.toCharArray(sensorstring_array, 30);   //convert the string to a char array 
  EC = strtok(sensorstring_array, ",");               //let's pars the array at each comma
  TDS = strtok(NULL, ",");                            //let's pars the array at each comma
  SAL = strtok(NULL, ",");                            //let's pars the array at each comma
  GRAV = strtok(NULL, ",");                           //let's pars the array at each comma

  Serial.print("EC:");                                //we now print each value we parsed separately
  Serial.println(EC);                                 //this is the EC value

  Serial.print("TDS:");                               //we now print each value we parsed separately
  Serial.println(TDS);                                //this is the TDS value

  Serial.print("SAL:");                               //we now print each value we parsed separately
  Serial.println(SAL);                                //this is the salinity value

  Serial.print("GRAV:");                              //we now print each value we parsed separately
  Serial.println(GRAV);                               //this is the specific gravity
  Serial.println();                                   //this just makes the output easier to read
  
  f_ec= atof(EC);                                     //uncomment this line to convert the char to a float
  Serial.print("FLOAT:");
  Serial.println(f_ec);
  Serial.println();
}

void getPHData(void){
 //  myserialEC.print("SLEEP\r");
   // put your main code here, to run repeatedly:
    //received_from_computer = Serial.readBytesUntil(13, computerdata, 20); //we read the data sent from the serial monitor(pc/mac/other) until we see a <CR>. We also count how many characters have been received.
    //computerdata[received_from_computer] = 0;                             //stop the buffer from transmitting leftovers or garbage.
    //computerdata[0] = tolower(computerdata[0]);                           //we make sure the first char in the string is lower case.
    //if (computerdata[0] == 'c' || computerdata[0] == 'r')time_ = 1800;    //if a command has been sent to calibrate or take a reading we wait 1800ms so that the circuit has time to take the reading.
    //else time_ = 300;                                                     //if any other command has been sent we wait only 300ms.
    time_=1800;

    Wire.beginTransmission(address); //call the circuit by its ID number.
    Wire.write('r');        //transmit the command that was sent through the serial port.
    Wire.endTransmission();          //end the I2C data transmission.


    delay(time_);                    //wait the correct amount of time for the circuit to complete its instruction.

    Wire.requestFrom(address, 20, 1); //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();             //the first byte is the response code, we read this separately.

    switch (code) {                 //switch case based on what the response code is.
      case 1:                       //decimal 1.
        Serial.println("Success");  //means the command was successful.
        break;                        //exits the switch case.

      case 2:                        //decimal 2.
        Serial.println("Failed");    //means the command has failed.
        break;                         //exits the switch case.

      case 254:                      //decimal 254.
        Serial.println("Pending");   //means the command has not yet been finished calculating.
        break;                         //exits the switch case.

      case 255:                      //decimal 255.
        Serial.println("No Data");   //means there is no further data to send.
        break;                       //exits the switch case.
    }





    while (Wire.available()) {         //are there bytes to receive.
      in_char = Wire.read();           //receive a byte.
      ph_data[i] = in_char;            //load this byte into our array.
      i += 1;                          //incur the counter for the array element.
      if (in_char == 0) {              //if we see that we have been sent a null command.
        i = 0;                         //reset the counter i to 0.
        Wire.endTransmission();        //end the I2C data transmission.
        break;                         //exit the while loop.
      }
    }

    Serial.println(ph_data);          //print the data.
  //Uncomment this section if you want to take the pH value and convert it into floating point number.
  //ph_float=atof(ph_data);
    ph_float=atof(ph_data);

//  myserialEC.print("r\r");
}

/*void timerIsr()
{
    // Toggle LED
    digitalWrite( 13, digitalRead( 13 ) ^ 1 );
}
*/


