// Rechteckgenerator von 31 Hz bis 65535 Hz (2^16)
// mit inkrementellen Drehregler
// kurzes Drücken:      Wechsel der Schrittweite (1, 10, 100 oder 1000 Hz)
// langes Drücken:      Beschleunigung aus/an

//ClickEncoder Bibliothek
#include <ClickEncoder.h> // https://github.com/0xPIT/encoder/tree/arduino
#include <TimerOne.h>   // Download bei Arduino IDE Bibliothek

//LCD
#include <Wire.h>  // Comes with Arduino IDE
#include <LiquidCrystal_I2C.h>    // https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

#define LCD_CHARS   16
#define LCD_LINES    2

byte sinus_1[8] = {
  0b00000, //    
  0b00000, //  
  0b00000,  //  
  0b11000, //   ***
  0b01000, //   * *
  0b01000, //   * *
  0b01111, //  ** **
  0b00000, //
};

byte sinus_2[8] = {
  0b00000, //    
  0b00000, //  
  0b00000,  //  
  0b01111, //   ***
  0b01000, //   * *
  0b01000, //   * *
  0b11000, //  ** **
  0b00000, //
};

byte sinus_3[8] = {
  0b00000, //    
  0b00000, //  
  0b00000,  //  
  0b11000, //   ***
  0b01000, //   * *
  0b01000, //   * *
  0b01111, //  ** **
  0b00000, //
};

byte delta[8] = {
  0b00000, //    
  0b00000, //  
  0b00000,  //  
  0b00100, //   
  0b01010, //    *
  0b10001, //   * *
  0b11111, //  *****
  0b00000, //
};

const int frequency_pin = 10;

const byte encoderPinA = A0;
const byte encoderPinB = A1;
const byte encoderPinPush = A2;

unsigned int frequency = 0; 
unsigned int frequency_step = 1;
unsigned int frequency_step_decimals = 1;
boolean cursor_blinking = true;

volatile unsigned long alteZeit;

ClickEncoder *encoder;
int16_t last;

void timerIsr() {
  encoder->service();
}

void change_frequency(unsigned int n){
  tone(frequency_pin,n);
}

void delete_LCDrow(int x, int y){
  lcd.setCursor(x, y);
  for (int i = 0; i < LCD_CHARS; i++){
    lcd.print(" ");
  }
}

void blinking_cursor(int k, int i, int j){
    lcd.setCursor(k,i);
    if ((millis() - alteZeit) < j) {
      lcd.cursor();
    }
    else if ((millis() - alteZeit) < 2*j){
      lcd.noCursor();
    }
    else 
      alteZeit = millis();
}

void show_frequency(){
    unsigned int khz = int(frequency/1000);
    unsigned int hz = frequency - khz*1000;

    char buf[LCD_CHARS];     //char buf[LCD_CHARS]; // RICHTIG Arraytiefe wählen, damit es zu keinem Bufferoverflow kommt
    sprintf(buf, "%3u %03u Hz", khz,hz);  // 10 Zeichen verbraucht
    // FORMATIERUNG
    // formatierter String wird in von der sprintf Funktion in buf gespeichert.
    // % Platzhalter für khz und zweiter % für hz
    // 3 für die Breite der Symbole. Falls keine vorhanden, wird der Platz mit Leerzeichen gefüllt
    // u steht für Unsigned decimal integer
    // 03 für die Breite der Symbole. Falls keine vorhanden, wird der Platz dieses Mal mit NULLEN gefüllt.
    
    Serial.println(buf);
    
    delete_LCDrow(0,1);
    lcd.setCursor(0, 1);
    lcd.print(buf);    
}

void show_frequency_step(){
    char buf[LCD_CHARS]; // RICHTIG Arraytiefe wählen, damit es zu keinem Bufferoverflow kommt
    sprintf(buf, "f %4u Hz",frequency_step );       
    Serial.print("Δ");
    Serial.println(buf);
    
    sprintf(buf, " f %4u Hz",frequency_step ); 
    delete_LCDrow(0,1);
    lcd.setCursor(0, 1);
    lcd.print(buf);
    lcd.setCursor(0,1);
    lcd.write(4);
}


void switch_frequency_step() { 
      frequency_step = frequency_step *10;   
      if (frequency_step > 1000)   // Fall Schrittweite größer als 10 00, sprint er wieder auf 1 zurück
        frequency_step = 1;
}


void setup() {
  
  encoder = new ClickEncoder(A1, A0, A2,4);   //initialisiert Inkrementalgeber mit Analogpin 0,1 und 2 sowie stepsPerNotch = 4 (Schritt pro kleinste Drehung)  Quelle https://github.com/0xPIT/encoder/tree/arduino

  Serial.begin(9600);

  lcd.begin(LCD_CHARS,LCD_LINES);   // oder 20,4 bei großem Display
  
  lcd.createChar(1, sinus_1); // Sonderzeichen
  lcd.createChar(2, sinus_2); 
  lcd.createChar(3, sinus_3); 
  lcd.createChar(4, delta); 
  
  lcd.setCursor(0,0); //Starte bei Spalte 1, Zeile 1
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.print(" Frequenzgen.");
  lcd.setCursor(0,1);
  lcd.print("Rechtecksignal");

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  
  last = -1;
  encoder->setAccelerationEnabled(true);
}

void loop() {
  if (cursor_blinking == true){
    if (frequency_step_decimals < 4)  // Abfrage nötig, da Leerzeichen bei 1000er
        blinking_cursor(7-frequency_step_decimals,1,500);
    else 
        blinking_cursor(6-frequency_step_decimals,1,500);       
  }
  else lcd.noCursor();
  
  frequency += frequency_step*encoder->getValue();
  if (frequency != last) { //Frequenzausgabe und Anzeig auffrischen, falls sich die gewählte Frequenz geändert hat
    change_frequency(frequency);
    last = frequency;
    show_frequency();
    cursor_blinking = true; // TODO: wird unnötigerweise immer und immmer wieder ausgeführt
  }
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Released:
          encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          Serial.print("Beschleun. ");
          Serial.println((encoder->getAccelerationEnabled()) ? "an" : "aus");
          delete_LCDrow(0,1);
          lcd.setCursor(0,1);
          lcd.print("Beschleun. ");
          lcd.print((encoder->getAccelerationEnabled()) ? "an" : "aus");
          break;
      case ClickEncoder::Clicked:
          switch_frequency_step();
          show_frequency_step();
          frequency_step_decimals = int(log10(frequency_step)) + 1; // Berechnet wie viele Dezimalstellen die Zahlen 1, 10, 100, 1000 haben MIT Typumwandlung, da log10 DOUBLE ist          
          cursor_blinking = false;
      break;
      }
  } 
}