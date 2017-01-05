#define laser 13
#define sensor A5


unsigned char data[] = { 0b10110011, 
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
	//Serial.begin(115200);
	
	digitalWrite(laser, HIGH);

	setupInterrupts();
}

enum EState
{
	Skipping,
	Scanning,
	Resting,
	Waiting
};

EState state = EState::Waiting;

unsigned int dataNdx = 0;
float skipNdx = 0;

unsigned long prevStart = 0;
unsigned long timer = 0;
float lapTime = 360;
float  pixel = 1;
const int lapDivider = 180;

unsigned long curPixel = 0;

float restNdx = 0;

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
			restNdx = 0;			
		}
		else
		{
			unsigned char byte = dataNdx / 8;
			bool hi = (data[byte] >> (7 - dataNdx % 8)) & 0x01;

			digitalWrite(laser, hi ? HIGH : LOW);
		}
		break;	

	case EState::Resting:	
		restNdx += pixel;

		if (restNdx < lapTime/2)
		{			
			return;
		}
		
		digitalWrite(laser, HIGH);
		state = EState::Waiting;
		return;

	case EState::Waiting:	
		if (analogRead(sensor) <100)
		{	
			int newLap = timer - prevStart;
			lapTime = (newLap + lapTime)/2;
			pixel = lapTime / lapDivider;
			
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

