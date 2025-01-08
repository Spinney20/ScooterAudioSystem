#include <SerialMP3.h>

// -----------------------------------
// PINURI & OBIECT MP3
// -----------------------------------
const int hallSensorPin = A0;  // Senzor Hall (analogic)
const int RX_PIN = 10;
const int TX_PIN = 11;
SerialMP3 mp3(RX_PIN, TX_PIN);

// -----------------------------------
// DURATE (ms) PENTRU MP3
// index 0 nefolosit (ca sa fie 1->001, 2->002 etc.)
// -----------------------------------
unsigned long trackDuration[] = {
  0,       // 0
  4000,    // 1: 001.mp3 (4s)      = START
  41000,   // 2: 002.mp3 (41s)     = IDLE
  56000,   // 3: 003.mp3 (56s)     = ACCEL
  6000,    // 4: 004.mp3 (nefolosit)
  18000,   // 5: 005.mp3 (nefolosit)
  51800    // 6: 006.mp3 (51,8s)   = DECEL+IDLE
};

// -----------------------------------
// STARI POSIBILE
// -----------------------------------
enum SoundState {
  STATE_START,
  STATE_IDLE,
  STATE_ACCEL,
  STATE_DECEL_IDLE
};

// Variabile de stare
SoundState currentState = STATE_START;

// Gestionare redare
unsigned long trackStartTime = 0; 
bool isPlaying = false; // devine true dupa mp3.play()

// Praguri pentru senzor
const int DECEL_THRESHOLD = 527;

// Citeste senzorul Hall
int readHallSensor() {
  return analogRead(hallSensorPin);
}

void setup() {
  Serial.begin(9600);
  mp3.init();
  mp3.setVolume(30); // volum global (0..30)
  // eu am pus maxim ca sa ajustez sunetu din boxa
}

void loop() {
  int sensorValue = readHallSensor();
  unsigned long now = millis();

  switch (currentState) {

    // -------------------------------------------------
    // 1) START = reda 001.mp3 (4s), apoi trece la IDLE
    // -------------------------------------------------
    case STATE_START:
      if (!isPlaying) {
        mp3.play(1);              // 001.mp3
        trackStartTime = now;
        isPlaying = true;
        Serial.println(">> START (001.mp3)");
      } else {
        // Verificam daca a trecut durata (4s)
        unsigned long elapsed = now - trackStartTime;
        if (elapsed >= trackDuration[1]) {
          // Trecem la IDLE
          currentState = STATE_IDLE;
          isPlaying = false;
        }
      }
      break;

    // -------------------------------------------
    // 2) IDLE = reda 002.mp3 (41s) in loop
    //     - sare la ACCEL daca sensorValue > 525
    // -------------------------------------------
    case STATE_IDLE:
      if (!isPlaying) {
        mp3.play(2); // 002.mp3
        trackStartTime = now;
        isPlaying = true;
        Serial.println(">> IDLE (002.mp3)");
      } else {
        // Daca s-a terminat (41s), il reluam instant (loop manual)
        unsigned long elapsed = now - trackStartTime;
        if (elapsed >= trackDuration[2]) {
          mp3.play(2); 
          trackStartTime = now;
          Serial.println(">> IDLE loop restart (002.mp3)");
        }
      }

      // Daca senzorul indica accelerare
      if (sensorValue > DECEL_THRESHOLD) {
        currentState = STATE_ACCEL;
        isPlaying = false;
        Serial.println(">> IDLE -> ACCEL");
      }
      break;

    // ----------------------------------------------------
    // 3) ACCEL = reda 003.mp3 (56s)
    //     - la final sau daca sensorValue <= 525 => DECEL_IDLE
    // ----------------------------------------------------
    case STATE_ACCEL:
      if (!isPlaying) {
        mp3.play(3); // 003.mp3
        trackStartTime = now;
        isPlaying = true;
        Serial.println(">> ACCEL (003.mp3)");
      } else {
        unsigned long elapsed = now - trackStartTime;
        // S-a terminat ACCEL?
        if (elapsed >= trackDuration[3]) {
          currentState = STATE_DECEL_IDLE;
          isPlaying = false;
          Serial.println(">> ACCEL done -> DECEL+IDLE (#6)");
        }
        // Sau sensorValue <= 525 => decelerare
        else if (sensorValue <= DECEL_THRESHOLD) {
          currentState = STATE_DECEL_IDLE;
          isPlaying = false;
          Serial.println(">> ACCEL interrupted -> DECEL+IDLE (#6)");
        }
      }
      break;

    // ----------------------------------------------------------------
    // 4) DECEL_IDLE = reda 006.mp3 (51,8s) care contine decelerarea + idle
    //    - FARA intreruperi manuale intre decel si idle, totul e un singur fisier.
    //    - Daca se termina complet, mergem la IDLE normal
    //    - Daca sensorValue > 525, intrerupem si revenim la ACCEL
    // ----------------------------------------------------------------
    case STATE_DECEL_IDLE:
      if (!isPlaying) {
        mp3.play(6); // 006.mp3 => decelerare + idle integrat ~51,8s
        trackStartTime = now;
        isPlaying = true;
        Serial.println(">> DECEL + IDLE (006.mp3)");
      } else {
        unsigned long elapsed = now - trackStartTime;
        if (elapsed >= trackDuration[6]) {
          // S-a terminat tot fisierul (decel + idle)
          currentState = STATE_IDLE; 
          isPlaying = false;
          Serial.println(">> DECEL+IDLE done -> IDLE normal");
        }

        // Daca user accelereaza din nou
        if (sensorValue > DECEL_THRESHOLD) {
          // Oprim track 006 si trecem la ACCEL
          mp3.stop();
          currentState = STATE_ACCEL;
          isPlaying = false;
          Serial.println(">> DECEL+IDLE interrupted -> ACCEL");
        }
      }
      break;
  }

  // Afisajul prin serial // debugging
  Serial.print("Sensor = "); Serial.print(sensorValue);
  Serial.print(" | State = "); Serial.println(currentState);
}
