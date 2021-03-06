#include <Arduino.h>

/*
  MAI Cup Junior Code
  made at 25 Jan 2022
  by Christoph & Emanuel @ MAI Robotics
  Home
  26.01.2022
*/

//Todos: Zeit für Kurve einfügen, Pins einstellen, Links und rechts verkabeln, Besser Verkabeln
// - Pins -
// Motor rechts
#define RIGHT_RPWM 5
#define RIGHT_LPWM 6
#define RIGHT_REN 8
#define RIGHT_LEN 9
//Motor links
#define LEFT_RPWM 10
#define LEFT_LPWM 11
#define LEFT_REN 2
#define LEFT_LEN 3
// Ultraschall vorne/MITTE
#define TRIGGER_VORNE P0 //auf portexpander 
#define ECHO_VORNE 13 
// Ultraschall links
#define TRIGGER_LINKS P1 //auf portexpander 
#define ECHO_LINKS 12
//Ultraschall rechts
#define TRIGGER_RECHTS P3 
#define ECHO_RECHTS 4
//Hall Sensor
//#define HALL_SENSOR A0          //analog output (optional)
#define HALL_SENSOR_D A2        // digital output (benutzt zum auslesen ob magnet oder nd)
int hallValAlt;
//Infrarot Sensor
#define IR_LEFT A3 // connect ir sensor to arduino pin 2 (left one)
#define IR_RIGHT A1
#define IR_MIDDLE A0
//LED
#define LED_PIN P2


// - Daten -
//motor
int outLeft; // Soll-Wert links
int outRight;// Soll-Wert rechts
int motorLeft;  // Ist-Wert links
int motorRight; // Ist-Wert rechts
unsigned long previousMillisMotor = 0;
#define MOTORSPEEDSYNCINTERVAL 20
#define MOTORSPEEDSYNCSTEP 5
// neue logic checks
int hindernisLinks;
int hindernisRechts;
//Hall Sensor
int Hall_Val1=0,Hall_Val2=0;
enum HallPosition {LINKS, RECHTS};
HallPosition magnetPosition = LINKS;                                                             //INFO: HIER KANN MAN DIE MAGNETPOSITION KONFIGURIEREN
// Ultraschall
long dauerVorne=0; // Dauer Speicher für Ultraschcallsensor vorne
long entfernungVorne=0; // Entfernung Speicher für Ultraschcallsensor vorne

long dauerLinks=0; 
long entfernungLinks=0; 

long dauerRechts=0; 
long entfernungRechts=0;

int entfernungLinksOld; //alte variable wird hier gespeichert
int entfernungRechtsOld;
//technik
int durchgangCounter=0;
unsigned long previousMillis = 0;
#define SPEEDSYNCINTERVAL 300
//90° Drehung
const int umdrehungZeit=1500;
const int umdrehungSpeed=70;
//Linienskript
enum LineDirection {NOTHING, LEFT, RIGHT};
LineDirection lastKnownLineDirection = NOTHING;

// - Funktionen -
//liniensensor
int readSensorLeft() { //sensor links
  return digitalRead(IR_LEFT);
}

int readSensorMiddle() { //sensor middle
  return digitalRead(IR_MIDDLE);
}

int readSensorRight() { //sensor rechts
  return digitalRead(IR_RIGHT);
}
// distance
long readDistance(int trigger, int echo) { //main
  digitalWrite(trigger, LOW); //Hier nimmt man die Spannung für kurze Zeit vom Trigger-Pin, damit man später beim Senden des Trigger-Signals ein rauschfreies Signal hat.
  delay(5); // Pause 5 Millisekunden
  digitalWrite(trigger, HIGH); //Jetzt sendet man eine Ultraschallwelle los.
  delay(10); //Dieser „Ton“ erklingt für 10 Millisekunden.
  digitalWrite(trigger, LOW);//Dann wird der „Ton“ abgeschaltet.
  long dauer = pulseIn(echo, HIGH); //Mit dem Befehl „pulseIn“ zählt der Mikrokontroller die Zeit in Mikrosekunden, bis der Schall zum Ultraschallsensor zurückkehrt.
  return (long)((dauer/2) * 0.03432); //Nun berechnet man die Entfernung in Zentimetern. Man teilt zunächst die Zeit durch zwei (Weil man ja nur eine Strecke berechnen möchte und nicht die Strecke hin- und zurück). Den Wert multipliziert man mit der Schallgeschwindigkeit in der Einheit Zentimeter/Mikrosekunde und erhält dann den Wert in Zentimetern.
 }

