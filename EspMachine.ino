//#include <avr/pgmspace.h>
//#include <EEPROMex.h>
/*TODO
 *точение на последнем шаге проверить положение 
 **/
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "BLEDevice.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <XTronical_ST7735.h> // Hardware-specific library
//#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
//#include "SSD1306.h" 
#include "EEPROM.h"
#define strcat_P      strcat

#include "BTSerial.h"

// The remote service we wish to connect to.
//static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
static BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");

// Color definitions
#define  ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF 

//#include "TimerOne.h"
//#define DEBUG
//#define BLE
enum manual_enum { AXIS_X, AXIS_Y,AXIS_Z,NONE};
manual_enum tuning_manual = NONE;
// Color definitions
//#define _cp437 false
#define SERIAL_SPEED  115200
#define MIN_START_SPHINDEL  0
#define STEP_START_SPHINDEL  5
#define STEP_START_SPHINDEL1  1
#define __CS 5
#define __RST 4
#define __DC 2 //a0
#define __LED 22
#define MAX_DISPLAY_LINES 15
#define MAX_DISPLAY_FILES 14
#define TABLE_SIZE 16
//контакты для esp32
#define  A0 21
#define  A1 34
#define  A2 35
#define  A3 32
/*
 * A0=21
 * A1=26
 * A2=35
 * A3=32
 * A4=32
 * A5=33
 * A6=34

 */
/*#define  A0 10
#define  A1 12
#define  A2 14*/


/*-----------------*/
#define DISPLAY_ROT 3 // display rotation - 1 - SD slot down, 3 - SD slot up
/*-----------------*/
#define __SDCS 14
#define MENU_DELAY 150
#define GRBL_RES_PIN 5

#define CHAR_HEIGHT 8
#define CHAR_WIDTH 6

#define CHAR_STOP  0x80 // 128 stop
#define CHAR_PLAY 0x110/*0x7E*//*0x80*/ // 129 play
#define CHAR_PAUSE 0xBA /*0x82*/ // 130 pause
#define CHAR_ARIGHT 0x83 // 131 ->
#define CHAR_ALEFT  0x84 // 132 <-
#define CHAR_AUP  0x85 // 133 up
#define CHAR_ADOWN  0x86 // 134 down
#define CHAR_MARK 0x87 // 135 mark
#define CHAR_STEP 0x88 // 136 step
#define CHAR_DRILL  0x89 // 137 drill
#define CHAR_ZLEFT  0x8A // 138 zero left
#define CHAR_ZRIGHT 0x8B // 139 zero right
#define CHAR_SLE  0x8C // 140 scale left empty
#define CHAR_SRE  0x8D // 141 scale right empty 
#define CHAR_SCE  0x8E // 142 scale center empty
#define CHAR_SLF  0x8F // 143 scale left full
#define CHAR_SRF  0x90 // 144 scale right full
#define CHAR_SCF  0x91 // 145 scale center full
#define CHAR_EPLAY  0x92 // 146 play empty

#define EXEC_PAUSE  0x0 // sleep
#define EXEC_PLAY   0x1 // send commands without pause
#define EXEC_STEP   0x2 // execute step by step
#define EXEC_EXIT   0x20 // exit from execute

//#define CONFIG_MAX 30 //

#define EEPROM_FEED   0
#define EEPROM_START  8
#define EEPROM_PAUSE  9
#define EEPROM_UNLOCK 10
#define EEPROM_TRESHOLD_X 11
#define EEPROM_TRESHOLD_Y 19
#define EEPROM_SHPINDEL  23

#define DEFAULT_FEED 1000
#define FEED_MAX 1250
#define FEED_TEXT_MAX 6
//контакты для 32

//TFT_ILI9163C tft = TFT_ILI9163C(__CS, __DC, __RST);
Adafruit_ST7735 tft = Adafruit_ST7735(__CS,  __DC, __RST);
//TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h


