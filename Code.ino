// =======================================================================
// ===== KREDENSIAL BLYNK & WIFI =====
// =======================================================================
#define BLYNK_TEMPLATE_ID "TMPL6SZ7bn_H3"
#define BLYNK_TEMPLATE_NAME "Quadruped Bot"
#define BLYNK_AUTH_TOKEN "M7eCe1F62cMe96_puYorHIKBz6bp-uyZ"

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "RR";
char pass[] = "wlanad1e27";
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <U8g2lib.h>

#define OLED_SDA 18
#define OLED_SCL 19
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ U8X8_PIN_NONE);

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();

// ===== PENGATURAN SERVO =====
#define SERVOMIN 150
#define SERVOMAX 600

// ===== PENGATURAN PIN LED =====
int led1 = 2; // GPIO2
int led2 = 4; // GPIO4
bool ledAktif = false;

// Variabel untuk State Machine (Non-blocking)
unsigned long prevMillisLED1 = 0, prevMillisLED2 = 0;
int stepLED1 = 0, stepLED2 = 0;

unsigned long prevMillisRobot = 0;
int robotStep = 0;
char currentCmd = ' ';
char lastCmd = ' ';

// Variabel untuk tracking tampilan OLED
char lastDisplayedCmd = '\0';
unsigned long prevMillisOLED = 0;
int oledAnimFrame = 0; // Frame untuk animasi OLED

#include "idle.h"
// #include <tidur.ino>
#include "saatGerak.h"
// #include <wlee.ino>

// ===== FUNGSI BANTU =====
int angleToPulse(int ang) {
    return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}

void updateOLEDDisplay() {
    unsigned long now = millis();
    // Update animasi setiap 50ms (sesuai dengan delay di kode asli)
    if (now - prevMillisOLED >= 1) {
        prevMillisOLED = now;
        
        // Reset frame jika command berubah
        if (currentCmd != lastDisplayedCmd) {
            lastDisplayedCmd = currentCmd;
            oledAnimFrame = 0;
        }
        
        u8g2.clearBuffer();
        
        switch(currentCmd) {
            case ' ': // Standby/Idle
                tampilkanIdleFrame(oledAnimFrame);
                break;
                
            case 'W': // Maju
            case 'S': // Mundur
            case 'A': // Belok Kiri
            case 'D': // Belok Kanan
                tampilkanSaatGerakFrame(oledAnimFrame);
                break;
                
            // case 'Q': // Tidur
            //     tampilkanTidurFrame(oledAnimFrame);
            //     break;
                
            // case 'E': // Duduk
            //     tampilkanWleeFrame(oledAnimFrame);
            //     break;
                
            default:
                tampilkanIdleFrame(oledAnimFrame);
                break;
        }
        
        u8g2.sendBuffer();
        
        // Increment frame untuk animasi berikutnya
        oledAnimFrame++;
    }
}

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
     // Inisialisasi OLED
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Quadruped Bot");
    u8g2.drawStr(0, 30, "Starting...");
    u8g2.sendBuffer();
    delay(2000);

    Blynk.begin(auth, ssid, pass);

    pca.begin();
    pca.setPWMFreq(50);
    delay(10);

    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);

    currentCmd = ' '; // 'Spasi' untuk Standby
    Serial.println("Robot siap!");
}

// =============================================================
// ===== KONTROL DARI APLIKASI BLYNK (MODEL TOMBOL TERPISAH) =====
// =============================================================
BLYNK_WRITE(V0) { // Tombol Maju
  if (param.asInt() == 1) { currentCmd = 'W'; } else { currentCmd = ' '; }
}

BLYNK_WRITE(V1) { // Tombol Kiri
  if (param.asInt() == 1) { currentCmd = 'A'; } else { currentCmd = ' '; }
}

BLYNK_WRITE(V2) { // Tombol Kanan
  if (param.asInt() == 1) { currentCmd = 'D'; } else { currentCmd = ' '; }
}