long readDistanceFront() { //front ultraschall
  return readDistance(TRIGGER_VORNE, ECHO_VORNE);
 }

long readDistanceLeft() { //left ultraschall
  return readDistance(TRIGGER_LINKS, ECHO_LINKS);
 }

long readDistanceRight() { //right ultraschall
  return readDistance(TRIGGER_RECHTS, ECHO_RECHTS);
 }

int readMagnetSensor() {
  return digitalRead(HALL_SENSOR_D);
}

// - Methoden -

void motorAnsteuern() {
  unsigned long currentMillis = millis(); //delay ohne delay
  /*
  Alle 100 ms (MOTORSPEEDSYNCINTERVAL) wird der Sollwert mit dem Istwert verglichen.
  Wenn der Sollwert größer ist, wird der Istwert um einen kleinen Schritt (5, MOTORSPEEDSYNCSTEP) erhöht.
  Wenn er kleiner ist, wird er sofort angepasst. 
  Warum das Ganze? Weil wir sonst große Sprünge im Strom haben, was unsere kleine Batterie überfordert. Die Folge: Reboots
  */
  if (currentMillis - previousMillisMotor >= MOTORSPEEDSYNCINTERVAL) {
    previousMillisMotor = currentMillis;
    if (outRight != motorRight) {
      if (outRight > motorRight) {
        motorRight += MOTORSPEEDSYNCSTEP;
      } else {
        motorRight = outRight;
      }
    }
    if (outLeft != motorLeft) {
      if (outLeft > motorLeft) {
        motorLeft += MOTORSPEEDSYNCSTEP;
      } else {
        motorLeft = outLeft;
      }
    }
  }
  analogWrite(RIGHT_LPWM,motorRight); //Schreibe Geschwindigkeit auf Pins
  analogWrite(RIGHT_RPWM,0);          //Schreibe Geschwindigkeit auf Pins
  analogWrite(LEFT_LPWM,motorLeft);   //Schreibe Geschwindigkeit auf Pins
  analogWrite(LEFT_RPWM,0);           //Schreibe Geschwindigkeit auf Pins
}

void motorAnsteuernGeradeausLauf() {
  if(outLeft >= 70 || outRight >= 70) {
    outLeft -= 30;
    outRight -= 30;
  }
  motorAnsteuern();
}

//AUSGABENFUNKTINIEN

void magnetLesen() {
  Hall_Val2=digitalRead(HALL_SENSOR_D);
  if(Hall_Val2 == 0) {
    Serial.print("Ja"); // das hier wird nur zum loggen ausgeführt, man kann danach die hall_val2 trz abfragen für eine andere aktion
  } else {
    Serial.print("Nein");
  }
  Serial.println("   ");
}

void linieLinks() {
  int statusSensorLeft = digitalRead(IR_LEFT);
  if(statusSensorLeft == 1) { // diese abfrage kann man später auch noch verwenden
    Serial.print("Linie");
    Serial.print("   ");
  } else {
    Serial.print("Boden");
    Serial.print("   ");
  }
}

void linieMitte() {
  int statusSensorMiddle = digitalRead(IR_MIDDLE);
  if(statusSensorMiddle == 1) { // diese abfrage kann man später auch noch verwenden
    Serial.print("Linie");
    Serial.print("   ");
  } else {
    Serial.print("Boden");
    Serial.print("   ");
  }
}

