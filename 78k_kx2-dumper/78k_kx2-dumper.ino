#include <Arduino.h>
#include <SoftwareSerial.h>

#define UART_RX_PIN 0  // TO TxD6
#define UART_TX_PIN 1  // TO RxD6
SoftwareSerial _uart_serial(UART_RX_PIN, UART_TX_PIN);
#define UART_SERIAL (_uart_serial)
#define FLMD0_PIN 2
#define RESET_PIN 3  // (Inverted)
#define OCDA_PIN 4  // Not using these yet.
#define OCDB_PIN 5  // Not using these yet.
#define PWM_PIN 5  // TO EXCLK
#define POWER_PIN 7  // TO VDD (3.3V)

typedef enum {
  SUCCESS = 0,
  BAD_PARAM,
  BAD_FRAME,
  BAD_CKSUM,
  BAD_LEN,
  TOO_LONG,
  NO_DATA,
} comm_error_t;

const int bitTime = 8500; // Time for one bit in nanoseconds
char sbuf[257] = { 0 };
byte res[259];
size_t res_sz = 0;
byte block[1024] = { 0 };

void
setup ()
{
  Serial.begin(115200); // Initialize the serial monitor for debugging
  Serial.println("+++");

  if (r78kkx2_init())
  {
    r78kkx2_work();
  }
  r78kkx2_deinit();

  Serial.println("===");
}

void
loop ()
{
  // we don't do that here
}


/// Renesas 78K/Kx2 Flash Memory Programming Functions

