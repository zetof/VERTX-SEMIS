/*
		LCD.cpp - Implémentation de la librairie de pilotage d'un écran LCD de type M18ST05A
		Ecrit par Christophe BURY
		Date de première release: 24/02/2017	

		L'écran LCD M18ST05A est de type communication série, dans le principe, une broche TX est suffisante pour le piloter
		Le choix a été fait d'utiliser la librairie SoftwareSerial pour la communication afin de libérer les pins TX/RX
*/

#include <Time.h>
#include <SoftwareSerial.h>
#include <LCD.h>

// Définition des codes pour caractères spéciaux
const char LCD::SYMBOL_DEGREE = 0xb0;

// Le M18ST05A a une horloge interne qui peut être configurée pour son affichage à l'écran
const char LCD::CLOCK_FORMAT_EU = 0x01;
const char LCD::CLOCK_FORMAT_US = 0x02;
const char LCD::CLOCK_STILL = 0x03;
const char LCD::CLOCK_MOBILE = 0x04;

// Définition des niveaux d'éclirement des éléménts (4 niveaux de complètement éteint à complètement allumé)
const char LCD::LEVEL_OFF = 0x00;
const char LCD::LEVEL_LOW = 0x01;
const char LCD::LEVEL_MEDIUM = 0x02;
const char LCD::LEVEL_HIGH = 0x03;

// Définition des vitesses de rotation pour les icônes CD et REC
const char LCD::SPEED_STOP = 0x00;
const char LCD::SPEED_MAX = 0x01;
const char LCD::SPEED_MEDIUM = 0x02;
const char LCD::SPEED_LOW = 0x03;

// Définition de la sélection des lignes de l'écran, on peut sélectionner la ligne haute, basse ou les deux à la fois
const char LCD::DISPLAY_TOP = 0x21;
const char LCD::DISPLAY_BOTTOM = 0x22;
const char LCD::DISPLAY_BOTH = 0x20;

// Définition de la commande d'allumage des différents icônes sous les deux lignes d'affichage
const char LCD::ICON_HDD = 0X00;
const char LCD::ICON_FIRE_WIRE = 0X01;
const char LCD::ICON_CD = 0X02;
const char LCD::ICON_USB = 0X03;
const char LCD::ICON_MOVIE = 0X04;
const char LCD::ICON_TV = 0X05;
const char LCD::ICON_MUSIC = 0X06;
const char LCD::ICON_PICTURE = 0X07;
const char LCD::ICON_REC = 0X08;
const char LCD::ICON_MAIL_OUT = 0X09;
const char LCD::ICON_MAIL_IN = 0X10;
const char LCD::ICON_SOUND_1 = 0X11;
const char LCD::ICON_SOUND_2 = 0X12;
const char LCD::ICON_SOUND_3 = 0X13;
const char LCD::ICON_SOUND_4 = 0X14;
const char LCD::ICON_SOUND_5 = 0X15;
const char LCD::ICON_SOUND_6 = 0X16;
const char LCD::ICON_SOUND_7 = 0X17;
const char LCD::ICON_SOUND_RED_LINE = 0X18;
const char LCD::ICON_SOUND = 0X19;
const char LCD::ICON_MUTE = 0X20;
const char LCD::ICON_MINI_SOUND_1 = 0X21;
const char LCD::ICON_MINI_SOUND_2 = 0X22;
const char LCD::ICON_MINI_SOUND_3 = 0X23;

// Définition des commandes pour l'allumage des quatre cadres sous les deux lignes d'affichage
const char LCD::FRAME_SOURCES = 0X24;
const char LCD::FRAME_MEDIAS = 0X25;
const char LCD::FRAME_VCR = 0X26;
const char LCD::FRAME_MAIL = 0X27;

void clearDisplay(char lines);														// Permet d'effacer une ligne au choix ou les deux lignes de l'écran LCD
void displayAt(const char* text, char line, int column);	// Permet d'afficher du texte sur une ligne et une colonne précises
void displayCenter(const char* text, char line);					// Permet d'afficher du texte centré sur une ligne
void displayAfter(const char* text);											// Permet d'afficher du texte à la position courante du curseur
void displayIcon(char icon, char level);									// Permet l'affichage des icônes sous les deux lignes d'affichage
void displayClock();																			// Permet l'affichage de l'horloge
void setClock();																					// Permet de régler la date et l'heure de l'horloge
void clockFormat(char format);														// Permet de sélectionner le type d'affichage de l'horloge
void cdSpeed(char speed);																	// Permet de définir la vitesse de rotation de l'icône CD
void recSpeed(char speed);																// Permet de définir la vitesse de clignotement de l'icône d'enregistrement
uint8_t int2BCD(int number);															// Convertit un nombre à deux chiffres au format BCD

