#include <Wire.h>              // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
// Wiring: SDA pin is connected to A4 and SCL pin to A5.
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // address, chars, rows.

// Реле
#define PIN_CCW 4   // Поворот против часовой стрелки
#define PIN_CW 5    // Поворот по часовой стрелки
#define PIN_UP 11   // Актуатор вверх
#define PIN_DOWN 12 // Актуатор вниз
#define STEP 1      // Шаг
// Датчики
#define AZSENSOR A2 // Номер пина для аналогового датчика азимута
#define ELSENSOR A1 // Номер пина для аналогового датчика элевации
// Кнопки
#define BTN_CW 10
#define BTN_CCW 13
#define BTN_UP 8
#define BTN_DOWN 9
#define BTN_APPLY_AZ 7
#define BTN_APPLY_EL 6
#define BTN_OPERATE 2
#define BTN_MODE 3
//петля усреднения
const int numReadings = 25;
int readIndexAz = 0;
int readIndexEl = 0;
int azAngle = 0;          // Угол азимута
int azOldSensorValue = 0; // Предыдущее значение с датчика азимута
int azTarget = 300;       // Цель для поворота
boolean azMove = false;   // Флаг включения/отключения вращения по азимуту
String strAzAngle;        // Текущее положение антенны
String strAzTarget;       // Цель для перемещения
int azCorrect = 0;        // Коррекция азимута нуля градусов
int oldsensorValue = 0;
int totalAz = 0;
int averageAz = 0; // усреднение азимута

int elAngle = 0;
int elOldSensorValue = 0;
int elTarget = 0;
boolean elMove = false;
String strElAngle;
String strElTarget;
int elCorrect = 0;
unsigned long timing;


int totalEl = 0;
int averageEl = 0; // усреднение элевации
boolean operate = false;
byte mode = 0;

String PortRead;
int azPortTarget = 0;
int elPortTarget = 0;
const byte averageFactor = 20;
String serialName = "                ";
// 0 - clear, 1 - CW or UP, 2 - CCW or DOWN
byte azArrow = 0;
byte elArrow = 0;

char motd[] = " R8CDF ROTATOR 2020 ";
byte heart[8] = {0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000};

byte upArrow[8] = {0b00000, 0b00000, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000};
byte dwArrow[8] = {0b00000, 0b00000, 0b10001, 0b01010, 0b00100, 0b00000, 0b00000, 0b00000};

byte queue[8] = {0b00001,
                 0b00011,
                 0b00001,
                 0b00100,
                 0b00110,
                 0b10100,
                 0b11000,
                 0b10000
                };

long previousMillisTimeSWOne = 0; //счетчик прошедшего времени для мигания изменяемых значений.
long previousMillisTimeSWTwo = 0; //счетчик прошедшего времени для мигания изменяемых значений.
long intervalTimeSWOne = 500;     //интервал мигания изменяемых значений.
long intervalTimeSWTwo = 100;     //интервал мигания изменяемых значений.
long iOperateView = 0;

int avaregeAprox(int sensorValue) {
  if (averageFactor > 0)        // усреднение показаний для устранения "скачков"
  {
    oldsensorValue = sensorValue;
    return (oldsensorValue * (averageFactor - 1) + sensorValue) / averageFactor;
  }
}

uint8_t btn(int KEY)
{
  bool currentValue = digitalRead(KEY);
  bool prevValue;
  if (currentValue != prevValue)
  {
    delay(75);
    currentValue = digitalRead(KEY);
    return 0;
  }

  prevValue = currentValue;
  return 1;
};

int azSensor()
{

  azAngle = avaregeAprox(analogRead(AZSENSOR));
  azAngle = int(azAngle / 1014.0 * 360);

  if (azAngle < 0)
  {
    azAngle = 0;
  }

  if (azAngle > 359)
  {
    azAngle = 359;
  }

  return azAngle;
}

int elSensor()
{
  elAngle = avaregeAprox(stabilitySensor(analogRead(ELSENSOR)));
  elAngle = int(elAngle / 1014.0 * 360);
  if (elAngle < 0)
  {
    elAngle = 0;
  }
  if (elAngle > 100)
  {
    elAngle = 0;
  }

  return elAngle;
}