/*class NewSSD:public SSD1306Wire
{
	public:
	NewSSD():SSD1306Wire(0x3c, 4, 15)
	{
    
	}
  
	};
	//NewSSD tft(0x3c, 4, 15);
	NewSSD tft();*/
	File root;
	bool SDINIT = false;
	bool MENU_KEY, OK_KEY, UP_KEY, DOWN_KEY, LOOP_TEMP = false;
	int8_t filecount, pp_menu, skip_page, skip, exec_mode, readcnt;
	long current_line;
	char fname[40];
	char buff[60];
	//char strPos[40];
	//char strPos2[50];
	//String buff
	long feed_dig = 0;
	long feed_safe = 0;
	int treshold_x;
	int treshold_y;
	int shpindel_speed = 30;
	bool IsShpindelStart = false;
	const int X_THRESHOLD_LOW = 500;
	const int X_THRESHOLD_HIGH = 508;
	const int Y_THRESHOLD_LOW = 511;
	const int Y_THRESHOLD_HIGH = 517;


	int dir = 2;
	int fade_width = 0;
	int fade_depth = 0;
	int fade_step = 0;
	int fade_Z = 0;
	int fade_step_Z = 0;
	char stepString[FEED_TEXT_MAX];
	char speedString[FEED_TEXT_MAX];
	#define TARGET_A5_MAX  3
	struct ButtonInfo
	{
		ButtonInfo(byte Pin)
		{
			this->Pin = Pin;
			this->counter = 0;
			this->target = TARGET_A5_MAX;
		}
		byte Pin;
		int counter;
		byte target;
		bool onDown;
		bool onUp;
	};

	#define swapChar(a, b) { char* t = a; a = b; b = t; }

	//TODO можно сделать по маске сэконоить 20 байт
	ButtonInfo btnInfoA5(A5), btnInfoA4(A4), btnInfoA3(A3);

	ButtonInfo* btnArray[] = { &btnInfoA3, &btnInfoA4, &btnInfoA5 };
	//const char acc0[]   = "0 F0\0";
	//const char acc1[]   = "0.01 F100\0";
	//const char acc2[]   = "0.02 F100\0";
	//const char acc3[]   = "0.03 F100\0";
	//const char acc4[]   = "0.04 F100\0";
	//const char acc5[]   = "0.05 F200\0";
	//const char acc6[]   = "0.06 F200\0";
	//const char acc7[]   = "0.07 F200\0";
	//const char acc8[]   = "0.08 F200\0";
	//const char acc9[]   = "0.09 F200\0";
	//const char acc10[]   = "0.1 F400\0";
	//const char acc11[]   = "0.2 F400\0";
	//const char acc12[]   = "0.3 F400\0";
	//const char acc13[]   = "0.4 F400\0";
	//const char acc14[]   = "0.5 F400\0";
	//const char acc15[]   = "1 F600\0";
	//const char acc16[]   = "2 F600\0";
	//const char acc17[]   = "3 F600\0";
	//const char acc18[]   = "4 F600\0";
	//const char acc19[]   = "5 F600\0";
	//const char acc20[]   = "0 F1\0";
	//const char * const acc_table[]   = {acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10, acc11, acc12, acc13, acc14, acc15, acc16, acc17, acc18, acc19, acc20 };

	/*const char acc0[]   = "0\0";
	  const char acc1[]   = "0.01\0";
	  const char acc2[]   = "0.02\0";
	  const char acc3[]   = "0.03\0";
	  const char acc4[]   = "0.04\0";
	  const char acc5[]   = "0.05\0";
	  const char acc6[]   = "0.06\0";
	  const char acc7[]   = "0.07\0";
	  const char acc8[]   = "0.08\0";
	  const char acc9[]   = "0.09\0";
	  const char acc10[]   = "0.1\0";
	  const char acc11[]   = "0.2\0";
	  const char acc12[]   = "0.3\0";
	  const char acc13[]   = "0.4\0";
	  const char acc14[]   = "0.5\0";
	  const char acc15[]   = "1\0";
	  const char acc16[]   = "2\0";
	  const char acc17[]   = "3\0";
	  const char acc18[]   = "4\0";
	  const char acc19[]   = "5\0";
	  const char acc20[]   = "6\0";

	  const char spd0[]   = " F0\0";
	  const char spd1[]   = " F100\0";
	  const char spd2[]   = " F100\0";
	  const char spd3[]   = " F100\0";
	  const char spd4[]   = " F100\0";
	  const char spd5[]   = " F200\0";
	  const char spd6[]   = " F200\0";
	  const char spd7[]   = " F200\0";
	  const char spd8[]   = " F200\0";
	  const char spd9[]   = " F200\0";
	  const char spd10[]   = " F400\0";
	  const char spd11[]   = " F400\0";
	  const char spd12[]   = " F400\0";
	  const char spd13[]   = " F400\0";
	  const char spd14[]   = " F400\0";
	  const char spd15[]   = " F600\0";
	  const char spd16[]   = " F600\0";
	  const char spd17[]   = " F600\0";
	  const char spd18[]   = " F600\0";
	  const char spd19[]   = " F600\0";
	  const char spd20[]   = " F600\0";*/


	  const char acc0[]   = "0.001\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.001 за 0.0016 сек // при скорости 1 время 0.16 сек
	const char acc1[]   = "0.005\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.005 за 0.0031 сек // при скорости 2 время 0.16 сек
	const char acc2[]   = "0.01\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.01 за 0.0062 сек // при скорости 4 время 0.16 сек
	const char acc3[]   = "0.02\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.02 за 0.0125 сек // при скорости 8 время 0.16 сек
	/* --- старые скорости
	const char acc4[]   = "0.04\0";//скорость 100 в 1 мин 1.6мм/с итого 0.04 за 0.025 сек // при скорости 15 время 0.16 сек
	const char acc5[]   = "0.06\0";//скорость 100 в 1 мин 1.6мм/с итого 0.06 за 0.0375 сек // при скорости 24 время 0.16 сек
	const char acc6[]   = "0.07\0";//скорость 100 в 1 мин 1.6мм/с итого 0.07 за 0.04375 сек // при скорости 28 время 0.16 сек
	const char acc7[]   = "0.08\0";//скорость 100 в 1 мин 1.6мм/с итого 0.08 за 0.05 сек // при скорости 31 время 0.16 сек
	--конец старых скоростей 
	*/
	/*новые скорости*/
	const char acc4[]   = "0.01\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.01 за 0.0062 сек // при скорости 4 время 0.16 сек
	const char acc5[]   = "0.02\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.02 за 0.0125 сек // при скорости 8 время 0.16 сек
	const char acc6[]   = "0.04\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.04 за 0.025 сек // при скорости 15 время 0.16 сек
	const char acc7[]   = "0.06\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.06 за 0.0375 сек // при скорости 24 время 0.16 сек
	/*конец новых скорости*/
	const char acc8[]   = "0.09\0"; //скорость 100 в 1 мин 1.6мм/с итого пщ0.09 за 0.056 сек  // при скорости 35 время 0.16 сек
	const char acc9[]   = "0.1\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.1 за 0.0625 сек //  при скорости 39 время 0.16 сек
	const char acc10[]   = "0.2\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.2 за 0.125 сек //  при скорости 78 время 0.16 сек
	//const char acc11[]   = "0.3\0";//скорость 100 в 1 мин 1.6мм/с итого 0.3 за 0.187 сек //  при скорости 117 время 0.16 сек
	const char acc12[]   = "0.4\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.4 за 0.25 сек //  при скорости 156 время 0.16 сек
	//const char acc13[]   = "0.5\0";//скорость 100 в 1 мин 1.6мм/с итого 0.5 за 0.25 сек //  при скорости 195 время 0.16 сек
	const char acc14[]   = "0.6\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.6 за 0.375 сек //  при скорости 234 время 0.16 сек
	//const char acc15[]   = "0.7\0";//скорость 100 в 1 мин 1.6мм/с итого 0.7 за 0.4375 сек //  при скорости 273 время 0.16 сек
	const char acc16[]   = "0.8\0"; //скорость 100 в 1 мин 1.6мм/с итого 0.8 за 0.5 сек //  при скорости 312 время 0.16 сек
	//const char acc17[]   = "0.9\0";//скорость 100 в 1 мин 1.6мм/с итого 1.0 за 0.563 сек //  при скорости 351 время 0.16 сек
	const char acc18[]   = "1\0"; //скорость 100 в 1 мин 1.6мм/с итого 1 за 0.625 сек //  при скорости 390 время 0.16 сек
	//const char acc19[]   = "1.3\0";//скорость 100 в 1 мин 1.6мм/с итого 1 за 0.8125 сек //  при скорости 507 время 0.16 сек
	const char acc20[]   = "2.0\0"; //скорость 100 в 1 мин 1.6мм/с итого 1 за 1,25 сек //  при скорости 781 время 0.16 сек
	const char acc21[]   = "3.0\0"; //скорость 100 в 1 мин 1.6мм/с итого 1 за 1,875 сек //  при скорости 1100 время 0.16 сек
	const char acc22[]   = "4.3\0"; //скорость 100 в 1 мин 1.6мм/с итого 1 за 2,6875 сек //  при скорости 1679 время 0.16 сек

	const char spd0[]   = " F1\0";
	const char spd1[]   = " F2\0";
	const char spd2[]   = " F4\0"; //
	const char spd3[]   = " F8\0";
	/*старые скорости
	const char spd4[]   = " F15\0";
	const char spd5[]   = " F24\0";
	const char spd6[]   = " F28\0";
	const char spd7[]   = " F31\0";
	*/
	/*новые скорости*/
	const char spd4[]   = " F4\0"; //
	const char spd5[]   = " F8\0";
	const char spd6[]   = " F15\0";
	const char spd7[]   = " F24\0";
	/*конец новых скоростей*/
	const char spd8[]   = " F35\0";
	const char spd9[]   = " F39\0";
	const char spd10[]   = " F78\0";
	//const char spd11[]   = " F117\0";
	const char spd12[]   = " F156\0";
	//const char spd13[]   = " F195\0";
	const char spd14[]   = " F234\0";
	//const char spd15[]   = " F273\0";
	const char spd16[]   = " F312\0";
	//const char spd17[]   = " F351\0";
	const char spd18[]   = " F390\0";
	//const char spd19[]   = " F507\0";
	const char spd20[]   = " F781\0";
	const char spd21[]   = " F1100\0";
	const char spd22[]   = " F1679\0";

	const char * const acc_table[] = { acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10, acc12, acc14, acc16, acc18, acc20, acc21, acc22 };
	const char * const speed_table[] = { spd0, spd1, spd2, spd3, spd4, spd5, spd6, spd7, spd8, spd9, spd10, spd12, spd14, spd16, spd18, spd20, spd21, spd22 };
	const char cmd_endl[] = { 0xA };


	const char empty_line[]   = "                          \n\0";
	const char trail[]   = " \0";
	const char zero[]   = " 0 \0";
	const char dir_none[]  = "None\0";
	const char dir_x[]  = "Axis X \0";
	const char dir_y[] = "Axis Y\0";


	const char init_error[]   = "Init SD failed!\0";


	const char text_manual[]   = " MANUAL\0";
	const char text_files[]   = " FILES\0";
	const char text_unlock[]   = " UNLOCK\0";
	const char text_setup[]   = " SETUP\0";
	//const char text_reset[]   = " RESET\0";
	const char text_start_spd[] = " START SPIND\0";
	const char text_stop_spd[]   = " STOP SP\0";
	const char text_exit[]   = " EXIT\0";
	const char text_home[]   = " HOME\0";
	const char text_start[]   = " START\0";
	const char text_facing[]   = " FACING\0";
	const char e[]   = "E\0";


	//facing
	const char text_facing_pos[]   = " SET POS\0";
	const char text_facing_run[]   = " START\0";
	const char text_facing_direction[]   = " DIR:\0";
	const char text_facing_speed[]   = " SPEED:\0";
	const char text_facing_step[]   = " STEP:\0";
	const char text_facing_width[]   = " WIDTH:\0";
	const char text_facing_depth[]   = " DEPTH:\0";
	const char text_facing_z[]   = " Axis Z:\0";
	const char text_facing_step_z[]   = " STEP Z:\0";


	const char text_feedmax[]   = " FEED Max(mm/min):\0";
	const char text_autostart[]   = " AUTO START:\0";
	const char text_pauseerr[]   = " PAUSE ON ERROR:\0";
	const char text_autounlk[]   = " AUTO UNLOCK:\0";
	const char text_calibration[]   = " CALIBRATION:\0";

	const char text_done[]   = "DONE\0";
	const char text_next[]   = " >>>\0";
	const char text_line[]   = "Line:\0";
	const char text_stepsize[]   = " STEP SIZE:\0";

	const char cmd_unlock[]   = "$X\n\0";
	const char cmd_position[]   = "?\n\0";
	//const char cmd_cyclestart[]   = "~\n\0";
	//const char cmd_feedhold[]   = "!\n\0";
	const char cmd_reset[] /* */ = { 0x18, '\n', '\0' };
	const char cmd_upz[]   = "G0Z10F500\0";
	const char cmd_zeroxy[]   = "G1X0Y0F500\0";
	const char cmd_absolute[]   = "G90\0";
	const char cmd_incremental[]   = "G91\0";
	const char cmd_setoffset[]   = "G92 X0 Y0 Z0\0";
	//const char cmd_seek[]   = "G0\0";
	const char cmd_linear[]   = "G1\0";
	const char cmd_homing[]   = "$H\0";

	byte xPin = A1;
	byte yPin = A2;
	byte buttonPin = A0;
	bool isStartPrint = true;
	bool isAlarmExit = false;
	/*
	  const char cfg0[]   = "$0=\0";
	  const char cfg1[]   = "$1=\0";
	  const char cfg2[]   = "$2=\0";
	  const char cfg3[]   = "$3=\0";
	  const char cfg4[]   = "$4=\0";
	  const char cfg5[]   = "$5=\0";
	  const char cfg6[]   = "$6=\0";
	  const char cfg7[]   = "$10=\0";
	  const char cfg8[]   = "$11=\0";
	  const char cfg9[]   = "$12=\0";
	  const char cfg10[]   = "$13=\0";
	  const char cfg11[]   = "$20=\0";
	  const char cfg12[]   = "$21=\0";
	  const char cfg13[]   = "$22=\0";
	  const char cfg14[]   = "$23=\0";
	  const char cfg15[]   = "$24=\0";
	  const char cfg16[]   = "$25=\0";
	  const char cfg17[]   = "$26=\0";
	  const char cfg18[]   = "$27=\0";
	  const char cfg19[]   = "$100=\0";
	  const char cfg20[]   = "$101=\0";
	  const char cfg21[]   = "$102=\0";
	  const char cfg22[]   = "$110=\0";
	  const char cfg23[]   = "$111=\0";
	  const char cfg24[]   = "$112=\0";
	  const char cfg25[]   = "$120=\0";
	  const char cfg26[]   = "$121=\0";
	  const char cfg27[]   = "$122=\0";
	  const char cfg28[]   = "$130=\0";
	  const char cfg29[]   = "$131=\0";
	  const char cfg30[]   = "$132=\0";
	  const char * const cfg_table[]   = {cfg0, cfg1, cfg2, cfg3, cfg4, cfg5, cfg6, cfg7, cfg8, cfg9, cfg10, cfg11, cfg12, cfg13, cfg14, cfg15, cfg16, cfg17, cfg18, cfg19, cfg20, cfg21, cfg22, cfg23, cfg24, cfg25, cfg26, cfg27, cfg28, cfg29, cfg30};
	  */

	const char axis0[]  = " X\0";
	const char axis1[]  = " Y\0";
	const char axis2[]  = " Z\0";
	const char axis3[]  = " F\0";
	const char * const axis_label[] = { axis0, axis1, axis2, axis3 };


	const char shpindel_text[]   = " SPIND SPEED: \0";
	/*
	  //joystick defines
	  #define joyPinX 1                 // X pin
	  #define joyPinY 0                 // Y pin
	  #define joyPinZ A3                 // Z pin
	  int xmin = 000;                  // X minimum value
	  int xmax = 1024;                  // X maximum value
	  int ymin = 000;                  // Y minimum value
	  int ymax = 1024;                  // Y maximum value
	  int xzero = 0;                   // the first X value (don't touch the joystick at startup!)
	  int yzero = 0;                   // the first Y value
	  int segs = 10;                   // number of intervalls to consider par half direction
	  double zerohold = 4;             // number of intervalls to consider as "zero position"
	  int x = 0;
	  int y = 0; 
	  static boolean joyClick = false;
	  */

	//encoder defines
	#define encoder0PinA  25 
	#define encoder0PinB  27
	#define encoder0PinC  15
	int8_t  encoder0Pos = 1;   // a counter for the dial
	int8_t  lastReportedPos = 1;    // change management
	static boolean rotating = false;     // debounce management

	// interrupt service routine vars
	boolean A_set = false;
	boolean B_set = false;
	static boolean encClick = false;

	//assistatnt procedures

	void printSerialString(const char *string)
	{
		 //print to serial from  
	  /*while (pgm_read_byte(string) != '\0') {
	    SerialBT.write(pgm_read_byte_near(string));
	    string++;
	  }*/
	  SerialBT.print(string);
		return;
		while (*string != '\0')
		{
			SerialBT.write(*string);
			Serial.println(*string);
			string++;
		}
	}
	void printTFTString(char *string)
	{
		 //print to lcd from  
	  while(pgm_read_byte(string) != '\0')
		{
			tft.write(pgm_read_byte_near(string));
			string++;
		}
	}

	void printColorString(char* text, int x, int y, byte Color)
	{
		tft.setTextColor(Color, ST7735_BLACK);
		tft.setCursor(x, y);
		printTFTString((char*)text);
	}


	//void printString(char *string) {
	//  while (*string != '\0') {
	//    tft.write(*string);
	//    string++;
	//  }
	//}

	void str_print(char s, byte size) //print to lcd cursor for menu choice
	{
		unsigned char x;
		tft.setTextSize(size);
		for (x = 0; x <= MAX_DISPLAY_LINES; x++)
		{
			tft.setCursor(0, x * (size * CHAR_HEIGHT));
			tft.write(32);
		}
		;
		tft.setCursor(0, s * (size * CHAR_HEIGHT));  //MAX_DISPLAY_LINES
		tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
		tft.write(CHAR_PLAY);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		tft.setTextSize(1);
	}

	/*void gauge(int8_t x, int8_t y, int8_t width, int8_t pos, int8_t scale) { //print gauge to display
	  tft.setCursor(x, y);
	  pos = pos / scale;
	  for (int8_t wdt = 0; wdt <= width; wdt++) {
	    int8_t center = (width / 2);
	    int8_t rpos = pos + center;
	    if (pos < (0 - center)) {
	      pos = 0 - center;
	  };
	  if (pos > center) {
	    pos = center;
	};
	if (rpos < 0) {
	  rpos = 0;
	}
	if (rpos > width) {
	  rpos = width;
	}
	if ((wdt == 0) && (rpos > 0)) {
	  tft.write(CHAR_SLE);
	} else if ((wdt == 0) && (rpos == 0)) {
	  tft.write(CHAR_SLF);
	} else if ((wdt == width) && (rpos < width)) {
	  tft.write(CHAR_SRE);
	} else if ((wdt == width) && (rpos == width)) {
	  tft.write(CHAR_SRF);
	} else if (wdt == center) {
	  tft.write(CHAR_SCF);
	} else {
	  if (pos > 0) {
	    if (wdt < center) {
	      tft.write(CHAR_SCE);
		} else if ((wdt > center) && (wdt <= rpos)) {
		  tft.write(CHAR_SCF);
		} else if (wdt > rpos) {
		  tft.write(CHAR_SCE);
		};
	  } else if (pos < 0) {
	    if (wdt > center) {
	      tft.write(CHAR_SCE);
		} else if ((wdt < center) && (wdt >= rpos)) {
		  tft.write(CHAR_SCF);
		} else if (wdt < rpos) {
		  tft.write(CHAR_SCE);
		};
	  } else tft.write(CHAR_SCE);
	}
  }
}*/
	void setShpindelSpeed(int speed)
	{
		if (speed > 0)
		{
			SerialBT.print("M3 S\0");
			SerialBT.print(speed);
		}
		else
			SerialBT.print("M5 S0\0");
		SerialBT.print(cmd_endl); //конец команды
	}
	void clearSerial()
	{
		//  while (SerialBT.available() > 0) 
		//  {        
		//    SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
		//    delay(50);
		//  }
		  while(SerialBT.available() > 0)
		{
			SerialBT.read();
			delay(1);
		}
	}
	void startShpindel()
	{ 
		//if  (shpindel_speed<=MIN_START_SPHINDEL)
		//  setShpindelSpeed(shpindel_speed);
		//else
		//{
		  int temp_speed = MIN_START_SPHINDEL;
		while (temp_speed < shpindel_speed) 
		{
			setShpindelSpeed(temp_speed);
			//SerialBT.flush();
			temp_speed += STEP_START_SPHINDEL1;
			//       clearSerial();
			       delay(50);
		}
		//}
		setShpindelSpeed(shpindel_speed);
		//   SerialBT.flush();
		//   clearSerial();
		    //SerialBT.flush();
	}

	void stopShpindel()
	{
		setShpindelSpeed(0);
	}

	//menu procedures
	void main_screen(void) //
	{
		tft.fillScreen(ST7735_BLACK);
		tft.setTextSize(2);
		tft.setCursor(0, 0);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		printTFTString((char*)text_manual);
		tft.println();
		printTFTString((char*)text_files);
		tft.println();
		printTFTString((char*)text_unlock);
		tft.println();
		printTFTString((char*)text_setup);
		tft.println();
		if (!IsShpindelStart)
			printTFTString((char*)text_start_spd);
		else
			printTFTString((char*)text_stop_spd);
		tft.println();
		printTFTString((char*)text_facing);
		tft.println();
		printTFTString((char*)text_exit);
		tft.setTextSize(1);
	}

	//void pro_screen(void) //
	//{
	//  tft.setTextSize(1);
	//  tft.fillScreen(ST7735_BLACK);
	//  tft.setCursor(0, 0);
	//  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
	//  printTFTString((char*)text_facing);
	//  tft.println();
	//  printTFTString((char*)text_exit);
	//}
	void printValue(int Value, int div, bool IsSerial)
	{
		if (!IsSerial)
		{
			if (Value < 0)
				tft.print('-');
			Value = abs(Value);
			tft.print(abs(Value) / div);
  
			tft.print('.');
			if (Value % div < 10)
				tft.print('0');
			tft.print(Value % div);
			tft.println("  ");
		}
		else
		{
			if (Value < 0)
				Serial.print('-');
			Value = abs(Value);
			Serial.print(Value / div);
			Serial.print('.');
			if (Value % div < 10)
				Serial.print('0');
			Serial.print(Value % div);
			//Serial.println("  ");
		}
		/*Serial.println(Value);
		Serial.println(Value/delimiter);*/
	}

	//void formatZ(int value,int div, char* output)
	//{
	//  char temp[FEED_TEXT_MAX];
	//  itoa(value, temp, 10);
	//  if ((value >= 0) && (value < 10))
	//  {
	//    memset(output, 0, sizeof(output));
	//    strcpy(output, "0.0\0");
	//    strcat(output, temp);
	//  }
	//  else if ((value >= 10) && (value < 100))
	//  {
	//    memset(output, 0, sizeof(output));
	//    strcpy(output, "0.\0");
	//    strcat(output, temp);
	//  }
	//  else if (value >= 100)
	//  {
	//    memset(output, 0, sizeof(output));
	//    itoa(value / 100, temp, 10);
	//    strcpy(output, temp);
	//  }
	//}

	void facing_screen()
	{
		tft.setTextSize(1);
		tft.fillScreen(ST7735_BLACK);
		tft.setCursor(0, 0);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		printTFTString((char*)text_facing_direction);
		tft.println(getDirFromDirection(dir));
		printTFTString((char*)text_facing_pos);
		tft.println();
		printTFTString((char*)text_facing_width);
		tft.println(fade_width);
		printTFTString((char*)text_facing_depth);
		tft.println(fade_depth);
		printTFTString((char*)text_facing_step);
		tft.println(stepString);
		printTFTString((char*)text_facing_speed);
		tft.println(speedString);
		printTFTString((char*)text_facing_z);
		printValue(fade_Z, 100, false);
		printTFTString((char*)text_facing_step_z);
		//tft.println(fade_step_Z);
		printValue(fade_step_Z, 100, false);
		printTFTString((char*)text_facing_run);
		tft.println();
		printTFTString((char*)text_exit);
		tft.setTextSize(1);
	}


	void main_menu(void) //
	{
		Serial.flush();
		isStartPrint = false;
		MENU_KEY = false;
		main_screen();
		pp_menu = 0;
		lastReportedPos = encoder0Pos = 0;
		str_print(pp_menu, 2);
		do
		{
			//    int buttonState1 = digitalRead(A4);//кнопка джойстика
			//    Serial.println(buttonState1);
			      //Serial.println(State);
			      //Serial.println(digitalRead(A5));
			  read_keys();
			if (UP_KEY)
			{
				if (pp_menu > 0)
				{
					pp_menu--;
				}
				else
				{
					pp_menu = 6;
				}
				str_print(pp_menu, 2);
			}
			;
			if (DOWN_KEY)
			{
				if (pp_menu < 6)
				{
					pp_menu++;
				}
				else
				{
					pp_menu = 0;
				}
				str_print(pp_menu, 2);
			}
			;
			if (OK_KEY)
			{
				if (pp_menu == 0) manual_menu();
				if (pp_menu == 1) files_menu();
				if (pp_menu == 2)
				{
					current_line = 0;
					printSerialString((char*)cmd_unlock);
					break;
				}
				if (pp_menu == 3)
				{
					setup_menu();
				}
				if (pp_menu == 4)
				{
					//reset
					//        current_line = 0;
					//        digitalWrite(GRBL_RES_PIN, HIGH);
					//        delay(200);
					//        digitalWrite(GRBL_RES_PIN, LOW);
					     // printSerialString(shpindel_speed);
					      //Serial.write(0x0A);//конец команды
					        IsShpindelStart = !IsShpindelStart;
					main_screen(); //надо обновить меню
					if (!IsShpindelStart)
						startShpindel();
					else
						stopShpindel();
       
					break;
				}
				if (pp_menu == 5)
				{
					facing_menu();
					//pro_menu();
				}
				if (pp_menu == 6)
				{
					SerialBT.flush();
					break;
				}
			}
			;
			//delay(MENU_DELAY);
			UpdateButton();
			if (btnInfoA5.onDown)
			{
				btnInfoA5.onDown = false;
				go_home();
			}
			if (btnInfoA4.onDown)
			{
				btnInfoA4.onDown = false;
				printSerialString((char*)cmd_homing);
			}
			if (btnInfoA3.onDown)
			{
				btnInfoA3.onDown = false;
				printSerialString((char*)cmd_setoffset); //Отправляем G91
				SerialBT.print(cmd_endl); //конец команды
				//Serial.println("btnInfoA3.onDown");
			}
		} while (!MENU_KEY);
		MENU_KEY = false;
		current_line = 0;
	}
	;


	void setup_screen(void)
	{
		tft.fillScreen(ST7735_BLACK);
		tft.setTextSize(1);
		tft.setCursor(0, 0);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		printTFTString((char*)text_feedmax);
		tft.println(EEPROM.readInt(EEPROM_FEED));
		printTFTString((char*)text_autostart);
		tft.println(EEPROM.readInt(EEPROM_START));
		printTFTString((char*)text_pauseerr);
		tft.println(EEPROM.read(EEPROM_PAUSE));
		printTFTString((char*)text_autounlk);
		tft.println(EEPROM.readByte(EEPROM_UNLOCK));
		printTFTString((char*)text_calibration);
		tft.print((int)EEPROM.readInt(EEPROM_TRESHOLD_X));
		tft.print(" ");
		tft.println((int)EEPROM.readInt(EEPROM_TRESHOLD_Y));
		//tft.println();
		printTFTString((char*)shpindel_text);
		tft.println(shpindel_speed);
		printTFTString((char*)text_exit);
		tft.setTextSize(1);
	}

	/*
	  #define CHAR_HEIGHT 8
	  #define CHAR_WIDTH 6
	  #define EEPROM_FEED   0
	  #define EEPROM_START  8
	  #define EEPROM_UNLOCK 9
	  */

	void setup_param(int8_t param)
	{
		unsigned int  min, max = 0;
		int temp;
		int temp_x;
		int temp_y;
		int treshold_x;
		tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
		if (param == 0)
		{
			temp = EEPROM.readInt(EEPROM_FEED);
			min = 0; max = FEED_MAX;
		}
		else if (param == 1)
		{
			temp = EEPROM.read(EEPROM_START);
			min = 0; max = 1;
		}
		else if (param == 2)
		{
			temp = EEPROM.readByte(EEPROM_PAUSE);
			min = 0; max = 1;
		}
		else if (param == 3)
		{
			temp = EEPROM.readByte(EEPROM_UNLOCK);
			min = 0; max = 1;
		}
		else if (param == 4)
		{
			treshold_x = EEPROM.readInt(EEPROM_TRESHOLD_X);
			temp_x = treshold_x;
			treshold_y = EEPROM.readInt(EEPROM_TRESHOLD_Y);
			temp_y = treshold_y;
		}
		else if (param == 5)
		{
			temp = EEPROM.readInt(EEPROM_SHPINDEL);
		}

		do
		{
			if (param == 0)
			{
				tft.setCursor(18 * CHAR_WIDTH, 0 * CHAR_HEIGHT);
			}
			else if (param == 1)
			{
				tft.setCursor(12 * CHAR_WIDTH, 1 * CHAR_HEIGHT);
			}
			else if (param == 2)
			{
				tft.setCursor(16 * CHAR_WIDTH, 2 * CHAR_HEIGHT);
			}
			else if (param == 3)
			{
				tft.setCursor(13 * CHAR_WIDTH, 3 * CHAR_HEIGHT);
			}
			else if (param == 4)
			{
				tft.setCursor(13 * CHAR_WIDTH, 4 * CHAR_HEIGHT);
			}
			else if (param == 5)
			{
				clearLine(14 * CHAR_WIDTH, 5 * CHAR_HEIGHT);
				tft.setCursor(14 * CHAR_WIDTH, 5 * CHAR_HEIGHT);
			}
			if (param != 4)
			{
				tft.print(temp);
				printTFTString((char*)trail);
				tft.println();
			}
			else
			{
				 //выводим параметры калибровки
			  tft.print(temp_x);
				printTFTString((char*)trail);
				tft.print(temp_y);
				printTFTString((char*)trail);
				tft.println();
			}
			if (param == 0)
			{
				if (encoder0Pos > 4)encoder0Pos = 4;
				if (encoder0Pos < -4)encoder0Pos = -4;
				if ((encoder0Pos > 1) || (encoder0Pos < 1))
				{
					temp += encoder0Pos * 5;
				}
				else
				{
					temp += encoder0Pos;
				}
			}
			else if (param == 1)
			{
				if (encoder0Pos > 1)encoder0Pos = 1;
				if (encoder0Pos < 0)encoder0Pos = 0;
				temp = encoder0Pos;
			}
			else if (param == 2)
			{
				if (encoder0Pos > 1)encoder0Pos = 1;
				if (encoder0Pos < 0)encoder0Pos = 0;
				temp = encoder0Pos;
			}
			else if (param == 3)
			{
				if (encoder0Pos > 1)encoder0Pos = 1;
				if (encoder0Pos < 0)encoder0Pos = 0;
				temp = encoder0Pos;
			}
			else if (param == 4)
			{
				if (encoder0Pos > 1)encoder0Pos = 1;
				if (encoder0Pos < 0)encoder0Pos = 0;
				if (encoder0Pos)
				{
					temp_x = analogRead(xPin);
					temp_y = analogRead(yPin);
				}
				else
				{
					temp_x = treshold_x;
					temp_y = treshold_y;
				}
			}
			if (param == 5)
			{
				//скорость шпинделя 
				if((temp + encoder0Pos) <= 30)
				     temp += encoder0Pos;
				else
				  temp += encoder0Pos*STEP_START_SPHINDEL;
				if (temp < 0)
					temp = 0;
				else if (temp > 1000)
					temp = 1000;
				if (IsShpindelStart)
					setShpindelSpeed(temp);
				encoder0Pos = 0;
			}
			delay(MENU_DELAY * 2);
			encRead(); if (encClick) break;
		} while (true);
		if (param == 0)
		{
			EEPROM.writeInt(EEPROM_FEED, temp);
		}
		else if (param == 1)
		{
			EEPROM.writeByte(EEPROM_START, (byte)temp);
		}
		else if (param == 2)
		{
			EEPROM.writeByte(EEPROM_PAUSE, (byte)temp);
		}
		else if (param == 3)
		{
			EEPROM.writeByte(EEPROM_UNLOCK, (byte)temp);
		}
		else if (param == 4)
		{
			EEPROM.writeInt(EEPROM_TRESHOLD_X, temp_x);
			EEPROM.writeInt(EEPROM_TRESHOLD_Y, temp_y);
		}
		else if (param == 5)
		{
			shpindel_speed = temp;
			EEPROM.writeInt(EEPROM_SHPINDEL, temp);
			//setShpindelSpeed(shpindel_speed);
		}
		EEPROM.commit();
	}


	const char* getDirFromDirection(int direction)
	{
		if (direction == 0)
			return dir_none;
		if (direction == 1)
			return dir_x;
		if (direction == 2)
			return dir_y;
	}

	void formatString(int value, char* output)
	{
		char temp[FEED_TEXT_MAX];
		itoa(value, temp, 10);
		if ((value >= 0) && (value < 10))
		{
			memset(output, 0, sizeof(output));
			strcpy(output, "0.0\0");
			strcat(output, temp);
		}
		else if ((value >= 10) && (value < 100))
		{
			memset(output, 0, sizeof(output));
			strcpy(output, "0.\0");
			strcat(output, temp);
		}
		else if (value >= 100)
		{
			memset(output, 0, sizeof(output));
			itoa(value / 100, temp, 10);
			strcpy(output, temp);
		}
	}

	void formatSpeed(int value, char* output)
	{
		itoa(value, output, 10);
	}

	int getValueByEncoder(int value)
	{
		int temp = value;
		if (encoder0Pos > 28)encoder0Pos = 28;
		if (encoder0Pos < 0)encoder0Pos = 28;
		if ((encoder0Pos > 0) && (encoder0Pos <= 9))
			temp = encoder0Pos;
		else if ((encoder0Pos > 9) && (encoder0Pos <= 19))
			temp = (encoder0Pos - 9) * 10;
		else if ((encoder0Pos > 19) && (encoder0Pos <= 28))
			temp = (encoder0Pos - 18) * 100;
		return temp;
        
	}
	void facing_param(int8_t param)
	{
		unsigned int  min, max = 0;
		int temp;
		int temp_x;
		int temp_y;
		int treshold_x;
		tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
		if (param == 0)
		{
			//    temp = EEPROM.readInt(EEPROM_FEED);
			//    min = 0; max = FEED_MAX;
			temp = dir;
		}
		else if (param == 1)
		{
			//    temp = EEPROM.read(EEPROM_START);
			//    min = 0; max = 1;
		}
		else if (param == 2)
		{
			//    temp = EEPROM.read(EEPROM_UNLOCK);
			//    min = 0; max = 1;
			temp = fade_width;
		}
		else if (param == 3)
		{
			//    treshold_x = EEPROM.readInt(EEPROM_TRESHOLD_X);
			//    temp_x = treshold_x;
			//    treshold_y = EEPROM.readInt(EEPROM_TRESHOLD_Y);
			//    temp_y = treshold_y;
			temp = fade_depth;
		}
		else if (param == 6)
		{
			temp = fade_Z;
		}
		else if (param == 7)
		{
			temp = fade_step_Z;
		}
		else if (param == 8)
		{
			printColorString((char*)text_facing_run, 0, 64, ST7735_GREEN);
			run_facing();
			printColorString((char*)text_facing_run, 0, 64, ST7735_WHITE);
		}
		do
		{
			if (param == 0)
			{
				tft.setCursor(5 * CHAR_WIDTH, 0 * CHAR_HEIGHT);
			}
			else if (param == 1)
			{
				tft.setCursor(4 * CHAR_WIDTH, 1 * CHAR_HEIGHT);
			}
			else if (param == 2)
			{
				tft.setCursor(7 * CHAR_WIDTH, 2 * CHAR_HEIGHT);
			}
			else if (param == 3)
			{
				tft.setCursor(7 * CHAR_WIDTH, 3 * CHAR_HEIGHT);
			}
			else if (param == 4)
			{
				tft.setCursor(7 * CHAR_WIDTH, 4 * CHAR_HEIGHT);
			}
			else if (param == 5)
			{
				tft.setCursor(7 * CHAR_WIDTH, 5 * CHAR_HEIGHT);
			}
			else if (param == 6)
			{
				tft.setCursor(8 * CHAR_WIDTH, 6 * CHAR_HEIGHT);
			}
			else if (param == 7)
			{
				tft.setCursor(8 * CHAR_WIDTH, 7 * CHAR_HEIGHT);
			}
			if (param == 0)
			{
				tft.print(getDirFromDirection(dir));
				printTFTString((char*)trail);
				tft.println();
			}
			else
			{
				if (param != 1)
				{
					if (param == 4)
					{
						formatString(temp, stepString);
						tft.print("      ");
						tft.setCursor(6 * CHAR_WIDTH, 4 * CHAR_HEIGHT); //TODO попробовать убрать мигание
						tft.print(stepString);
						printTFTString((char*)trail);
						tft.println();
					}
					else if (param == 5)
					{
						//formatString(temp,speedString);
						formatSpeed(temp, speedString);
						tft.print("      ");
						tft.setCursor(7 * CHAR_WIDTH, 5 * CHAR_HEIGHT);
						tft.print(speedString);
						printTFTString((char*)trail);
						tft.println();
					}
					else if ((param == 7) || (param == 6))
					{
						printValue(temp, 100, false);
					}
					else if (param == 8)
					{
						return;
					}
					else
					{
						tft.print(temp);
						printTFTString((char*)trail);
						//tft.println();
					}
				}
			}
			if (param == 1)
			{
				printColorString((char*)text_facing_pos, 0, 8, ST7735_GREEN);
				go_manual_stick();
				printColorString((char*)text_facing_pos, 0, 8, ST7735_WHITE);
				//      serial_flush();
				//      getCurrentPos(buff);
				return;
				//    } else if (param == 6) {
				//      //Serial.print("go_manual_stick()");
				//      //go_manual_stick(dir);
				//      return;
			}
			else if (param == 0)
			{
				//Serial.print("encoder0Pos > 3 =");
				//Serial.println(encoder0Pos);
				if(encoder0Pos > 1)encoder0Pos = 0;
				if (encoder0Pos < 0)encoder0Pos = 1;
				dir = encoder0Pos + 1;
			}
			else if (param == 2)
			{
				if (encoder0Pos > 100)encoder0Pos = 100;
				if (encoder0Pos < -100)encoder0Pos = -100;
				temp = encoder0Pos;
			}
			else if (param == 3)
			{
				if (encoder0Pos > 100)encoder0Pos = 100;
				if (encoder0Pos < -100)encoder0Pos = -100;
				temp = encoder0Pos;
			}
			else if ((param == 4) || (param == 5))
			{
				temp = getValueByEncoder(temp);
			}
			else if ((param == 6) || (param == 7))
			{
				if (encoder0Pos > 10000)encoder0Pos = 10000;
				if (encoder0Pos < -10000)encoder0Pos = -10000;
				temp = encoder0Pos;
			}
			delay(MENU_DELAY * 2);
			encRead(); if (encClick) break;
		} while (true);
		if (param == 2)
		{
			//EEPROM.update(EEPROM_UNLOCK, (byte)temp);
			fade_width = temp;
		}
		else if (param == 3)
		{
			fade_depth = temp;
		}
		else if (param == 4)
		{
			fade_step = temp;
		}
		else if (param == 6)
		{
			fade_Z = temp;
		}
		else if (param == 7)
		{
			fade_step_Z = temp;
		}
	}

	void setup_menu(void)
	{
		SerialBT.flush();
		setup_screen();
		pp_menu = 0;
		lastReportedPos = encoder0Pos = 0;
		str_print(pp_menu, 1);
		do
		{
			read_keys();
			if (UP_KEY)
			{
				if (pp_menu > 0)
				{
					pp_menu--;
				}
				else
				{
					pp_menu = 6;
				}
				str_print(pp_menu, 1);
			}
			;
			if (DOWN_KEY)
			{
				if (pp_menu < 6)
				{
					pp_menu++;
				}
				else
				{
					pp_menu = 0;
				}
				str_print(pp_menu, 1);
			}
			;
			if (OK_KEY)
			{
				if (pp_menu == 0) setup_param(0);
				if (pp_menu == 1) setup_param(1);
				if (pp_menu == 2) setup_param(2);
				if (pp_menu == 3) setup_param(3);
				if (pp_menu == 4) setup_param(4);
				if (pp_menu == 5) setup_param(5);
				if (pp_menu == 6)
				{
					break;
				}
				setup_screen();
				str_print(pp_menu, 1);
			}
			;
			delay(MENU_DELAY);
		} while (true);
		main_screen();
		pp_menu = 0;
		str_print(pp_menu, 2);
	}

	//const int wdt=42;
	void manual_screen()
	{
		tft.fillScreen(ST7735_BLACK);
		//  tft.setCursor(0, 0 * CHAR_HEIGHT);
		//  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		//  printTFTString((char*)text_home);

		/*tft.setCursor(0, 0 * CHAR_HEIGHT);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		printTFTString((char*)text_start);*/

		//  tft.setCursor(0, 2 * CHAR_HEIGHT);
		//  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		//  printTFTString((char*)text_reset);

		tft.setCursor(0, 0 * CHAR_HEIGHT);
		tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
		printTFTString((char*)text_exit);

		//tft.drawRect(80-wdt, 64-wdt, wdt*2, wdt*2, ST7735_WHITE);
	}


	void go_home(void)
	{
		printSerialString((char*)cmd_absolute);
		SerialBT.print(cmd_endl);
		printSerialString((char*)cmd_upz);
		SerialBT.print(cmd_endl);
		printSerialString((char*)cmd_zeroxy);
		SerialBT.print(cmd_endl);
	}

	//void move_menu(char axis) {
	//  printSerialString((char*)cmd_incremental);
	//  Serial.write(0x0A);
	//  do {
	//    tft.setCursor(0, axis * CHAR_HEIGHT);
	//    if (axis == 0) {
	//      tft.setTextColor(ST7735_RED, ST7735_BLACK);
	//    } else if (axis == 1) {
	//      tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
	//    } else if (axis == 2) {
	//      tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
	//    }
	//    printTFTString((char*)pgm_read_word(&(axis_label[axis])));
	//    if (encoder0Pos > 20)encoder0Pos = 20;
	//    if (encoder0Pos < -20)encoder0Pos = -20;
	//    if (lastReportedPos > 20)lastReportedPos = 20;
	//    if (lastReportedPos < -20)lastReportedPos = -20;
	//    gauge(12, axis * CHAR_HEIGHT, 20, lastReportedPos, 2);
	//    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
	//    printTFTString((char*)trail);
	//    tft.print(abs(lastReportedPos));
	//    printTFTString((char*)trail);
	//    tft.setCursor(10, 50);
	//    printTFTString((char*)text_stepsize);
	//
	//    if (lastReportedPos != 0) {
	//      printSerialString((char*)cmd_linear);
	//      printSerialString((char*)pgm_read_word(&(axis_label[axis])));
	//    }
	//    if (lastReportedPos < 0) {
	//      tft.write(0x2D);
	//      if (lastReportedPos != 0) Serial.write(0x2D);
	//    }
	//    if (lastReportedPos != 0) {
	//      char pos = abs(lastReportedPos);
	//      printSerialString((char*)pgm_read_word(&(acc_table[pos])));
	//      Serial.write(0x0A);
	//      printTFTString((char*)pgm_read_word(&(acc_table[pos])));
	//    }
	//
	//    printTFTString((char*)empty_line);
	//    if (lastReportedPos != 0) {
	//      while (true) {
	//        if (Serial.available())
	//        {
	//            break;
	//        }
	//        encRead(); if (encClick) break;
	//      }
	//      if (Serial.available()) {
	//        memset(buff, 0, sizeof(buff));
	//        readcnt = Serial.readBytesUntil(0x0A, buff, sizeof(buff));
	//        if (readcnt > 0) {
	//          for (int8_t pe = 0; pe < sizeof(buff); pe++) {
	//            if ((buff[pe] == 0x0D) || (buff[pe] == 0x0A)) {
	//              buff[pe] = 0x00;
	//              break;
	//            }
	//          }
	//          encRead(); if (encClick) break;
	//          //if(Serial.peek()==0x0A) Serial.read();
	//          //buff[readcnt] = 0;
	//          //tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
	//          /*if (strncmp(buff, "ok", 2)) {
	//            tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
	//            tft.print(buff);
	//          }*/
	//          if (strncmp(buff, "error", 5) == 0) {
	//            tft.setCursor(0, 60);
	//            printTFTString((char*)empty_line);
	//            tft.setCursor(0, 60);
	//            tft.setTextColor(ST7735_RED, ST7735_BLACK);
	//            tft.print(buff);
	//          }
	//        }
	//      }
	//    }
	//    //delay(MENU_DELAY);
	//    encRead(); if (encClick) break;
	//  } while (true);
	//  encClick = false;
	//  encoder0Pos = lastReportedPos = 0;
	//  delay(MENU_DELAY);
	//}

	#define NO_STICK_MOVE 100

	void printDirection(int direction, int axis, int pos, char* output)
	{
		if (direction != 0)
		{
			SerialBT.print((axis_label[axis]));
			strcat_P(output, (axis_label[axis]));
			if (direction < 0)
			{
				SerialBT.write(0x2D); // отправляем '-'
				strcat(output, "-");
			}
			SerialBT.print(acc_table[pos]);
			strcat_P(output, acc_table[pos]);
		}
	}

	void printSpeed(int pos, char* output)
	{
		SerialBT.print(speed_table[pos]);
		//SerialBT.print(cmd_endl);//конец команды
		SerialBT.print(cmd_endl); //конец команды
		strcat_P(output, speed_table[pos]);
	}

	void clearLine(int posX, int posY)
	{
		tft.setCursor(posX, posY);
		//tft.print("                           ");
		printTFTString((char*)empty_line);
	}

	bool waitCommand(bool isPrint/* = false*/)
	{
		serial_flush();

		int currentline = 10;
		while (true)
		{
			byte avail_idx = 0;
			printSerialString(cmd_position);
			while (!SerialBT.available() && avail_idx < 200)
			{
				delay(50);
				avail_idx++;
				if (!digitalRead(A5))//выход по кнопке A5
				{
					isAlarmExit = true;
					Serial.println("exit A5");
				}
			}
			if (avail_idx >= 200)
			{
				clearLine(0, currentline * CHAR_HEIGHT);
				tft.setCursor(0, currentline * CHAR_HEIGHT);
				tft.println("Error: Serial not available");
				delay(1000);
				return false;
			}
			memset(buff, 0, sizeof(buff));
			Serial.println("waiting readBytesUntil");
			readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
			while (readcnt > 0)
			{
				buff[readcnt] = 0;

				char* idle = strstr(buff, "Idle");
				if (isPrint)
				{
					clearLine(0, currentline * CHAR_HEIGHT);
					tft.setCursor(0, currentline * CHAR_HEIGHT);
					tft.println(buff);
					currentline++;
					if (currentline > 15)
						currentline = 10;
				}
				if (idle != NULL)
				{
					Serial.println("idle != NULL");
					return true;
				}
				delay(50);
				if (!digitalRead(A5))
					isAlarmExit = true;
				avail_idx++;
				readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
			}
		}
		return false;
	}

	bool getPosition(char* output, bool isPrint /*= false*/)
	{
		serial_flush();

		int currentline = 10;

		while (true)
		{
			byte avail_idx = 0;
			printSerialString(cmd_position);
			while (!SerialBT.available() && avail_idx < 200)
			{
				delay(50);
				avail_idx++;
			}
			if (avail_idx >= 200)
			{
				clearLine(0, currentline * CHAR_HEIGHT);
				tft.setCursor(0, currentline * CHAR_HEIGHT);
				tft.println("Error: Serial not available");
				delay(1000);
				return false;
			}
			memset(buff, 0, sizeof(buff));
			readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
			while (readcnt > 0)
			{
				buff[readcnt] = 0;

				//char* token = strstr(buff, "WPos:");
				char* token = strstr(buff, "MPos:");
				if (isPrint)
				{
					clearLine(0, currentline * CHAR_HEIGHT);
					tft.setCursor(0, currentline * CHAR_HEIGHT);
					tft.println(buff);
					currentline++;
					if (currentline > 11)
						currentline = 10;
				}
				if (token != NULL)
				{
					int count = strlen(buff) - (token - buff) - 1 - 5;
					//            Serial.println(count);
					//            Serial.println(strlen(buff));
					strncpy(output, token + 5, count);
					output[count] = 0;
					return true;
				}
				delay(50);
				avail_idx++;
				readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
			}
		}
		return false;
	}

	bool waitCommandWithExit()
	{
		if (!waitCommand(true) || isAlarmExit)
		{
			isAlarmExit = false;
			return false;
		}
		return true;
	}
	void run_facing()
	{
		if ((fade_width == 0) /*|| (fade_depth == 0)*/)
			return;
		char* axisX = (char*)axis0;
		char* axisY = (char*)axis1;

 
        
		if (dir == 1)
			swapChar(axisX, axisY);
 
		//printSerialString((char*)cmd_incremental);//Отправляем G91
		SerialBT.print(cmd_incremental);
  
  
		//ESP_LOGE("facing",(char*)cmd_incremental);
		Serial.print("facing");
		//Serial.println(cmd_incremental);
		//SerialBT.print(cmd_endl);//конец команды
		//int count = abs(fade_depth * 100) / fade_step;
		int count = abs(fade_depth * 100) / fade_step;
		int countZ = 0;
		if (fade_step_Z != 0)
			countZ = abs(fade_Z) / fade_step_Z;
		for (int j = -1; j < countZ; j++)
		{
			for (int i = -1; i < count; i++)
			{
				//прямое направление
				//Serial.print("cmd_linear");
				printSerialString((char*)cmd_linear);
				printSerialString(axisX);
				SerialBT.print(fade_width);
				SerialBT.print(" F\0");
				SerialBT.print(speedString);
				SerialBT.print(cmd_endl); //конец команды
				//Serial.println("before waiting");
				if(!waitCommandWithExit())
				{
					isAlarmExit = false;
					return;
				}
     
  
				//обратное направление
				printSerialString((char*)cmd_linear);
				printSerialString(axisX);
				SerialBT.print(-fade_width);
				SerialBT.print(" F\0");
				SerialBT.print(speedString);
				SerialBT.print(cmd_endl); //конец команды
				//      if (!waitCommandWithExit())
				//      {
				//        isAlarmExit = false;
				//        return;
				//      }
  
				      //ось Y
				      if(i < count - 1)
				{
					printSerialString((char*)cmd_linear);
					printSerialString(axisY);
					if (fade_depth < 0)
						SerialBT.write(0x2D);
					//SerialBT.print("0.1");
					SerialBT.print(stepString);
					//printValue(fade_step_Z,100,true);
					SerialBT.print(" F\0");
					SerialBT.print(speedString);
					SerialBT.print(cmd_endl); //конец команды
					//        if (!waitCommand() || isAlarmExit)
					//         {
					//           isAlarmExit = false;
					//           return;
					//         }
				}
  
				//прервать точение ? 
				//      tft.print("U");
				//      UpdateButton();
  
  
			}// конец цикла 2D
			//вернуться обратно
			printSerialString((char*)cmd_linear);
			printSerialString(axisY);
			//  if (fade_depth > 0)
			//    SerialBT.write(0x2D);
			  SerialBT.print(-fade_depth);
			SerialBT.print(" F\0");
			SerialBT.print(speedString);
			SerialBT.print(cmd_endl); //конец команды
    
			if (j < countZ - 1)
			{
				//SerialBT.print("j=");
				//SerialBT.println(j);
				printSerialString((char*)cmd_linear);
				printSerialString((char*)axis2);
				if (fade_Z < 0)
					SerialBT.write(0x2D);
				//Serial.print("0.1");
				//Serial.print(stepString);
				printValue(fade_step_Z, 100, true);
				SerialBT.print(" F\0");
				SerialBT.print(speedString);
				SerialBT.print(cmd_endl); //конец команды
				//      if (!waitCommand() || isAlarmExit)
				//       {
				//         isAlarmExit = false;
				//         return;
				//       }
			}
		}
  
	}

	bool isOutputPosition = true;

	//включен режим тонкой настройки по энкодеру
	bool isManualTuningMode()
	{
		return tuning_manual != NONE;
	}

	void tununigByEncoder(manual_enum tuning)
	{
		//точная настройка по энкодеру 
		 if(encoder0Pos != 0)
		{
			printSerialString((char*)cmd_linear);
			//printSerialString(axis_label[tuning]);
			printSerialString((char*)pgm_read_word(&(axis_label[tuning])));
			if (encoder0Pos < 0)
				SerialBT.write(0x2D);
			//Serial.print(fade_width);
			SerialBT.print("0.01 F4\0");
			SerialBT.print(cmd_endl); //конец команды
			encoder0Pos = 0;
			isOutputPosition = true;
		}
	}

	void testManual()
	{
		printSerialString((char*)cmd_incremental); //Отправляем G91
		SerialBT.print(cmd_endl);
		delay(5);
		do
		{
			
			printSerialString("G1 X-0.005 Y0.06 F24");
			SerialBT.print(cmd_endl);
			delay(120);//ждем
     
			encRead(); //нажат энкодер?
			if (encClick)
			  break;
		} while (true);
	}
