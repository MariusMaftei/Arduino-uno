#include <Servo.h>
#include <SPI.h>
#include "Ucglib.h"

#define SERVO_PIN 3 // Servo Signal->D3
#define ECHO_PIN 5  // Ultrasonic Module Echo->D5
#define TRIG_PIN 6  // Ultrasonic Module Trig->D6
#define RESET_PIN 8 // TFT Module RESET->D8
#define AO_PIN 9    // TFT Module AO->D9
#define CS_PIN 10   // TFT Module CS->D10
#define BAUD 115200

int ScreenHeight = 128;
int ScreenWidth = 160;
int CenterX = 80;
int BasePosition = 118;
int ScanLength = 105;

Servo BaseServo;
Ucglib_ST7735_18x128x160_HWSPI ucg(AO_PIN, CS_PIN, RESET_PIN);

const int maxDataPoints = 91; // Store 0° to 90° (every 2°)
int angles[maxDataPoints];
int distances[maxDataPoints];
int dataIndex = 0;

// Function declarations
void displayStartupScreen();
void ClearScreen();
int GetDistance();
void DrawRadarCurves();
void DrawRadarRanges();
void LogScannedData();

void setup()
{
  ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.setRotate90();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(BAUD);

  BaseServo.attach(SERVO_PIN);
  BaseServo.write(90); // Start at 0° position (center/front)

  displayStartupScreen();
  ClearScreen();

  ucg.setFont(ucg_font_orgv01_hr);
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
}

void displayStartupScreen()
{
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setColor(0, 0, 100, 0);
  ucg.drawGradientBox(0, 0, 160, 128);
  ucg.setColor(0, 255, 0);
  ucg.setPrintPos(25, 40);
  ucg.setFont(ucg_font_logisoso18_tf);
  ucg.print("Z.I.L.G.A.M.R");
  ucg.setFont(ucg_font_helvB08_tf);
  ucg.setPrintPos(40, 100);
  ucg.print("Testing . . . OK");
  delay(2000);
}

void ClearScreen()
{
  ucg.setColor(0, 0, 0, 0);
  ucg.clearScreen();
}

int GetDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  if (duration == 0)
    return 999;                // No valid response
  return duration * 0.034 / 2; // Convert to cm
}

void DrawRadarCurves()
{
  ucg.setColor(0, 40, 0);
  ucg.drawDisc(CenterX, BasePosition, 3, UCG_DRAW_ALL);
  for (int r = 29; r <= 115; r += 29)
  {
    ucg.drawCircle(CenterX, BasePosition, r, UCG_DRAW_UPPER_LEFT);
    ucg.drawCircle(CenterX, BasePosition, r, UCG_DRAW_UPPER_RIGHT);
  }
  ucg.drawLine(0, BasePosition, ScreenWidth, BasePosition);
}

void DrawRadarRanges()
{
  ucg.setColor(0, 180, 0);
  ucg.setPrintPos(CenterX - 10, ScreenHeight - 113);
  ucg.print("100cm");
  ucg.setPrintPos(CenterX - 10, ScreenHeight - 96);
  ucg.print("75cm");
  ucg.setPrintPos(CenterX - 10, ScreenHeight - 68);
  ucg.print("50cm");
  ucg.setPrintPos(CenterX - 10, ScreenHeight - 39);
  ucg.print("25cm");
}

void LogScannedData()
{
  ClearScreen();
  ucg.setColor(0, 255, 0);
  ucg.setFont(ucg_font_helvB08_tf);
  ucg.setPrintPos(10, 20);
  ucg.print("Scanned Data:");

  // Send scanned data to the serial port for Node.js
  for (int i = 0; i < dataIndex; i++)
  {
    String logData = String("Angle: ") + angles[i] + " Dist: " + distances[i];
    Serial.println(logData); // Send data to Serial
  }
}

void loop()
{
  DrawRadarCurves();
  DrawRadarRanges();

  // Sweep from -180° to 90° (with center at 0°)
  for (int angle = -180; angle <= 90; angle += 2)
  {
    int servoAngle = map(angle, -180, 90, 0, 180); // Map -180° to 90° into servo range 0° to 180°
    BaseServo.write(servoAngle);
    delay(30);

    int distance = GetDistance();
    int x = CenterX + ScanLength * cos(radians(angle));
    int y = BasePosition - ScanLength * sin(radians(angle));

    // Draw radar sweep line
    ucg.setColor(0, 255, 0);
    ucg.drawLine(CenterX, BasePosition, x, y);
    delay(20);
    ucg.setColor(0, 0, 0);
    ucg.drawLine(CenterX, BasePosition, x, y);

    // Draw detected object
    if (distance < 100)
    {
      ucg.setColor(255, 0, 0);
      int pointX = CenterX + 1.15 * distance * cos(radians(angle));
      int pointY = BasePosition - 1.15 * distance * sin(radians(angle));
      ucg.drawDisc(pointX, pointY, 2, UCG_DRAW_ALL);
    }

    // Save scanned data
    if (dataIndex < maxDataPoints)
    {
      angles[dataIndex] = angle;
      distances[dataIndex] = distance;
      dataIndex++;
    }
  }

  LogScannedData(); // Display and log the scanned data
  delay(1000);      // Pause between scans
}