void linieRechts() {
  int statusSensorRight = digitalRead(IR_RIGHT);
  if(statusSensorRight == 1) {
    Serial.print("Linie");
    Serial.print("   ");
  } else
    Serial.print("Boden");
    Serial.print("   ");
}

/*
 * Motoren starten (beiden fahren)
 */
void fahrenBeide() {
  outLeft = 40;
  outRight = 40;
  motorAnsteuern();
//  Serial.println("fahren beide laut Methode");
}

/*
 * Motoren stoppen (beide)
 */
void stehenbleiben() {
  outLeft = 0;
  outRight = 0;
  motorAnsteuern();
}
//old, dienen nur zur ausgabe
void entfernungMessenVorne() {
  digitalWrite(TRIGGER_VORNE, LOW); //Hier nimmt man die Spannung für kurze Zeit vom Trigger-Pin, damit man später beim Senden des Trigger-Signals ein rauschfreies Signal hat.
  delay(5); // Pause 5 Millisekunden
  digitalWrite(TRIGGER_VORNE, HIGH); //Jetzt sendet man eine Ultraschallwelle los.
  delay(10); //Dieser „Ton“ erklingt für 10 Millisekunden.
  digitalWrite(TRIGGER_VORNE, LOW);//Dann wird der „Ton“ abgeschaltet.
  dauerVorne = pulseIn(ECHO_VORNE, HIGH); //Mit dem Befehl „pulseIn“ zählt der Mikrokontroller die Zeit in Mikrosekunden, bis der Schall zum Ultraschallsensor zurückkehrt.
  entfernungVorne = (long)((dauerVorne/2) * 0.03432); //Nun berechnet man die Entfernung in Zentimetern. Man teilt zunächst die Zeit durch zwei (Weil man ja nur eine Strecke berechnen möchte und nicht die Strecke hin- und zurück). Den Wert multipliziert man mit der Schallgeschwindigkeit in der Einheit Zentimeter/Mikrosekunde und erhält dann den Wert in Zentimetern.
  
  if (entfernungVorne >= 500 || entfernungVorne <= 0) {//Wenn die gemessene Entfernung über 500cm oder unter 0cm liegt,…
    Serial.print("Vorne: ");
    Serial.print(entfernungVorne); //dann soll der serial monitor ausgeben „Kein Messwert“, weil Messwerte in diesen Bereichen falsch oder ungenau sind.
  }
  else {
    Serial.print("Vorne: ");
    Serial.print(entfernungVorne); //…soll der Wert der Entfernung an den serial monitor hier ausgegeben werden.
  //  Serial.println(" cm Vorne"); // Hinter dem Wert der Entfernung soll auch am Serial Monitor die Einheit "cm" angegeben werden, danach eine neue Zeile
  }
  Serial.print("cm   ");
}

void entfernungMessenLinks() {
  entfernungLinksOld = entfernungLinks;
  digitalWrite(TRIGGER_LINKS, LOW); //Hier nimmt man die Spannung für kurze Zeit vom Trigger-Pin, damit man später beim Senden des Trigger-Signals ein rauschfreies Signal hat.
  delay(5); // Pause 5 Millisekunden
  digitalWrite(TRIGGER_LINKS, HIGH); //Jetzt sendet man eine Ultraschallwelle los.
  delay(10); //Dieser „Ton“ erklingt für 10 Millisekunden.
  digitalWrite(TRIGGER_LINKS, LOW);//Dann wird der „Ton“ abgeschaltet.
  dauerLinks = pulseIn(ECHO_LINKS, HIGH); //Mit dem Befehl „pulseIn“ zählt der Mikrokontroller die Zeit in Mikrosekunden, bis der Schall zum Ultraschallsensor zurückkehrt.
  entfernungLinks = (long)((dauerLinks/2) * 0.03432); //Nun berechnet man die Entfernung in Zentimetern. Man teilt zunächst die Zeit durch zwei (Weil man ja nur eine Strecke berechnen möchte und nicht die Strecke hin- und zurück). Den Wert multipliziert man mit der Schallgeschwindigkeit in der Einheit Zentimeter/Mikrosekunde und erhält dann den Wert in Zentimetern.
  
  if (entfernungLinks >= 500 || entfernungLinks <= 0) {//Wenn die gemessene Entfernung über 500cm oder unter 0cm liegt,…
    Serial.print("Links: ");
    Serial.print(entfernungLinks); //dann soll der serial monitor ausgeben „Kein Messwert“, weil Messwerte in diesen Bereichen falsch oder ungenau sind.
  }
  else {
    Serial.print("Links: ");
    Serial.print(entfernungLinks); //…soll der Wert der Entfernung an den serial monitor hier ausgegeben werden.
  //  Serial.print(" cm Links"); // Hinter dem Wert der Entfernung soll auch am Serial Monitor die Einheit "cm" angegeben werden, danach eine neue Zeile
  }
  Serial.print("cm   ");
  //Serial.println(" Alte Entfernung links");
}


