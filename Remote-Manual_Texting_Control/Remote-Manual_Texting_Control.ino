#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

SoftwareSerial SIM900(7, 8); // RX, TX

// All data wire are plugged into port 2 on the Arduino
#define temp_sensors 2
#define relay_switch 10
float temp_in;
float temp_out;

String t_in;
String t_out;
String temp_sense;

String pumpStatus;

String inData = "";
int check = 0;
String Address = "";
String IP = "";

int controlMode = 0;
String control;

unsigned long currentTime;
unsigned long startTime;
const unsigned long period = 5000; 

int dataCheck = 0;


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
// call the library oneWire, provide a name to your Onewire devices (we called them "Sajjie")
// and provide the pin number of these "Sajje" devices (our pin number is temp_sensor = 2)
OneWire Sajje(temp_sensors);

// Pass our oneWire reference to Dallas Temperature.
// call the library oneWire, name your Dallas library (we called them "Dallas")
// and provide the "Sajje" devices reference to Dallas
// The & symbol in a C++ variable declaration means it's a reference. https://en.wikipedia.org/wiki/Reference_%28C%2B%2B%29
DallasTemperature Dallas(&Sajje);

void controlmodeCheck() {
  if(controlMode == 0){
    if (temp_out - temp_in > 2) {
      Serial.println("turn on the pump.");
      Serial.println(" ");
      digitalWrite(relay_switch, HIGH);
      pumpStatus = "ON";
    }
    else {
      Serial.println("turn off the pump.");
      digitalWrite(relay_switch, LOW);
      Serial.println(" ");
      pumpStatus = "OFF";
    }
  }

  if(controlMode == 1){
    if(pumpStatus == "ON"){
      digitalWrite(relay_switch, HIGH);
      Serial.println(" ");
    }
    if(pumpStatus == "OFF"){
      digitalWrite(relay_switch, LOW);
      Serial.println(" ");
    }  
  }
}
void checkSMS() {
  if(inData == "Mode 1\r" || inData == "mode 1\r"){
    Serial.println("Mode Set to: Manual Pump Control");
    controlMode = 1;
    control = "Manual";
    sendSMS("Mode Set to: Manual Pump Control");
  }
  if(inData == "Mode 0\r" || inData == "mode 0\r"){
    Serial.println("Mode Set to: Automated Pump Control");
    controlMode = 0;
    control = "Auto";
    sendSMS("Mode Set to: Automated Pump Control");
  }
  if(inData == "Pump On\r" || inData == "pump on\r" || inData == "Pump on\r"){
    Serial.println("Pump Status: ON");
    pumpStatus = "ON";
    if(controlMode == 1){
      sendSMS("Pump Status: ON");
    }
    else{
      sendSMS("Mode Set is not Manual Control");
    }
  }  
  if(inData == "Pump Off\r" || inData == "pump off\r" || inData == "Pump off\r"){
    Serial.println("Pump Status: OFF");
    pumpStatus = "OFF";
    if(controlMode == 1){
      sendSMS("Pump Status: OFF");
    }
    else{
      sendSMS("Mode Set is not Manual Control");
    }    
  } 
  if(inData == "Status\r" || inData == "status\r"){
    Serial.print(temp_sense);
    sendSMS(temp_sense);
  }
}

void dataWrite(String toSend, int tDelay = 500) {
  SIM900.println(toSend);
  delay(tDelay);

  while (SIM900.available()){
     inData = SIM900.readStringUntil('\n');
     if(inData == "10.59.8.243\r"){
        Serial.println("SetupTCP Complete");
        check = 1; 
        Serial.print("Check is: ");
        Serial.println(check);
    }
     Serial.println(inData);
  }
}

void setupTCP() {

  IP = String("10.59.8.243");
  
  while(check == 0){
    
    dataWrite("AT+CREG?");
    dataWrite("AT+CGREG?");
    dataWrite("AT+CMEE=1");
    dataWrite("AT+CGACT?");
    dataWrite("AT+CIPSHUT");
    dataWrite("AT+CSTT=\"hologram\"");      //Set the APN to hologram
    dataWrite("AT+CIICR", 1000);
    dataWrite("AT+CIFSR", 1000);            //Get confirmation of the IP address
    delay(1000);
  }
}


void sendSMS(String inData) {
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  // +1 is US code not +01
  dataWrite("AT + CMGS = \"+1XXXXXXXXXX\""); //"AT + CMGS = \"+XXXXXXXXXXXX\"" 
  delay(100);
  
  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  dataWrite(inData); 
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26); 
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);
  inData = ""; 
}

void sendData(String data) {
  
    dataWrite("AT+CIPSTART=\"TCP\",\"cloudsocket.hologram.io\",\"9999\"",5000);
    dataWrite("AT+CIPSEND",100);
    dataWrite("{\"k\":\"nEPN%q2_\",\"d\":\"" + String(data) + "\",\"t\":\"data\"}",100);
    SIM900.write(0x1a);
    delay(1000);
    while (SIM900.available()){
       inData = SIM900.readStringUntil('\n');
       delay(1000);
  }
}

void setup() {
  SIM900.begin(19200);
  delay(2000);
  Serial.begin(19200);
  delay(2000);
  setupTCP();
  delay(1000);

  SIM900.print("AT+CMGF=1\r"); 
  delay(100);
  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);
  //These commands initialize the GSM for texting

  // setup the pin for pump
  pinMode(relay_switch, OUTPUT);

  sendSMS("Baja Solar Water Heater has been initialized!");
  startTime = millis();
  controlMode = 0;
  control = "Auto";
}

void loop() {

  while(SIM900.available() > 0) {
    inData = SIM900.readStringUntil('\n');
    delay(30);
    Serial.println("Received Message is: " + inData);
    checkSMS();    
    //controlmodeCheck();        
  }  

  controlmodeCheck();
  currentTime = millis();
  
  
  
  t_in = String(temp_in);
  t_out = String(temp_out);
  temp_sense = "Control Mode: " + control + " / " + "Inlet Temp: " + t_in + " / " + "Outlet Temp: " + t_out + " / " + "Pump Status: " + pumpStatus;



}
