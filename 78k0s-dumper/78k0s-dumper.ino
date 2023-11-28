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

void setup() {
  Serial.begin(115200); // Initialize the serial monitor for debugging

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

  //Serial.println("# Erasing chip...");
  //flashChipErase();
  //verifyChipErase();
  delay(1);
  for (int b = 0; b < 16; b++) {
    //programPatternPage(b);
    //programZeroesPage(b, 255);
    delay(1);
  }
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

  Serial.println("## Unadulterated checksum:");
  for (int p = 0; p < 16; p++)
  {
    printf("# Of pages [0, %d]:\n", p);
    printf("0x%04X\n", readChecksum(p, 0xFF));
    printf("0x%04X\n", readChecksum(p, 0xFF));
    printf("0x%04X\n", readChecksum(p, 0xFF));
  }

  bool ok;
  int b, a, d;
  for (b = 0; b < 16; b++) {
    for (a = 0; a < 256; a++) {
      for (d = 0; d < 256; d++) {
        data[a] = d;
        ok = programDataByte(b, a, d);
        delayMicroseconds(1);
        if (ok) {
          snprintf(buf, sizeof(buf), "0x%02X%02X: 0x%02X", b, a, d);
          Serial.println(buf);
          Serial.flush();
          break;
        }
      }
      if (d == 256)
      {
        Serial.println("you're fucked");
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
    Serial.println("parity fucked");
  }

  // Wait for stop bit
  delayNanoseconds(bitTime / 2);

  interrupts(); // Re-enable interrupts

  return value;
}