void entfernungMessenRechts() {
  entfernungRechtsOld = entfernungRechts;
  digitalWrite(TRIGGER_RECHTS, LOW); //Hier nimmt man die Spannung für kurze Zeit vom Trigger-Pin, damit man später beim Senden des Trigger-Signals ein rauschfreies Signal hat.
  delay(5); // Pause 5 Millisekunden
  digitalWrite(TRIGGER_RECHTS, HIGH); //Jetzt sendet man eine Ultraschallwelle los.
  delay(10); //Dieser „Ton“ erklingt für 10 Millisekunden.
  digitalWrite(TRIGGER_RECHTS, LOW);//Dann wird der „Ton“ abgeschaltet.
  dauerRechts = pulseIn(ECHO_RECHTS, HIGH); //Mit dem Befehl „pulseIn“ zählt der Mikrokontroller die Zeit in Mikrosekunden, bis der Schall zum Ultraschallsensor zurückkehrt.
  entfernungRechts = (long)((dauerRechts/2) * 0.03432); //Nun berechnet man die Entfernung in Zentimetern. Man teilt zunächst die Zeit durch zwei (Weil man ja nur eine Strecke berechnen möchte und nicht die Strecke hin- und zurück). Den Wert multipliziert man mit der Schallgeschwindigkeit in der Einheit Zentimeter/Mikrosekunde und erhält dann den Wert in Zentimetern.
  
  if (entfernungRechts >= 500 || entfernungRechts <= 0) {//Wenn die gemessene Entfernung über 500cm oder unter 0cm liegt,…
    Serial.print("Rechts: ");
    Serial.print(entfernungRechts); //dann soll der serial monitor ausgeben „Kein Messwert“, weil Messwerte in diesen Bereichen falsch oder ungenau sind.
  }
  else {
    Serial.print("Rechts: ");
    Serial.print(entfernungRechts); //…soll der Wert der Entfernung an den serial monitor hier ausgegeben werden.
  //  Serial.println(" cm Rechts"); // Hinter dem Wert der Entfernung soll auch am Serial Monitor die Einheit "cm" angegeben werden, danach eine neue Zeile
  }
  Serial.print("cm   ");
}




void umdrehungZeitVoid() {
    delay(umdrehungZeit);                          //HIER ZEIT EINFÜGEN WIE LANG ES DAUERT FÜR EINE KURVE
}
// outdated
void kursUmdrehungZeit() { //Zeit um wieder auf den Kurs zu kommen
    delay(100);                          //HIER ZEIT EINFÜGEN WIE LANG ES DAUERT FÜR EINE UMDREHUNG
}

void halbUmdrehungRechts() { //Quasi 90* Drehung nach rechts
  stehenbleiben();
  outLeft = umdrehungSpeed;
  outRight = 0;
  motorAnsteuern();
  delay(umdrehungZeit);
  outLeft = 0;
  motorAnsteuern();
}

void halbUmdrehungLinks() { //Quasi 90* Drehung nach links
  stehenbleiben();
  outLeft = 0;
  outRight = umdrehungSpeed;
  motorAnsteuern();
  delay(umdrehungZeit);
  outRight = 0;
  motorAnsteuern();
}


