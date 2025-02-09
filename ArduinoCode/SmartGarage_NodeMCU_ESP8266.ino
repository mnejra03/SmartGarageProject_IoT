#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Servo.h>

#define WIFI_SSID "WIFI"
#define WIFI_PASSWORD "PASS"

#define FIREBASE_HOST "YOURS"
#define FIREBASE_AUTH "YOURS"

//Pinovi
#define trigPin D3
#define echoPin D4
#define servoPin D2
#define ledDetekcija D1  
#define ledUbrzano D5   
//Firebase objekti
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
//Servo motor
Servo servoMotor;

//Funkcija za mjerenje udaljenosti
float getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;

    return distance;
}

//Funkcija za brzo treperenje LED diode dok se vrata pomjeraju
unsigned long previousMillis = 0;  // Početno vrijeme za treperenje
const long interval = 200;         // Interval treperenja u milisekundama

void blinkFast() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        int ledState = digitalRead(ledUbrzano); // Provjerava trenutni status LED diode
        digitalWrite(ledUbrzano, !ledState);    // Ako je uključena, gasi je, i obrnuto
    }
}

void setup() {
    Serial.begin(115200);
    
    //WiFi povezivanje
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Povezivanje na WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Povezano!");

    //Firebase podešavanje
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    //Podešavanje pinova
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(ledDetekcija, OUTPUT);
    pinMode(ledUbrzano, OUTPUT);
    
    //Servo motor
    servoMotor.attach(servoPin);
    servoMotor.write(90); // Početna pozicija

    Serial.println("Sistem spreman!");
}

void loop() {
    //Čitanje ultrazvučnog senzora
    float distance = getDistance();
    Serial.println("Udaljenost: " + String(distance) + " cm");

    //Ažuriranje Firebase-a i kontrola LED diode za detekciju auta
    if (distance < 15) {
        Firebase.setInt(firebaseData, "/garage/detekcija", 1);
        Firebase.setFloat(firebaseData, "/garage/udaljenost", distance);
        digitalWrite(ledDetekcija, HIGH); // LED D7 svijetli dok je auto blizu
    } else {
        Firebase.setInt(firebaseData, "/garage/detekcija", 0);
        Firebase.setFloat(firebaseData, "/garage/udaljenost", distance);
        digitalWrite(ledDetekcija, LOW); // LED D7 se gasi kad auta nema
    }

    //Kontrola vrata preko Firebase-a
    if (Firebase.getInt(firebaseData, "/garage/status")) {
    int status = firebaseData.intData();
    Serial.println("Primljen status: " + String(status));

    if (status == 1) { 
        Serial.println("Otvaranje garaže...");
        digitalWrite(ledUbrzano, HIGH); // LED D8 svijetli dok se motor pomjera
        
        // Postepeno otvaranje vrata
        for (int pos = 90; pos >= 0; pos -= 1) { 
            servoMotor.write(pos);
            delay(50); // Pauza od 200ms između pomjeranja
        }

        delay(200); // Čekaj dok su vrata otvorena
        servoMotor.write(90); // Vraća u neutralnu poziciju
        digitalWrite(ledUbrzano, LOW); // LED D8 se gasi
        Firebase.setInt(firebaseData, "/garage/status", 2);
    }
    else if (status == 0) {
        Serial.println("Zatvaranje garaže...");
        digitalWrite(ledUbrzano, HIGH); // LED D8 svijetli dok se motor pomjera
        
        // Postepeno zatvaranje vrata
        for (int pos = 90; pos <= 180; pos += 1) { 
            servoMotor.write(pos);
            delay(50); // Pauza od 200ms između pomjeranja
        }

        delay(200); // Čekaj dok su vrata zatvorena
        servoMotor.write(90); // Vraća u neutralnu poziciju
        digitalWrite(ledUbrzano, LOW); // LED D8 se gasi
        Firebase.setInt(firebaseData, "/garage/status", 2);
    }
} else {
    Serial.println("Greška pri čitanju Firebase podataka: " + firebaseData.errorReason());
}
    // Pozivamo funkciju za brzo treperenje LED diode SAMO kad je servo motor u pokretu
    if (digitalRead(ledUbrzano) == HIGH) {
        blinkFast(); // LED treperi dok je motor u pokretu
    }

    delay(100); // Malo usporiti ciklus, ali ne previše
}