// Constructeur de la classe LCD
// Les paramètres d'initialisation permettent de définir les broches RX et TX utilisées par la librairie SoftwareSerial
//
LCD::LCD(uint8_t rx, uint8_t tx){

	// On initialise le software serial sur les pins rx et tx pour piloter l'écran LCD
	lcd = new SoftwareSerial(rx, tx);
	pinMode(rx, INPUT);
	pinMode(tx, OUTPUT);
	lcd->begin(9600);
}

// Méthode permettant l'effacement d'une ligne ou de la totalité de l'écran
// Le paramètre line permet de choisir le type d'effacement:
//	- DISPLAY_TOP: ligne supérieure
//	- DISPLAY_BOTTOM: ligne inférieure
//	- DISPLAY_BOTH: l'écran dans sa totalité
//
void LCD::clearDisplay(char line){
	lcd->write(0x1b);
	lcd->write(line);
	lcd->write(0x1b);
	lcd->write(0x50);
}

// Méthode affichant une ligne de texte sur la ligne et à la colonne sélectionnées
//	- text: la ligne à afficher
//	- line: la ligne sur laquelle le texte est affiché (DISPLAY_TOP ou DISPLAY_BOTTOM)
//	- column: la colonne à partir de laquelle le texte est affiché (0 est la première colonne, 15 la dernière)
//
void LCD::displayAt(const char* text, char line, int column){
	clearDisplay(line);
	char charArray[17] = "";
	char formatter[5] = "";
	if(column < 16){
		snprintf(formatter, 5, "%%%ds", (unsigned)strlen(text) + column);
		snprintf(charArray, 17, formatter, text);
		lcd->print(charArray);
	}
}

// Méthode affichant une ligne de texte au centre de la ligne sélectionnée
//	- text: la ligne à afficher
//	- line: la ligne sur laquelle le texte est affiché (DISPLAY_TOP ou DISPLAY_BOTTOM)
//
void LCD::displayCenter(const char* text, char line){
	int textLength = strlen(text);
	int pos = 0;
	if(textLength < 16) pos = (16 - textLength) / 2;
	displayAt(text, line, pos);
}

// Méthode affichant du texte à partir de la postion actuelle du curseur
// Le curseur est positionné à la colonne juste après le dernier caractère affiché
//	- text: la ligne à afficher
//
void LCD::displayAfter(const char* text){
	lcd->print(text);
}

// Méthode permettant d'allumer ou d'éteindre une icône sous les deux lignes d'affichage
//	- icon: le nom de l'icône à modifier (par exemple ICON_HDD)
//	- level: le niveau de luminosité d'affichage, LEVEL_OFF est éteint, LEVEL_HIGH pleine luminosité
//
void LCD::displayIcon(char icon, char level){
	lcd->write(0x1b);
	lcd->write(0x30);
	lcd->write(icon);
	lcd->write(level);
}

// Méthode permettant d'afficher l'horloge à l'écran
//
void LCD::displayClock(){
	lcd->write(0x1b);
	lcd->write(0x05);
}

// Méthode permettent d'ajuster l'horloge interne du LCD
// L'horloge est ajustée à partir de la base de temps définie sur l'Arduino
//
void LCD::setClock(){
	time_t currentTime = now();
	lcd->write(0x1b);
	lcd->write((uint8_t)0x00);
	lcd->write(int2BCD(minute(currentTime)));
	lcd->write(int2BCD(hour(currentTime)));
	lcd->write(int2BCD(day(currentTime)));
	lcd->write(int2BCD(month(currentTime)));
	lcd->write(int2BCD(year(currentTime) / 100));
	lcd->write(int2BCD(year(currentTime) % 100));
}

// Méthode permettant de définir la façon d'afficher l'horloge
//	-format: sélectionne le format US ou Européen, une horloge fixe ou qui se déplace sur l'écran
//
void LCD::clockFormat(char format){
	lcd->write(0x1b);
	lcd->write(format);
}

// Méthode permettant de sélectionner la vitesse de rotation de l'icône CD
//	- speed: la vitesse de rotation, SPEED_STOP est immobile, SPEED_MAX le plus rapide
//
void LCD::cdSpeed(char speed){
	lcd->write(0x1b);
	lcd->write(0x32);
	lcd->write(speed);
}

// Méthode permettant de sélectionner la vitesse de clignotement de l'icône enregistrement (REC)
//	- speed: la vitesse de clignotement, SPEED_STOP est fixe, SPEED_MAX le plus rapide
//
void LCD::recSpeed(char speed){
	lcd->write(0x1b);
	lcd->write(0x33);
	lcd->write(speed);
}

// Méthode privée permettant de convertir un nombre de deux chiffres au format BCD
//	- number: le nombre à convertir
//
uint8_t LCD::int2BCD(int number){
	return (uint8_t)((number / 10) << 4 | number % 10);
}
