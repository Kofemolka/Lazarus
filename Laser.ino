// Definition of interrupt names
// ISR interrupt service routine


#define laser 13
#define sensor A4
#define interruptPin 2

volatile bool fanSensorOn = false;

unsigned char data[] = { 0b10110011, 
						 0b10001111,
						 0b00001110,
						 0b00110010,
	0b10110011,
	0b10001111,
	0b00001110,
	0b00110010,
	0b10110011,
	0b10001111,
	0b00001110,
	0b00110010,
	0b10110011,
	0b10001111,
	0b00001110,
	0b00110010,
	0b10110011,
	0b10001111,
	0b00001110,
	0b00110010 };

void setupInterrupts()
{
	cli();

	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 0;
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

	TCCR2A = 0x23;
	TCCR2B = 0x09;  // select clock
	OCR2A = 79;  // aiming for 25kHz
	pinMode(3, OUTPUT);  // enable the PWM output (you now have a PWM signal on digital pin 3)
	OCR2B = 62;  // set the PWM duty cycle

	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
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

unsigned long prevStart = 0;
unsigned long timer = 0;
unsigned long lapTime = 360;
unsigned long  pixel = 1;

void blink() {
	fanSensorOn = true;

	int val = analogRead(sensor);

	int duty = map(val, 0, 1023, 0, 255);
	OCR2B = duty;	
}

ISR(TIMER1_COMPA_vect)
{
	timer++;	
	
	switch (state)
	{
	case EState::Skipping:	
		skipNdx += pixel;
		if (skipNdx < lapTime/4)
		{			
			return;
		}

		state = EState::Scanning;
		return;

	case EState::Scanning:
		dataNdx++;		

		if (dataNdx >= sizeof(data) * 8)
		{
			dataNdx = 0;			
			state = EState::Resting;
			digitalWrite(laser, LOW);					
		}
		else
		{
			unsigned char byte = dataNdx / 8;
			bool hi = (data[byte] >> (7 - dataNdx % 8)) & 0x01;

			digitalWrite(laser, hi ? HIGH : LOW);
		}
		break;	

	case EState::Resting:
		if(fanSensorOn)		
		{	
			fanSensorOn = false;			

			lapTime = timer - prevStart;		
			
			prevStart = timer;			

			state = EState::Skipping;
			skipNdx = 0;
			digitalWrite(laser, LOW);
		}
		
		break;
	}
}

void loop()
{		
	//Serial.println(lapTime);
	
}

