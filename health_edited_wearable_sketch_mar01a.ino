#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include<SoftwareSerial.h>
#include <TinyGPS.h>
TinyGPS gps;
SoftwareSerial ss(D6, D0);
float flat, flon;
unsigned long age;
#define ASEN_PIN A0//tilt
#define ONE_WIRE_BUS D5 //temperature 
#define DSEN2_PIN D2//pulse
#define tch D1//touch
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define buz D7
String buf;
String asen;
String dsen1;
String dsen2;
String dsen3;
int asenc;
int dsenc1;
int dsenc2;
int dsenc3;
int t = 0;//heartbeat timer
int tc;//touch sensor variable
int t1;//timer
int tm = 0; //timer flag

// Update these with values suitable for your network.
const char* ssid = "project";
const char* password = "project123";
//const char* mqtt_server = "test.mosquitto.org";
//const char* mqtt_server = "iot.eclipse.org";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "wre";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      //once connected to MQTT broker, subscribe command if any
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //end reconnect()

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  ss.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(ASEN_PIN, INPUT);
  pinMode(DSEN2_PIN, INPUT);
  pinMode(tch, INPUT);
  pinMode(buz, OUTPUT);
  digitalWrite(buz, LOW);
}

void loop() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      //Serial.write(c); // uncomment this line if you want to see the GPS data flowing      delay(1000);
      if (gps.encode(c))
      {
        newData = true;
      }
    }
  }

  if (newData)
  {
    gps.f_get_position(&flat, &flon, &age);
  }
  //buf = F("lat:");
  buf = String(flat, 6);
  buf += F(",");
  //buf += F("\nlon:");
  buf += String(flon, 6);
  sensors.requestTemperatures();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    asen = analogRead(ASEN_PIN);
    asenc = analogRead(ASEN_PIN);
    dsen1 = sensors.getTempCByIndex(0);
    dsenc1 = sensors.getTempCByIndex(0);
    dsen2 = digitalRead(DSEN2_PIN);
    dsenc2 = digitalRead(DSEN2_PIN);
    tc = digitalRead(tch);
    char message1[8];
    char message2[4];
    dsen1.toCharArray(message1, 8);
    dsen2.toCharArray(message2, 4);
        client.publish("temm", message1);
    //    client.publish("sen2", message1);
    //    client.publish("sen3", message2);
    //    client.publish("sen4", message3);
    Serial.print("sen2=");
    Serial.println(dsen1);
//    Serial.print("sen3=");
//    Serial.println(dsen2);
    if (dsenc1 > 40)
    {
      Serial.println("TEMPERATURE HIGH");
      digitalWrite(buz, HIGH);
      client.publish("wre2", "TEMPERATURE HIGH/FEVER");
      delay(500);
    }
    if (dsenc1 < 40)
    {
      Serial.println("TEMPERATURE NORMAL");
      digitalWrite(buz, LOW);
      client.publish("wre2", "TEMPERATURE NORMAL");
      delay(500);
    }
    if (dsenc2 == 1)
    {
      t = t + 1;
      Serial.println(t);
    }
    if (t > 15)
    {
      Serial.println("PULSE LOW");
      digitalWrite(buz, HIGH);
      client.publish("wre3", "PULSE LOW");
      delay(500);
    }
    if (dsenc2 == 0)
    {
      t = 0;
      Serial.println("PULSE NORMAL");
      digitalWrite(buz, LOW);
      client.publish("wre3", "PULSE NORMAL");
      delay(500);
    }
    if (tc == 1)
    {
      t1 = 1;
    }
    if (t1 == 1)
    {
      tm = tm + 1;
    }
    if ((tm > 2) && (tc == 1) && (t1 == 1))
    {
      digitalWrite(buz, HIGH);
      Serial.println("EMERGENCY BUTTON PRESSSED");
      client.publish("wre4", "EMERGENCY BUTTON PRESSSED");
      t1 = 0;
      tm = 0;
    }
    if (tc == 0)
    {
      digitalWrite(buz, LOW);
      //Serial.println("NO EMERGENCY");
    }
    if (tm > 12)
    {
      t1 = 0;
      tm = 0;
    }
    if(asenc<40)
{
  Serial.println("USER FELT DOWN");
  client.publish("wre5", "USER FELT DOWN");
}
if(asenc>41)
{
  Serial.println("USER PLANE NORMAL");
  client.publish("wre5", "USER PLANE NORMAL");
}
    char charBuf[50];
    buf.toCharArray(charBuf, 50);
    client.publish("wre1", charBuf);
    Serial.print("LOCATION= ");
    Serial.println(charBuf);
    buf = "";
  }
}
