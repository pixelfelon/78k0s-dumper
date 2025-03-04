// ReSharper disable CppDFAUnreachableCode
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
#define POWER_PIN 6  // TO VDD (3.3V)

typedef enum {
	SUCCESS = 0,
	BAD_PARAM,
	BAD_FRAME,
	BAD_CKSUM,
	BAD_LEN,
	TOO_LONG,
	NO_DATA,
	MORE_FRAMES,  // not exactly an error, means ETB was received
} comm_error_t;


void r78kkx2_work (void);
void r78kkx2_a4_experiment (void);
bool r78kkx2_check_signature (void);
void r78kkx2_checksum_experiment (void);
void r78kkx2_verify_experiment (void);
void r78kkx2_program_experiment (void);
void r78kkx2_block_erase (void);
void r78kkx2_chip_erase (void);
void r78kkx2_test_cmd (uint8_t cmdb);
bool r78kkx2_init (void);
void r78kkx2_run (void);
void r78kkx2_deinit (void);
void print_hexbuf (const byte * buf, int buf_len);
comm_error_t rx_data (byte * out, size_t * out_len);
comm_error_t rx_status (byte * stat_out);
comm_error_t tx_command (byte cmd, const byte * data, size_t data_len);
comm_error_t tx_data (const byte * data, size_t data_len, bool continues);
void err_print (comm_error_t err);
void stat_print (uint8_t stat);


char sbuf[257] = { 0 };
byte res[259];
size_t res_sz = 0;
byte block[1024] = { 0 };