void go_manual_stick()
{
  ///параметры "мертвой зоны"
  /* const int X_THRESHOLD_LOW = 528;
    const int X_THRESHOLD_HIGH = 538;
    const int Y_THRESHOLD_LOW = 537;
    const int Y_THRESHOLD_HIGH = 547; */


  int x_direction = 0;
  int y_direction = 0;
  int xPosition = 0;
  int yPosition = 0;
  int buttonState = 1;
  int prevbuttonState = 1;
  bool IsZAxis = false;
  isOutputPosition = true;

  printSerialString((char*)cmd_incremental);//Отправляем G91
  SerialBT.print(cmd_endl);
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);


  treshold_x = EEPROM.readInt(EEPROM_TRESHOLD_X);
  
  if (treshold_x <= 0)
    treshold_x = X_THRESHOLD_LOW;
  if (treshold_x <= 0)
    treshold_y = Y_THRESHOLD_LOW;
  int offset = 4;
  do {
    char Texttft[50];
    strcpy(Texttft, "");
    x_direction = 0;
    y_direction = 0;
    xPosition = analogRead(xPin);//положение джойстика по оси X
    yPosition = analogRead(yPin);//положение джойстика по оси Y
    buttonState = digitalRead(buttonPin);//кнопка джойстика
    
    if (buttonState != prevbuttonState)
    {
      if (!buttonState)
        IsZAxis = !IsZAxis;
      prevbuttonState = buttonState;
      //          Serial.print("buttonState=");
      //          Serial.println(buttonState);

    }
    if (IsZAxis)
    {
      tft.setCursor(120, 0 * CHAR_HEIGHT);
      tft.print("Axis Z");
    }
    else
      clearLine(120, 0 * CHAR_HEIGHT);
    //        char Text[150];
    //        sprintf(Text,"initial:x=%d,y=%d",xPosition ,yPosition);
    //Serial.println(Text);

    ///направление определяем, возможно понадобится в будущем
    if (xPosition > treshold_x + offset)
      x_direction = 1;
    else if (xPosition < treshold_x - offset)
      x_direction = -1;
    if (yPosition > treshold_y + offset) {
      y_direction = 1;
    } else if (yPosition < treshold_y - offset) {
      y_direction = -1;
    }
    
    int segX = -1;
    int segY = -1;
    int MaxRangeX = 1022;
    int MinRangeX = 0;
    int MaxRangeY = 1022;
    int MinRangeY = 0;

    char Position[50];
    if ((x_direction == 0) && (y_direction == 0))
    {
      if (isOutputPosition && getPosition(Position, false))
      {
        clearLine(0, 10 * CHAR_HEIGHT);
        tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
        tft.setCursor(0, 10 * CHAR_HEIGHT);
        tft.println("| X | Y | Z |");
        tft.setCursor(0, 12 * CHAR_HEIGHT);
        tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
        tft.println(Position);
        tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
        isOutputPosition = false;
      }
    }
    else
      isOutputPosition = true;
    UpdateButton();
    if (btnInfoA4.onDown)
    {
      btnInfoA4.onDown = false;
      if (tuning_manual == AXIS_X)
        tuning_manual = NONE;
      else
        tuning_manual = AXIS_X;
    } else if (btnInfoA3.onDown)
    {
      btnInfoA3.onDown = false;
      if (tuning_manual == AXIS_Z)
        tuning_manual = NONE;
      else
        tuning_manual = AXIS_Z;
    } else if (btnInfoA5.onDown)
    {
      btnInfoA5.onDown = false;
      if (tuning_manual == AXIS_Y)
        tuning_manual = NONE;
      else
        tuning_manual = AXIS_Y;
          //Serial.println("AXIS_Y");
    }
    
    if (isManualTuningMode())
    {
        tununigByEncoder(tuning_manual);
        //Serial.println("ManualTuningMode");
        tft.setCursor(110, 1 * CHAR_HEIGHT);
        tft.print("Tune ");
        printTFTString((char*)pgm_read_word(&(axis_label[tuning_manual])));
    }
    else
      clearLine(100, 1 * CHAR_HEIGHT);
    
    
    
    if ( x_direction != 0 )
    {
      if (x_direction == 1)
        {
          //segX = (xPosition - treshold_x - offset)*TABLE_SIZE / ((MaxRangeX - treshold_x - offset) /*/ TABLE_SIZE*/);
          segX = (xPosition - treshold_x - offset)/ ((MaxRangeX - treshold_x - offset) / TABLE_SIZE);
          /*Serial.print("xPosition=");
          Serial.print(xPosition);
          Serial.print("treshold_x=");
          Serial.print(treshold_x);
          Serial.print("offset=");
          Serial.print(offset);
          Serial.print("MaxRangeX=");
          Serial.print(MaxRangeX);
          Serial.print("segXion=");
          Serial.print(segX);*/
        }
      else
        segX = (treshold_x - xPosition + offset) / ((treshold_x - MinRangeX - offset) / TABLE_SIZE);
    }
    if ( y_direction != 0)
    {
      if (y_direction == 1)
        segY = (yPosition - treshold_y - offset) / ((MaxRangeY - treshold_y - offset) / TABLE_SIZE);
      else
        segY = (treshold_y - yPosition + offset) / ((treshold_y - MinRangeY - offset) / TABLE_SIZE);
    }
   
    //        sprintf(Text,"x=%d,y=%d,segX = %d segY = %d tresh_X=%d div=%d",xPosition ,yPosition ,segX,segY,treshold_x,(MaxRangeX-treshold_x - offset)/21);
    //        Serial.println(Text);
    if ((((x_direction != 0) || (y_direction != 0)) && !IsZAxis) || ((x_direction != 0)  && IsZAxis) )
    {
      printSerialString((char*)cmd_linear);//если позиция не равна 0 отправляем G1
      strcat_P(Texttft, cmd_linear);
    }
    else
      clearLine(0, 15 * CHAR_HEIGHT);

    char posX = 0; char posY = 0;
    if (!IsZAxis)
    {
      //ось X
      if (( x_direction != 0))
        printDirection(x_direction, 0, segX, Texttft);
      //ось Y
      if (( y_direction != 0))
        printDirection(y_direction, 1, segY, Texttft);
      //скорость
      if ((x_direction != 0) || (y_direction != 0))
      {
        char pos;
        pos = std::max(segX, segY); //позиция в таблице скоростей
        printSpeed(pos, Texttft);
        clearLine(0, 15 * CHAR_HEIGHT);
        tft.setCursor(0, 15 * CHAR_HEIGHT);
        tft.print(Texttft);
      }
      //Serial.println("go_manual_stick5");
    }
    else
    { //ось Z
      if (y_direction != 0)
        printDirection(y_direction, 2, segY, Texttft);
      //скорость
      if (y_direction != 0)
      {
        printSpeed(segY, Texttft);
        clearLine(0, 15 * CHAR_HEIGHT);
        tft.setCursor(0, 15 * CHAR_HEIGHT);
        tft.print(Texttft);
      }
    }
    //        if (!waitCommand(true))
    //          return;
    delay(120);//ждем
     
    encRead(); //нажат энкодер?
    if (encClick)
      break;
  } while (true);
  encClick = false;//здесь внимание!!
  tuning_manual = NONE;
  encoder0Pos = lastReportedPos = 0;
  delay(MENU_DELAY);
}

