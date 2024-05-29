#include <Arduino.h>
#include <SoftwareSerial.h>

#define UART_PIN 1
#define RESET_PIN 3
#define PWM_PIN 5
#define POWER_PIN 7

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

const int bitTime = 8500; // Time for one bit in nanoseconds

const uint8_t guesschart[256] = {
  0xFF, 0x00, 0x0A, 0x22, 0x30, 0xFE, 0x03, 0xE6,
  0x01, 0x02, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
  0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
  0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x23,
  0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
  0x2C, 0x2D, 0x2E, 0x2F, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
  0x3D, 0x3E, 0x3F, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4,
  0xE5, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
  0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
  0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
  0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
  0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
  0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
  0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF
};


void setup() {
  Serial.begin(115200); // Initialize the serial monitor for debugging
  Serial.println("ready");
  while (!Serial.available());
  Serial.flush();

  pinMode(UART_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);

  digitalWrite(POWER_PIN, false);
  delayMicroseconds(20000);

  digitalWrite(UART_PIN, true);
  digitalWrite(RESET_PIN, false);
  digitalWrite(PWM_PIN, true);

  delayMicroseconds(1000);
  digitalWrite(POWER_PIN, true);

  delayMicroseconds(10000);
  digitalWrite(PWM_PIN, false);
  delayMicroseconds(10);
  digitalWrite(PWM_PIN, true);

  for (int i = 0; i < 5; i++){
    delayMicroseconds(10);
    digitalWrite(UART_PIN, false);
    delayMicroseconds(10);
    digitalWrite(UART_PIN, true);
  }

  delayMicroseconds(10);
  digitalWrite(RESET_PIN, true);
  delayMicroseconds(2000);

  pinMode(UART_PIN, INPUT_PULLUP);

  analogWriteFrequency(PWM_PIN, 8000000);
  analogWrite(PWM_PIN, 128); // Set to 50% duty cycle

/*
  //Serial.println("# Erasing chip...");
  //flashChipErase();
  //verifyChipErase();
  delay(1);
  for (int b = 0; b < 16; b++) {
    //programPatternPage(b);
    //programZeroesPage(b, 255);
    //delay(1);
  }
*/
  Serial.println("init");
  if (!programSecurityByte(0xCC))
  {
    Serial.println("SECURITY BIT SET FAILED");
    while (true) delay(10);
  }
  delay(10);
  Serial.println("# OK!");
  Serial.flush();

  unsigned char data[256] = { 0 };
  char buf[100];

  /*
  Serial.println("## Unadulterated checksum:");
  for (int p = 0; p < 256; p++)
  {
    printf("# Of pages [0, %d]:\r\n", p);
    printf("0x%04X\r\n", readChecksum(p, 0xFF));
    printf("0x%04X\r\n", readChecksum(p, 0xFF));
    printf("0x%04X\r\n", readChecksum(p, 0xFF));
  }
  */

  bool ok;
  int b, a, d;
  for (b = 0; b < 16; b++) {
    for (a = 0; a < 256; a++) {
      for (d = 0; d < 256; d++) {
        data[a] = guesschart[d];
        ok = programDataByte(b, a, data[a]);
        delayMicroseconds(1);
        if (ok) {
          snprintf(buf, sizeof(buf), "0x%02X%02X: 0x%02X", b, a, data[a]);
          Serial.println(buf);
          Serial.flush();
          break;
        }
      }
      if (d == 256)
      {
        Serial.println("Failed to find correct byte!?");
        snprintf(buf, sizeof(buf), "(For 0x%02X%02X)", b, a);
        Serial.println(buf);
        break;
      }
    }
  }
  Serial.println("~~~~~~~~~~~~");
  for (b = 128; b < 130; b++) {
    for (a = 0; a < 256; a++) {
      for (d = 0; d < 256; d++) {
        data[a] = guesschart[d];
        ok = programDataByte(b, a, data[a]);
        delayMicroseconds(1);
        if (ok) {
          snprintf(buf, sizeof(buf), "0x%02X%02X: 0x%02X", b, a, data[a]);
          Serial.println(buf);
          Serial.flush();
          break;
        }
      }
      if (d == 256)
      {
        Serial.println("Failed to find correct byte!?");
        snprintf(buf, sizeof(buf), "(For 0x%02X%02X)", b, a);
        Serial.println(buf);
        break;
      }
    }
  }

/*
  Serial.println("## Unadulterated checksum:");
    for (int p = 0; p < 16; p++)
    {
      printf("# Of pages [0, %d]:\n", p);
      printf("0x%04X\n", readChecksum(p, 0xFF));
      printf("0x%04X\n", readChecksum(p, 0xFF));
      printf("0x%04X\n", readChecksum(p, 0xFF));
    }
    */

/*
  for (int b = 0; b < 1; b++) {
    for (int i = 0; i < 2; i++) {
      programZeroesPage(b, i);
      delay(1);
      printf("## Checksum after setting %d x %d bytes to zero:\n", b, i);
      for (int p = 0; p < 16; p++)
      {
        printf("# Of pages [0, %d]:\n", p);
        printf("0x%04X\n", readChecksum(p, 0xFF));
        printf("0x%04X\n", readChecksum(p, 0xFF));
        printf("0x%04X\n", readChecksum(p, 0xFF));
      }
    }
  }


  Serial.println("# Full checksum:");
  printf("0x%04X\n", readChecksum(15, 0xFF));
  printf("0x%04X\n", readChecksum(15, 0xFF));
  printf("0x%04X\n", readChecksum(15, 0xFF));

  Serial.println("# Security checksum?");
  printf("0x%04X\n", readChecksum(0x80, 0));
  printf("0x%04X\n", readChecksum(0x80, 0));
  printf("0x%04X\n", readChecksum(0x80, 0));
  */

  Serial.println("# DONE!");

  delay(100);
  // CPU_RESTART
}

