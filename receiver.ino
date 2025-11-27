#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "********";
const char* password = "********";

WiFiUDP Udp;
unsigned int localUdpPort = 12345;
char incomingPacket[255];

const int MOTOR_PIN1 = D1;
const int MOTOR_PIN2 = D2;
const int LED_PIN = LED_BUILTIN;
const unsigned long PRESS_TIME = 350;
const unsigned long HEARTBEAT_INTERVAL = 20000;

bool heaterOn = false;
unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  Udp.begin(localUdpPort);
  Serial.print("Listening for UDP broadcast on port ");
  Serial.println(localUdpPort);
  Serial.print("Heater status: ");
  Serial.println(heaterOn ? "ON" : "OFF");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    heartbeatFlash();
    lastHeartbeat = currentMillis;
  }
  
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) incomingPacket[len] = 0;

    Serial.print("Received packet: ");
    Serial.println(incomingPacket);

    if (strcasecmp(incomingPacket, "heater-on") == 0) {
      if (heaterOn) {
        Serial.println("Heater already ON → No action");
        broadcastStatus("ALREADY_ON");
      } else {
        pressButton();
        heaterOn = true;
        Serial.println("Heater turned ON");
        broadcastStatus("ON");
      }
    } 
    else if (strcasecmp(incomingPacket, "heater-off") == 0) {
      if (!heaterOn) {
        Serial.println("Heater already OFF → No action");
        broadcastStatus("ALREADY_OFF");
      } else {
        pressButton();
        heaterOn = false;
        Serial.println("Heater turned OFF");
        broadcastStatus("OFF");
      }
    }
    else if (strcasecmp(incomingPacket, "heater-press") == 0) {
      Serial.println("Manual press command → Pressing button...");
      pressButton();
      heaterOn = !heaterOn;
      Serial.print("Heater toggled to: ");
      Serial.println(heaterOn ? "ON" : "OFF");
      broadcastStatus(heaterOn ? "ON" : "OFF");
    }
  }

  delay(10);
}

void pressButton() {
  digitalWrite(LED_PIN, LOW);
  
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  delay(PRESS_TIME);
  
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  delay(150);
  
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, HIGH);
  delay(PRESS_TIME);
  
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Button press complete.");
}

void broadcastStatus(const char* status) {
  char responseMsg[50];
  snprintf(responseMsg, sizeof(responseMsg), "HEATER_STATUS:%s", status);
  
  Udp.beginPacket(IPAddress(192, 168, 1, 255), 12346);
  Udp.write(responseMsg);
  Udp.endPacket();
  
  Serial.print("Broadcast status: ");
  Serial.println(responseMsg);
}

void heartbeatFlash() {
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
}
