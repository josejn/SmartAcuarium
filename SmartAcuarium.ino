#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <RTClib.h>
#include <Time.h>
#include <DallasTemperature.h>
#include <Servo.h>

#define TEMP_PIN 2 //Se declara el pin donde se conectará la DATA
#define LIGHT_PIN 8 //Se declara el pin donde se conectará la LUZ
#define FEED_PIN 9
#define FEED2_PIN 10

#define FEED_TIME 800 //Milisengudos de espera entre posiciones del servo
#define FEED_MIN 170 //Angulo minimo del servo
#define FEED_MED 82 //Angulo medio del servo
#define FEED_MAX 0 //Angulo maximo del servo

#define LIGHT_HOUR_ON 18 //Hora de comienzo de la luz
#define LIGHT_MIN_ON 30 //Minuto de comienzo de la luz

#define LIGHT_HOUR_OFF 23 //Hora de apagado de la luz
#define LIGHT_MIN_OFF 30 //Minuto de apagado de la luz

#define BACKLIGHT_HOUR_ON 8 //Hora de comienzo de la luz LCD
#define BACKLIGHT_MIN_ON 30 //Minuto de comienzo de la luz LCD

#define BACKLIGHT_HOUR_OFF 23 //Hora de apagado de la luz LCD
#define BACKLIGHT_MIN_OFF 30 //Minuto de apagado de la luz LCD

#define HORA_COMER_1 8 //Hora de comer (Minuto 00)
#define HORA_COMER_2 20 //Hora 2 de comer (Minuto 00)

#define MANUAL 1
#define AUTO 0

OneWire ourWire(TEMP_PIN); //Se establece el pin declarado como bus para la comunicación OneWire
DallasTemperature sensors(&ourWire); //Se instancia la librería DallasTemperature
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display
Servo myServo;  // create a servo object
RTC_DS1307 RTC;              // RTC

int TOTAL_FEED; //Acumulador de alimentaciones total
bool HAN_COMIDO = false;
String input = "";
int luz = 2;
int lcdluz = 2;
int feed_times = 1;

struct TIME {
  byte Hour;
  byte Minute;
  byte Second;
};

TIME T;
int BEGIN_LIGHT; //Minutos de comienzo de luz
int END_LIGHT; //Minutos de fin de luz

TIME T2;
int BEGIN_BACKLIGHT; //Minutos de comienzo de luz
int END_BACKLIGHT; //Minutos de fin de luz

void setup()
{
  
  lcd.init();                      // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();

  pinMode(LIGHT_PIN, OUTPUT);           // set pin to input
  lcd.print("SmartAcuarium v1.0");

  Serial.begin(9600);
  while (!Serial) ; // wait for serial

  delay(200);
  short feedAcum = 0;
  sensors.requestTemperatures();
  myServo.attach(FEED_PIN);
  myServo.write(FEED_MIN);
  delay(FEED_TIME);
  myServo.detach();
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FEED2_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH);
  digitalWrite(FEED2_PIN, HIGH);

  BEGIN_LIGHT = LIGHT_HOUR_ON * 60 + LIGHT_MIN_ON;
  END_LIGHT = LIGHT_HOUR_OFF * 60 + LIGHT_MIN_OFF;

  BEGIN_BACKLIGHT = BACKLIGHT_HOUR_ON * 60 + BACKLIGHT_MIN_ON;
  END_BACKLIGHT = BACKLIGHT_HOUR_OFF * 60 + BACKLIGHT_MIN_OFF;
  RTC.begin();
}
void loop()
{
  while (Serial.available() > 0) {
    delay(10);
    char c = Serial.read(); // Read a character
    if (c == ';') {
      delay(200);
      break;
    }
    input += c;
  }

  if (input.length() > 0) {
    
    reconfig();
    input = ""; //clears variable for new input
  }

  printDate();
  light(T);
  backLight(T2);
  printTemp();
  feed(T,feed_times);

}

void reconfig() {
  //Strange workarrownd for Clock setting
  int setDate = false;
  
  //setDate=20160331190021;
  if (input.startsWith("setDate=") && input.length() == 22) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    
    int yyyy = input.substring(8, 12).toInt();
    int MM = input.substring(12, 14).toInt();
    int dd = input.substring(14, 16).toInt();
    int HH = input.substring(16, 18).toInt();
    int mm = input.substring(18, 20).toInt();
    int ss = input.substring(20, 22).toInt();

    setTime(HH, mm, ss, dd, MM, yyyy);
    setDate = true;
    
    Serial.print("NewDate:");
    Serial.print(HH); Serial.print(mm); Serial.print(ss); 
    Serial.print(" ");
    Serial.print(dd); Serial.print("/"); Serial.print(MM); Serial.print( "/" ); Serial.print(yyyy);Serial.print(";");

  }

  //setLight=1800,2330;
  else if (input.startsWith("setLight=") && input.length() == 18) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    
    int h1 = input.substring(9, 11).toInt();
    int m1 = input.substring(11, 13).toInt();
    int h2 = input.substring(14, 16).toInt();
    int m2 = input.substring(16, 18).toInt();
    
    Serial.print("NewLight:");
    Serial.print(h1); Serial.print(":"); Serial.print(m1);
    Serial.print(",");
    Serial.print(h2); Serial.print(":"); Serial.print(m2);
    Serial.println(";");
  }

   //setLcdLight=1800,2330;
  else if (input.startsWith("setLcdLight=") && input.length() == 21) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    
    int h1 = input.substring(11, 14).toInt();
    int m1 = input.substring(14, 16).toInt();
    int h2 = input.substring(17, 19).toInt();
    int m2 = input.substring(19, 21).toInt();
    
    Serial.print("NewLcdLight:");
    Serial.print(h1); Serial.print(":"); Serial.print(m1);
    Serial.print(",");
    Serial.print(h2); Serial.print(":"); Serial.print(m2);
    Serial.println(";");
  }
  
  //feedNow;
  else if (input.equals("feedNow")) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    feed(feed_times);
  }
  //light=2;
  else if (input.startsWith("light=") && input.length() == 7) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    luz = input.substring(6, 7).toInt();
  }
  //backlight=2;
  else if (input.startsWith("backlight=") && input.length() == 11) {
    Serial.print("SerialInput:");
    Serial.println(input); //prints string to serial port out
    lcdluz = input.substring(10, 11).toInt();
  }
  else {
    Serial.print("ERROR:SerialInput,");
    Serial.println(input);
    Serial.println("************************************************");
    Serial.println("* Valid commands are:                          *");
    Serial.println("* setDate=YYYYMMDDHHmmSS;| 20160101210030;     *");
    Serial.println("* setLight=HHMM,HHMM;    | Start,End           *");
    Serial.println("* setLcdLight=HHMM,HHMM; | Start,End           *");
    Serial.println("* feedNow;               | Triggers the feeder *");
    Serial.println("* light=Mode;     | Modes: 0:Off; 1:On; 2:Auto *");
    Serial.println("* backlight=Mode; | Modes: 0:Off; 1:On; 2:Auto *");
    delay(2000);
  }

  if (setDate){
    setDate = false;
    RTC.adjust(now());   
  }
}


