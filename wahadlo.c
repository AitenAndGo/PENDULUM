/*
##########-----------------------------------------------------------------------##########
##########       measurement of the oscillation period of the pendulum           ##########
##########-----------------------------------------------------------------------##########
*/

// ------- Preamble -------- //
#include <avr/io.h>                        /* Defines pins, ports, etc */
#include <util/delay.h>                    /* Functions to waste time  */
#include <string.h>


                                             /* define Fotodiode Sensor:*/
#define Fotodiode_Port PORTB                 /*   - Port                */
#define Fotodiode_DDR DDRB                   /*   - Register            */
#define Fotodiode_PIN 0                      /*   - Pin                 */



                                             /* define Temperature Sensor:*/
                                             /* needs pullup resistor     */
#define Temperature_Port PORTC               /*   - Port                  */
#define Temperature_DDR DDRC                 /*   - Register              */
#define Temperature_PIN 5                    /*   - Pin                   */



                                             /* define LCD connections:*/
#define LCD_Port PORTD                       /*   - Port               */
#define LCD_DDR DDRD                         /*   - Register           */
#define LCD_Enable_Pin PD6                   /*   - Enable Pin         */
#define LCD_RS_Pin PD7                       /*   - RS Pin             */
// Note that LCD is in 4 bit mode //
// connection is as follows: //
// LCD Pin7 <----> PD2 //
// LCD Pin6 <----> PD3 //
// LCD Pin5 <----> PD4 //
// LCD Pin4 <----> PD5 //
#define LCD_DB7 PD2
#define LCD_DB6 PD3
#define LCD_DB5 PD4
#define LCD_DB4 PD5


// LCD instructions
#define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turn display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, don't blink character
#define lcd_FunctionReset   0b00110000          // reset the LCD
#define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
#define lcd_SetCursor       0b10000000          // set cursor position


#define Time TCNT1    // macro for timer1


// - Functions prototypes - //
void TIMER_INIT(void);
void LCD_INIT(void);
void FOTODIODE_INIT(void);
void TEMPERATURE_INIT(void);

void LCD_WRITE_HalfByte(uint8_t half_byte);
void LCD_INSTRUCTION(uint8_t byte);
void LCD_PRINT(uint8_t string[]);
void LCD_PRINT_number(uint16_t number);
void LCD_PRINT_LETTER(uint8_t letter);

int main(void) {

  // -------- Inits --------- //
  TIMER_INIT();
  LCD_INIT();
  FOTODIODE_INIT();
  TEMPERATURE_INIT();

  // LCD_PRINT("123");
  // LCD_PRINT_number(404);
  Time = 0;



  // ------ Event loop ------ //
  while (1) {
    LCD_INSTRUCTION(lcd_Home);
    LCD_PRINT_number(Time / 1000);
    _delay_ms(100);                             /* waste some time */
  }                                       
  return 0;
}

void TIMER_INIT(void){
  TCCR1B |= (1 << CS12) | (1 << CS10);          // Timer1 16bit, prescaler = 1024, 1 step ==> 1ms
  return;
}

void LCD_INIT(void){

  LCD_DDR |= (1 << LCD_DB4) | (1 << LCD_DB5) |
             (1 << LCD_DB6) | (1 << LCD_DB7) |
             (1 << LCD_Enable_Pin) | (1 << LCD_RS_Pin);

  _delay_ms(100); // wait requaired 40ms after Power ON

  LCD_Port = 0x00;

  // following the Instructions from datasheet to init in 4-bit mode
  LCD_WRITE_HalfByte(lcd_FunctionReset);
  _delay_ms(10);
  LCD_WRITE_HalfByte(lcd_FunctionReset);
  _delay_us(100);
  LCD_WRITE_HalfByte(lcd_FunctionReset);
  _delay_us(100);


  LCD_WRITE_HalfByte(lcd_FunctionSet4bit);
  _delay_us(100);
  LCD_INSTRUCTION(lcd_FunctionSet4bit);
  _delay_us(100);

  LCD_INSTRUCTION(lcd_DisplayOff);
  _delay_us(100);
  LCD_INSTRUCTION(lcd_Clear);
  _delay_ms(4);
  LCD_INSTRUCTION(lcd_EntryMode);
  _delay_us(100);
  LCD_INSTRUCTION(lcd_DisplayOn);
  _delay_ms(4);

  return;
}

void LCD_PRINT(uint8_t string[]){
  
  for (uint8_t i = 0; i < strlen(string); i++){
    LCD_PRINT_LETTER(string[i]);
    _delay_us(100);
  }

  return;
}

void LCD_PRINT_LETTER(uint8_t letter){
  LCD_Port |= (1 << LCD_RS_Pin);
  LCD_Port &= ~(1 << LCD_Enable_Pin);

  LCD_WRITE_HalfByte(letter);         // write higher part of byte
  LCD_WRITE_HalfByte(letter << 4);    // write lower part of byte

  return;
}

void LCD_PRINT_number(uint16_t number){

  char* temp = malloc(sizeof(uint8_t));
  if (temp == NULL) {
      return;
  }

  uint8_t i = 0;
  while (number > 0) {
      temp = realloc(temp, (i + 1) * sizeof(uint8_t)); 
      if (temp == NULL) {
          free(temp);
          return;
      }
      uint8_t num = number % 10;
      temp[i] = num + '0';
      number /= 10;
      i++;
  }

  char* new_value = malloc((i) * sizeof(uint8_t)); 
  if (new_value == NULL) {
      free(temp); 
      return;
  }
  
  for (int j = 0; j < i; j++) {
      new_value[j] = temp[i - 1 - j]; 
  }

  free(temp); 
  LCD_PRINT(new_value);
  free(new_value); 

  return;
}

void LCD_WRITE_HalfByte(uint8_t half_byte){

  LCD_Port &= ~(1<<LCD_DB7);
  if (half_byte & 1 << 7) LCD_Port |= (1 << LCD_DB7);   

  LCD_Port &= ~(1<<LCD_DB6);
  if (half_byte & 1 << 6) LCD_Port |= (1 << LCD_DB6);

  LCD_Port &= ~(1<<LCD_DB5);
  if (half_byte & 1 << 5) LCD_Port |= (1 << LCD_DB5);

  LCD_Port &= ~(1<<LCD_DB4);
  if (half_byte & 1 << 4) LCD_Port |= (1 << LCD_DB4);
  
  //this enables to send half of byte
  LCD_Port |= (1 << LCD_Enable_Pin);                   // Enable pin high
  _delay_us(1);                                   
  LCD_Port &= ~(1 << LCD_Enable_Pin);                  // Enable pin low
  _delay_us(1);
}

void LCD_INSTRUCTION(uint8_t byte){
  LCD_Port &= ~(1 << LCD_RS_Pin);
  LCD_Port &= ~(1 << LCD_Enable_Pin);
  LCD_WRITE_HalfByte(byte);
  LCD_WRITE_HalfByte(byte << 4);
}

void FOTODIODE_INIT(void){
  return;
}

void TEMPERATURE_INIT(void){
  return;
}