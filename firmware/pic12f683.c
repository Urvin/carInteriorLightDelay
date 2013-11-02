/*
 Lifan Smily (320) Roof light PWM control module
 
 MCU pic12F683, hardware PWM
 Hi-Tech C STD 9.60PL3
 v0.1
 
 Yuriy Gorbachev, 2013
 urvindt@gmail.com
*/

//----------------------------------------------------------------------------//

#include <htc.h>

//----------------------------------------------------------------------------//
// CONFIGURATION
//----------------------------------------------------------------------------//

/* Program device configuration word
 * Oscillator = Internal RC No Clock
 * Watchdog Timer = Off
 * Power Up Timer = Off
 * Master Clear Enable = Internal
 * Code Protect = Off
 * Data EE Read Protect = Off
 * Brown Out Detect = BOD and SBOREN disabled
 * Internal External Switch Over Mode = Disabled
 * Monitor Clock Fail-safe = Disabled
 */
__CONFIG(INTIO & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & UNPROTECT & BORDIS & IESODIS & FCMDIS);

//----------------------------------------------------------------------------//

// Useful variable types
#define uint32 unsigned long
#define uint16 unsigned int
#define uint8 unsigned char

//----------------------------------------------------------------------------//

// DOORS
#define DOOR_PIN GPIO1
#define DOORS_OPEN 0
#define DOORS_CLOSED 1
#define DOORS_DEBOUNCE_MAX 10

volatile bit fDoorsState;
volatile uint8 fDoorsDebouncer;

// ENGINE
#define ENGINE_OFF 0
#define ENGINE_ON 1
#define ENGINE_DEBOUNCE_MAX 10

volatile bit fEngineState;
volatile bit fMomentEngineState;
volatile uint8 fEngineDebouncer;

// LAMP
// lamp fades out FADEOUT_TIMER_MAX seconds
// lamp delay before turn off is FADEOUT_WAIT_TIME ticks (1220 / 122 = 10 seconds)
// lamp automatically turns off after AUTO_OFF_TIME ticks (14640 / 122 = 120 seconds)
#define BRIGHTNESS_MAX 101
#define FADEOUT_TIMER_MAX 4
#define FADEOUT_WAIT_TIME 1220
#define AUTO_OFF_TIME 14640

volatile uint8 fBrightness;
volatile uint16 fLightWaitTimer;
volatile bit fFadeDirection;
volatile bit fNextFadeDirection;
volatile uint8 fFadeOutTimer;
volatile uint16 fAutoOffTimer;

//----------------------------------------------------------------------------//

void initSoftware(void)
{
	fBrightness = 0;
	fFadeDirection = 0;
	fNextFadeDirection = 0;
	fLightWaitTimer = 0;
	fFadeOutTimer = 0;
	
	fDoorsState = DOORS_CLOSED;
	fDoorsDebouncer = 0;
	
	fEngineState = ENGINE_OFF;
	fMomentEngineState = ENGINE_OFF;
	fEngineDebouncer = 0;
	
	fAutoOffTimer = 0;
}

void initHardware(void)
{
	// Disable watch dog
	WDTCON = 0;
	
	// IO Port direction
	TRISIO = 0b00111011;
	GPIO = 0;
	
	//Internal oscillator set to 8MHz
	OSCCON = 0b01110000;
	
	// Prescale to 1:64 (122 owerflows per second)
	OPTION = 0b10000101;
	//         ||||||||- Prescaler rate
	//         |||||---- Prescaler assighnment (0 - Timer0, 1 - WDT)
	//         ||||----- Timer0 edge (0 - low to high)
	//         |||------ Timer0 source (0 - internal)
	//         ||------- Interrupt edge (0 - falling)
	//         |-------- GPIO Pull up resistors (1 to disable)
	
	// Timer 2
	T2CON = 0b00000111; //16 prescaler for 1.9 kHz PWM
	//        ||||||||- Prescaler
	//        ||||||--- Timer2 is ON
	//        |||||---- Postscaler
	//        |-------- always 0
		
	// Comparators/PWM
	CCP1CON = 0b00001100;
	//CCP1CON = 0;
	//            |||||| - PWM mode, active high 
	//            || - PWM last significant bits	
		
	// PWM period to 0x65
	PR2	= 0b01100101;
	
	// Set 0 brightness
	CCPR1L = 0;
	
	// Disable comparators
	CMCON1 = 0x07;
	CMCON0 = 0x07;
	
	
	// ADC Configuration 
	ADCON0 = 0b00000001;
	//         ||  ||||- ADC is ON
	//         ||  |||-- No conversion now
	//         ||  ||--- Analog channel select (00 - AN0)
	//         ||------- Voltage reference (0 - Vdd, 1 - Vref)
	//         |-------- Justification (0 - left)
		
    ANSEL = 0b01010001;
	//         |||||||- AN0 digital/analog (0 - digital)
	//         ||||||-- AN1 digital/analog (0 - digital)
	//         |||||--- AN2 digital/analog (0 - digital)
	//         ||||---- AN3 digital/analog (0 - digital)    
	//         |||----- Prescaler
	
	// Interrupts
	INTCON = 0b01100000;	
	//         |||||- GPIO change
	//         ||||-- GP2/INT
	//         |||--- Timer0
	//         ||---- Peripherial interrupts
	//         |----- Global intterupt (off while initialization)
	
	// Peripherial interrupts
	PIE1 = 0b01000000;
	//       ||| ||||- TMR1IE
	//       ||| |||-- TMR2IE
	//       ||| ||--- OSFIE 
	//       ||| |---- CMIE
	//       |||------ CCP
	//       ||------- ADC
    //       |-------- EEPROM
		
}