// ------------------------------------------------------------------------------------
// -                                Ende der Methoden                                 -
// ------------------------------------------------------------------------------------



// - Erster Start -
void setup() {
  Serial.begin(9600); //Starte den Serial Monitor
  // Motor rechts
  pinMode(RIGHT_RPWM,OUTPUT); //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(RIGHT_LPWM,OUTPUT); //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(RIGHT_LEN,OUTPUT);  //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(RIGHT_REN,OUTPUT);  //Pin-Modus setzen --> Pulsweitenmodulation
  digitalWrite(RIGHT_REN,HIGH); //Pin beschreiben
  digitalWrite(RIGHT_LEN,HIGH); //Pin beschreiben
  // Motor links
  pinMode(LEFT_RPWM,OUTPUT); //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(LEFT_LPWM,OUTPUT); //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(LEFT_LEN,OUTPUT);  //Pin-Modus setzen --> Pulsweitenmodulation
  pinMode(LEFT_REN,OUTPUT);  //Pin-Modus setzen --> Pulsweitenmodulation
  digitalWrite(LEFT_REN,HIGH); //Pin beschreiben
  digitalWrite(LEFT_LEN,HIGH); //Pin beschreiben
  // Abstandssensor vorne
  pinMode(TRIGGER_VORNE, OUTPUT); // Trigger-Pin ist ein Ausgang
  pinMode(ECHO_VORNE, INPUT); // Echo-Pin ist ein Eingang
  // Abstandssensor links
  pinMode(TRIGGER_LINKS, OUTPUT); // Trigger-Pin ist ein Ausgang
  pinMode(ECHO_LINKS, INPUT); // Echo-Pin ist ein Eingang
  // Abstandssensor rechts
  pinMode(TRIGGER_RECHTS, OUTPUT); // Trigger-Pin ist ein Ausgang
  pinMode(ECHO_RECHTS, INPUT); // Echo-Pin ist ein Eingang
  // Infrarotsensoren
  pinMode(IR_LEFT, INPUT); // sensor pin INPUT
  pinMode(IR_RIGHT, INPUT); // sensor pin INPUT
  pinMode(IR_MIDDLE, INPUT);
  // Hall Sensor
  pinMode(HALL_SENSOR_D,INPUT);

  
  Serial.println("----- INFO: Pins gesetzt");
  fahrenBeide(); //Bot startet das Fahren
  Serial.println("----- INFO: Im Setup Fahren gestartet");
//  delay(1500); //Delay dass nicht direkt irgendwelche Hindernisse erkannt werden
}


/*
 * Main Loop
 */