void loop() {
  // readChecksum(0, 0xFF);
}


void flashChipErase(void){
  unsigned char ack1, ack2;

  sendByte(0x20);
  sendByte(0x0F);
  sendByte(0x00);
  sendByte(0xFF);

  ack1 = receiveByte();
  ack2 = receiveByte();

  if (ack1 != 6) Serial.println("NACK1");
  if (ack2 != 6) Serial.println("NACK2");
}


void verifyChipErase(void){
  unsigned char ack1, ack2;

  sendByte(0x30);
  sendByte(0x0F);
  sendByte(0x00);
  sendByte(0xFF);

  ack1 = receiveByte();
  ack2 = receiveByte();

  if (ack1 != 6) Serial.println("NACK1");
  if (ack2 != 6) Serial.println("NACK2");
}


void programPatternPage(unsigned char block) {
  unsigned char ack;

  sendByte(0x40);
  sendByte(block);
  sendByte(0x00);
  sendByte(0xFF);

  ack = receiveByte();
  if (ack != 6) Serial.println("NACK1");

  for (int i = 0; i < 256; i++)
  {
    sendByte(i);
    ack = receiveByte();
    if (ack != 6){
      Serial.println("NACK2");
      return;
    }
  }
}


void programZeroesPage(unsigned char block, unsigned char maxAddr) {
  unsigned char ack;

  sendByte(0x40);
  sendByte(block);
  sendByte(0x00);
  sendByte(maxAddr);

  ack = receiveByte();
  if (ack != 6) Serial.println("NACK1");

  for (int i = 0; i <= maxAddr; i++)
  {
    sendByte(0);
    ack = receiveByte();
    if (ack != 6){
      Serial.println("NACK2");
      return;
    }
  }
}


bool programSecurityByte(unsigned char sec) {
  unsigned char ack;

  sendByte(0x40);
  sendByte(0x80);
  sendByte(0x00);
  sendByte(0x00);

  ack = receiveByte();
  if (ack != 6) {
    Serial.println("NACK1");
    return false;
  }

  sendByte(sec);
  ack = receiveByte();
  if (ack != 6){
    Serial.println("NACK2");
    return false;
  }

  ack = receiveByte();
  if (ack != 6) {
    Serial.println("NACK3");
    return false;
  }

  return true;
}