void
r78kkx2_work (void)
{
  comm_error_t err = SUCCESS;
  byte stat = 0;

  // SILICON SIGNATURE COMMAND
  err = tx_command(0xC0, NULL, 0);
  if (!err) err = rx_status(&stat);
  if ((stat != 6) || err)
  {
    Serial.print("Bad SiSig: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return;
  }
  res_sz = sizeof(res);
  err = rx_data(res, &res_sz);
  if (err)
  {
    Serial.print("Bad SiSig data: ");
    err_print(err);
    return;
  }
  else
  {
    Serial.println("SiSig OK:");
    print_hexbuf(res, res_sz);
  }

  // CHECKSUM COMMAND
  byte cmd[6] = { 0 };
  cmd[0] = 0x00;
  cmd[1] = 0xEF;
  cmd[2] = 0x00;
  cmd[3] = 0x00;
  cmd[4] = 0xEF;
  cmd[5] = 0xff;
  err = tx_command(0xB0, cmd, sizeof(cmd));
  if (!err) err = rx_status(&stat);
  if (stat != 6 || err)
  {
    Serial.print("Bad Checksum status: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return;
  }
  //delayMicroseconds(0);
  //digitalWrite(POWER_PIN, false);
  //delayMicroseconds(10);
  //digitalWrite(POWER_PIN, true);
  //analogWriteFrequency(PWM_PIN, 800000);
  res_sz = sizeof(res);
  err = rx_data(res, &res_sz);
  if ((res_sz != 2) || err)
  {
    Serial.print("Bad Checksum result: ");
    if (err) err_print(err);
    else Serial.println(res_sz);
    return;
  }
  else
  {
    Serial.print("Checksum result: ");
    print_hexbuf(res, 2);
  }

  /*
  // BLOCK BLANK CHECK COMMAND
  int blknum = 0;
  for (blknum = 0; blknum < 64; blknum++)
  {
    cmd[0] = 0x00;
    cmd[1] = blknum * 4;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    cmd[4] = blknum * 4 + 3;
    cmd[5] = 0xff;
    err = tx_command(0x32, cmd, sizeof(cmd));
    delay(3);
    if (!err) err = rx_status(&stat);
    if (((stat != 6) && (stat != 0x1B)) || err)
    {
      Serial.print("Bad BlankChk status: ");
      if (err) err_print(err);
      else Serial.println(stat);
      return;
    }
  }
  //*/

  // VERIFY TOMFOOLERY
  // Now we're REALLY getting somewhere -
  // The MCU will verify exactly however many bytes you send it, as long as that's
  // a multiple of four, regardless of what range you told it in the command frame.
  // This does mean you have to start on a 256-byte alignment.
  // No observed exploitable timing within those 4 bytes, unfortunately.
  cmd[0] = 0x00;
  cmd[1] = 0xEF;
  cmd[2] = 0x00;
  cmd[3] = 0x00;
  cmd[4] = 0xEF;
  cmd[5] = 0xFF;
  Serial.println("---");
  for (int i = 0; i < 256; i++)
  {
    block[i] = 0xFF;
  }
  // block[0] = 0;
  // block[3] = 0;
  for (int i = 0; i < 32; i++)
  {
    // block[i] = 0x00;
  }
  err = tx_command(0x13, cmd, sizeof(cmd));
  if (!err) err = rx_status(&stat);
  if (stat != 6 || err)
  {
    Serial.print("Bad Verify status: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return;
  }
  delayMicroseconds(12);
  err = tx_data(block, 256, false);
  if (err)
  {
    Serial.print("Failed to transmit Verify data: ");
    err_print(err);
    return;
  }
  res_sz = sizeof(res);
  err = rx_data(res, &res_sz);
  if ((res[0] != 6) || (res_sz != 2) || err)
  {
    Serial.print("Bad Verify result: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return;
  }
  else
  {
    Serial.print("Verify result: ");
    Serial.println(res[1]);
  }
}

bool
r78kkx2_init (void)
{
  comm_error_t err = SUCCESS;
  byte stat = 0;

  pinMode(UART_RX_PIN, INPUT);
  pinMode(UART_TX_PIN, INPUT);
  pinMode(FLMD0_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);

  // bootleg ground pin for logic analyzer because my little header has only 8 pins
  pinMode(6, OUTPUT);
  digitalWrite(6, false);

  digitalWrite(POWER_PIN, false);
  digitalWrite(RESET_PIN, false);
  digitalWrite(FLMD0_PIN, false);
  digitalWrite(PWM_PIN, false);
  // delay(2000);  If it was previously powered on this delay should happen.
  // But, we power off at the end, so should be fine without.

  digitalWrite(POWER_PIN, true);
  delayMicroseconds(20000);

  digitalWrite(FLMD0_PIN, true);
  delayMicroseconds(20000);

  digitalWrite(RESET_PIN, true);
  analogWriteFrequency(PWM_PIN, 8000000);  // 8MHz
  analogWrite(PWM_PIN, 128); // Set to 50% duty cycle
  delayMicroseconds(15000);

  for (int i = 0; i < 3; i++){  // Three pulses = EXCLK mode
    delayMicroseconds(30);
    digitalWrite(FLMD0_PIN, false);
    delayMicroseconds(30);
    digitalWrite(FLMD0_PIN, true);
  }

  delayMicroseconds(70000);
  pinMode(UART_TX_PIN, OUTPUT);
  UART_SERIAL.begin(9600); // Tool port

  // Reset preamble
  UART_SERIAL.write('\0');
  UART_SERIAL.flush();
  delayMicroseconds(2000);
  UART_SERIAL.write('\0');
  UART_SERIAL.flush();
  delayMicroseconds(2000);
  UART_SERIAL.flush();

  // RESET COMMAND
  Serial.println("We begin:");
  if ((err = tx_command(0x00, NULL, 0)))
  {
    Serial.print("Failed to send Reset: ");
    err_print(err);
    return false;
  }
  err = rx_status(&stat);
  if ((stat != 6) || err)
  {
    Serial.print("Bad Reset: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return false;
  }

  // SET OSCILLATION FREQUENCY COMMAND
  delayMicroseconds(2000);
  if ((err = tx_command(0x90, (const byte[]){0x08, 0x00, 0x00, 0x04}, 4)))
  {
    Serial.print("Failed to send Set Fosc: ");
    err_print(err);
    return false;
  }
  UART_SERIAL.flush();
  UART_SERIAL.begin(115200); // Tool port changes rate
  err = rx_status(&stat);
  if ((stat != 6) || err)
  {
    Serial.print("Bad Set Fosc: ");
    if (err) err_print(err);
    else Serial.println(stat);
    return false;
  }

  Serial.println("Init OK");
  return true;
}

void
r78kkx2_deinit (void)
{
  digitalWrite(RESET_PIN, false);
  pinMode(UART_RX_PIN, INPUT);
  pinMode(UART_TX_PIN, INPUT);
  pinMode(FLMD0_PIN, INPUT);
  pinMode(PWM_PIN, INPUT);
  digitalWrite(POWER_PIN, false);
}


/// MISC UTIL

void
print_hexbuf (byte * buf, int buf_len)
{
  static char hexbuf[40];
  int i = 0;
  int j;

  while (i < buf_len)
  {
    j = 0;
    while ((i < buf_len) && (j < ((buf_len <= 12) ? 12 : 8)))
    {
      snprintf(&(hexbuf[j * 3]), sizeof(hexbuf) - (j * 3), "%02X ", buf[i]);
      i++;
      j++;
    }
    if (j > 0)
    {
      hexbuf[(j * 3) - 1] = '\0';
      Serial.println(hexbuf);
    }
  }
}


/// COMMS FUNCTIONS
// For after it's already initialized.

static byte dump_buf[280];
static size_t dump_sz = 0;

comm_error_t
rx_data (byte * out, size_t * out_len)
{
  byte stx, len_byte, sum, sum_actual, etx;
  size_t len, i, readlen = 0;

  if ((out == NULL) || (out_len == NULL))
  {
    Serial.println("rx_data error: BAD_PARAM");
    return BAD_PARAM;
  }

  readlen += UART_SERIAL.readBytes(&stx, 1);
  if (readlen != 1)
  {
    Serial.println("rx_data error: NO_DATA (STX)");
    return NO_DATA;
  }
  dump_buf[0] = stx;
  dump_sz = 1;
  if (stx != '\x02')
  {
    dump_sz += UART_SERIAL.readBytes(&dump_buf[1], 270);
    Serial.println("rx_data error: BAD_FRAME (STX)");
    print_hexbuf(dump_buf, dump_sz);
    return BAD_FRAME;
  }

  readlen += UART_SERIAL.readBytes(&len_byte, 1);
  if (readlen != 2)
  {
    Serial.println("rx_data error: NO_DATA (LEN)");
    print_hexbuf(dump_buf, dump_sz);
    return NO_DATA;
  }
  len = len_byte ? len_byte : 256;  // 0 means 256
  dump_buf[1] = len_byte;
  dump_sz = 2;
  if (len > *out_len)
  {
    dump_sz += UART_SERIAL.readBytes(&dump_buf[2], len + 2);
    Serial.println("rx_data error: TOO_LONG");
    print_hexbuf(dump_buf, dump_sz);
    return TOO_LONG;
  }

  *out_len = UART_SERIAL.readBytes(out, len);
  readlen += UART_SERIAL.readBytes(&sum, 1);
  readlen += UART_SERIAL.readBytes(&etx, 1);
  memcpy(&dump_buf[2], out, *out_len);
  dump_buf[*out_len + 2] = sum;
  dump_buf[*out_len + 3] = etx;
  dump_sz = *out_len + readlen;
  if (readlen != 4)
  {
    Serial.println("rx_data error: NO_DATA (SUM/ETX)");
    print_hexbuf(dump_buf, dump_sz);
    return NO_DATA;
  }

  if (*out_len != len)
  {
    Serial.println("rx_data error: BAD_LEN");
    print_hexbuf(dump_buf, dump_sz);
    return BAD_LEN;
  }

  if (etx != '\x03')
  {
    // we don't support receiving multi-frame messages
    Serial.println("rx_data error: BAD_FRAME (ETX)");
    print_hexbuf(dump_buf, dump_sz);
    return BAD_FRAME;
  }

  sum_actual = 0 - len_byte;
  for (i = 0; i < len; i++)
  {
    sum_actual -= out[i];
  }

  if (sum_actual != sum)
  {
    Serial.println("rx_data error: BAD_CKSUM");
    print_hexbuf(dump_buf, dump_sz);
    return BAD_CKSUM;
  }

  // Serial.println("rx_data SUCCESS");
  // print_hexbuf(dump_buf, dump_sz);
  dump_sz = 0;
  return SUCCESS;
}


comm_error_t
rx_status (byte * stat_out)
{
  size_t stat_len = 1;
  comm_error_t result;

  if (stat_out == NULL)
  {
    return BAD_PARAM;
  }

  result = rx_data(stat_out, &stat_len);
  if (result != SUCCESS)
  {
    return result;
  }
  if (stat_len != 1)
  {
    return BAD_LEN;
  }

  return SUCCESS;
}


comm_error_t
tx_command (byte cmd, const byte * data, size_t data_len)
{
  byte len_byte, sum;
  size_t i, out;

  if ((data == NULL) && (data_len > 0))
  {
    Serial.println("tx_command error: BAD_PARAM");
    return BAD_PARAM;
  }
  if (data_len > 255)
  {
    Serial.print("tx_command error: TOO_LONG (");
    Serial.print(data_len);
    Serial.println(")");
    return TOO_LONG;
  }
  if (data_len == 255)
  {
    len_byte = 0;
  }
  else
  {
    len_byte = data_len + 1;  // plus 1 for cmd
  }

  sum = 0 - len_byte;
  sum -= cmd;
  for (i = 0; i < data_len; i++)
  {
    sum -= data[i];
  }

  dump_buf[0] = '\x01';  // SOH
  dump_buf[1] = len_byte;
  dump_buf[2] = cmd;
  memcpy(&dump_buf[3], data, data_len);
  dump_buf[data_len + 3] = sum;
  dump_buf[data_len + 4] = '\x03';  // ETX
  dump_sz = data_len + 5;
  out = 0;
  for (i = 0; i < dump_sz; i++)
  {
    out += UART_SERIAL.write(dump_buf[i]);
    UART_SERIAL.flush();
    // flush because there's supposed to be a >=10us delay between bytes
  }

  if (out != dump_sz)
  {
    Serial.println("tx_command error: BAD_LEN");
    print_hexbuf(dump_buf, dump_sz);
    return BAD_LEN;
  }

  // Serial.println("tx_command SUCCESS");
  // print_hexbuf(dump_buf, dump_sz);
  dump_sz = 0;
  return SUCCESS;
}


comm_error_t
tx_data (const byte * data, size_t data_len, bool continues)
{
  byte len_byte, sum;
  size_t i, out, len_now, frame = 0;

  if ((data == NULL) && (data_len > 0))
  {
    Serial.println("tx_data error: BAD_PARAM");
    return BAD_PARAM;
  }

  while (data_len > 0)
  {
    if (data_len > 0xFFFFFF)
    {
      // Sanity check + 78K/Kx2 only has 24-bit address space
      Serial.print("tx_data error: TOO_LONG (");
      Serial.print(data_len);
      Serial.println(")");
      return TOO_LONG;
    }
    if (data_len > 256)
    {
      len_now = 256;
    }
    else
    {
      len_now = data_len;
    }
    data_len -= len_now;

    if (len_now == 256)
    {
      len_byte = 0;
    }
    else
    {
      len_byte = len_now;
    }
  
    sum = 0 - len_byte;
    for (i = 0; i < len_now; i++)
    {
      sum -= data[i];
    }
  
    dump_buf[0] = '\x02';  // STX
    dump_buf[1] = len_byte;
    memcpy(&dump_buf[2], data, len_now);
    dump_buf[len_now + 2] = sum;
    if ((data_len > 0) || (continues))
    {
      // More frames to follow
      dump_buf[len_now + 3] = '\x17';  // ETB
    }
    else
    {
      // Final (maybe only) frame
      dump_buf[len_now + 3] = '\x03';  // ETX
    }
    dump_sz = len_now + 4;
    out = 0;
    for (i = 0; i < dump_sz; i++)
    {
      out += UART_SERIAL.write(dump_buf[i]);
      UART_SERIAL.flush();
      // flush because there's supposed to be a >=10us delay between bytes
    }
  
    if (out != dump_sz)
    {
      Serial.println("tx_data error: BAD_LEN");
      print_hexbuf(dump_buf, dump_sz);
      return BAD_LEN;
    }

    /*
    if (data_len > 0)
    {
      Serial.print("tx_data SUCCESS frame ");
      Serial.print(frame);
      Serial.print(" remaining ");
      Serial.println(data_len);
    }
    else
    {
      Serial.println("tx_data SUCCESS");
    }
    print_hexbuf(dump_buf, dump_sz);
    */
    dump_sz = 0;
    frame++;
  }

  return SUCCESS;
}


void
err_print (comm_error_t err)
{
  switch (err)
  {
    case SUCCESS:
      Serial.println("SUCCESS");
      break;
    case BAD_PARAM:
      Serial.println("BAD_PARAM");
      break;
    case BAD_FRAME:
      Serial.println("BAD_FRAME");
      break;
    case BAD_CKSUM:
      Serial.println("BAD_CKSUM");
      break;
    case BAD_LEN:
      Serial.println("BAD_LEN");
      break;
    case TOO_LONG:
      Serial.println("TOO_LONG");
      break;
    case NO_DATA:
      Serial.println("NO_DATA");
      break;
    default:
      Serial.print("Unknown comm_error_t ");
      Serial.println((int)err);
      break;
  }
}