byte program[1024] = {
	0x90, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6F, 0x00, 0x00, 0x00,
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x61, 0xD0, 0xEE, 0x1C, 0xB8, 0xFE, 0x9A, 0x1F,
	0x02, 0x16, 0x1F, 0xFE, 0xA3, 0xC0, 0xA1, 0x00, 0xBB, 0x8B, 0xFD, 0x10,
	0x00, 0x00, 0x03, 0x54, 0xFB, 0x03, 0x40, 0xFB, 0x03, 0x44, 0xFB, 0x40,
	0x03, 0x42, 0xFB, 0x10, 0x58, 0xFB, 0x03, 0x56, 0xFB, 0x16, 0x85, 0x00,
	0x14, 0x78, 0xFB, 0xC6, 0xEA, 0x85, 0x00, 0xAD, 0x06, 0x87, 0x95, 0x86,
	0x84, 0xFA, 0xF4, 0x16, 0x78, 0xFB, 0xC6, 0xEA, 0x78, 0xFB, 0xAD, 0x06,
	0xA1, 0x00, 0x97, 0x86, 0xFA, 0xF4, 0x16, 0x85, 0x00, 0x14, 0x78, 0xFB,
	0xC6, 0xEA, 0x85, 0x00, 0xAD, 0x06, 0x87, 0x95, 0x86, 0x84, 0xFA, 0xF4,
	0x16, 0x78, 0xFB, 0xC6, 0xEA, 0x78, 0xFB, 0xAD, 0x06, 0xA1, 0x00, 0x97,
	0x86, 0xFA, 0xF4, 0x9A, 0x57, 0x02, 0x10, 0x00, 0x00, 0x9A, 0x20, 0x02,
	0xFA, 0xFE, 0xB1, 0x4E, 0xBB, 0xBD, 0x0F, 0x30, 0x4E, 0xBA, 0xBD, 0x0A,
	0xF0, 0xBD, 0x4E, 0xB9, 0xBD, 0x04, 0xF0, 0xBC, 0x4E, 0xB8, 0xB0, 0xAD,
	0x01, 0x01, 0xAF, 0xAF, 0x02, 0x40, 0xFB, 0x30, 0x61, 0x01, 0x30, 0x27,
	0xD6, 0x67, 0x61, 0x6E, 0xAD, 0x12, 0x96, 0x96, 0xB7, 0xC6, 0xCA, 0x00,
	0xFB, 0xD6, 0x87, 0x30, 0xAE, 0x01, 0xB6, 0x9A, 0x42, 0x02, 0xFA, 0xE9,
	0xFA, 0xFE, 0x31, 0x98, 0xB7, 0xB1, 0x89, 0x1C, 0xD6, 0xF4, 0x55, 0x4D,
	0x00, 0xAD, 0x02, 0xFA, 0xF8, 0x87, 0xF2, 0x0B, 0xB0, 0xB6, 0xAF, 0xB7,
	0x89, 0x1C, 0xDA, 0x0A, 0x00, 0x99, 0x1C, 0xD6, 0xA1, 0x00, 0xBE, 0x08,
	0xBE, 0x09, 0xBE, 0x06, 0xBE, 0x07, 0x13, 0x50, 0x00, 0x13, 0x56, 0x01,
	0x13, 0x57, 0x14, 0x13, 0x50, 0xC4, 0x13, 0x21, 0xF7, 0x11, 0x01, 0x08,
	0x13, 0xC2, 0x00, 0x13, 0xC0, 0xA5, 0x13, 0xC4, 0x09, 0x13, 0xC4, 0xF6,
	0x13, 0xC4, 0x09, 0xF4, 0xC2, 0x31, 0x0F, 0x14, 0x10, 0x4E, 0x00, 0x9A,
	0x44, 0x02, 0x10, 0x47, 0x00, 0x9A, 0x44, 0x02, 0x10, 0x20, 0x00, 0x9A,
	0x44, 0x02, 0xFA, 0xFE, 0xAE, 0x06, 0x30, 0xAE, 0x07, 0xD4, 0x85, 0xBE,
	0x05, 0x24, 0x24, 0x24, 0x24, 0x5D, 0x0F, 0xBE, 0x03, 0xAE, 0x05, 0x5D,
	0x0F, 0xBE, 0x04, 0xA1, 0x09, 0x49, 0x03, 0x9D, 0x0A, 0xAE, 0x03, 0xA0,
	0x00, 0x30, 0xCA, 0x37, 0x00, 0xFA, 0x08, 0xAE, 0x03, 0xA0, 0x00, 0x30,
	0xCA, 0x30, 0x00, 0x60, 0xBE, 0x01, 0xA1, 0x09, 0x49, 0x04, 0x9D, 0x0A,
	0xAE, 0x04, 0xA0, 0x00, 0x30, 0xCA, 0x37, 0x00, 0xFA, 0x08, 0xAE, 0x04,
	0xA0, 0x00, 0x30, 0xCA, 0x30, 0x00, 0x60, 0xBE, 0x02, 0xAE, 0x01, 0x70,
	0x27, 0x61, 0x31, 0x9A, 0x44, 0x02, 0xAE, 0x02, 0x70, 0x27, 0x61, 0x31,
	0x9A, 0x44, 0x02, 0x10, 0x20, 0x00, 0x9A, 0x44, 0x02, 0xAE, 0x06, 0x30,
	0xAE, 0x07, 0x80, 0xBE, 0x07, 0x30, 0xBE, 0x06, 0xAE, 0x08, 0x30, 0xAE,
	0x09, 0x80, 0xBE, 0x09, 0x30, 0xBE, 0x08, 0x30, 0xEA, 0x10, 0x00, 0x8D,
	0x12, 0x10, 0x0D, 0x00, 0x9A, 0x44, 0x02, 0x10, 0x0A, 0x00, 0x9A, 0x44,
	0x02, 0xA1, 0x00, 0xBE, 0x08, 0xBE, 0x09, 0xAE, 0x06, 0x30, 0xAE, 0x07,
	0x99, 0xB8, 0xEE, 0xBA, 0x00, 0x00, 0xEE, 0xBC, 0xFF, 0xFF, 0x10, 0x00,
	0x00, 0x9A, 0x06, 0x02, 0x8D, 0x06, 0xAD, 0x04, 0xBE, 0x06, 0xBE, 0x07,
	0x9B, 0xA4, 0x02, 0xC6, 0xCA, 0x0A, 0x00, 0x99, 0x1C, 0xB6, 0xAF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF
};


[[noreturn]] void
setup ()
{
	Serial.begin(115200); // Initialize the serial monitor for debugging
	Serial.println("+++");

	if (r78kkx2_init() && r78kkx2_check_signature())
	{
		r78kkx2_work();
	}

	Serial.println("===");

	r78kkx2_run();
	while (true)
	{
		int b = UART_SERIAL.read();
		if (b > 0)
		{
			Serial.print((char)b);
			digitalWrite(FLMD0_PIN, true);
		}
	}
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
	r78kkx2_chip_erase();
	// r78kkx2_a4_experiment();
	r78kkx2_program_experiment();
	r78kkx2_checksum_experiment();
	r78kkx2_checksum_experiment();
	// r78kkx2_block_erase();
	// r78kkx2_verify_experiment();
	return;
	for (int cmd = 0; cmd < 256; cmd++)
	{
		if (cmd == 0x00) continue;  // Reset
		if (cmd == 0x13) continue;  // Verify *
		if (cmd == 0x20) continue;  // Chip Erase
		// if (cmd == 0x22) continue;  // Block Erase
		// if (cmd == 0x32) continue;  // Block Blank Check
		if (cmd == 0x40) continue;  // Programming *
		// if (cmd == 0x70) continue;  // Status
		// if (cmd == 0x90) continue;  // Oscillating Frequency Set
		if (cmd == 0x9E) continue;  // ?? *
		if (cmd == 0xA0) continue;  // Security Set
		// if (cmd == 0xA4) continue;  // ??
		if (cmd == 0xB0) continue;  // Checksum
		// if (cmd == 0xC0) continue;  // Silicon Signature
		// if (cmd == 0xC5) continue;  // Version Get
		r78kkx2_test_cmd(cmd);
	}
}