void loop() {
  fahrenBeide(); //losfahren, Wird UMBEDINGT benötigt für MotorSpeedSync!!
  hindernisLinks = 0;
  hindernisRechts = 0;
  //ausgabe start
  entfernungMessenLinks(); // er misst durchgehend die entfernung nach vorne
  entfernungMessenVorne(); //entfernung links und rechts messen wenn vorne nh wand is
  entfernungMessenRechts();
  linieLinks();
  linieMitte();
  linieRechts();
  magnetLesen();
  //ausgabe ende
  if(readMagnetSensor() != 0) { //wenn ein magnet da ist
    if(readSensorLeft() != 1 && readSensorMiddle() != 1 && readSensorRight() != 1 && lastKnownLineDirection == NOTHING) { //wenn keiner der sensoren eine linie erkannt hat

        //entfernung zu variable
        if (readDistanceFront() <= 23 && readDistanceFront() >= 1) { //wenn vorne eine wand ist dann fängt er an links und rechts zu messen
          if (readDistanceLeft() <= 23) { //wenn links eine wand ist wird hindernisLinks auf 1 gesetzt (wenn links weniger als 0 cm entfernt ist auch, also bei einem messfehler)
            hindernisLinks = 1;
          }
          if (readDistanceRight() <= 23) { //wenn rechts eine wand ist wird hindernisRechts auf 1 gesetzt
            hindernisRechts = 1;
          }

          //Hindernis Abfrage 
          if (2 == hindernisLinks + hindernisRechts) { //wenn 2 hindernisse vorhanden sind --> stehen bleiben (noch kein richtiger code hier gefunden)
            stehenbleiben();
            Serial.println("----- INFO: Stehen geblieben, da 2 Hindernisse vorhanden sind");
          }
          else if (hindernisLinks == 1) { // wenn links ein hindernis ist, fährt er wieder los und gibt eine ausgabe (als erstes mal zum testen)
            halbUmdrehungRechts();
            fahrenBeide();
            //Serial.println("----- INFO: Links hindernis fährt also nach rechts -----");
          }
          else if (hindernisRechts == 1) { 
            halbUmdrehungLinks();
            fahrenBeide();
            //Serial.println("----- INFO: Rechts hindernis fährt also nach links -----");
          }
          else if (0 == hindernisLinks + hindernisRechts) { //bei keinem hindernis und nur vorne fährt er halt rechts
            //Serial.println("----- INFO: Kein Hindernis links/rechts --> fährt nach rechts");
            halbUmdrehungRechts();
            fahrenBeide();
          }
        }
        /*
        //Speedsync
        unsigned long currentMillis = millis(); //delay ohne delay
        if (currentMillis - previousMillis >= SPEEDSYNCINTERVAL  && readSensorMiddle() == 0) {
          previousMillis = currentMillis;
          if(readDistanceLeft() > readDistanceRight() || readDistanceRight() <= 8 || readDistanceRight() >= 45) { //größer als 45 weil so viel gar nicht sein kann, das ergebnis muss falsch sein
            outRight += 30;
            motorAnsteuernGeradeausLauf();
          }
          if(readDistanceRight() > readDistanceLeft() || readDistanceLeft() <= 8 || readDistanceLeft() >= 45) {
            outLeft += 30;
            motorAnsteuernGeradeausLauf();
          }
        }*/
    }

    //Linienabfrage
    else { //wenn doch eine linie erkannt wurde

    //wenn linker sensor 1 ist und der mittlere 0: stehen bleiben, nahc links drehen solange bis mittlerer sensor auch linie sieht und der linke nicht mehr
      if(readSensorLeft() == 1 && readSensorMiddle() == 0) {
        stehenbleiben();
        lastKnownLineDirection = LEFT; //er "merkt" sich auf welcher seite zuletzt eine linie war
        while(readSensorMiddle() == 0) {
          outRight == 20;
          motorAnsteuern();
        }
        stehenbleiben();
      } 
    //wenn rechter sensor 1 ist und der mittlere 0: stehen bleiben, nach rechts drehen solange bis mittlerer sensor 1 ist und rechter 0
      else if(readSensorRight() == 1 && readSensorMiddle() == 0) {
        stehenbleiben();
        lastKnownLineDirection = RIGHT; //er "merkt" sich auf welcher seite zuletzt eine linie war
        while(readSensorMiddle() == 0) {
          outLeft == 20;
          motorAnsteuern();
        }
        stehenbleiben();
      } 
    //wenn alle sensoren 0 sind: dreht in die jeweilige richtung, bis mittlerer sensor wieder 1, dann lastDirectionRecognized = 0
      if(readSensorLeft() != 1 && readSensorMiddle() != 1 && readSensorRight() != 1 && lastKnownLineDirection != NOTHING) {
        if(lastKnownLineDirection == LEFT) {
          lastKnownLineDirection = NOTHING;
          stehenbleiben();
          while(readSensorMiddle() == 0) {
            outRight == 20;
            motorAnsteuern();
          }
          stehenbleiben();
        }
        else if(lastKnownLineDirection == RIGHT) {
          lastKnownLineDirection = NOTHING;
          stehenbleiben();
          while(readSensorMiddle() == 0) {
            outLeft == 20;
            motorAnsteuern();
          }
          stehenbleiben();
        }
      }
    }
 }
}