void manual_menu(void) {
  SerialBT.flush();
  manual_screen();
  pp_menu = 0;
  lastReportedPos = encoder0Pos = 0;
  str_print(pp_menu, 1);
  do {
    read_keys();
    
	  go_manual_stick();
	  //testManual();
    break; 
    delay(MENU_DELAY);
  } while (true);
  main_screen();
  pp_menu = 0;
  str_print(pp_menu, 2);
}

//void pro_menu(void) {
//  Serial.flush();
//  pro_screen();
//  pp_menu = 0;
//  lastReportedPos = encoder0Pos = 0;
//  str_print(pp_menu, 1);
//  do {
//    read_keys();
//    if (UP_KEY) {
//      if (pp_menu > 0) {
//        pp_menu--;
//      } else {
//        pp_menu = 1;
//      }
//      str_print(pp_menu, 1);
//    };
//    if (DOWN_KEY) {
//      if (pp_menu < 1) {
//        pp_menu++;
//      } else {
//        pp_menu = 0;
//      }
//      str_print(pp_menu, 1);
//    };
//    if (OK_KEY) {
//     if (pp_menu == 0) {
//        facing_menu();
//        delay(MENU_DELAY);
//      }
//      if (pp_menu == 1) break;
//      pro_screen();
//      str_print(pp_menu, 1);
//    };
//    delay(MENU_DELAY);
//  } while (true);
//  main_screen();
//  pp_menu = 0;
//  str_print(pp_menu, 1);
//}

