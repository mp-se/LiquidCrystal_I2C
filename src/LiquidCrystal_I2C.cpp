/***************************************************************************************************/
/*
  This is an Arduino library for HD44780, S6A0069, KS0066U, NT3881D, LC7985, ST7066, SPLC780,
  WH160xB, AIP31066, GDM200xD, ADM0802A LCD displays.

  Screens are operated in 4 bit mode over I2C bus with 8-bit I/O expander PCF8574x.
  Typical displays sizes: 8x2, 16x1, 16x2, 16x4, 20x2, 20x4 & etc.

  written by : enjoyneering79
  sourse code: https://github.com/enjoyneering/

  This chip uses I2C bus to communicate, specials pins are required to interface
  Board:                                    SDA                    SCL
  Uno, Mini, Pro........................... A4                     A5
  Mega2560, Due............................ 20                     21
  Leonardo................................. 2                      3
  Trinket/ATtiny85......................... 0/physical pin no.5    2/physical pin no.7
  Blue Pill/STM32F103xxxx boards........... B7                     B6 with 5v->3v logich converter
  ESP8266 ESP-01:.......................... GPIO0/D5               GPIO2/D3
  NodeMCU 1.0, WeMos D1 Mini............... GPIO4/D2               GPIO5/D1

  Frameworks & Libraries:
  ATtiny Core           - https://github.com/SpenceKonde/ATTinyCore
  ATtiny I2C Master lib - https://github.com/SpenceKonde/TinyWireM
  ESP8266 Core          - https://github.com/esp8266/Arduino
  ESP8266 I2C lib fixed - https://github.com/enjoyneering/ESP8266-I2C-Driver
  STM32 Core            - https://github.com/rogerclarkmelbourne/Arduino_STM32

  GNU GPL license, all text above must be included in any redistribution, see link below for details:
  - https://www.gnu.org/licenses/licenses.html
*/
/***************************************************************************************************/

#include "LiquidCrystal_I2C.h"


