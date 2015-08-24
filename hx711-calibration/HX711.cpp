#include "HX711.h"

HX711::HX711(char dout, char pd_sck, char gain) {
	PD_SCK 	= pd_sck;
	DOUT 	= dout;
	
	pinMode(PD_SCK, OUTPUT);
	pinMode(DOUT, INPUT);

	set_gain(gain);
}

HX711::~HX711() {

}

bool HX711::is_ready() {
	return digitalRead(DOUT) == LOW;
}

void HX711::set_gain(char gain) {
	switch (gain) {
		case 128:		// channel A, gain factor 128
			GAIN = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN = 2;
			break;
	}

	digitalWrite(PD_SCK, LOW);
	read();
}

long HX711::read() {
	// wait for the chip to become ready
	while (!is_ready());

	char data[3];

	// pulse the clock pin 24 times to read the data
	for (char j = 3; j--;) {
		for (char i = 8; i--;) {
			digitalWrite(PD_SCK, HIGH);
			bitWrite(data[j], i, digitalRead(DOUT));
			digitalWrite(PD_SCK, LOW);
		}
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (int i = 0; i < GAIN; i++) {
		digitalWrite(PD_SCK, HIGH);
		digitalWrite(PD_SCK, LOW);
	}

	data[2] ^= 0x80;

	return ((uint32_t) data[2] << 16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
}

long HX711::read_average(char times) {
	long sum = 0;
	for (char i = 0; i < times; i++) {
		sum += read();
	}
	return sum / times;
}

double HX711::get_value(char times) {
	return read_average(times) - OFFSET;
}

float HX711::get_units(char times) {
	return get_value(times) / SCALE;
}

void HX711::tare(char times) {
	double sum = read_average(times);
	set_offset(sum);
}

void HX711::set_scale(float scale) {
	SCALE = scale;
}

void HX711::set_offset(long offset) {
	OFFSET = offset;
}

void HX711::power_down() {
	digitalWrite(PD_SCK, LOW);
	digitalWrite(PD_SCK, HIGH);	
}

void HX711::power_up() {
	digitalWrite(PD_SCK, LOW);	
}