void
r78kkx2_a4_experiment (void)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;
	byte cmd[255] = { 0 };

	for (unsigned int i = 0; i < sizeof(cmd); i++)
	{
		cmd[i] = 0xFF;
	}

	// UNKNOWN COMMAND 0xA4
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	Serial.println("---");
	err = tx_command(0xA4, cmd, 2);
	if (!err) err = rx_status(&stat);
	if (stat != 6 || err)
	{
		Serial.print("Bad A4 status: ");
		if (err) err_print(err);
		else stat_print(stat);
		return;
	}

	for (int i = 0; i < 1024; i++)
	{
		res_sz = sizeof(res);
		err = rx_data(res, &res_sz);
		if ((res_sz < 1) || ((err != SUCCESS) && (err != MORE_FRAMES)))
		{
			Serial.print("Bad A4 result: ");
			if (err) err_print(err);
			if (res_sz >= 1) stat_print(res[0]);
			else Serial.println("Too Short!");
			return;
		}
		else
		{
			if (i == 0) Serial.println("A4 result: ");
			print_hexbuf(res, res_sz);
		}

		if (err != MORE_FRAMES) break;
	}
}


bool
r78kkx2_check_signature (void)
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
		else stat_print(stat);
		return false;
	}
	res_sz = sizeof(res);
	err = rx_data(res, &res_sz);
	if (err)
	{
		Serial.print("Bad SiSig data: ");
		err_print(err);
		return false;
	}
	else
	{
		Serial.println("SiSig OK:");
		print_hexbuf(res, res_sz);
		return true;
	}
}


void
r78kkx2_checksum_experiment (void)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;
	byte cmd[255] = { 0 };

	for (unsigned int i = 0; i < sizeof(cmd); i++)
	{
		cmd[i] = 0xFF;
	}

	// CHECKSUM COMMAND
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x03;
	cmd[5] = 0xFF;
	Serial.println("---");
	err = tx_command(0xB0, cmd, 6);
	if (!err) err = rx_status(&stat);
	if (stat != 6 || err)
	{
		Serial.print("Bad Checksum status: ");
		if (err) err_print(err);
		else stat_print(stat);
		return;
	}
	//delayMicroseconds(0);
	//digitalWrite(POWER_PIN, false);
	//delayMicroseconds(10);
	//digitalWrite(POWER_PIN, true);
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
}


void
r78kkx2_verify_experiment (void)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;
	byte cmd[6] = { 0 };

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
		else stat_print(stat);
		return;
	}
	delayMicroseconds(12);
	err = tx_data(block, 4, false);
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
		else if (res_sz > 0) stat_print(res[0]);
		else Serial.println(res_sz);
		return;
	}
	else
	{
		Serial.print("Verify result: ");
		stat_print(res[1]);
	}
}


void
r78kkx2_program_experiment (void)
{
	for (int b = 0; b < 4; b++)
	{
		comm_error_t err = SUCCESS;
		byte stat = 0;
		byte cmd[6] = { 0 };

		cmd[0] = 0x00;  // SAH
		cmd[1] = b;  // SAM
		cmd[2] = 0x00;  // SAL
		cmd[3] = 0x00;  // EAH
		cmd[4] = b;  // EAM
		cmd[5] = 0xFF;  // EAL
		Serial.println("---");
		// for (int i = 0; i < 256; i++)
		// {
		// 	block[i] = 0xFF;
		// }
		err = tx_command(0x40, cmd, sizeof(cmd));
		if (!err) err = rx_status(&stat);
		if (stat != 6 || err)
		{
			Serial.print("Bad Program status: ");
			if (err) err_print(err);
			else stat_print(stat);
			return;
		}
		delayMicroseconds(12);
		err = tx_data(&program[b * 256], 256, false);
		if (err)
		{
			Serial.print("Failed to transmit Program data: ");
			err_print(err);
			return;
		}
		res_sz = sizeof(res);
		err = rx_data(res, &res_sz);
		if ((res_sz != 2) || (res[0] != 6) || (res[1] != 6) || err)
		{
			Serial.print("Bad Program status (ST2, ST1): ");
			if (err) err_print(err);
			else if (res_sz > 2) print_hexbuf(res, res_sz);
			else if (res_sz > 1) stat_print(res[1]);
			else if (res_sz > 0) stat_print(res[0]);
			else Serial.println("no data");
			return;
		}
		err = rx_status(&stat);
		if (stat != 6 || err)
		{
			Serial.print("Bad Program result: ");
			if (err) err_print(err);
			else stat_print(stat);
			return;
		}
		else
		{
			Serial.print("Program result: ");
			stat_print(res[1]);
		}
	}
}