void getSensors()
{
  azSensor();
  elSensor();
}

void getKeysMain()
{
  if (btn(BTN_CW) == 0)
  {
    delay(1);
    if (azTarget + STEP <= 359)
      azTarget += STEP;
  }

  if (btn(BTN_CCW) == 0)
  {
    delay(1);
    if (azTarget - STEP >= 0)
      azTarget -= STEP;
  }

  if (btn(BTN_UP) == 0)
  {
    delay(10);
    if (elTarget + STEP <= 99)
      elTarget += STEP;
  }

  if (btn(BTN_DOWN) == 0)
  {
    delay(10);
    if (elTarget - STEP >= 0)
      elTarget -= STEP;
  }

  if (btn(BTN_APPLY_AZ) == 0)
  {
    delay(50);
    azMove = true;
  }

  if (btn(BTN_APPLY_EL) == 0)
  {
    delay(50);
    elMove = true;
  }
}

void getKeysOperate()
{

  /*
      0 MANUAL
      1 PC
      2 MANUAL BUTTON
  */
  if (btn(BTN_MODE) == 0) {
    if (mode == 2) {
      mode = 0;
      azTarget = 300;
      elTarget = 0;
    } else if (mode == 0) {
      mode = 1;
    } else if (mode == 1) {
      mode = 2;
      azTarget = 300;
      elTarget = 0;
    }
  }

  if (btn(BTN_OPERATE) == 0)
  {
    delay(100);
    if (operate)
    {
      operate = false;
      azArrow = 0;
      elArrow = 0;
      iOperateView = 0;
      if (azMove)
      {
        azMove = false;
      }

      if (elMove)
      {
        elMove = false;
      }

      digitalWrite(PIN_CW, LOW);
      digitalWrite(PIN_CCW, LOW);
      digitalWrite(PIN_UP, LOW);
      digitalWrite(PIN_DOWN, LOW);

    }
    else
    {
      operate = true;
      lcd.setCursor(17, 3);
      lcd.print(char(1));
    }
  }
}

void buttonManual(int az, int el) {

  if (btn(BTN_CW) == 0 && az < 352)
  {
    digitalWrite(PIN_CW, HIGH);
    azArrow = 1;
  } else {
    digitalWrite(PIN_CW, LOW);
    azArrow = 0;
  } 
  
  if (btn(BTN_CCW) == 0 && az > 1)
  {
    digitalWrite(PIN_CCW, HIGH);
    azArrow = 2;
  } else {
    digitalWrite(PIN_CCW, LOW);
  // azArrow = 0;
  }

  if (btn(BTN_UP) == 0 && el <= 99)
  {
    digitalWrite(PIN_UP, HIGH);
    elArrow = 1;
  } else {
    digitalWrite(PIN_UP, LOW);
    elArrow = 0;
  }

  if (btn(BTN_DOWN) == 0 && el >= 0)
  {
    digitalWrite(PIN_DOWN, HIGH);
    elArrow = 2;
  } else {
    digitalWrite(PIN_DOWN, LOW);
  //  elArrow = 0;
  }
}

String AzElString(int someIntVolue)
{
  if (someIntVolue < 0)
  {
    return "" + String(someIntVolue);
  }
  if (someIntVolue < 10)
  {
    return "  " + String(someIntVolue);
  }

  if (someIntVolue < 100)
  {
    return " " + String(someIntVolue);
  }

  if (someIntVolue >= 100)
  {
    return String(someIntVolue);
  }
}

void cw(boolean azMoveFlag)
{
  if (azMoveFlag)
  {
    digitalWrite(PIN_CCW, LOW);
    azArrow = 1;
    digitalWrite(PIN_CW, HIGH);
  }
  else
  {
    azArrow = 0;
    digitalWrite(PIN_CW, LOW);
  }
}