void facing_menu(void) {
  SerialBT.flush();
  facing_screen();
  pp_menu = 0;
  lastReportedPos = encoder0Pos = 0;
  str_print(pp_menu, 1);
  do {
    read_keys();
    if (UP_KEY) {
      if (pp_menu > 0) {
        pp_menu--;
      } else {
        pp_menu = 9;
      }
      str_print(pp_menu, 1);
    };
    if (DOWN_KEY) {
      if (pp_menu < 9) {
        pp_menu++;
      } else {
        pp_menu = 0;
      }
      str_print(pp_menu, 1);
    };
    if (OK_KEY) {
      if (pp_menu < 9 )
        facing_param(pp_menu);
      if (pp_menu == 9) {
        break;
      }
      facing_screen();
      str_print(pp_menu, 1);
    };
    delay(MENU_DELAY);
  } while (true);
  //pro_menu();
  main_screen();
  pp_menu = 0;
  str_print(pp_menu, 0);
}


void exec_screen(void) {
  //tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  printTFTString((char*)text_line);

  tft.setCursor(30, 0);
  tft.println(current_line);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.setCursor(70, 0 * CHAR_HEIGHT);
  tft.write(0x46);
  /*tft.setCursor(70, 0 * CHAR_HEIGHT);
    tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
    tft.write(0x58);
    tft.setCursor(110, 0 * CHAR_HEIGHT);
    tft.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
    tft.write(0x59);
    tft.setCursor(70, 1 * CHAR_HEIGHT);
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.write(0x5A);
    tft.setCursor(110, 1 * CHAR_HEIGHT);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.write(0x46);*/
  tft.setCursor(0, 0);
}


