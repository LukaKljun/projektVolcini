#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <Servo.h>

// === PIN SETUP ===
#define DHTPIN 2
#define DHTTYPE DHT11

#define SERVO_PIN 9
#define BUZZER_PIN 8

#define LDR_PIN A0
#define MQ_PIN A1

#define JOY_X A2
#define JOY_Y A3
#define JOY_SW 4

#define RED_BUTTON 3

Adafruit_SSD1306 display(128, 64, &Wire);
DHT dht(DHTPIN, DHTTYPE);
Servo door;
//-----------------------------------------spremenljivke
unsigned long lastOpenTime = 0;
bool doorOpen = false;

int page = 0;
const int totalPages = 3;

//--------------------------nastavitve
int darkThreshold = 680;   // light sensor
int mqThreshold = 130;
int doorOpenPos = 0;
int doorClosedPos = 90;

void playChime(int pin) {
  for (int f = 600; f <= 900; f += 5) {
    tone(pin, f);
    delay(2);
  }

  noTone(pin);
  delay(80);

  // Bright confirmation beep
  tone(pin, 1400, 120);
  delay(150);

  // Soft fade-out sweep
  for (int f = 1200; f >= 700; f -= 8) {
    tone(pin, f);
    delay(3);
  }

  noTone(pin);
}

void alarmBeep() {
  tone(BUZZER_PIN, 2000, 150);
}

void openDoor() {
  door.write(doorOpenPos);
  doorOpen = true;
  lastOpenTime = millis();
}

void closeDoor() {
  door.write(doorClosedPos);
  doorOpen = false;
}

void setup() {
  Serial.begin(9600);

  dht.begin();
  door.attach(SERVO_PIN);
  closeDoor();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(RED_BUTTON, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  tone(BUZZER_PIN, 1500, 200);
  delay(200);
}

// === DISPLAY FUNCTION ===
void showPage(int pg, int ldrVal, int mqVal, float temp, float hum, bool doorOpen) {
  display.clearDisplay();
  display.setCursor(0,0);

  if (pg == 0) {
    display.println("SMART HOUSE STATUS");
    display.print("Light: ");
    display.println(ldrVal);
    display.print("MQ: ");
    display.println(mqVal);
    display.print("Door: ");
    display.println(doorOpen ? "OPEN" : "CLOSED");
  }

  if (pg == 1) {
    display.println("ENVIRONMENT");
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");
    display.print("Hum : ");
    display.print(hum);
    display.println(" %");
  }

  if (pg == 2) {
    display.println("CONTROLS:");
    display.println("Joystick = Scroll");
    display.println("JoyBtn = Open");
    display.println("Red Btn = Chime");
    display.println("Auto-close active");
  }

  display.display();
}

void loop() {
  // === READ SENSORS ===
  int ldrVal = analogRead(LDR_PIN);
  int mqVal = analogRead(MQ_PIN);
  int joyX = analogRead(JOY_X);
  int joyY = analogRead(JOY_Y);
  bool joyPress = !digitalRead(JOY_SW);
  bool redPress = !digitalRead(RED_BUTTON);

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // === JOYSTICK PAGE SCROLLING ===
  if (joyY < 300) {  // UP
    page--;
    if (page < 0) page = totalPages - 1;
    delay(200);
  }
  if (joyY > 700) {  // DOWN
    page++;
    if (page >= totalPages) page = 0;
    delay(200);
  }

  // === DOOR CONTROL ===
  if (joyPress) {
    openDoor();
    tone(BUZZER_PIN, 1000, 120);
    delay(200);
  }

  // === AUTO-CLOSE CONDITIONS ===
  if (doorOpen) {
    bool tooDark = ldrVal < darkThreshold;
    bool timedOut = (millis() - lastOpenTime > 3000);

    if (tooDark && timedOut) {
      closeDoor();
      tone(BUZZER_PIN, 600, 150);
      delay(150);
    }
  }

  // === MQ BREATHING ALARM ===
  if (mqVal > mqThreshold) {
    alarmBeep();
  }

  // === RED BUTTON CHIME ===
  if (redPress) {
    playChime(BUZZER_PIN);
    delay(300);
  }

  // === SHOW INFO ON SCREEN ===
  showPage(page, ldrVal, mqVal, temp, hum, doorOpen);

  delay(50);
}
