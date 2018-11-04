
#include <NTPtimeESP.h>
#include <ESP8266WiFi.h>

#define DEBUG_ON

#ifdef DEBUG_ON
  #define Sprint(a) (Serial.print(a))
  #define Sprintln(a) (Serial.println(a)
#else
  #define Sprint(a)
  #define Sprintln(a)
#endif

#ifdef SECONDS_MODE
  #define dateTime.hour dateTime.second
#endif

//NTP-clock
NTPtime NTPch("nl.pool.ntp.org");   // Choose server pool as required
strDateTime dateTime;
int previousHour;

//Wifi & thingspeak
String apiKey = "IKOLI389JPQ71CKZ";
const char *ssid =  "Catharijnesingel93b";
const char *pass =  "Catharijnesingel93b!";
const char* server = "api.thingspeak.com";
WiFiClient client;

const int MOISTURE_PIN = A0;
const char WATERPUMP_PIN = D5;
const int WATER_THRESHOLD = 650; //higher means dryer
const int WATERING_TIME = 15000;
const int WATERING_HOUR = 20;

//AnalogRead
int reading = -1;

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(WATERPUMP_PIN, OUTPUT);

  //Connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Connection succesfull
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  //Get NTP time
  Serial.println("Getting NTP time"); 
  dateTime = NTPch.getNTPtime(1.0, 1);
  while (!(dateTime.valid)) {
    Serial.println(".");
    dateTime = NTPch.getNTPtime(1.0, 1);
    delay(10);
  }
  //NTP time retrieved
  Serial.println(""); 
  previousHour = dateTime.hour;
  Serial.print("Fetched current hour is: ");
  Serial.println(previousHour);
  digitalWrite(WATERPUMP_PIN, LOW);
}

void loop() {
  dateTime = NTPch.getNTPtime(1.0, 1);
  
  if (dateTime.valid) { //check if time in in right format
    int currentHour = dateTime.hour;
    if (currentHour == 0 && previousHour == 23 || currentHour == previousHour + 1) { // the only correct hour transitions
      executeCycle(currentHour);
    }
      else if (currentHour == previousHour) {
        Serial.print("not a new hour, currentHour: " + currentHour);
        delay(100);
        //TODO: deepsleep
        //delay(100); //remove this delay after deepsleep has been implemented
      }
      else {
        Serial.println("faulty time detected");
      }
  }
}

void executeCycle(int pCurrentHour) {
  Serial.println("new hour, executing cycle");
  Serial.print("previous hour: " + previousHour);
  Serial.println("     current hour: " + pCurrentHour);
  int reading = analogRead(MOISTURE_PIN);
  Serial.println("current reading: " + reading);
  updateWeb(reading);
  
  if (pCurrentHour == WATERING_HOUR) {
    Serial.println("It's watering time: " + dateTime.hour);
    if (reading > WATER_THRESHOLD) {
      Serial.print("threshold broken, threshold: "+WATER_THRESHOLD);
      Serial.println("      reading: "+reading);
      waterPlants();
      reading = analogRead(MOISTURE_PIN);
      delay(1000); //Small delay for analogRead
      Serial.println("reading after watering: "+reading);
      updateWeb(reading); //Upload again after watering
    }
  }
  Serial.print("Update 'previousHour' -> previousHour:"+previousHour);
  Serial.println("    currentHour:"+pCurrentHour);
  previousHour = pCurrentHour;
  Serial.println("updated 'previousHour' is now: " +previousHour);
}

void waterPlants() {
  //activates waterpump
  digitalWrite(WATERPUMP_PIN, HIGH);
  delay(WATERING_TIME);
  digitalWrite(WATERPUMP_PIN, LOW);
  delay(30000); //Let water get into soil before measuring
}

void updateWeb(int value) {
  //uploads 'value' to thingspeak
  if (client.connect(server, 80)) { //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += value;
    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
}
