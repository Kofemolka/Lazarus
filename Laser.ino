#define laser 13
#define sensor A4
#define XSensor 2
#define YSensor 3
#define XSpeed 5
#define YSpeed 6


volatile bool XSensorOn = false;
volatile bool YSensorOn = false;

unsigned char data[] = {
	0b11100000, 0b00000001,
	0b11110000, 0b00000001,
	0b10111000, 0b00000001,
	0b10011100, 0b00000001,
	0b10001110, 0b00000001,
	0b10000111, 0b00000001,
	0b10000011, 0b10000001,
	0b10000001, 0b11000001,
	0b10000000, 0b11100001,
	0b10000000, 0b01110001, 
	0b10000000, 0b00111001, 
	0b10000000, 0b00011101, 
	0b10000000, 0b00001111, 
	0b10000000, 0b00000111, 
	0b10000000, 0b00000011, 
	0b10000000, 0b00000001, 
};

const int dataSize = sizeof(data);
int frameNdx = 0;
const int lineWidth = 2;

const int fanPin = 10;     // OC1B
const unsigned char maxFanSpeed = 80;   // this is calculated as 16MHz divided by 8 (prescaler), divided by 25KHz (target PWM frequency from Intel specification)


void setupFan()
{
	// Set up the PWM pins for a PWM frequency close to the recommended 25KHz for the Intel fan spec.
	// We can't get this frequency using the default TOP count of 255, so we have to use a custom TOP value.
	// Only timer/counter 1 is free because TC0 is used for system timekeeping (i.e. millis() function),
	// and TC2 is used for our 1-millisecond tick. TC1 controls the PWM on Arduino pins 9 and 10.
	// However, we can only get PWM on pin 10 (controlled by OCR1B) because we are using OCR1A to define the TOP value.
	// Using a prescaler of 8 and a TOP value of 80 gives us a frequency of 16000/(8 * 80) = 25KHz exactly.
	TCCR1A = (1 << COM1B1) | (1 << COM1B0) | (1 << WGM11) | (1 << WGM10);  // OC1A (pin 9) disconnected, OC1B (pin 10) = inverted fast PWM  

	OCR1AH = 0;
	OCR1AL = 79;  // TOP = 79
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // TOP = OCR0A, prescaler = 8

	OCR1BH = 0;
	OCR1BL = maxFanSpeed;  // max fan speed 

	TCNT1H = 0;
	TCNT1L = 0;

	// We have to enable the ports as outputs before PWM will work.
	pinMode(fanPin, OUTPUT);
}

void setTransistorFanSpeed(unsigned char fanSpeed)
{
	OCR1BH = 0;
	OCR1BL = fanSpeed;
}

void setupInterrupts()
{
	cli();

	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 15;
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS10);	
	TIMSK1 |= (1 << OCIE1A);

	sei();
}

void setup()
{
	pinMode(laser, OUTPUT);
	digitalWrite(laser, LOW);
	Serial.begin(115200);	
	
	pinMode(sensor, INPUT);
	
	setupInterrupts();

	setupFan();
	setTransistorFanSpeed(80);	

	pinMode(XSensor, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(XSensor), xLap, RISING);

	pinMode(YSensor, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(YSensor), yLap, RISING);
}

enum EState
{
	Skipping,
	Scanning,
	Resting,
	Waiting
};

EState state = EState::Resting;

unsigned int dataNdx = 0;
unsigned int skipNdx = 0;

unsigned long prevStartX = 0;
unsigned long lapTimeX = 0;
unsigned long prevStartY = 0;
unsigned long lapTimeY = 0;
unsigned long timer = 0;
unsigned long  pixel = 1;

unsigned long skipPixels = 0;

void xLap() {
	XSensorOn = true;

	int val = analogRead(sensor);

	skipPixels = map(val, 0, 1023, 0, lapTimeX);	
}

void yLap() {
	YSensorOn = true;
}

ISR(TIMER1_COMPA_vect)
{
	timer++;	
	
	switch (state)
	{
	case EState::Skipping:	
		skipNdx += pixel;
		if (skipNdx < skipPixels)
		{			
			return;
		}
		
		state = EState::Scanning;
		return;

	case EState::Scanning:
		if (dataNdx == lineWidth * 8)
		{
			dataNdx = 0;
			frameNdx += lineWidth;

			if (frameNdx >= dataSize)
			{
				frameNdx = 0;
			}
			
			state = EState::Resting;
			digitalWrite(laser, LOW);				
		}
		else
		{
			unsigned char byte = dataNdx / 8;
			bool hi = (data[frameNdx+ byte] >> (7 - dataNdx % 8)) & 0x1;

			digitalWrite(laser, hi ? HIGH : LOW);

			dataNdx++;
		}
		break;	

	case EState::Resting:
		if(XSensorOn)
		{	
			XSensorOn = false;

			lapTimeX = timer - prevStartX;			
			prevStartX = timer;						

			state = EState::Skipping;
			skipNdx = 0;
			digitalWrite(laser, LOW);
		}
		
		if (YSensorOn)
		{
			YSensorOn = false;
			lapTimeY = timer - prevStartY;
			prevStartY = timer;
		}

		break;
	}
}

void loop()
{		
	
}