float printTemp() {
  sensors.requestTemperatures(); //Prepara el sensor para, la lectura
  float temp = 0;
  
  temp = sensors.getTempCByIndex(0);

  Serial.print("Temp:");
  Serial.print(temp);
  Serial.println("C;");

  lcd.setCursor(0, 2);
  lcd.print("T: ");
  lcd.print(temp);
  lcd.print("C");
  return temp;
}

void printDate() {
  char buffer[9];

  DateTime now = RTC.now(); 
  
  T = {now.hour(), now.minute(), now.second()};
  Serial.print("Time:");
  sprintf(buffer, "%02d:%02d:%02d  %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  
  Serial.print(buffer);
  Serial.println(";");

  lcd.setCursor(0, 3);
  lcd.print(buffer);

}

bool feed(TIME t, int repeats) {
  Serial.print("Feed:");
  Serial.print(TOTAL_FEED);
  Serial.println(";");
  
  if ((T.Hour == HORA_COMER_1 || T.Hour == HORA_COMER_2) && T.Minute == 0) {
    if (!HAN_COMIDO) {
      feed(repeats);
      HAN_COMIDO = true;
    }
  } else {
    HAN_COMIDO = false;
  }
}

void feed(int repeats) {
  TOTAL_FEED++;
  lcd.setCursor(0, 1);
  lcd.print("Veces aliment.: ");
  lcd.print(TOTAL_FEED);
  lcd.setCursor(0, 3);

  lcd.setCursor(0, 3);
  lcd.print("Alimentando bichos! ");
  
  myServo.attach(FEED_PIN);

  for (int i=0; i<repeats; i++){ 
    
    myServo.write(FEED_MAX);
    delay(FEED_TIME);
  
    myServo.write(FEED_MIN);
    delay(100);
    myServo.write(FEED_MAX);
    delay(FEED_TIME);
  
    myServo.write(FEED_MIN);
    delay(FEED_TIME);
    myServo.write(FEED_MAX);
    delay(150);
    myServo.write(FEED_MIN);
    delay(150);
    myServo.write(FEED_MAX);
    delay(150);
    myServo.write(FEED_MIN);
    delay(350);
  }

  myServo.detach();

}

bool light(TIME t) {
  int ahora = T.Hour * 60 + T.Minute;

  if (luz == 1) {
    luzOn(MANUAL);
  } else if (luz == 0) {
    luzOff(MANUAL);
  } else if (BEGIN_LIGHT <= ahora && ahora < END_LIGHT) {
    luzOn(AUTO);
  } else {
    luzOff(AUTO);
  }
}

bool backLight(TIME t) {
  int ahora = T.Hour * 60 + T.Minute;

  if (lcdluz == 1) {
    lcdLuzOn(MANUAL);
  } else if (lcdluz == 0) {
    lcdLuzOff(MANUAL);
  } else if (BEGIN_BACKLIGHT <= ahora && ahora < END_BACKLIGHT) {
    lcdLuzOn(AUTO);
  } else {
    lcdLuzOff(AUTO);
  }
}

void lcdLuzOn(int mode) {
  Serial.print("LCD:ON,");
  Serial.println((mode == AUTO)?"AUTO;":"MANUAL;");
  lcd.backlight();
}

void lcdLuzOff(int mode) {
  Serial.print("LCD:OFF,");
  Serial.println((mode == AUTO)?"AUTO;":"MANUAL;");
  lcd.noBacklight();
}

void luzOn(int mode) {
  Serial.print("Light:ON,");
  Serial.println((mode == AUTO)?"AUTO;":"MANUAL;");
  lcd.setCursor(10, 2);
  lcd.print("Luz ON ");
  lcd.print((mode == AUTO)?"AUT":"MAN");
  digitalWrite(LIGHT_PIN, LOW);
}

void luzOff(int mode) {
  Serial.print("Light:OFF,");
  Serial.println((mode == AUTO)?"AUTO;":"MANUAL;");
  lcd.setCursor(10, 2);
  lcd.print("Luz OFF");
  lcd.print((mode == AUTO)?" AT":" MN");
  digitalWrite(LIGHT_PIN, HIGH);
}