void buttons_screen(void) {
  //tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(30, 0);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.println(current_line);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  /*play button*/
  tft.setCursor(0, 1 * CHAR_HEIGHT);//MAX_DISPLAY_LINES
  if (exec_mode == EXEC_PAUSE) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_PLAY) tft.setTextColor(ST7735_RED, ST7735_BLACK);
  if (exec_mode == EXEC_STEP) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if ((pp_menu == 0) && (skip_page > 2)) tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.write(CHAR_PLAY);
  /*step button*/
  tft.setCursor(CHAR_WIDTH * 2, 1 * CHAR_HEIGHT);//MAX_DISPLAY_LINES
  if (exec_mode == EXEC_PAUSE) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_PLAY) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_STEP) tft.setTextColor(ST7735_RED, ST7735_BLACK);
  if ((pp_menu == 1) && (skip_page > 2)) tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.write(CHAR_STEP);
  /*pause button*/
  tft.setCursor(CHAR_WIDTH * 4, 1 * CHAR_HEIGHT); //MAX_DISPLAY_LINES
  if (exec_mode == EXEC_PAUSE) tft.setTextColor(ST7735_RED, ST7735_BLACK); else tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_PLAY) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_STEP) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if ((pp_menu == 2) && (skip_page > 2)) tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.write(CHAR_PAUSE);
  /*stop button*/
  tft.setCursor(CHAR_WIDTH * 6, 1 * CHAR_HEIGHT); //MAX_DISPLAY_LINES
  if (exec_mode == EXEC_PAUSE) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_PLAY) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if (exec_mode == EXEC_STEP) tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  if ((pp_menu == 3) && (skip_page > 2)) tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.write(CHAR_STOP);
}
/*
  const char cmd_cyclestart[]   = "~\0x0A\0";
  const char cmd_feedhold[]   = "!\0x0A\0";

*/

void process_keys(void) {
  read_keys();
  /*  if (exec_mode == EXEC_PLAY) {
    if (UP_KEY) {
        if (execSpeed > 0) {
    execSpeed = execSpeed - 10;
        } else {
    execSpeed = 100;
        }
    };
    if (DOWN_KEY) {
        if (execSpeed < 100 ) {
    execSpeed = execSpeed + 10;
        } else {
    execSpeed = 0;
        }
    };
    };*/
  if ((exec_mode == EXEC_PLAY) && (OK_KEY == true)) {
    exec_mode = EXEC_PAUSE;
  }
  if ((exec_mode == EXEC_PAUSE) ||  (exec_mode == EXEC_STEP)) {
    skip_page = 0;
    OK_KEY = false;
    //if (current_line > 0) printSerialString((char*)cmd_feedhold);
    do {
      read_keys();
      if (UP_KEY) {
        if (pp_menu > 0) {
          pp_menu--;
        } else {
          pp_menu = 3;
        }
      };
      if (DOWN_KEY) {
        if (pp_menu < 3) {
          pp_menu++;
        } else {
          pp_menu = 0;
        }
      };
      if (OK_KEY) {
        OK_KEY = encClick = false;
        break;
      }
      buttons_screen();
      delay(MENU_DELAY);
      skip_page++;
      if (skip_page > 3)skip_page = 0;
    } while (true);
    if (pp_menu == 0) {
      exec_mode = EXEC_PLAY;
      /*if (current_line > 0) {
        printSerialString((char*)cmd_cyclestart);
        while (SerialBT.available() > 0) {
          memset(buff, 0, sizeof(buff));
          readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
          if (readcnt > 0) {
            buff[readcnt] = 0;
            if ((strncmp(buff, "ok", 2) == 0)) break;
          }
        }
        }*/
    }
    if (pp_menu == 1) {
      exec_mode = EXEC_STEP;
      /*if (current_line > 0) {
        /*printSerialString((char*)cmd_cyclestart);
        while (SerialBT.available() > 0) {
          memset(buff, 0, sizeof(buff));
          readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
          if (readcnt > 0) {
            buff[readcnt] = 0;
            if ((strncmp(buff, "ok", 2) == 0)) break;
          }
        }
        }*/
    }
    if (pp_menu == 2) exec_mode = EXEC_PAUSE;
    if (pp_menu == 3) {
      //printSerialString((char*)cmd_reset);
      exec_mode = EXEC_EXIT;
    }
    delay(MENU_DELAY);
    OK_KEY = encClick = false;
    buttons_screen();
  }
  update_pos();
}

void update_pos(void) {

  tft.setTextSize(1);
  tft.setCursor(78, 0 * CHAR_HEIGHT);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.print(feed_dig);
  printTFTString((char*)trail);
  /* tft.print(execSpeed);
    tft.print(F("% "));*/
}