/**************************************************************************/
/*
    LiquidCrystal_I2C()

    Constructor. Initializes the class variables, defines I2C address,
    LCD & PCF8574 pins.

    NOTE: The constructor does not reset & initialize the LCD.
*/
/**************************************************************************/  
LiquidCrystal_I2C::LiquidCrystal_I2C(PCF8574_address addr, uint8_t P0, uint8_t P1, uint8_t P2, uint8_t P3, uint8_t P4, uint8_t P5, uint8_t P6, uint8_t P7, switchPolarity polarity)
{
  uint8_t PCF8574_TO_LCD[8] = {P0, P1, P2, P3, P4, P5, P6, P7}; //PCF8574 ports to LCD pins mapping array

  _PCF8574_address        = addr;
  _PCF8574_initialisation = true;
  _switchPolarity         = polarity;

  /* maping LCD pins to PCF8574 ports */
  for (uint8_t i = 0; i < 8; i++)
  {
    switch(PCF8574_TO_LCD[i])
    {
      case 4:         //RS pin
        _LCD_TO_PCF8574[0] = i;
        break;

      case 5:         //RW pin
        _LCD_TO_PCF8574[1] = i;
        break;

      case 6:         //EN pin
        _LCD_TO_PCF8574[2] = i;
        break;

      case 14:        //D7 pin
        _LCD_TO_PCF8574[3] = i;
        break;

      case 13:        //D6 pin
        _LCD_TO_PCF8574[4] = i;
        break;

      case 12:        //D5 pin
        _LCD_TO_PCF8574[5] = i;
        break;

      case 11:        //D4 pin
        _LCD_TO_PCF8574[6] = i;
        break;

      case 16:        //BL pin
        _LCD_TO_PCF8574[7] = i;
        break;

      default:
        _PCF8574_initialisation = false; //safety check, make sure the declaration of lcd pins is right
        break;
    }
  }

  /* backlight control via PCF8574 */
  switch (_switchPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_ON;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_ON;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[7];
}

/**************************************************************************/
/*
    begin()

    Initializes, resets & configures I2C and LCD (call this function before
    doing anything else)

    NOTE:
    - Wire.endTransmission() returned code:
      0 - success
      1 - data too long to fit in transmit data16
      2 - received NACK on transmit of address
      3 - received NACK on transmit of data
      4 - other error
*/
/**************************************************************************/
#if defined(ESP8266)
bool LiquidCrystal_I2C::begin(uint8_t lcd_colums, uint8_t lcd_rows, lcd_font_size f_size, uint8_t sda, uint8_t scl)
{
  Wire.begin(sda, scl);
  Wire.setClock(100000UL);                            //experimental! ESP8266 i2c bus speed: 100kHz..400kHz/100000UL..400000UL, default 100000UL
  Wire.setClockStretchLimit(230);                     //experimental! default 230
#else
bool LiquidCrystal_I2C::begin(uint8_t lcd_colums, uint8_t lcd_rows, lcd_font_size f_size)
{
  Wire.begin();
  Wire.setClock(100000UL);                            //experimental! AVR i2c bus speed: AVR 31kHz..400kHz/31000UL..400000UL, default 100000UL
#endif

  if (_PCF8574_initialisation == false) return false; //safety check, make sure the declaration of lcd pins is right

  Wire.beginTransmission(_PCF8574_address);
  if (Wire.endTransmission() != 0) return false;      //safety check, make sure the PCF8574 is connected

  writePCF8574(PCF8574_ALL_LOW);                      //safety check, set all PCF8574 pins low

  _lcd_colums 	 = lcd_colums;
  _lcd_rows      = lcd_rows;
  _lcd_font_size = f_size;

  initialization();                                   //soft reset & 4-bit mode initialization

  return true;
}

/**************************************************************************/
/*
    clear()

    Clears display & move cursor to home position

    NOTE:
    - clear by fill it with space
    - cursor home position (0, 0)
    - command duration > 1.53 - 1.64ms
*/
/**************************************************************************/
void LiquidCrystal_I2C::clear(void)
{
  send(LCD_INSTRUCTION_WRITE, LCD_CLEAR_DISPLAY, LCD_CMD_LENGTH_8BIT);

  delay(LCD_HOME_CLEAR_DELAY);
}

/**************************************************************************/
/*
    home()

    Moves cursor position to home position

    NOTE:
    - sets DDRAM address to 0 in address counter, returns display to
      home position, but DDRAM contents remain unchanged
    - command duration > 1.53 - 1.64ms
*/
/**************************************************************************/
void LiquidCrystal_I2C::home(void)
{
  send(LCD_INSTRUCTION_WRITE, LCD_RETURN_HOME, LCD_CMD_LENGTH_8BIT);

  delay(LCD_HOME_CLEAR_DELAY);
}

/**************************************************************************/
/*
    setCursor()

    Sets cursor position

    NOTE:
    - cursor start position (0, 0)
    - cursor end   position (lcd_colums - 1, lcd_rows - 1)
    - DDRAM data/text is sent & received after this setting
*/
/**************************************************************************/
void LiquidCrystal_I2C::setCursor(uint8_t colum, uint8_t row)
{
  uint8_t row_address_offset[] = {0x00, 0x40, uint8_t(0x00 + _lcd_colums), uint8_t(0x40 + _lcd_colums)};

  /* safety check, cursor position & array are zero indexed */
  if (row   >= _lcd_rows)   row   = _lcd_rows   - 1;
  if (colum >= _lcd_colums) colum = _lcd_colums - 1;

  send(LCD_INSTRUCTION_WRITE, LCD_DDRAM_ADDR_SET | (row_address_offset[row] + colum), LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    noDisplay()

    Clears text from the screen

    NOTE:
    - data/text remains in DDRAM
*/
/**************************************************************************/
void LiquidCrystal_I2C::noDisplay(void)
{
  _displayControl &= ~LCD_DISPLAY_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    display()

    Retrives text from DDRAM

    NOTE:
    - text & data remains in DDRAM !!!
*/
/**************************************************************************/
void LiquidCrystal_I2C::display(void)
{
  _displayControl |= LCD_DISPLAY_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    noCursor()

    Turns OFF the underline cursor
*/
/**************************************************************************/
void LiquidCrystal_I2C::noCursor(void)
{
  _displayControl &= ~LCD_UNDERLINE_CURSOR_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    cursor()

    Turns ON the underline cursor
*/
/**************************************************************************/
void LiquidCrystal_I2C::cursor(void)
{
  _displayControl |= LCD_UNDERLINE_CURSOR_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    noBlink()

    Turns OFF the blinking cursor
*/
/**************************************************************************/
void LiquidCrystal_I2C::noBlink(void)
{
  _displayControl &= ~LCD_BLINK_CURSOR_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    blink()

    Turns ON the blinking cursor
*/
/**************************************************************************/
void LiquidCrystal_I2C::blink(void)
{
  _displayControl |= LCD_BLINK_CURSOR_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    scrollDisplayLeft()

    Scrolls the current row with text on the display to the left.
*/
/**************************************************************************/
void LiquidCrystal_I2C::scrollDisplayLeft(void)
{
  send(LCD_INSTRUCTION_WRITE, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    scrollDisplayRight()

    Scrolls the current row with text on the display to the right.
*/
/**************************************************************************/
void LiquidCrystal_I2C::scrollDisplayRight(void)
{
  send(LCD_INSTRUCTION_WRITE, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    leftToRight()

    Sets text direction from left to right
*/
/**************************************************************************/
void LiquidCrystal_I2C::leftToRight(void)
{
  _displayMode |= LCD_ENTRY_LEFT;

  send(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    rightToLeft()

    Sets text direction from right to left
*/
/**************************************************************************/
void LiquidCrystal_I2C::rightToLeft(void)
{
  _displayMode &= ~LCD_ENTRY_LEFT;

  send(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    autoscroll()

    Autoscrolls the text on the display

    NOTE:
    - whole text on the display shift when byte written, but cursor stays
*/
/**************************************************************************/
void LiquidCrystal_I2C::autoscroll(void) 
{
  _displayMode |= LCD_ENTRY_SHIFT_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}


/**************************************************************************/
/*
    noAutoscroll()

    Stops text autoscrolling on the display

    NOTE:
    - whole text on the display stays, cursor shifts when byte written
*/
/**************************************************************************/
void LiquidCrystal_I2C::noAutoscroll(void)
{
  _displayMode &= ~LCD_ENTRY_SHIFT_ON;

  send(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    createChar()

    Fills the 64 bytes CGRAM, with custom characters

    NOTE:
    - 8 locations for 5x8DOTS  display, at index 0-7
    - 4 locations for 5x10DOTS display, at index 0-3
*/
/**************************************************************************/
void LiquidCrystal_I2C::createChar(uint8_t CGRAM_address, uint8_t char_pattern[])
{
  uint8_t CGRAM_capacity = 0;
  uint8_t font_size      = 0;

  /* set CGRAM capacity */
  switch (_lcd_font_size)
  {
    case LCD_5x8DOTS:
      CGRAM_capacity = 0x07;
      font_size      = 8;
      break;

    case LCD_5x10DOTS:
      CGRAM_capacity = 0x3;
      font_size      = 10;
      break;
  }

  /* safety check, make sure "CGRAM_address" never exceeds the "CGRAM_capacity" */
  CGRAM_address &= CGRAM_capacity;

  send(LCD_INSTRUCTION_WRITE, LCD_CGRAM_ADDR_SET | (CGRAM_address << 3), LCD_CMD_LENGTH_8BIT); //set CGRAM address

  for (uint8_t i = 0; i < font_size; i++)
  {
    send(LCD_DATA_WRITE, char_pattern[i], LCD_CMD_LENGTH_8BIT);                                //write data to CGRAM address
  }
}

void LiquidCrystal_I2C::createChar(uint16_t CGRAM_address, uint8_t *char_pattern[])
{
  createChar((uint8_t)CGRAM_address, char_pattern);
}

/**************************************************************************/
/*
    noBacklight()

    Turns OFF the backlight with PCF8574. 

    NOTE:
    - doesn't affect lcd controller, because we are working with
      transistor conncted to the PCF8574 port
*/
/**************************************************************************/
void LiquidCrystal_I2C::noBacklight(void)
{
  switch (_switchPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_OFF;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_OFF;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[7];

  writePCF8574(PCF8574_ALL_LOW);
}

/**************************************************************************/
/*
    backlight()

    Turns ON the backlight with PCF8574.

    NOTE:
    - doesn't affect lcd controller, because we are working with
      transistor conncted to the PCF8574 port
*/
/**************************************************************************/
void LiquidCrystal_I2C::backlight(void)
{
  switch (_switchPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_ON;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_ON;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[7];

  writePCF8574(PCF8574_ALL_LOW);
}

/**************************************************************************/
/*
    write()

    Replaces function "write()" in Arduino class "Print" & sends character
    to the LCD
*/
/**************************************************************************/
#if defined(ARDUINO) && ARDUINO >= 100
size_t LiquidCrystal_I2C::write(uint8_t value)
#else
void LiquidCrystal_I2C::write(uint8_t value)
#endif
{
  send(LCD_DATA_WRITE, value, LCD_CMD_LENGTH_8BIT);

  #if defined(ARDUINO) && ARDUINO >= 100
  return 1;
  #endif
}

/**************************************************************************/
/*
    initialization()

    Soft reset lcd & activate 4-bit interface

    NOTE:
    - for correct LCD operation it is necessary to do the internal circuit
      reset & initialization procedure. See 4-bit initializations
      procedure fig.24 on p.46 of HD44780 datasheet and p.17 of 
      WH1602B/WH1604B datasheet for details.
*/
/**************************************************************************/
void LiquidCrystal_I2C::initialization(void)
{
  uint8_t displayFunction = 0; //don't change!!! default bits value DB7, DB6, DB5, DB4=(DL), DB3=(N), DB2=(F), DB1, DB0

  /*
     HD44780/WH160xB & etc need ~40ms after voltage rises above 2.7v
     some Arduino can start & run the code at 2.4v, so we'll wait 700ms
  */
  delay(700);

  /*
     FIRST ATTEMPT: set 8-bit mode
     wait min 4.1ms some LCDs need more than 4.5ms. so we'll wait 7ms
     NB: used for Hitachi & Winstar
  */
  send(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  delay(7);

  /*
     SECOND ATTEMPT: set 8-bit mode
     wait min 100us.
     NB: used for Hitachi only, not needed for Winstar displays
  */
  send(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  delayMicroseconds(350);
	
  /*
     THIRD ATTEMPT: set 8 bit mode
     NB: used for Hitachi only, not needed for Winstar displays
  */
  send(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  delayMicroseconds(250);
	
  /*
     FINAL ATTEMPT: set 4-bit interface
     NB: Busy Flag can be checked after this instruction
  */
  send(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_4BIT_MODE, LCD_CMD_LENGTH_4BIT);

  /* sets qnt. of lines, 1 line by default */
  if (_lcd_rows > 1) displayFunction |= LCD_2_LINE;   //line bit located at BD3 & zero by default

  /* sets font size, 5x8 by default */
  if (_lcd_font_size == LCD_5x10DOTS)
  {
    displayFunction |= LCD_5x10DOTS;                   //font bit located at BD2
    if(_lcd_rows != 1) displayFunction &= ~LCD_2_LINE; //safety check, two rows displays can't display 10 pixel font
  }

  /* initializes lcd functions: qnt. of lines, font size, etc. (can't be changed after this point) */
  send(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_4BIT_MODE | displayFunction, LCD_CMD_LENGTH_8BIT);
	
  /* initializes lcd controls: turn display off, underline cursor off & blinking cursor off (by default) */
  _displayControl = LCD_UNDERLINE_CURSOR_OFF | LCD_BLINK_CURSOR_OFF;
  noDisplay();

  /* clear display */
  clear();

  /* initializes lcd basics: sets text direction "left to right" & cursor movement to the right (by default) */
  _displayMode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_OFF;
  send(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);

  display();
}

/**************************************************************************/
/*
    send()

    The most anvanced & fastes way to write COMMAND or DATA/TEXT to LCD
    
    NOTE:
    - all inputs formated as follow: 
      mode  - RS,RW,E=1,DB7,DB6,DB5,DB4,BCK_LED=0
      value - DB7,DB6,DB5,DB4,DB3,DB2,DB1,DB0
    - duration of command > 37usec
    - duration of the En pulse > 450nsec
*/
/**************************************************************************/
void LiquidCrystal_I2C::send(uint8_t mode, uint8_t value, uint8_t length)
{
  uint8_t data = 0;
  uint8_t msb  = 0;
  uint8_t lsb  = 0;

  /* 4-bit or first part of 8-bit command */
  lsb  = value >> 3;                       //0,0,0,DB7,DB6,DB5,DB4,DB3
  lsb &= 0x1E;                             //0,0,0,DB7,DB6,DB5,DB4,BCK_LED=0 
  data = portMapping(mode | lsb);          //RS,RW,E=1,DB7,DB6,DB5,DB4,BCK_LED=0

  writePCF8574(data);                      //send command
  delayMicroseconds(LCD_EN_PULSE_DELAY);   //En pulse duration
  bitClear(data, _LCD_TO_PCF8574[2]);      //RS,RW,E=0,DB7,DB6,DB5,DB4,BCK_LED=0
  writePCF8574(data);                      //execute command
  delayMicroseconds(LCD_COMMAND_DELAY);    //command duration

  /* second part of 8-bit command */
  if (length == LCD_CMD_LENGTH_8BIT)
  {
    msb  = value << 1;                     //DB6,DB5,DB4,DB3,DB2,DB1,DB0,0
    msb &= 0x1E;                           //0,0,0,DB3,DB2,DB1,DB0,BCK_LED=0 
    data = portMapping(mode | msb);        //RS,RW,E=1,DB3,DB2,DB1,DB0,BCK_LED=0

    writePCF8574(data);                    //send command
    delayMicroseconds(LCD_EN_PULSE_DELAY); //En pulse duration
    bitClear(data, _LCD_TO_PCF8574[2]);    //RS,RW,E=0,DB3,DB2,DB1,DB0,BCK_LED=0
    writePCF8574(data);                    //execute command
    delayMicroseconds(LCD_COMMAND_DELAY);  //command duration
  }	
}

/**************************************************************************/
/*
    portMapping()

    All magic of pins to ports mapping is happening here!!!

    NOTE:
    - isolates HD44780 bits & shifts to the right PCF8574 ports position
      RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED -> P0,P1,P2,P3,P4,P5,P6,P7
      RS,RW,E,DB3,DB2,DB1,DB0,BCK_LED -> P0,P1,P2,P3,P4,P5,P6,P7  
*/
/**************************************************************************/
uint8_t LiquidCrystal_I2C::portMapping(uint8_t value)
{
  uint8_t data = 0;

  for (uint8_t i = 0; i < 8; i++)
  {
    bitWrite(data, _LCD_TO_PCF8574[i], bitRead(value, (7 - i)));
  }
  return data; 
}

/**************************************************************************/
/*
    writePCF8574()

    Masks backlight with data & writes it to PCF8574 over I2C

    NOTE:
    - Wire.endTransmission() returned code:
      0 - success
      1 - data too long to fit in transmit data16
      2 - received NACK on transmit of address
      3 - received NACK on transmit of data
      4 - other error
*/
/**************************************************************************/
void LiquidCrystal_I2C::writePCF8574(uint8_t value)
{
  int8_t pollCounter = LCD_I2C_POLLING_LIMIT;

  do
  {
    pollCounter--;

    Wire.beginTransmission(_PCF8574_address);
    #if ARDUINO >= 100
    Wire.write(value | _backlightValue);
    #else
    Wire.send(value | _backlightValue);
    #endif
  }
  #if defined(_VARIANT_ARDUINO_STM32_)
  while ((Wire.endTransmission() != 0) || (pollCounter > 0));
  #else
  while ((Wire.endTransmission(true) != 0) || (pollCounter > 0));
  #endif
}

/**************************************************************************/
/*
    readPCF8574()

    Reads byte* from PCF8574 over I2C

    * - the logic values on the PCF8574 pins P0...P7

    NOTE:
    - if the PCF8574 output is written low before a read, a low is always
      read from the I/O, regardless of the state of the device connected
      to the I/O.
      Therefore, if the PCF8574 outputs are all high (either from power-up
      reset initialization or from a write) before reading is performed,
      any devices connected to the I/O can fully control the I/O.
*/
/**************************************************************************/
uint8_t LiquidCrystal_I2C::readPCF8574()
{
  int8_t pollCounter = LCD_I2C_POLLING_LIMIT;

  do
  {
    if (pollCounter-- == 0) return 0;            //fail to read

    #if defined(_VARIANT_ARDUINO_STM32_)
    Wire.requestFrom(_PCF8574_address, 1);
    #else
    Wire.requestFrom(_PCF8574_address, 1, true); //true = stop message after transmission & releas the I2C bus
    #endif  
  }
  while (Wire.available() != 1);                 //check "wire.h" rxBuffer

  /* reads byte from "wire.h" rxBuffer */
  #if ARDUINO >= 100
  return Wire.read();
  #else
  return Wire.receive();
  #endif
}

/**************************************************************************/
/*
    readBusyFlag()

    Checks the LCD's Busy Flag (BF)

    NOTE:
    - to retrive Busy Flag set RS=0 & RW=1
    - if DB7=1 than lcd is busy
      if DB7=0 than lcd is readyf or a new command
    - bits DB6, DB5, DB4, DB3, DB2, DB1, DB0 content of
      address counter/cursor position
*/
/**************************************************************************/
bool LiquidCrystal_I2C::readBusyFlag()
{
  send(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT);

  return bitRead(readPCF8574(), _LCD_TO_PCF8574[3]);
}

/**************************************************************************/
/*
    readAddressCounter()

    Returns the contents of address counter (cursor position)

    NOTE:
    - to retrive Busy Flag set RS=0 & RW=1
    - if DB7=1 than lcd is busy
      if DB7=0 than lcd is readyf or a new command
    - bits DB6, DB5, DB4, DB3, DB2, DB1, DB0 content of
      address counter/cursor position
*/
/**************************************************************************/
uint8_t LiquidCrystal_I2C::readAddressCounter()
{
  uint8_t data  = 0;
  uint8_t value = 0;

  send(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT);
  data = readPCF8574();

  for (uint8_t i = 4; i < 7; i++)  //D6, D5, D4
  {
    bitWrite(value, 10 - i, bitRead(data, _LCD_TO_PCF8574[i]));
  }

  send(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT);
  data = readPCF8574();

  for (uint8_t i = 3; i < 7; i++) //D3, D2, D1, D0
  {
    bitWrite(value, 6 - i, bitRead(data, _LCD_TO_PCF8574[i]));
  }

  return value;
}

/* !!! Arduino Unsupported API functions !!! */

/**************************************************************************/
/*
    printHorizontalGraph(name, row, value, maxValue)

    Prints horizontal graph
*/
/**************************************************************************/
void LiquidCrystal_I2C::printHorizontalGraph(char name, uint8_t row, uint16_t currentValue, uint16_t maxValue)
{
    uint16_t currentGraph = 0;
    uint8_t  colum        = 0;

    if (currentValue > maxValue) currentValue = maxValue;          //safety check, without it ESP8266 reboots

    currentGraph = map(currentValue, 0, maxValue, 0, _lcd_colums);

    setCursor(colum, row);
    send(LCD_DATA_WRITE, name, LCD_CMD_LENGTH_8BIT);

    /* draw the horizontal bar without clearing the display, to eliminate flickering */
    for (colum = 1; colum < currentGraph; colum++)
    {
      setCursor(colum, row);
      send(LCD_DATA_WRITE, 0xFF, LCD_CMD_LENGTH_8BIT);             //print 0xFF - built in "solid square" char. see p.17 & p.30 of HD44780 datasheet
    }

    /* fill the left overs (from the previous draw) with spaces */
    for (colum; colum < _lcd_colums; colum++)
    {
      send(LCD_DATA_WRITE, 0x20, LCD_CMD_LENGTH_8BIT);             //print 0x20 - built in "space" char. see p.17 & p.30 of HD44780 datasheet
    }
}

/**************************************************************************/
/*
    printVerticalGraph()

    Prints vertical graph
*/
/**************************************************************************/
void LiquidCrystal_I2C::printVerticalGraph(uint8_t colum, uint8_t row, uint16_t currentValue, uint16_t maxValue)
{
  uint16_t currentGraph = 0;

  uint8_t verticalBar_0[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  uint8_t verticalBar_1[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F};
  uint8_t verticalBar_2[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F};
  uint8_t verticalBar_3[8] = {0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F};
  uint8_t verticalBar_4[8] = {0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F};
  uint8_t verticalBar_5[8] = {0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F};
  uint8_t verticalBar_6[8] = {0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
  uint8_t verticalBar_7[8] = {0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
  uint8_t verticalBar_8[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};

  if (currentValue > maxValue) currentValue = maxValue; //safety check, without it ESP8266 reboots

  currentGraph = map(currentValue, 0, maxValue, 0, 8);

  switch(currentGraph)
  {
    case 0:
      createChar(0x07, verticalBar_0);
      break;
    case 1:
      createChar(0x07, verticalBar_1);
      break;
    case 2:
      createChar(0x07, verticalBar_2);
      break;
    case 3:
      createChar(0x07, verticalBar_3);
      break;
    case 4:
      createChar(0x07, verticalBar_4);
      break;
    case 5:
      createChar(0x07, verticalBar_5);
      break;
    case 6:
      createChar(0x07, verticalBar_6);
      break;
    case 7:
      createChar(0x07, verticalBar_7);
      break;
    case 8:
      createChar(0x07, verticalBar_8);
      break;
    default:
      break;
  }

  setCursor(colum, row);
  send(LCD_DATA_WRITE, 0x07, LCD_CMD_LENGTH_8BIT);      //print custom char from 7th CGRAM address
}

/**************************************************************************/
/*
    setBrightness()

    NOTE:
    - to use this function, the "LED" jumper on the back has to be
      removed & the bottom pin has to be connected to one of Arduino PWM
      pin in series with 470 Ohm resistor
    - recomended min. value = 25, max. value = 255 (0.5v .. 4.5v)
                 min. value = 0,  max. value = 255 (0.0v .. 4.5v)  
*/
/**************************************************************************/
void LiquidCrystal_I2C::setBrightness(uint8_t pin, uint8_t value, switchPolarity polarity)
{
  pinMode(pin, OUTPUT);

  if (polarity == NEGATIVE) value = 255 - value;

  analogWrite(pin, value);
}
