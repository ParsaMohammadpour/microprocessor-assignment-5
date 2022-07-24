#include<stm32f4xx.h>

//Defining a variable to store the port number 7 use them easier
// LCD pins number in port A
#define d0 0 //PA0
#define d1 1 //PA1
#define d2 2 //PA2
#define d3 3 //PA3
#define d4 4 //PA4
#define d5 5 //PA5
#define d6 6 //PA6
#define d7 7 //PA07

#define E 8 //PA8
#define RW 9 //PA9
#define RS 10 //PA10

// push buttons in port B
#define button1 0 // PB0 button one
#define button2 1 // PB1 button two
#define button3 2 // PB2 button three

//MASKS:
#define SET1(x) (1ul << (x))
#define SET0(x) (~(SET1(x)))

//Variables
static char array [9] ;
static int status = -2; // 0 => nothing(counting)       1 => turn off was shown          -1 => turn off wasn't shown         2 => stop (2nd push button)   
//    -2 => start

//Defining Functions
void initialize(void);
void LCD_put_char(char data);
void delay(int n);
void LCD_init(void);
void LCD_command(unsigned char command);
void LCD_resetCommand(void);
void LCD_setCommand(void);
int getNumber(char c);
char getChar(int digit);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void resetArray(void);
void TIM2_IRQHandler(void);
void setUpTIM2(void);
void incMiliSecond(int index, int value);
void disableTIM2(void);
void thirdButtonClick(void);
void thirdButtonPressed(void);
void print_turnOff(void);
void setUpTIM3(void);
void disableTIM3(void);
void TIM3_IRQHandler(void);
void clearTurnOff(void);
void enableTIM2(void);
void enableTIM3(void);
/*
 ******************************************** FUNCTIONS ************************************************
 */
void initialize(void){
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // turning on the clocks for GPIOA
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // turning on the clocks for GPIOB
	
	//GPIOA is output. So we set the MODE for them (pin 0 to 10) writing mode which is "01". There are 11 pins, so 
	//the number will be 01 0101 0101 0101 0101 0101
	GPIOA -> MODER = 0x155555;
	
	//in GPIOB the first 3 pins are push buttons output(micro's input) and reading MODE id : "00". There are 3 pins, so
	GPIOB -> MODER = 0x00;
	
	//in keypad, first 3 pins in GPIOB which are connected to push buttons output(micro's input) should be pull-down. pull-down is: "10"
	//=>10 1010
	GPIOB -> PUPDR = 0x2A;
	
	//writing names to LCD
	LCD_init();
	delay(10);
	LCD_setCommand();
	
	//turning on the system configuration clock
	RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
	// initializing for TIMs
	setUpTIM2();
	setUpTIM3();
	
	//onnecting intrrupt line 1 and 2 to port B
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB; // push button 1
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PB; // push button 2
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB; // push button 2
	
	//removing intrrupt mask register for line 1, 2 and 3. And we also set 1 and 2 in falling edge and 3 in both rising and falling
	EXTI -> IMR |= SET1(0) | SET1(1) | SET1(2);
	EXTI -> FTSR |= SET1(0) | SET1(1) | SET1(2);
	EXTI -> RTSR |= SET1(2);
	
	//turning on the intrrupts line
	__enable_irq();
	
	//setting priority
	NVIC_SetPriority(EXTI0_IRQn, 0);
	NVIC_SetPriority(EXTI1_IRQn, 0);
	NVIC_SetPriority(EXTI2_IRQn, 0);
	
	//clearing pending registers
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
	NVIC_ClearPendingIRQ(EXTI2_IRQn);
	
	//enabling
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI2_IRQn);
	
	// setting all array indexes to " ". (we consider this as a null index in array)
	resetArray();
}


void LCD_put_char(char data) {
	GPIOA -> ODR = data;
  GPIOA -> ODR |= SET1(RS);
	GPIOA -> ODR &= SET0(RW);
  GPIOA -> ODR |= SET1(E);
	GPIOA -> ODR &= SET0(E);
}

void LCD_init(void){
	LCD_command(0x38);
	delay(1);
	LCD_command(0x06);
	delay(1);
	LCD_command(0x08);
	delay(1);
	LCD_command(0x0C);
	delay(1);
}