void execute_file(char* fname) {
  current_line = skip = skip_page = 0;
  int8_t pos_start = 0;
  int8_t pos_end = 0;
  char feed[10];
  char temp[40];
  feed_dig = feed_safe;

  if (EEPROM.read(EEPROM_START) == 1) {
    exec_mode = EXEC_PLAY;
  } else if (EEPROM.read(EEPROM_START) == 0) {
    exec_mode = EXEC_PAUSE;
  }

  SerialBT.flush();
  pp_menu = 0;
  exec_screen();
  buttons_screen();
  process_keys();
  File text = SD.open(fname, FILE_READ);
  
  if (text) {
    while (text.available()) {
      process_keys();
      if (exec_mode == EXEC_EXIT) {
        text.close();
        exit;
      }
      current_line++;
      memset(buff, 0, sizeof(buff));
      readcnt = text.readBytesUntil(0x0A, buff, sizeof(buff));
      if (readcnt > 0) {
        buff[readcnt] = 0;
        pos_start = 0;
        for (int8_t pe = 0; pe < sizeof(buff); pe++) {
          if ((buff[pe] == 0x46)) {
            pos_start = pe;
            break;
          }
        }
        if (pos_start > 0) {
          for (int8_t pe = pos_start + 1; pe < sizeof(buff); pe++) {
            if ((buff[pe] > 0x39) || (buff[pe] == 0)) {
              pos_end = pe;
              break;
            }
          }
          strncpy(feed, buff + pos_start + 1, pos_end - pos_start);
          feed_dig = strtol(feed, 0, 0);
          if (feed_dig > feed_safe) feed_dig = feed_safe;
          //feed_dig = (feed_dig * execSpeed) / 100;
          ltoa(feed_dig, feed, 10);
          for (int8_t pe = 0; pe < 10; pe++) {
            if (feed[pe] == 0) {
              pos_end = pe - 1;
              break;
            }
          }
          strncpy(buff + pos_start + 1, feed, pos_end);
        }
        for (int8_t pe = 0; pe < sizeof(buff); pe++) {
          /*if ((buff[pe]>0x00)&&((buff[pe]!=0x0D)||(buff[pe]!=0x0A))){
            if (((buff[pe]<0x20)||(buff[pe]>0x5A))) buff[pe]=0x20;
            }*/
          if (buff[pe] == 0x0D) {
            buff[pe] = 0x0A;
            buff[pe + 1] = 0x00;
            break;
          }
          if (buff[pe] == 0x0A) {
            buff[pe + 1] = 0x00;
            break;
          }

        }
        SerialBT.print(buff);
        SerialBT.flush();
        memset(temp, 0, sizeof(temp));
        strncpy(temp, buff, sizeof(temp));
        for (int8_t pe = 0; pe < sizeof(temp); pe++) {
          if ((temp[pe] == 0x0D) || (temp[pe] == 0x0A)) {
            temp[pe] = 0x00;
            break;
          }
        }

        if (exec_mode == EXEC_STEP) {
          tft.setTextSize(1);
          tft.setCursor(0, (skip + 3)* CHAR_HEIGHT);
          printTFTString((char*)empty_line); tft.println();
          printTFTString((char*)empty_line); tft.println();
          //tft.println(F("                           "));
          //tft.println(F("                           "));
          /*tft.setCursor(0, (skip + 4)* CHAR_HEIGHT);
            tft.println(F("                           "));*/
          tft.setCursor(0, (skip + 3)* CHAR_HEIGHT);
          tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
          //tft.print(F("-"));
          tft.print(buff);
          skip++;
        }
        SerialBT.flush();
      }

      while (SerialBT.available() == 0) {
        encRead(); if ((encClick) && (exec_mode == EXEC_PLAY)) {
          pp_menu = 2;
          exec_mode = EXEC_PAUSE;
          process_keys();
        }
        if (exec_mode == EXEC_EXIT) break;
        //if (SerialBT.available() > 0) break;
      }
      while (SerialBT.available() > 0) {
        memset(buff, 0, sizeof(buff));
        readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
        if (readcnt > 0) {
          buff[readcnt] = 0;
          if (strncmp(buff, "error", 5) == 0) {
            delay(10);
            SerialBT.flush();
            tft.setTextSize(1);
            tft.setCursor(0, (skip + 3)* CHAR_HEIGHT);
            printTFTString((char*)empty_line); //tft.println();
            printTFTString((char*)empty_line); //tft.println();
            printTFTString((char*)empty_line); //tft.println();
            //tft.println(F("                           "));
            //tft.println(F("                           "));
            tft.setCursor(0, (skip + 3)* CHAR_HEIGHT);
            tft.setTextColor(ST7735_RED, ST7735_BLACK);
            tft.print(current_line);
            printTFTString((char*)trail);
            tft.println(temp);
            tft.print(buff);
            memset(temp, 0, sizeof(temp));
            if (EEPROM.read(EEPROM_PAUSE) == 1) exec_mode = EXEC_PAUSE;
            skip++;
            //break;
          }
          /*if (exec_mode == EXEC_STEP) {
            skip++;

            }*/
          if ((strncmp(buff, "ok", 2) == 0)) {
            //tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
            //(SerialBT.available()==0)
            delay(10);
            SerialBT.flush();
            //break;
          }
        }
        encRead();
        if ((encClick) && (exec_mode == EXEC_PLAY)) {
          pp_menu = 2;
          exec_mode = EXEC_PAUSE;
          OK_KEY = encClick = false;
          process_keys();
        }
        //if (SerialBT.available()==0)break;
        if (exec_mode == EXEC_EXIT) break;
      }

      /*if (skip_page == 0) {
        SerialBT.print(F("?"));
        SerialBT.print(cmd_endl);
        memset(buff, 0, sizeof(buff));
        while (SerialBT.available() > 0) {
        readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
        if (readcnt > 0) {
        buff[readcnt] = 0;
        tft.setCursor(0, 3 * CHAR_HEIGHT);
        tft.println(buff);
        }
        }
        }*/
      //skip++;
      //skip_page++;
      exec_screen();
      if (skip > (MAX_DISPLAY_LINES - 5))skip = 0;
      //if (skip_page > 10)skip_page = 0;
    }
  }
  text.close();
}

boolean init_card(void) {
  if (root) root.close();
  if (!SD.begin(__SDCS) /*&& !SDINIT*/) {
    /*Serial.write("error code=");
      Serial.print(SD.errorCode());*/
    printTFTString((char*)init_error);
    digitalWrite(__SDCS, LOW);
    delay(MENU_DELAY * 4);
    digitalWrite(__SDCS, HIGH);
    tft.fillScreen(ST7735_BLACK);
    return false;
  }
  else
    SDINIT = true;
  root = SD.open("/");
  if (!root) {
    printTFTString((char*)init_error);
    delay(MENU_DELAY * 4);
    tft.fillScreen(ST7735_BLACK);
    return false;
  }
  root.rewindDirectory();
  tft.fillScreen(ST7735_BLACK);
  return true;
}

void printDirectory(File dir, char* file, bool getfile) {
  while (filecount < MAX_DISPLAY_FILES) {
    File entry = dir.openNextFile(FILE_READ);
    if (!entry) {
      entry.close();
      break;
    }
    if (entry.isDirectory()) {
      printDirectory(entry, file, getfile);
    } else {
      if ((getfile) && (skip == 0)) {
        strcpy(file,entry.name());
        break;
      }
      if (skip == 0) {
        filecount++;
        printTFTString((char*)trail);
        tft.print(entry.name());
        tft.println();
      };
      if (skip > 0) skip--;
    }
    entry.close();
  }
}

void list(void) {
  skip = skip_page * MAX_DISPLAY_FILES;
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  filecount = 0;
  root.rewindDirectory();
  printDirectory(root, fname, false);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  if (filecount == MAX_DISPLAY_FILES) {
    printTFTString((char*)text_next);
  } else {
    printTFTString((char*)text_exit);
  }
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  pp_menu = 0;
  str_print(pp_menu, 1);
}

void files_menu(void) {
  skip_page = skip = 0;
  if (init_card()) {
    list();
  } else {
    MENU_KEY = true;
  }
  do {
    read_keys();
    if (UP_KEY) {
      if (pp_menu > 0) {
        pp_menu--;
      } else {
        pp_menu = filecount;
      }
      str_print(pp_menu, 1);
    };
    if (DOWN_KEY) {
      if (pp_menu < filecount) {
        pp_menu++;
      } else {
        pp_menu = 0;
      }
      str_print(pp_menu, 1);
    };
    if (OK_KEY) {
      if (pp_menu < filecount) {
        root.rewindDirectory();
        skip = (skip_page * MAX_DISPLAY_FILES) + pp_menu;
        filecount = 0;
        fname[0] = '\0';
        printDirectory(root, fname, true);
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(0, 0);
        execute_file(fname);
        tft.fillScreen(ST7735_BLACK);
        go_home();
        tft.setTextSize(5);
        do {
          encRead();
          tft.setCursor(22, 45);
          tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
          printTFTString((char*)text_done);
        } while (!encClick);
        tft.setTextSize(0);
        break;
      }
      if ((filecount == MAX_DISPLAY_FILES) && (pp_menu == filecount)) {
        skip_page++;
      } else {
        /*Serial.write("root.close()");
          root.close();*/
        break;
      }
      list();
    };
    delay(MENU_DELAY);
  } while (!MENU_KEY);
  tft.fillScreen(ST7735_BLACK);
  filecount = skip_page = skip = 0;
  main_screen();
  pp_menu = 1;
  str_print(pp_menu, 2);
};

void rotEncoder() {
  rotating = true;
}

//void pciSetup(byte pin)
//{
//    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
//    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
//    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
//}


//ISR (PCINT1_vect) // handle pin change interrupt for A0 to A5 here
//{
//  PrevState = State;
//  if (!digitalRead(A5) )
//    {
//      State|=BtnA5;
//      Serial.println("A5 = 0");
//    }
//  else  
//    {
//      State&=~BtnA5;
//      Serial.println("A5 = 1");
//    }
//  if (!digitalRead(A4) )
//    {
//      State|=BtnA4;
//      Serial.println("A4 = 0");
//    }
//  else  
//    {
//      State&=~BtnA4;
//      Serial.println("A4  = 1");
//    }
//    Serial.println("Interrupt");
//}



void UpdateButton()
{
  int count= 3;
  #ifdef BUTTON_TRESHOLD
   while (count--)
    {
      for (int i = 0;i<sizeof(btnArray)/sizeof(btnArray[0]);i++)
      {
        btnArray[i]->counter += (!digitalRead(btnArray[i]->Pin)) ? 1:-1;
        if (btnArray[i]->counter>TARGET_A5_MAX)
        {
          btnArray[i]->counter = TARGET_A5_MAX;
          //цели достигли
          if (btnArray[i]->counter==btnArray[i]->target)
          {
            btnArray[i]->target=0;
            btnArray[i]->onDown = true;
            btnArray[i]->onUp = false;
          }
        }
        else if (btnArray[i]->counter<0)
        {
          btnArray[i]->counter = 0;
          //цели достигли
          if (btnArray[i]->counter==btnArray[i]->target)
          {
            btnArray[i]->target=TARGET_A5_MAX;
            btnArray[i]->onDown = false;
            btnArray[i]->onUp = true;
          }
        }
      }
      delay(50);
    }
   #endif
   #ifndef  BUTTON_TRESHOLD
   while (count--)
   {
      for (int i = 0;i<sizeof(btnArray)/sizeof(btnArray[0]);i++)
      {
        bool isDown = !digitalRead(btnArray[i]->Pin);
        if (!btnArray[i]->onDown)
          btnArray[i]->onDown = isDown;
      }
   }
   #endif
}

#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_RGB 0x00
/*void setRotation()
{
 
  tft.writecommand(ST7735_MADCTL);
  tft.writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
  tft._width = ST7735_TFTHEIGHT_18;
  tft._height = ST7735_TFTWIDTH;
}*/
static uint8_t buf_BT;

