/*
		LCD.h - Librairie de pilotage d'un écran LCD de type M18ST05A
		Ecrit par Christophe BURY
		Date de première release: 24/02/2017	
*/

#ifndef LCD_h
#define LCD_h

#include <Arduino.h>
#include <Time.h>
#include <SoftwareSerial.h>

class LCD{

	public:
		static const char SYMBOL_DEGREE;
		static const char CLOCK_FORMAT_EU;
		static const char CLOCK_FORMAT_US;
		static const char CLOCK_STILL;
		static const char CLOCK_MOBILE;
		static const char LEVEL_OFF;
		static const char LEVEL_LOW;
		static const char LEVEL_MEDIUM;
		static const char LEVEL_HIGH;
		static const char SPEED_STOP;
		static const char SPEED_MAX;
		static const char SPEED_MEDIUM;
		static const char SPEED_LOW;
		static const char DISPLAY_TOP;
		static const char DISPLAY_BOTTOM;
		static const char DISPLAY_BOTH;
		static const char ICON_HDD;
		static const char ICON_FIRE_WIRE;
		static const char ICON_CD;
		static const char ICON_USB;
		static const char ICON_MOVIE;
		static const char ICON_TV;
		static const char ICON_MUSIC;
		static const char ICON_PICTURE;
		static const char ICON_REC;
		static const char ICON_MAIL_OUT;
		static const char ICON_MAIL_IN;
		static const char ICON_SOUND_1;
		static const char ICON_SOUND_2;
		static const char ICON_SOUND_3;
		static const char ICON_SOUND_4;
		static const char ICON_SOUND_5;
		static const char ICON_SOUND_6;
		static const char ICON_SOUND_7;
		static const char ICON_SOUND_RED_LINE;
		static const char ICON_SOUND;
		static const char ICON_MUTE;
		static const char ICON_MINI_SOUND_1;
		static const char ICON_MINI_SOUND_2;
		static const char ICON_MINI_SOUND_3;
		static const char FRAME_SOURCES;
		static const char FRAME_MEDIAS;
		static const char FRAME_VCR;
		static const char FRAME_MAIL;

		LCD(uint8_t rx, uint8_t tx);

		void clearDisplay(char lines);
		void displayAt(const char* text, char line, int column);
		void displayCenter(const char* text, char line);
		void displayAfter(const char* text);
		void displayIcon(char icon, char level);
		void displayClock();
		void setClock();
		void clockFormat(char format);
		void cdSpeed(char speed);
		void recSpeed(char speed);

	private:
		SoftwareSerial* lcd;
		uint8_t int2BCD(int number);
};

#endif