void
r78kkx2_block_erase (void)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;
	byte cmd[6] = { 0 };

	cmd[0] = 0x00;  // SAH
	cmd[1] = 0xEC;  // SAM
	cmd[2] = 0x00;  // SAL
	cmd[3] = 0x00;  // EAH
	cmd[4] = 0xEF;  // EAM
	cmd[5] = 0xFF;  // EAL
	Serial.println("---");
	err = tx_command(0x22, cmd, sizeof(cmd));
	// delayMicroseconds(10000);
	if (!err) err = rx_status(&stat);
	if (stat != 6 || err)
	{
		Serial.print("Bad Block Erase status: ");
		if (err) err_print(err);
		else stat_print(stat);
	}
	else
	{
		Serial.println("Block Erase OK");
	}
}


void
r78kkx2_chip_erase (void)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;

	Serial.println("---");
	err = tx_command(0x20, NULL, 0);
	if (!err) err = rx_status(&stat);
	if (stat != 6 || err)
	{
		Serial.print("Bad Chip Erase status: ");
		if (err) err_print(err);
		else stat_print(stat);
	}
	else
	{
		Serial.println("Chip Erase OK");
	}
}


void
r78kkx2_test_cmd (uint8_t cmdb)
{
	comm_error_t err = SUCCESS;
	byte stat = 0;
	err = tx_command(cmdb, NULL, 0);
	res_sz = sizeof(res);
	if (!err) err = rx_data(res, &res_sz);
	Serial.print("CMD 0x");
	Serial.print(cmdb, HEX);
	Serial.print(" Status: ");
	stat = res[0];
	if (err)
	{
		err_print(err);
	}
	else
	{
		stat_print(stat);
		if (res_sz > 1) print_hexbuf(res, res_sz);
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
	UART_SERIAL.write((byte)'\0');
	UART_SERIAL.flush();
	delayMicroseconds(2000);
	UART_SERIAL.write((byte)'\0');
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
		else stat_print(stat);
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
#ifdef ARDUINO_TEENSY31
	UART_SERIAL.begin(100000); // who knows why
#else
	UART_SERIAL.begin(115200); // Tool port changes rate
#endif
	err = rx_status(&stat);
	if ((stat != 6) || err)
	{
		Serial.print("Bad Set Fosc: ");
		if (err) err_print(err);
		else stat_print(stat);
		return false;
	}

	Serial.println("Init OK");
	return true;
}

void
r78kkx2_run (void)
{
	digitalWrite(RESET_PIN, false);
	delay(10);
	digitalWrite(FLMD0_PIN, false);
	delay(10);
	digitalWrite(RESET_PIN, true);
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
print_hexbuf (const byte * buf, int buf_len)
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

	UART_SERIAL.setTimeout(5000);
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

	if ((etx != '\x03') && (etx != '\x17'))
	{
		Serial.println("rx_data error: BAD_FRAME (ETX/ETB)");
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
	if (etx == '\x17')
	{
		return MORE_FRAMES;
	}
	else
	{
		return SUCCESS;
	}
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
		// ReSharper disable CppDFANullDereference
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
		dump_sz = 0;
		frame++;
		data += len_now;
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
		Serial.println(err);
		break;
	}
}


void
stat_print (uint8_t stat)
{
	Serial.print("0x");
	Serial.print(stat, HEX);
	switch (stat)
	{
		case 0x04:
			Serial.print(": Command number error");
			break;
		case 0x05:
			Serial.print(": Parameter error");
			break;
		case 0x06:
			Serial.print(": ACK");
			break;
		case 0x07:
			Serial.print(": Checksum (frame) error");
			break;
		case 0x0F:
			Serial.print(": Verify error");
			break;
		case 0x10:
			Serial.print(": Protect error");
			break;
		case 0x15:
			Serial.print(": NACK");
			break;
		case 0x1A:
			Serial.print(": Erase verify error");
			break;
		case 0x1B:
			Serial.print(": Internal verify or blank check error");
			break;
		case 0x1C:
			Serial.print(": Write error");
			break;
		case 0x20:
			Serial.print(": Read error");
			break;
		case 0xFF:
			Serial.print(": Busy");
			break;
		default:
			// nothing
			break;
	}
	Serial.println();
}