void setup() {
  Serial.begin(SERIAL_SPEED);
  //Serial.begin(9600);
  Serial.setTimeout(1);
  while (!Serial) {
  }

 
  
  pinMode(__LED, OUTPUT);
  digitalWrite(__LED, HIGH);
    tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(0, 0);
  
  esp_log_level_set("*", ESP_LOG_NONE); 
  /*Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);*/
	ESP_LOGE("EspMashine", "Start button");
  Serial.println("\nTesting EEPROM Library\n");
  if (!EEPROM.begin(64)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  Serial.println("\nEnd Testing EEPROM Library\n");

  SerialBT.begin();
  SerialBT.setBuffer(&buf_BT);
  SerialBT.connect(&serviceUUID,1000);
  
  Serial.println("\nEnd connect ble\n");
  /*pinMode(__SDCS, OUTPUT);
  digitalWrite(__SDCS, HIGH);*/

  pinMode(__LED, OUTPUT);
  digitalWrite(__LED, HIGH);
  //для esp32
  tft.init();   // initialize a ST7735S chip,
  tft.cp437(true);
  
  tft.setRotation(DISPLAY_ROT);
  //для экономии памяти заменил на вызов
  //tft.setRotation();
  
  

  
  
  /*
   GFXfont* font = tft.getFont();
   if (font!=NULL)
  {
    Serial.println("font!=NULL");
  }
  else
    Serial.println("font==NULL");*/
  Serial.print("A0=");
  Serial.println(A0);
  Serial.print("A1=");
  Serial.println(A1);
  Serial.print("A2=");
  Serial.println(A2);
  Serial.print("A3=");
  Serial.println(A3);
  Serial.print("A4=");
  Serial.println(A4);
  Serial.print("A5=");
  Serial.println(A5);
  Serial.print("A6=");
  Serial.println(A6);

  analogReadResolution(10);
  /*analogSetCycles(128);
  analogSetSamples(1);
  analogSetClockDiv(64);*/
  
  //printSerialString(text_start_spd);
  
  //конец esp32
   //interrupts();
  //джойстик
  //pinMode(xPin, INPUT);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  Serial.println(analogRead(xPin));
  Serial.println(analogRead(yPin));
  
  //
  //analogSetPinAttenuation(xPin,ADC_11db);

  ESP_LOGE("EspMashine","Start button");
  // активируем подтягивающий резистор на пине кнопки
  pinMode(buttonPin, INPUT_PULLUP);
  
  
  // инициализация кнопок
  pinMode(A4, INPUT_PULLUP);
  digitalWrite(A4, HIGH);

  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A5, HIGH);

  pinMode(A3, INPUT_PULLUP);
  digitalWrite(A3, HIGH);
  ESP_LOGE("EspMashine","End button");
    //установка pull_up
    /*volatile uint8_t *reg, *out; 
    reg = portModeRegister(3);
    out = portOutputRegister(3); 
    
    uint8_t oldSREG = SREG; 
    cli();
    *reg &= ~0x38;
    *out |= 0x38; 
    
    SREG = oldSREG; */ /*для arduino было сделано в целях экономии*/

    //для esp32
    pinMode(A4, INPUT_PULLUP);
    digitalWrite(A4, HIGH);
  //
    pinMode(A5, INPUT_PULLUP);
    digitalWrite(A5, HIGH);
  //
    pinMode(A3, INPUT_PULLUP);
    digitalWrite(A3, HIGH);
    

   // initialize external pin interrupt.
//  PCICR =  0b00000010; // 1. PCIE1: Pin Change Interrupt Enable 1
//  EICRA |= bit (ISC10);    // set wanted flags (falling edge causes interrupt)
//  PCMSK1 = 0b00110000; // Enable Pin Change Interrupt for A5
// for (int i=A0; i<=A5; i++)
//      digitalWrite(i,HIGH); 
//  pciSetup(A0);
//  pciSetup(A1);
//  pciSetup(A2);
//  pciSetup(A3);
 //   pciSetup(A4);
   // pciSetup(A5);
      
  //tft.initR(INITR_BLACKTAB);
  //tft.colorSpace(1);
  tft.setRotation(DISPLAY_ROT);
  //для экономии памяти заменил на вызов
  //tft.setRotation();
  
  
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(0, 0);
  /*
    yzero = analogRead(joyPinY);
    xzero = analogRead(joyPinX);
    pinMode(joyPinZ, INPUT_PULLUP);
  */
  
  pinMode(encoder0PinA, INPUT);
  digitalWrite(encoder0PinA, HIGH);     // РїРѕРґРєР»СЋС‡РёС‚СЊ РїРѕРґС‚СЏРіРёРІР°СЋС‰РёР№ СЂРµР·РёСЃС‚РѕСЂ
  pinMode(encoder0PinB, INPUT);
  digitalWrite(encoder0PinB, HIGH);    // РїРѕРґРєР»СЋС‡РёС‚СЊ РїРѕРґС‚СЏРіРёРІР°СЋС‰РёР№ СЂРµР·РёСЃС‚РѕСЂ
  pinMode(encoder0PinC, INPUT_PULLUP);
  Serial.println("attachInterrupt");
  
  attachInterrupt(digitalPinToInterrupt(encoder0PinA), doEncoderA, CHANGE);  // РЅР°СЃС‚СЂРѕРёС‚СЊ РїСЂРµСЂС‹РІР°РЅРёРµ interrupt 0 РЅР° pin 2
  attachInterrupt(digitalPinToInterrupt(encoder0PinB), doEncoderB, CHANGE);  // РЅР°СЃС‚СЂРѕРёС‚СЊ РїСЂРµСЂС‹РІР°РЅРёРµ interrupt 0 РЅР° pin 3
  Serial.println("attachInterrupt");
  //attachInterrupt(PCINT12, doButtonHome, CHANGE);
  
  lastReportedPos = encoder0Pos = 0;
  // The internal 1.1V reference provides for better
  // resolution from the LM35, and is also more stable
  // when poweST7735_RED from either a battery or USB...
  //analogReference(INTERNAL);
//  pinMode(GRBL_RES_PIN, OUTPUT);
//  digitalWrite(GRBL_RES_PIN, HIGH);
//  delay(200);
//  digitalWrite(GRBL_RES_PIN, LOW);
  //tft.defineScrollArea(8,152);
  current_line = skip = 0;
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  feed_safe = EEPROM.readInt(EEPROM_FEED);
  if (feed_safe > FEED_MAX) {
    feed_safe = FEED_MAX;
    EEPROM.writeInt(EEPROM_FEED, FEED_MAX);
  }
  treshold_x = EEPROM.readInt(EEPROM_TRESHOLD_X);
  //Serial.print("EEPROM_TRESHOLD_X=");
  //Serial.print(treshold_x);

  treshold_y = EEPROM.readInt(EEPROM_TRESHOLD_Y);

  shpindel_speed = EEPROM.readInt(EEPROM_SHPINDEL);

  ESP_LOGE("EspMashine","BLethoth begin");
 
  
  //reset
  ESP_LOGE("loop","start reset");
  printSerialString((char*)cmd_reset);
  ESP_LOGE("loop","end reset");
  SerialBT.print(cmd_endl);
  
  //Timer1.initialize(100000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  //Timer1.attachInterrupt( timerIsr ); // attach the service routine here
  Serial.println("end setup");
  
  

}

/*
  void joy_read() {
  joyClick = (digitalRead(joyPinZ) == HIGH ? false : true);
  int value = 0;                   // variable to hold the actual reading
  value = analogRead(joyPinX);
  // push limits
  if (value < xmin) xmin = value;
  else if (value > xmax) xmax = value;
  // start at zero
  x = 0;
  // divide lower half
  int seg = (xzero - xmin) / (segs + zerohold);
  // find what segment we are in
  {
    if (value >= xzero - ((segs - i) + zerohold)*seg) x = i;
  }
  // devide upper part
  seg = (xmax - xzero) / (segs + zerohold);
  // find what segment we are in
  for (int i = segs + 1; i < 2 * segs + 1; i++)
  {
    if (value >= xzero + ((i - segs - 1) + zerohold)*seg) x = i;
  }
  // make X range from [-10,10] instead of [0,20]
  x -= segs;
  // invert X
  x = -x;
  //delay (10);
  // the same as before but with "Y" :-)
  value = analogRead(joyPinY);
  if (value < ymin) ymin = value;
  else if (value > ymax) ymax = value;
  y = 0;
  seg = (yzero - ymin) / (segs + 1);
  for (int i = 0; i < segs + 1; i++)
  {
    if (value >= yzero - ((segs - i) + 1)*seg) y = i;
  }
  seg = (ymax - yzero) / (segs + zerohold);
  for (int i = segs + 1; i < 2 * segs + 1; i++)
  {
    if (value >= yzero + ((i - segs - 1) + zerohold)*seg) y = i;
  }
  y -= segs;

  }
*/
void doEncoderA() {
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done
  // Test transition, did things really change?
  if ( digitalRead(encoder0PinA) != A_set ) { // debounce once more
    A_set = !A_set;
    // adjust counter + if A leads B
    if ( A_set && !B_set ) encoder0Pos++;
    //if (encoder0Pos > encoder0Max) encoder0Pos = encoder0Min+1;
    rotating = false;  // no more debouncing until loop() hits again
  }
}
// Interrupt on B changing state
void doEncoderB() {
  if ( rotating ) delay (1);
  if ( digitalRead(encoder0PinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if ( B_set && !A_set ) encoder0Pos --;
    //if (encoder0Pos<=encoder0Min) encoder0Pos = encoder0Max;
    rotating = false;
  }
}

void encRead() {
  
  encClick = (digitalRead(encoder0PinC) == HIGH ? false : true);
  if (encoder0Pos != lastReportedPos) lastReportedPos = encoder0Pos;
  if (encClick==true)
    Serial.print("encClick=true");
  //  Serial.print("encClick=");
  //  Serial.println(encClick);
}


void read_keys(void) {
  if (encoder0Pos == 0) {
    UP_KEY = false;
    DOWN_KEY = false;
  }
  encRead();
  OK_KEY = encClick;
  if (encoder0Pos > 0) {
    DOWN_KEY = true;
    encoder0Pos = lastReportedPos = 0;
  } else if (encoder0Pos < 0) {
    UP_KEY = true;
    encoder0Pos = lastReportedPos = 0;
  }
}

/*void getCurrentPos(char* strPos)
  {
  printSerialString(cmd_position);
  delay(100);
  int currentline=10;
  while (Serial.available()) {
    memset(strPos, 0, sizeof(strPos));
    readcnt = Serial.readBytesUntil(0x0A, strPos, sizeof(strPos));
    if (readcnt > 0)
      strPos[readcnt] = 0;
    clearLine(0, currentline * CHAR_HEIGHT);
    tft.setCursor(0, currentline * CHAR_HEIGHT);
    tft.println(strPos);
    currentline++;
    if (currentline>15)
      currentline=10;
    delay(300);
   }
  }*/

void serial_flush()
{
  //while (SerialBT.available())
  //  SerialBT.read();
  while (SerialBT.available())
    if (SerialBT.read()==-1)
      return;
}
//void getCurrentPos(char* strPos)
//{
//  printSerialString(cmd_position);
//  delay(100);
//  int currentline=10;
//  memset(strPos, 0, sizeof(strPos));
//  int idx=0;
//  while (Serial.available() && idx<80) {
//
//    //readcnt = Serial.readBytesUntil(0x0A, strPos, sizeof(strPos));
//    char ch = Serial.read();
//    if (ch!=0x0A)
//      strPos[idx] = ch;
//    idx++;
//  }
//  clearLine(0, currentline * CHAR_HEIGHT);
//  tft.setCursor(0, currentline * CHAR_HEIGHT);
//  tft.println(strPos);
//  float Mpos[3];
//  float Wpos[3];
//  strcpy(strPos,"<Idle,MPos:5.529,0.560,7.000,WPos:1.529,-5.440,-0.000>");
//  parsePosition(strPos,Mpos,Wpos,3);
//  currentline++;
//  if (currentline>15)
//    currentline=10;
//  delay(300);
//
//}

//void parsePosition(char* message,float* Mpos,float* Wpos,int size)
//{
//  char strTemp[10];
//  int len = strlen(message);
//  if ((message[0]=='<') && (message[len-1]='>'))
//  {
//    message++;
//    while ((*message!=NULL) && (*message!='M'))
//      message++;
//    if (*message!=NULL)
//    {
//      if (strncmp(message, "MPos:", 5) == 0)
//      {
//        message+=5;
//        int idx =0;
//        while ((*message!=NULL) && (*message!='W'))
//        {
//          char* temp=message;
//          int count = 0;
//          while ((*message!=NULL) && (*message!=','))
//          {
//            message++;
//            count++;
//          }
//          if (*message!=NULL)
//          {
//            strncpy(strTemp,temp,count);
//            strTemp[count]=0;
//            Mpos[idx++]=atof(strTemp);
//            message++;
//          }
//       }
//      if (strncmp(message, "WPos:", 5) == 0)
//      {
//        idx =0 ;
//        message+=5;
//        while ((*message!=NULL) && (*message!='>'))
//        {
//          char* temp=message;
//          int count = 0;
//          while ((*message!=NULL) && (*message!=',') && (*message!='>'))
//          {
//            message++;
//            count++;
//          }
//          if (*message!=NULL)
//          {
//            strncpy(strTemp,temp,count);
//            strTemp[count]=0;
//            Wpos[idx++]=atof(strTemp);
//            message++;
//          }
//       }
//      }
//    }
//  }
// }
//}

//void parsePosition(char* message,float* Mpos,float* Wpos,int size)
//{
//  int idx = 0;
//  char strTempM[20];
//  char strTempW[20];
//  int len = strlen(message);
//  if ((message[0]=='<') && (message[len-1]='>'))
//  {
//    char* pos_M = strstr(message, "MPos:");
//    char* pos_W = strstr(message, "WPos:");
//    if ((pos_M) && (pos_W))
//    {
//      strncpy(strTempM,pos_M+5,pos_W-pos_M-5);
//      strTempM[pos_W-pos_M-5]=0;
//      char *token = strtok(strTempM, ",");
//      Mpos[idx++]=atof(token);
//      while(token) {
//        Mpos[idx++]=atof(token);
//        Serial.println(token);
//        token = strtok(NULL, ",");
//      }
//
//      strncpy(strTempW,pos_W+5,message+len-pos_W-5-1);
//      strTempW[message+len-pos_W-5-1]=0;
//      idx = 0;
//      Wpos[idx++]=atof(token);
//      token = strtok(strTempW, ",");
//      while(token) {
//        Wpos[idx++]=atof(token);
//        Serial.println(token);
//        token = strtok(NULL, ",");
//      }
//    }
//  }
//}
void loop() {
  
  main_menu();
   if (SerialBT.available()) {
    memset(buff, 0, sizeof(buff));
    readcnt = SerialBT.readBytesUntil(0x0A, buff, sizeof(buff));
    if (readcnt > 0) {
      buff[readcnt] = 0;
      //tft.setCursor(0, (MAX_DISPLAY_LINES+current_line) * CHAR_HEIGHT);
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      if (isStartPrint)
      {
        tft.setCursor(0, current_line * CHAR_HEIGHT);
        printTFTString((char*)empty_line); tft.println();
        tft.setCursor(0, current_line * CHAR_HEIGHT);
      }
      if (strncmp(buff, "ok", 2) == 0) {
        tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
      }
      if (strncmp(buff, "error", 5) == 0) {
        tft.setTextColor(ST7735_RED, ST7735_BLACK);
      }
      if (isStartPrint)
      {
        tft.println(buff);
        current_line++;
      }
      if (current_line > MAX_DISPLAY_LINES)current_line = 0;
      SerialBT.flush();
      if ((strncmp(buff, "['$", 3) == 0) && (EEPROM.readByte(EEPROM_UNLOCK) == 1)) {
        printSerialString((char*)cmd_unlock);
        delay(10);
      }
      /*printSerialString(cmd_position);
        delay(10);*/

    }
  }
  /*encRead();
  if (encClick) main_menu();*/
  
}