void ccw(boolean azMoveFlag)
{
  if (azMoveFlag)
  {
    digitalWrite(PIN_CW, LOW);
    azArrow = 2;
    digitalWrite(PIN_CCW, HIGH);
  }
  else
  {
    azArrow = 0;
    digitalWrite(PIN_CCW, LOW);
  }
}

void up(boolean elMoveFlag)
{
  if (elMoveFlag)
  {
    digitalWrite(PIN_DOWN, LOW);
    elArrow = 1;
    digitalWrite(PIN_UP, HIGH);
  }
  else
  {
    digitalWrite(PIN_UP, LOW);
  }
}
void down(boolean elMoveFlag)
{
  if (elMoveFlag)
  {
    digitalWrite(PIN_UP, LOW);
    elArrow = 2;
    digitalWrite(PIN_DOWN, HIGH);
  }
  else
  {
    digitalWrite(PIN_UP, LOW);
  }
}

int stabilitySensor(int SENSOR) {

  int currentValue = SENSOR;
  int prevValue;
  if (currentValue != prevValue)
  {

    if (millis() - timing > 10000) { // Вместо 10000 подставьте нужное вам значение паузы
      timing = millis();
      currentValue = SENSOR;
      return currentValue;
    }

  }

  prevValue = currentValue;
  // return prevValue;

}

void applyKeys() {
  if (operate)
  {
    if (btn(BTN_APPLY_AZ) == 0)
    {
      delay(50);
      azMove = true;
    }

    if (btn(BTN_APPLY_EL) == 0)
    {
      delay(50);
      elMove = true;
    }
  }
}

// Views display func
void operateView() {
  if (operate) {

    for (int i = 0; i <= 2; i++) {
      if (millis() - previousMillisTimeSWOne >= intervalTimeSWOne)
      {
        iOperateView = !iOperateView;
        previousMillisTimeSWOne = previousMillisTimeSWOne + intervalTimeSWOne;
      }


      if (millis() - previousMillisTimeSWTwo >= intervalTimeSWTwo)
      {
        iOperateView = !iOperateView;
        previousMillisTimeSWTwo = previousMillisTimeSWTwo + intervalTimeSWTwo;
      }

      if (azMove || elMove) {
        if (iOperateView == 1) {
          lcd.setCursor(17, 3);
          lcd.print(char(1));
        }
        if (iOperateView == 0) {
          lcd.setCursor(17, 3);
          lcd.print(" ");
        }
      } else {
        lcd.setCursor(17, 3);
        lcd.print(char(1));
      }
    }
  } else {
    lcd.setCursor(17, 3);
    lcd.print(" ");
    lcd.setCursor(18, 3);
    lcd.print(" ");
    lcd.setCursor(19, 3);
    lcd.print(" ");
  }
}

void modeView() {
  if (mode == 1) {
    lcd.setCursor(0, 3);
    lcd.print("PC           ");
    lcd.setCursor(0, 2);
    lcd.print("SN: ");
    lcd.setCursor(4, 2);
    lcd.print(serialName);
  } else if (mode == 0) {
    lcd.setCursor(0, 3);
    lcd.print("MANUAL       ");
  } else  if (mode == 2) {
    lcd.setCursor(0, 3);
    lcd.print("BUTTON MANUAL");
  }
}

void azArrowView() {
  if (azArrow == 0) {
    lcd.setCursor(18, 3);
    lcd.print(" ");
  } else if (azArrow == 1) {
    lcd.setCursor(18, 3);
    lcd.print(">");
  } else if (azArrow == 2) {
    lcd.setCursor(18, 3);
    lcd.print("<");
  }
}