void LCD_command(unsigned char command){
	GPIOA -> ODR = command;
	GPIOA -> ODR &= SET0(RS);
	GPIOA -> ODR &= SET0(RW);
  GPIOA -> ODR |= SET1(E);
  delay(0);
  GPIOA -> ODR &= SET0(E);

  if (command < 4)
      delay(2);           /* command 1 and 2 needs up to 1.64ms */
  else
      delay(1);           /* all others 40 us */
}


// delay 0.n milliseconds
void delay(int n) {
	/* configure SysTick */
	SysTick -> LOAD = 1600;
	SysTick -> VAL = 0;
	SysTick -> CTRL = 0x5;
	
	for(int i = 0; i < n; i++){
		SysTick -> CTRL = 0x5;
		while((SysTick -> CTRL & 0x10000) == 0);
	}
}

int getNumber(char c){
	switch(c){
		case '0' : return 0;
		case '1' : return 1;
		case '2' : return 2;
		case '3' : return 3;
		case '4' : return 4;
		case '5' : return 5;
		case '6' : return 6;
		case '7' : return 7;
		case '8' : return 8;
		case '9' : return 9;
	}
	return -1;
}
void LCD_setCommand(void){ 
	LCD_command(0x01);
	delay(10);
	LCD_command(0x38);
	delay(10);
	//Welcome
	LCD_put_char('W');
	delay(1);
	LCD_put_char('e');
	delay(1);
	LCD_put_char('l');
	delay(1);
	LCD_put_char('c');
	delay(1);
	LCD_put_char('o');
	delay(1);
	LCD_put_char('m');
	delay(1);
	LCD_put_char('e');
	delay(1);
	LCD_command(0xC0);
	delay(10);
}

void LCD_resetCommand(void){
	LCD_command(0xC0);
	delay(1);
	int i = 0;
	while(i < 9){
		LCD_put_char(array[i++]);
		delay(1);
	}
}


char getChar(int digit){
	switch(digit){
		case 0 : return '0';
		case 1 : return '1';
		case 2 : return '2';
		case 3 : return '3';
		case 4 : return '4';
		case 5 : return '5';
		case 6 : return '6';
		case 7 : return '7';
		case 8 : return '8';
		case 9 : return '9';
	}
	return '&';
}
void resetArray(void){
	array[0] = '0';
	array[1] = '0';
	array[2] = ':';
	array[3] = '0';
	array[4] = '0';
	array[5] = ':';
	array[6] = '0';
	array[7] = '0';
	array[8] = '0';
	
}

void incMiliSecond(int index, int value){
	LCD_command(0x10);
	delay(1);
	delay(1);
	delay(1);
	delay(1);
	delay(1);
	if(index == 2 || index == 5){
		incMiliSecond(index - 1, value);
		LCD_put_char(':');
		delay(1);
		return;
	}
	int a = getNumber(array[index]);
	a += value;
	if(index == 3 && a == 6){
		incMiliSecond(index - 1, 1);
		delay(1);
		a-= 6;
		array[index] = getChar(a);
		LCD_put_char(getChar(a));
		delay(1);
	}else if((a >= 10)){
		incMiliSecond(index - 1, a / 10);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		delay(0);
		a %= 10;
		array[index] = getChar(a);
		LCD_put_char(getChar(a));
		delay(1);
	}else{
		array[index] = getChar(a);
		LCD_put_char(array[index]);
		delay(2);
	}
}
void setUpTIM2(void){
	RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2 -> PSC = 16000000 - 1;
	TIM2 -> ARR = 3 - 1;
	TIM2 -> CR1 = 0; // inactive
	TIM2 -> DIER |= 1;
	NVIC_EnableIRQ(TIM2_IRQn);
}
void enableTIM2(void){
	TIM2 -> CR1 = 1; // active
}
void disableTIM2(void){
	TIM2 -> CR1 = 0;
}
void print_turnOff(void){
	LCD_command(0xC0);
	delay(1);
	LCD_put_char('T');
	delay(1);
	LCD_put_char('u');
	delay(1);
	LCD_put_char('r');
	delay(1);
	LCD_put_char('n');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char('O');
	delay(1);
	LCD_put_char('f');
	delay(1);
	LCD_put_char('f');
	delay(1);
	LCD_put_char(' ');
	delay(1);
}
void clearTurnOff(void){ // for better looking, we set delays 30 mili seconds
	// but even 1 mili second was enough
	LCD_command(0xC0);
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
	LCD_put_char(' ');
	delay(1);
}
void thirdButtonClick(void){
	status = -2;
	disableTIM2();
	disableTIM3();
	resetArray();
	LCD_resetCommand();
}
void thirdButtonPressed(void){
	print_turnOff();
	resetArray();
	status = 1;
	enableTIM3();
}
void setUpTIM3(void){
	RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3 -> PSC = 2000 - 1;
	TIM3 -> ARR = 1000 - 1;
	TIM3 -> CR1 = 0; // inactive
	TIM3 -> DIER |= 1;
	NVIC_EnableIRQ(TIM3_IRQn);
}
void enableTIM3(void){
	TIM3 -> CR1 = 1; // active	
}
void disableTIM3(void){
	TIM3 -> CR1 = 0;
}
/*
 ************************************* INTRRUPT HANDLER ********************************
 */
