#include <Keypad.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define WIFI_SSID "JotaPe"
#define WIFI_PASSWORD "senha123"

/* Define the API Key */
#define API_KEY "AIzaSyCKS7ouEvn4bP0Bxalemw0GuHK6yWLbQhs"

/* Define the RTDB URL */
#define DATABASE_URL "https://onlinelab-dfd7c-default-rtdb.firebaseio.com/"

/* Define the user Email and password that already registered or added in your project */
#define USER_EMAIL "jppelioterio@gmail.com"
#define USER_PASSWORD "12345678"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long accessStartTime = 0;

const int ledPin = 2;
const int accessPin = 4;

RTC_DS3231 rtc;

const uint8_t ROWS = 4;
const uint8_t COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

uint8_t colPins[COLS] = { 26, 25, 33, 32 };
uint8_t rowPins[ROWS] = { 13, 12, 14, 27 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputPassword = "";
String correctPassword = "";
int timeSet = 0;
bool accessGranted = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  rtc.adjust(DateTime(__DATE__, __TIME__));

  pinMode(ledPin, OUTPUT);
  pinMode(accessPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(accessPin, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;
}

void loop() {
  if (Firebase.ready()) {
    if (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0) {
      sendDataPrevMillis = millis();

      if (Firebase.RTDB.getString(&fbdo, "/Acesso/senha", &correctPassword)) {
        Serial.println("Password fetched from Firebase");
      } else {
        Serial.println(fbdo.errorReason().c_str());
      }

      if (Firebase.RTDB.getInt(&fbdo, "/Acesso/time_set", &timeSet)) {
        Serial.println("Time set fetched from Firebase");
      } else {
        Serial.println(fbdo.errorReason().c_str());
      }
    }

    if (accessGranted && (millis() - accessStartTime >= timeSet * 60000)) {
      accessGranted = false;
      digitalWrite(accessPin, LOW);
      Firebase.RTDB.setBool(&fbdo, "/Acesso/status", false);
    }
  }

  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      if (inputPassword == correctPassword) {
        digitalWrite(accessPin, HIGH);
        Firebase.RTDB.setBool(&fbdo, "/Acesso/status", true);
        accessGranted = true;
        accessStartTime = millis();
      } else {
        for (int i = 0; i < 3; i++) {
          digitalWrite(ledPin, HIGH);
          delay(200);
          digitalWrite(ledPin, LOW);
          delay(200);
        }
      }
      inputPassword = "";
    } else if (key == '*') {
      inputPassword = "";
    } else {
      inputPassword += key;
    }
  }
}

void printTwoDigits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}
