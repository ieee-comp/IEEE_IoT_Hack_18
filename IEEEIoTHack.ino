#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

//-------- Customise these values -----------
const char* ssid = "sup";
const char* password = "supperss";

#define ORG "umxc6e"
#define DEVICE_TYPE "Hello"
#define DEVICE_ID "123"
#define TOKEN "1234567891011121314"
//-------- Customise the above values --------





int ledPin = 13;

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char eventTopic[] = "iot-2/evt/status/fmt/json";
const char cmdTopic[] = "iot-2/cmd/led/fmt/json";

int Vin = 9;

int Vout = 1;

int ratio = Vin / Vout; // Calculated from Vin / Vout
int batMonPin = 16; // input pin for the voltage divider
int ADCVal = 0; // variable for the A/D value
float pinVoltage = 0; // variable to hold the calculated voltage
float batteryVoltage = 0;

int analogInPin = A0; // Analog input pin that the carrier board OUT is connected to
int sensorValue = 0; // value read from the carrier board
int outputValue = 0; // output in milliamps
unsigned long msec = 0;

int sample = 0;
float totalCharge = 0.0;
float t = 0.0;
float averageAmps = 0.0;
float ampSeconds = 0.0;
float ampHours = 0.0;
float wattHours = 0.0;
float amps = 0.0;

int R1 = 9000; // Resistance of R1 in ohms
int R2 = 1000; // Resistance of R2 in ohms


WiFiClient wifiClient;
void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < payloadLength; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
 
  if (payload[0] == '1') {
    digitalWrite(14, LOW);   
 
  } else {
    digitalWrite(14, HIGH);  // turns on transistor which shuts down circuit
  }

}
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 1000; // 1 seconds//Send adc every 1sc
long lastPublishMillis;

void setup() {
  Serial.begin(9600); 
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  wifiConnect();
  mqttConnect();

  

  delay(100);
}

void loop() {
  if (millis() - lastPublishMillis > publishInterval) {
    publishData();
    lastPublishMillis = millis();
  }

  if (!client.loop()) {
    mqttConnect();
  }
}

void wifiConnect() {
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());

}

void mqttConnect() {
  if (!!!client.connected()) {
    Serial.print("Reconnecting MQTT client to "); Serial.println(server);
    while (!!!client.connect(clientId, authMethod, token)) {
      Serial.print(".");
      delay(500);
    }
    if (client.subscribe(cmdTopic)) {
      Serial.println("subscribe to responses OK");
    } else {
      Serial.println("subscribe to responses FAILED");
    }
    Serial.println();
  }
}


void publishData() {

int sampleADCVal = 0;
int avgADCVal = 0; 
int sampleAmpVal = 0;
int avgSAV = 0;

for (int x = 0; x < 10; x++){ // run through loop 10x

// read the analog in value:
sensorValue = analogRead(analogInPin); 
sampleAmpVal = sampleAmpVal + sensorValue; // add samples together

ADCVal = analogRead(batMonPin); // read the voltage on the divider
sampleADCVal = sampleADCVal + ADCVal; // add samples together

delay (10); // let ADC settle before next sample

}

avgSAV = sampleAmpVal / 10;

// convert to milli amps
outputValue = (((long)avgSAV * 5000 / 1024) - 500 ) * 1000 / 133; 

/* sensor outputs about 100 at rest.
Analog read produces a value of 0-1023, equating to 0v to 5v.
"((long)sensorValue * 5000 / 1024)" is the voltage on the sensor's output in millivolts.
There's a 500mv offset to subtract.
The unit produces 133mv per amp of current, so
divide by 0.133 to convert mv to ma

*/


avgADCVal = sampleADCVal / 10; //divide by 10 (number of samples) to get a steady reading

pinVoltage = 9 * .00488; // Calculate the voltage on the A/D pin
/* A reading of 1 for the A/D = 0.0048mV
if we multiply the A/D reading by 0.00488 then
we get the voltage on the pin. 



Also, depending on wiring and
where voltage is being read, under
heavy loads voltage displayed can be
well under voltage at supply. monitor
at load or supply and decide.
*/


batteryVoltage = pinVoltage * ratio; // Use the ratio calculated for the voltage divider
// to calculate the battery voltage


amps = (float) outputValue / 1000;
float watts = amps * batteryVoltage;


sample = sample + 1;

msec = millis();



t = (float) msec / 1000.0;

totalCharge = totalCharge + amps;

averageAmps = totalCharge / sample;

ampSeconds = averageAmps*t;

ampHours = ampSeconds/3600;

wattHours = batteryVoltage * ampHours;

  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  String payload = "{\"d\":{\"adc\":";
  payload += String(wattHours, DEC);
  payload += "}}";

  Serial.print("Sending payload: "); Serial.println(payload);

  if (client.publish(eventTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
  } else {
    Serial.println("Publish FAILED");
  }
}
