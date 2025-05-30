/*
  Simple wemos D1 mini  MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.
  It connects to the provided access point using dhcp, using ssid and pswd
  It connects to an MQTT server ( using mqtt_server ) then:
  - publishes "connected"+uniqueID to the [root topic] ( using topic )
  - subscribes to the topic "[root topic]/composeClientID()/in"  with a callback to handle
  - If the first character of the topic "[root topic]/composeClientID()/in" is an 1,
    switch ON the ESP Led, else switch it off
  - after a delay of "[root topic]/composeClientID()/in" minimum, it will publish
    a composed payload to
  It will reconnect to the server if the connection is lost using a blocking
  reconnect function.

*/

#include <WiFi.h>
#include <PubSubClient.h>



// Update these with values suitable for your network.

const char *ssid = "Accessphone";
const char *password = "Accessphone";
const char *mqtt_server = "145.14.158.14";
const char *topic = "wemos"; // rhis is the [root topic]

long timeBetweenreadings = 25;
long timeBetweenMessages = 500 * 1;
const int readingLenght = 20;
int lijstReadings[readingLenght];
int plaatsInLijst = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastReading = 0;
int value = 0;
int pin = 34;
unsigned long frequency;

int status = WL_IDLE_STATUS; // the starting Wifi radio's status

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    #ifdef BUILTIN_LED
    digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    #endif
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  }
  else
  {
    #ifdef BUILTIN_LED
    digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
    #endif
  }
}

String macToStr(const uint8_t *mac)
{
  String result;
  for (int i = 0; i < 6; ++i)
  {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

String composeClientID()
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientId;
  clientId += "esp-";
  clientId += macToStr(mac);
  return clientId;
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    String clientId = composeClientID();
    clientId += "-";
    clientId += String(micros() & 0xff, 16); // to randomise. sort of

    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, ("connected " + composeClientID()).c_str(), true);
      // ... and resubscribe
      // topic + clientID + in
      String subscription;
      subscription += topic;
      subscription += "/";
      subscription += composeClientID();
      subscription += "/in";
      client.subscribe(subscription.c_str());
      Serial.print("subscribed to : ");
      Serial.println(subscription);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{

#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  #endif
  Serial.begin(115200);
  pinMode(pin, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  // confirm still connected to mqtt server
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastReading > timeBetweenreadings)
  {
    int tmp = pulseIn(pin, HIGH);
    Serial.println("frequency: " + String(tmp));

    lijstReadings[plaatsInLijst] = tmp;
    plaatsInLijst++;
    lastReading = now;
  }
  if (now - lastMsg > timeBetweenMessages)
  {
    lastMsg = now;
    ++value;
    plaatsInLijst = 0;
    String payload = "{\"readings\": [";

    for (int i = 0; i < readingLenght; i++)
    {

      payload += lijstReadings[i];

      if (i < readingLenght - 1)
        payload += ",";
    }

    payload += "]}";
    String pubTopic;
    pubTopic += topic;
    pubTopic += "/";
    pubTopic += composeClientID();
    pubTopic += "/out";
    Serial.print("Publish topic: ");
    Serial.println(pubTopic);
    Serial.print("Publish message: ");
    Serial.println(payload);
    client.publish((char *)pubTopic.c_str(), (char *)payload.c_str(), true);
  }
}