void EXTI0_IRQHandler(void){ // buttons
	disableTIM3();
	
	
	//masking pending register
	EXTI -> PR |= SET1(0);
	
	//masking this intrrupt to avoid repeating this intrrupt being executed
	EXTI -> IMR &= SET0(0);
	
	//clearing pending IRQ
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	
	
	if(status != 0){
		status = 0;
		LCD_resetCommand();
		enableTIM2();
	}
	
	
	//unmasking this intrrupt
	EXTI -> IMR |= SET1(0);
}


void EXTI1_IRQHandler(void){ // buttons
	disableTIM2();




	//masking pending register
	EXTI -> PR |= SET1(1);
	
	//masking this intrrupt to avoid repeating this intrrupt being executed
	EXTI -> IMR &= SET0(1);
	
	//clearing pending IRQ
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	
	status = 2;
	
	
	
	//unmasking this intrrupt
	EXTI -> IMR |= SET1(1);
}

void EXTI2_IRQHandler(void){ // buttons
	disableTIM2();
	disableTIM3();
	//we disable these timers in the beggining because if we don't, lcd would resmue another mili second
	
	
	
	//masking pending register
	EXTI -> PR |= SET1(2);
	
	//masking this intrrupt to avoid repeating this intrrupt being executed
	EXTI -> IMR &= SET0(2);
	
	//clearing pending IRQ
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	
	
	/* configure SysTick */
	SysTick -> LOAD = 16000;
	SysTick -> VAL = 0;
	SysTick -> CTRL = 0x5;
	
	int a = (GPIOB -> IDR & SET1(2)) == 0 ? 0 : 1;
	int isPressed = 1;
	
	// if we just checked the GPIOB -> IDR at the end of the 3rd minuet, we would waste a lot of time
	for(int i = 0; i < 890; i++){
		SysTick -> CTRL = 0x5;
		while((SysTick -> CTRL & 0x10000) == 0){
			if((a == 1) && ((GPIOB -> IDR & SET1(2)) == 0)){
				isPressed = 0;
				break;
			}
			if((a == 0) && ((GPIOB -> IDR & SET1(2)) != 0)){
				isPressed = 0;
				break;
			}
		}
		if(isPressed == 0)
			break;
	}
	if(isPressed == 1){
		thirdButtonPressed();
	}else{
		thirdButtonClick();
	}
	
	//unmasking this intrrupt
	EXTI -> IMR |= SET1(2);
}


void TIM2_IRQHandler(void){
	TIM2 -> SR = 0;
	incMiliSecond(8, 13);
}
void TIM3_IRQHandler(void){
	TIM3 -> SR = 0;
	disableTIM2();
	if(status == 1){
		clearTurnOff();
		status = -1;
	}else if (status == -1){
		print_turnOff();
		status = 1;
	}
}



int main(void){
	
	initialize();
	
	while(1){
		
	}
}