bool programDataPage(unsigned char block, unsigned char maxAddr, unsigned char * data) {
  unsigned char ack;

  sendByte(0x40);
  sendByte(block);
  sendByte(0x00);
  sendByte(maxAddr);

  ack = receiveByte();
  if (ack != 6) {
    Serial.println("NACK1");
    return false;
  }

  for (int i = 0; i <= maxAddr; i++)
  {
    sendByte(data[i]);
    ack = receiveByte();
    if (ack != 6){
      // Serial.println("NACK2");
      return false;
    }
  }

  ack = receiveByte();
  if (ack != 6) {
    // Serial.println("NACK3");
    return false;
  }

  return true;
}


bool programDataByte(unsigned char block, unsigned char addr, unsigned char data) {
  unsigned char ack;

  sendByte(0x40);
  sendByte(block);
  sendByte(addr);
  sendByte(addr);

  ack = receiveByte();
  if (ack != 6) {
    Serial.println("NACK1");
    return false;
  }

  sendByte(data);
  ack = receiveByte();
  if (ack != 6){
    // Serial.println("NACK2");
    return false;
  }

  ack = receiveByte();
  if (ack != 6) {
    // Serial.println("NACK3");
    return false;
  }

  return true;
}


unsigned short readChecksum(unsigned char block, unsigned char lastAddr) {
  unsigned char ack, sum1, sum2;
  sendByte(0xB0);
  sendByte(block);
  sendByte(0x00);
  sendByte(lastAddr);

  // delayMicroseconds(868);
  // pinMode(PWM_PIN, INPUT);
  // delayMicroseconds(380);
  // pinMode(PWM_PIN, OUTPUT);
  // analogWrite(PWM_PIN, 128); // Set to 50% duty cycle

  ack = receiveByte();
  sum1 = receiveByte();
  sum2 = receiveByte();

  if (ack != 6) Serial.println("NACK!");

  return (unsigned short)sum1 | ((unsigned short)sum2 << 8);
}

void sendByte(uint8_t data) {
  uint8_t parity = calculateEvenParity(data);
  noInterrupts(); // Disable interrupts for accurate timing
  pinMode(UART_PIN, OUTPUT); // Set the pin as output

  // Start bit
  digitalWrite(UART_PIN, LOW);
  delayNanoseconds(bitTime);

  // Data bits
  for (int i = 0; i < 8; i++) {
    digitalWrite(UART_PIN, (data >> i) & 0x01);
    delayNanoseconds(bitTime);
  }

  // Parity bit
  digitalWrite(UART_PIN, parity);
  delayNanoseconds(bitTime);

  // Stop bit
  digitalWrite(UART_PIN, HIGH);
  delayNanoseconds(bitTime);

  pinMode(UART_PIN, INPUT_PULLUP); // Set the pin back to input
  interrupts(); // Re-enable interrupts
}

uint8_t calculateEvenParity(uint8_t byte) {
  uint8_t parity = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (byte & (1 << i)) {
      parity = !parity;
    }
  }
  return parity;
}

uint8_t receiveByte() {
  uint8_t value = 0;
  uint8_t parity = 0;

  noInterrupts(); // Disable interrupts for accurate timing

  // Wait for start bit
  while (digitalRead(UART_PIN)) {
    // Idle loop until line goes low
  }

  // Delay to the middle of the start bit
  delayNanoseconds(bitTime / 2);

  // Delay until start of first data bit
  delayNanoseconds(bitTime);

  // Read data bits
  for (int i = 0; i < 8; i++) {
    value |= digitalRead(UART_PIN) << i;
    delayNanoseconds(bitTime);
  }

  // Read parity bit
  parity = digitalRead(UART_PIN);
  delayNanoseconds(bitTime);

  // Check parity
  if (parity != calculateEvenParity(value)) {
    // Handle parity error
    Serial.println("bad parity");
    // (i guess we don't handle it)
  }

  // Wait for stop bit
  delayNanoseconds(bitTime / 2);

  interrupts(); // Re-enable interrupts

  return value;
}