void elArrowView() {

  if (elArrow == 0 ) {
    lcd.setCursor(19, 3);
    lcd.print(" ");
  } else if (elArrow == 1 ) {
    lcd.setCursor(19, 3);
    lcd.print(char(2));
    //lcd.print("U");
  } else if (elArrow == 2) {
    lcd.setCursor(19, 3);
    lcd.print(char(3));
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_CCW, OUTPUT);
  pinMode(PIN_CW, OUTPUT);
  pinMode(PIN_UP, OUTPUT);
  pinMode(PIN_DOWN, OUTPUT);
  pinMode(AZSENSOR, INPUT);
  pinMode(ELSENSOR, INPUT);

  pinMode(BTN_OPERATE, INPUT_PULLUP);
  pinMode(BTN_APPLY_AZ, INPUT_PULLUP);
  pinMode(BTN_APPLY_EL, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_CW, INPUT_PULLUP);
  pinMode(BTN_CCW, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  lcd.begin(20, 4);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.createChar(1, heart);
  lcd.createChar(2, upArrow);
  lcd.createChar(3, dwArrow);
  lcd.createChar(4, queue);

  // lcd.print("R8CDF ROTATOR 2020 ");
  for (int i = 0; i <= 19; i++) {
    lcd.setCursor(i, 1);
    lcd.print("_");
    delay(150);
    lcd.setCursor(i, 1);
    lcd.print(motd[i]);
    delay(30);
  }

  delay(1000);
  for (int i = 0; i <= 19; i++) {
    lcd.setCursor(i, 1);
    lcd.print(" ");
    delay(1);
  }
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("AZT: ");
  lcd.setCursor(0, 1);
  lcd.print("AZA: ");
  lcd.setCursor(9, 0);
  lcd.print("ELT: ");
  lcd.setCursor(9, 1);
  lcd.print("ELA: ");
  serialName = "                ";
  getSensors();
  operateView();
  modeView();
  azArrowView();
  elArrowView();
}

void loop()
{
  getSensors();
  getKeysOperate();
  operateView();
  modeView();
  azArrowView();
  elArrowView();
  if (operate)
  {
    if (mode == 1) {

      if (Serial.available())
      {
        PortRead = Serial.readString();
        int buffer_len = PortRead.length() + 1;
        char port_char_array[buffer_len];
        char az[7], el[7], sn[16];
        sscanf(PortRead.c_str(), "%s %s %s", &az, &el, &sn);
        azTarget = int(round(atof(az)));
        elTarget = int(round(atof(el)));

        lcd.setCursor(4, 2);
        lcd.print("                ");
        serialName = sn;

        if (azTarget >= 0 && azTarget <= 359)
        {
          azMove = true;
        }

        if ((elTarget >= 0) && (elTarget <= 99))
        {
          elMove = true;
        }
      }
    }
    if (mode == 0) {
      applyKeys();
      getKeysMain();
      serialName = "          ";
    }

    if (mode == 2) {
      buttonManual(azAngle, elAngle);
      serialName = "          ";
    }

    if (azMove)
    {
      if (azTarget == azAngle)
      {
        azArrow = 0;
        digitalWrite(PIN_CW, LOW);
        digitalWrite(PIN_CCW, LOW);
        azMove = false;
      } else if (azTarget - azAngle >= 1)
      {
        cw(azMove);
      } else if (azAngle - azTarget >= 1)
      {
        ccw(azMove);
      }
    }

    if (elMove)
    {
      if (elTarget == elAngle)
      {
        elArrow = 0;
        digitalWrite(PIN_UP, LOW);
        digitalWrite(PIN_DOWN, LOW);
        elMove = false;
      }
      if (elTarget - elAngle >= 1)
      {
        up(elMove);
      }

      if (elAngle - elTarget >= 1)
      {
        down(elMove);
      }
    }
  }

  // Отображение азимута текущей цели антенны
  lcd.setCursor(5, 0);
  lcd.print(strAzTarget);
  // Отображение азимута текущего положения антенны
  lcd.setCursor(5, 1);
  lcd.print(strAzAngle);
  // Отображение данных с датчика азимута
  strAzAngle = AzElString(azAngle);
  // Отображение цели азимута
  strAzTarget = AzElString(azTarget);

  // Отображение елевации цели антенны
  lcd.setCursor(13, 0);
  lcd.print(strElTarget);
  // Отображение элевации
  lcd.setCursor(13, 1);
  lcd.print(strElAngle);
  // Отображение данных с датчика элевации
  strElAngle = AzElString(elAngle);
  // Отображение цели элевации
  strElTarget = AzElString(elTarget);


}
