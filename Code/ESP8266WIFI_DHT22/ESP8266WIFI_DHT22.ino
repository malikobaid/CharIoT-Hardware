// Send DHT22 and Pir data to influx db hoseted on <<Server_IP>> using an ESP8266
// July 25 2017
// Author: Obaid Malik

#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>

String strLocation = "Lounge"; const int SensorID = 1; const int TC = 0.0; const int HC = 0.0; //Sensor 1

////// Testing
const char* ssid = "SSID1";
const char* password = "PASS1";
const char* ssid2 = "SSID2";
const char* password2 = "PASS2";

const float timebwreadings = 0.9L;
const char* server = "<<Server_IP>>";

// New sensors
#define DHTPIN 12 // what pin we’re connected to
#define PIRPIN 13 // what pin we’re connected to
#define DHTTYPE DHT22
#define WIFIOKPIN 16
#define SENDOKPIN 14

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void init_components() {
  Serial.begin(9600);
  delay(30);
  Serial.println();
  Serial.print("Initilizing components for Sensor: ");
  Serial.print(SensorID);
  Serial.println(" .....");
  pinMode(WIFIOKPIN, OUTPUT);
  pinMode(SENDOKPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);

  //Test LEDS
  digitalWrite(WIFIOKPIN, HIGH);    // turn the LED off by making the voltage LOW
  digitalWrite(SENDOKPIN, HIGH);    // turn the LED off by making the voltage LOW
  delay(400);
  digitalWrite(WIFIOKPIN, LOW);     // turn the LED off by making the voltage LOW
  digitalWrite(SENDOKPIN, LOW);     // turn the LED off by making the voltage LOW
  delay(30);
  dht.begin();
}

void setup() {
  init_components();
  connectToAvailableWifi();
  Serial.println("Waiting for PIR sensor Initilization:");
  delay(55000);                     // The Pir Sensor need 60 sec to initilize we wait 55 sec the remaining will be covered doing wifi setup
  Serial.println("PIR sensor Initilizied:");
}


int firstReading = 1;
void loop() {

  digitalWrite(SENDOKPIN, LOW);

  if (firstReading == 0) {
    digitalWrite(WIFIOKPIN, LOW);
  }

  bool isConnected = connectToAvailableWifi();
  if (isConnected) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    h = h + HC;
    t = t + TC;
    float tF = ((t * 9) / 5) + 32;
    float dP = (dewPointFast(t, h));
    float hi = heatIndex(tF, h);
    int presence = (digitalRead(PIRPIN) == HIGH) ? 1 : 0; // check if the sensor is HIGH, For some reason assigning direct value from digitread to presence variable doesnt seem to work. check Later?
    delay(100);

    if (client.connect(server, 8086)) { 
      String postStr = "SensorDataPIR,SensorID=" + String(SensorID) + " Temperature=" + String(t) + ",Humidity=" + String(h) + ",Dewpoint=" + String(dP) + ",HeatIndex=" + String(hi) + ",Presence=" + String(presence);
      Serial.println(postStr);
      client.print("POST /write?db=chariot HTTP/1.1\n");
      client.print("Host: <<Server_IP>>\n");
      client.print("Connection: close\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      delay(3000);
      while (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        if (line.indexOf("204 No Content") > 0) {
          Serial.println("Post to Server successfull");
          if (firstReading) {
            digitalWrite(SENDOKPIN, HIGH); //Only light up led if first reading
            firstReading = 0;
          }
        }
      }
    }
    client.stop();
  } else {
    Serial.println("No suitable wifi available");
    digitalWrite(SENDOKPIN, LOW);
    digitalWrite(WIFIOKPIN, LOW);
  }
  Serial.println("yaaaawn .... going to sleep for 1 min");
  delay(60000 * timebwreadings);
}

bool connectToAvailableWifi() {
  bool connectionOk = true;
  if (WiFi.status() != WL_CONNECTED) {
    connectionOk = waitForWifiConnect(40, ssid, password);
    if (connectionOk == false) {
      connectionOk = waitForWifiConnect(40, ssid2, password2);
    }

    if (connectionOk && firstReading) { //Activate LED if first reading
      digitalWrite(WIFIOKPIN, HIGH);
    }
  } else {
    if (firstReading) {
      digitalWrite(WIFIOKPIN, HIGH);
    }
    Serial.println("WiFi connected");
  }
  return connectionOk;
}
bool waitForWifiConnect(int retrys, const char* sid, const char* pass) {
  WiFi.begin(sid, pass);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(sid);
  delay(1000);
  bool connectionOk = false;
  for (int i = 1; i < retrys; i++) {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(WIFIOKPIN, HIGH);
      delay(200);
      Serial.print(".");
      digitalWrite(WIFIOKPIN, LOW);
      delay(200);
      connectionOk = false;
    } else {
      //digitalWrite(WIFIOKPIN, HIGH); //Let the calling function handle the LED output
      Serial.println("WiFi connected");
      i = retrys + 1;
      connectionOk = true;
    }
  }
  Serial.println("");
  return connectionOk;
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

double heatIndex(double tempF, double humidity)
{
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5 = -6.838e-3, c6 = -5.482e-2, c7 = 1.228e-3, c8 = 8.528e-4, c9 = -1.99e-6  ;
  double T = tempF;
  double R = humidity;

  double A = (( c5 * T) + c2) * T + c1;
  double B = ((c7 * T) + c4) * T + c3;
  double C = ((c9 * T) + c8) * T + c6;

  double rv = (C * R + B) * R + A;
  return rv;
}