BLYNK_WRITE(V6) { // Tombol mundur
  if (param.asInt() == 1) { currentCmd = 'S'; } else { currentCmd = ' '; }
}

BLYNK_WRITE(V3) { // Tombol LED ON
  if (param.asInt() == 1) {
    ledAktif = true;
    Blynk.virtualWrite(V4, 0); // Matikan tombol V4 di app
  }
}

BLYNK_WRITE(V4) { // Tombol LED OFF
  if (param.asInt() == 1) {
    ledAktif = false;
    Blynk.virtualWrite(V3, 0); // Matikan tombol V3 di app
  }
}

BLYNK_WRITE(V5) { // Tombol eshait
  if (param.asInt() == 1) { currentCmd = 'E'; } else { currentCmd = ' '; }
}
BLYNK_WRITE(V7) { // Sleepy nigga
  if (param.asInt() == 1) { currentCmd = 'Q'; } else { currentCmd = ' '; }
}


// ===== KONTROL DARI SERIAL MONITOR (CMD) =====
void checkSerialCommands() {
  if (Serial.available() > 0) {
    char c = toupper(Serial.read());
    if (c == 'W' || c == 'A' || c == 'D' || c == ' ' || c == 'E' || c == 'S' || c == 'Q') {
        currentCmd = c;
        Serial.print("Perintah Gerak: "); Serial.println(c);
    } else if (c == '1') {
        ledAktif = true;
        Blynk.virtualWrite(V3, 1); Blynk.virtualWrite(V4, 0);
        Serial.println("Perintah: LED ON");
    } else if (c == '2') {
        ledAktif = false;
        Blynk.virtualWrite(V3, 0); Blynk.virtualWrite(V4, 1);
        Serial.println("Perintah: LED OFF");
    }
  }
}

// ===== FUNGSI UPDATE NON-BLOCKING =====
void updateLED() {
    if (!ledAktif) {
        digitalWrite(led1, LOW);
        digitalWrite(led2, LOW);
        return;
    }
    unsigned long now = millis();
    switch (stepLED1) {
        case 0: digitalWrite(led1, HIGH); if (now - prevMillisLED1 >= 50) { prevMillisLED1 = now; stepLED1 = 1; } break;
        case 1: digitalWrite(led1, LOW);  if (now - prevMillisLED1 >= 50) { prevMillisLED1 = now; stepLED1 = 2; } break;
        case 2: digitalWrite(led1, HIGH); if (now - prevMillisLED1 >= 50) { prevMillisLED1 = now; stepLED1 = 3; } break;
        case 3: digitalWrite(led1, LOW);  if (now - prevMillisLED1 >= 50) { prevMillisLED1 = now; stepLED1 = 4; } break;
        case 4: if (now - prevMillisLED1 >= 500) { prevMillisLED1 = now; stepLED1 = 0; } break;
    }
    switch (stepLED2) {
        case 0: digitalWrite(led2, HIGH); if (now - prevMillisLED2 >= 50) { prevMillisLED2 = now; stepLED2 = 1; } break;
        case 1: digitalWrite(led2, LOW);  if (now - prevMillisLED2 >= 50) { prevMillisLED2 = now; stepLED2 = 2; } break;
        case 2: digitalWrite(led2, HIGH); if (now - prevMillisLED2 >= 50) { prevMillisLED2 = now; stepLED2 = 3; } break;
        case 3: digitalWrite(led2, LOW);  if (now - prevMillisLED2 >= 50) { prevMillisLED2 = now; stepLED2 = 4; } break;
        case 4: if (now - prevMillisLED2 >= 700) { prevMillisLED2 = now; stepLED2 = 0; } break;
    }
}