//----------------------------------------------------------------------------//

void setLightOn(void)
{
	// Enable light at moment
	fLightWaitTimer = 0;
	// Change light fade direction
	fNextFadeDirection = 1;
	// Set auto off timer to start
	fAutoOffTimer = AUTO_OFF_TIME;
}

void setLightOff(void)
{
	// Turn light off only after some time
	fLightWaitTimer = FADEOUT_WAIT_TIME;
	// Change light fade direction
	fNextFadeDirection = 0;
	// Disable auto off timer
	fAutoOffTimer = 0;
}

void setLightOffImmediately(void)
{
	// Turn light off immediately
	fLightWaitTimer = 0; 
	// Change light fade direction
	fNextFadeDirection = 0;
	// Disable auto off timer
	fAutoOffTimer = 0;
}

//----------------------------------------------------------------------------//


void updateBrightness(void)
{
	CCPR1L = fBrightness;
	
	// Update lowest bits
	if(fBrightness == BRIGHTNESS_MAX)
	{
		DC1B0 = 1;
		DC1B1 = 1;
	}
	else
	{
		DC1B0 = 0;
		DC1B1 = 0;
	}
}

//----------------------------------------------------------------------------//

void updateFadeDirection(void)
{
	fFadeDirection = fNextFadeDirection;
	fFadeOutTimer = 0;
}

void processLightWaitTimer(void)
{
	if(fLightWaitTimer > 0)
		fLightWaitTimer--;
	
	if(fLightWaitTimer == 0 && fFadeDirection != fNextFadeDirection)
		updateFadeDirection();
}

void processFading(void)
{
	if(fFadeDirection == 0)
	{
		if(fBrightness > 0)
		{
			fFadeOutTimer++;
			if(fFadeOutTimer >= FADEOUT_TIMER_MAX)
			{
				fFadeOutTimer = 0;
				fBrightness--;
				updateBrightness();
			}
		}
		
		if(fBrightness == 0 && fLightWaitTimer == 0)
			updateFadeDirection();
	}
	else
	{
		if(fBrightness < BRIGHTNESS_MAX)
		{
			fBrightness++;
			updateBrightness();
		}
		if(fBrightness == BRIGHTNESS_MAX  && fLightWaitTimer == 0)
			updateFadeDirection();
	}
}


void processDoors(void)
{
	if(fDoorsDebouncer == 0)
	{
		if(DOOR_PIN != fDoorsState)		
			fDoorsDebouncer = DOORS_DEBOUNCE_MAX;
	}
	else
	{
		if(fDoorsDebouncer == 1 && DOOR_PIN != fDoorsState)
		{
			fDoorsState = DOOR_PIN;
			
			if(fDoorsState == DOORS_CLOSED)
			{
				if(fEngineState == ENGINE_ON)
					setLightOffImmediately();
				else
					setLightOff();
			}
			else
				setLightOn();
		}
		
		fDoorsDebouncer--;
	}
}

void processEngine(void)
{			
	if(fEngineDebouncer == 0)
	{
		if(fMomentEngineState != fEngineState)		
			fEngineDebouncer = ENGINE_DEBOUNCE_MAX;		
	}
	else
	{
		if(fEngineDebouncer == 1 && fMomentEngineState != fEngineState)
		{
			fEngineState = fMomentEngineState;
			
			if(fEngineState == ENGINE_ON)
			{
				if(fDoorsState == DOORS_CLOSED)				
					setLightOffImmediately();
			}
			else
			{
				setLightOn();
			}			
		}
		
		fEngineDebouncer--;
	}
}

void processADC(void)
{
	// 13V is 221 at ADRESH
	fMomentEngineState = (ADRESH >= 221) ? ENGINE_ON : ENGINE_OFF;
}

void restartADC(void)
{	
	if(GODONE == 0)
		GODONE = 1;
}

void processAutoOff(void)
{
	if(fAutoOffTimer > 0)
	{
		fAutoOffTimer--;
		if(fAutoOffTimer == 1)
		{
			setLightOffImmediately();
		}
	}
}

//----------------------------------------------------------------------------//

void interrupt isr(void)
{

	// Timer0
	if((T0IE) && (T0IF))
	{
		processLightWaitTimer();
		processFading();
		processDoors();
		processEngine();
		restartADC();
		processAutoOff();
		T0IF = 0;
	}
	// ADC
	else if((ADIE) && (ADIF))
	{
		processADC();
		ADIF = 0;
	}
}

//----------------------------------------------------------------------------//

void main(void)
{
	initHardware();
	initSoftware();
	ei();	
		
	while (1)
	{

	}
}