void updateRobotMovement() {
    unsigned long now = millis();
    if (currentCmd != lastCmd) {
        robotStep = 0;
        lastCmd = currentCmd;
    }
    
    // ===== MUNDUR STATE MACHINE =====
    else if (currentCmd == 'S') {
        switch (robotStep) {
            case 0: // PERSIS seperti kode original - Step 1
                if (now - prevMillisRobot >= 250) { // Tambahkan delay 500ms pada case 0
                    pca.setPWM(4, 0, angleToPulse(120));
                    pca.setPWM(5, 0, angleToPulse(60));
                    pca.setPWM(2, 0, angleToPulse(90)); 
                    pca.setPWM(3, 0, angleToPulse(160));
                    prevMillisRobot = now; robotStep = 1;
                }
                break;
                
            case 1: // PERSIS seperti kode original - Step 2
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(6, 0, angleToPulse(45)); 
                    pca.setPWM(7, 0, angleToPulse(130));
                    pca.setPWM(0, 0, angleToPulse(157)); 
                    pca.setPWM(1, 0, angleToPulse(30));
                    prevMillisRobot = now; robotStep = 2;
                } 
                break;
                
            case 2: // PERSIS seperti kode original - Step 3a
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(2, 0, angleToPulse(30));
                    pca.setPWM(3, 0, angleToPulse(90));
                    pca.setPWM(4, 0, angleToPulse(150)); 
                    pca.setPWM(5, 0, angleToPulse(90));
                    prevMillisRobot = now; robotStep = 3;
                } 
                break;
                
            case 3: // PERSIS seperti kode original - Step 3b 
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(0, 0, angleToPulse(100));  
                    pca.setPWM(1, 0, angleToPulse(20));
                    pca.setPWM(6, 0, angleToPulse(60));  
                    pca.setPWM(7, 0, angleToPulse(120));
                    prevMillisRobot = now; robotStep = 4;
                } 
                break;
                
            case 4: // PERSIS seperti kode original - Step 4
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(4, 0, angleToPulse(130));	
                    pca.setPWM(5, 0, angleToPulse(45));
                    pca.setPWM(2, 0, angleToPulse(40));
                    pca.setPWM(3, 0, angleToPulse(150));
                    prevMillisRobot = now; robotStep = 5;
                } 
                break;
                
            case 5: // PERSIS seperti kode original - Step 5
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(0, 0, angleToPulse(160)); 
                    pca.setPWM(1, 0, angleToPulse(90));
                    pca.setPWM(6, 0, angleToPulse(30));
                    pca.setPWM(7, 0, angleToPulse(90));
                    prevMillisRobot = now; robotStep = 0; // Kembali ke step 0 untuk siklus berikutnya
                } 
                break;
        }
    }

    // ===== STANDBY STATE MACHINE =====
    if (currentCmd == ' ') {
        if (robotStep == 0) {
            pca.setPWM(0,0,angleToPulse(120)); pca.setPWM(1,0,angleToPulse(60));
            pca.setPWM(2,0,angleToPulse(60));  pca.setPWM(3,0,angleToPulse(120));
            pca.setPWM(4,0,angleToPulse(120)); pca.setPWM(5,0,angleToPulse(60));
            pca.setPWM(6,0,angleToPulse(60));  pca.setPWM(7,0,angleToPulse(120));
            robotStep = 1;
        }
    }

    // ===== SECOND MODE (DUDUK) =====
    else if (currentCmd == 'E') {
      if (robotStep == 0) {
          pca.setPWM(0, 0, angleToPulse(120)); pca.setPWM(1, 0, angleToPulse(60));
          pca.setPWM(2, 0, angleToPulse(60)); pca.setPWM(3, 0, angleToPulse(120));
          pca.setPWM(4, 0, angleToPulse(155)); pca.setPWM(5, 0, angleToPulse(20));
          pca.setPWM(6, 0, angleToPulse(20)); pca.setPWM(7, 0, angleToPulse(155));
          robotStep = 1;
      }
    }
    
    // ===== MAJU STATE MACHINE =====
    else if (currentCmd == 'W') {
        switch (robotStep) {
            case 0: // PERSIS seperti kode original - Step 1
                if (now - prevMillisRobot >= 250) { // Tambahkan delay 500ms pada case 0
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(0, 0, angleToPulse(160)); // kanan depan keatas
                    pca.setPWM(1, 0, angleToPulse(30));
                    pca.setPWM(6, 0, angleToPulse(45));  // kiri belakang keatas  
                    pca.setPWM(7, 0, angleToPulse(130));
                    prevMillisRobot = now; robotStep = 1;
                }
                break;
                
            case 1: // PERSIS seperti kode original - Step 2
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(0, 0, angleToPulse(160)); // kanan depan kedepan
                    pca.setPWM(1, 0, angleToPulse(90));
                    pca.setPWM(2, 0, angleToPulse(105)); // kiri depan kebelakang (dorongan)
                    pca.setPWM(3, 0, angleToPulse(150));
                    pca.setPWM(4, 0, angleToPulse(90));  // kanan belakang kebelakang (dorongan)
                    pca.setPWM(5, 0, angleToPulse(45));
                    prevMillisRobot = now; robotStep = 2;
                } 
                break;
                
            case 2: // PERSIS seperti kode original - Step 3a
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(4, 0, angleToPulse(145));
                    pca.setPWM(5, 0, angleToPulse(45));
                    pca.setPWM(6, 0, angleToPulse(30));  // kiri belakang kedepan
                    pca.setPWM(7, 0, angleToPulse(90));
                    prevMillisRobot = now; robotStep = 3;
                } 
                break;
                
            case 3: // PERSIS seperti kode original - Step 3b 
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(2, 0, angleToPulse(30)); // kiri depan keatas
                    pca.setPWM(3, 0, angleToPulse(160));
                    prevMillisRobot = now; robotStep = 4;
                } 
                break;
                
            case 4: // PERSIS seperti kode original - Step 4
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(0, 0, angleToPulse(90));  // kanan depan kebelakang
                    pca.setPWM(1, 0, angleToPulse(30));
                    pca.setPWM(2, 0, angleToPulse(30));  // kiri depan kedepan
                    pca.setPWM(3, 0, angleToPulse(90));
                    pca.setPWM(6, 0, angleToPulse(90));  // kiri belakang kebelakang
                    pca.setPWM(7, 0, angleToPulse(160));
                    prevMillisRobot = now; robotStep = 5;
                } 
                break;
                
            case 5: // PERSIS seperti kode original - Step 5
                if (now - prevMillisRobot >= 250) {
                    // Semua servo bergerak bersamaan seperti delay version
                    pca.setPWM(4, 0, angleToPulse(150)); // kanan belakang kedepan
                    pca.setPWM(5, 0, angleToPulse(90));
                    prevMillisRobot = now; robotStep = 0; // Kembali ke step 0 untuk siklus berikutnya
                } 
                break;
        }
    }

    // ===== BELOK KANAN STATE MACHINE =====
    else if (currentCmd == 'D') {
        switch (robotStep) {
            case 0: // Step 1: Stand
                pca.setPWM(0,0,angleToPulse(120)); pca.setPWM(1,0,angleToPulse(60));
                pca.setPWM(4,0,angleToPulse(120)); pca.setPWM(5,0,angleToPulse(60));
                pca.setPWM(6,0,angleToPulse(60)); pca.setPWM(7,0,angleToPulse(120));
                prevMillisRobot = now; robotStep = 1;
                break;
            case 1: // Step 2: Kaki kanan atas
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(2,0,angleToPulse(40)); pca.setPWM(3,0,angleToPulse(150));
                    prevMillisRobot = now; robotStep = 2;
                }
                break;
            case 2: // Step 3: Kaki kanan depan
                if (now - prevMillisRobot >= 500) {
                    pca.setPWM(2,0,angleToPulse(30)); pca.setPWM(3,0,angleToPulse(65));
                    prevMillisRobot = now; robotStep = 3;
                }
                break;
            case 3: // Step 4: Badan geser
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(0,0,angleToPulse(150)); pca.setPWM(1,0,angleToPulse(30));
                    pca.setPWM(6,0,angleToPulse(50)); pca.setPWM(7,0,angleToPulse(130));
                    prevMillisRobot = now; robotStep = 4;
                }
                break;
            case 4: // Step 5: Belakang
                if (now - prevMillisRobot >= 500) {
                    pca.setPWM(2,0,angleToPulse(100)); pca.setPWM(3,0,angleToPulse(150));
                    pca.setPWM(4,0,angleToPulse(155)); pca.setPWM(5,0,angleToPulse(115));
                    pca.setPWM(6,0,angleToPulse(110)); pca.setPWM(7,0,angleToPulse(160));
                    prevMillisRobot = now; robotStep = 5;
                }
                break;
            case 5: // Step 6: Kembali ke posisi awal
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(0,0,angleToPulse(120)); pca.setPWM(1,0,angleToPulse(60));
                    prevMillisRobot = now; robotStep = 0; // Kembali ke awal siklus belok
                }
                break;
        }
    }
    
    // ===== BELOK KIRI STATE MACHINE =====
    else if (currentCmd == 'A') {
        switch (robotStep) {
            case 0: // Step 1: Stand
                pca.setPWM(2,0,angleToPulse(60)); pca.setPWM(3,0,angleToPulse(120));
                pca.setPWM(6,0,angleToPulse(60)); pca.setPWM(7,0,angleToPulse(125));
                pca.setPWM(4,0,angleToPulse(120)); pca.setPWM(5,0,angleToPulse(60));
                prevMillisRobot = now; robotStep = 1;
                break;
            case 1: // Step 2: Kaki kiri atas
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(0,0,angleToPulse(155)); pca.setPWM(1,0,angleToPulse(20));
                    prevMillisRobot = now; robotStep = 2;
                }
                break;
            case 2: // Step 3: Kaki kiri depan
                if (now - prevMillisRobot >= 500) {
                    pca.setPWM(0,0,angleToPulse(160)); pca.setPWM(1,0,angleToPulse(90));
                    prevMillisRobot = now; robotStep = 3;
                }
                break;
            case 3: // Step 4: Badan geser
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(2,0,angleToPulse(40)); pca.setPWM(3,0,angleToPulse(150));
                    pca.setPWM(4,0,angleToPulse(130)); pca.setPWM(5,0,angleToPulse(45));
                    prevMillisRobot = now; robotStep = 4;
                }
                break;
            case 4: // Step 5: Belakang
                if (now - prevMillisRobot >= 500) {
                    pca.setPWM(0,0,angleToPulse(100)); pca.setPWM(1,0,angleToPulse(30));
                    pca.setPWM(4,0,angleToPulse(100)); pca.setPWM(5,0,angleToPulse(30));
                    pca.setPWM(6,0,angleToPulse(20)); pca.setPWM(7,0,angleToPulse(70));
                    prevMillisRobot = now; robotStep = 5;
                }
                break;
            case 5: // Step 6: Kembali ke posisi awal
                if (now - prevMillisRobot >= 250) {
                    pca.setPWM(2,0,angleToPulse(60)); pca.setPWM(3,0,angleToPulse(120));
                    prevMillisRobot = now; robotStep = 0; // Kembali ke awal siklus belok
                }
                break;
        }
    }

    // Tidur 
    else if (currentCmd == 'Q') {
      if (robotStep == 0) {
          pca.setPWM(0, 0, angleToPulse(160)); pca.setPWM(1, 0, angleToPulse(15));
          pca.setPWM(2, 0, angleToPulse(20)); pca.setPWM(3, 0, angleToPulse(155));
          pca.setPWM(4, 0, angleToPulse(160)); pca.setPWM(5, 0, angleToPulse(15));
          pca.setPWM(6, 0, angleToPulse(15)); pca.setPWM(7, 0, angleToPulse(160));
          robotStep = 1;
      }
    }
}

// ===== LOOP UTAMA =====
void loop() {
    Blynk.run();
    checkSerialCommands();
    updateLED();
    updateRobotMovement();
    updateOLEDDisplay();
}
