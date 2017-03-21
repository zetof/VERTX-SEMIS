#include <Arduino.h>
#include <LCD.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Temps de repos en millisecondes entre deux itérations de la boucle principale
#define LOOP_DELAY 100

// Une seconde en millisecondes, utilisé pour vérifier toutes les action à dérouler de seconde en seconde
#define SECOND_DELAY 1000

// Une minute en millisecondes, utilisé pour vérifier toutes les action à dérouler de minute en minute
#define MINUTE_DELAY 60000

// Un quart d'heure en millisecondes, utilisé pour vérifier toutes les action à dérouler de 15 min en 15 min
#define QUARTER_DELAY 900000

// Temps d'affichage d'un paramètre en millisecondes
#define DISPLAY_TIME 2000

// Vitesse de communication sur le port série
#define SERIAL_SPEED 115200

// Longueur maximale d'une phrase à envoyer par le port série
#define SERIAL_MAX_LENGTH 50

// Longueur maximale d'un réel à transformer en string par la fonction dtostfr
#define FLOAT_MAX_LENGTH 10

// Longueur maximale d'une ligne à afficher à l'écran LCD
#define LCD_MAX_LENGTH 16

// Tolérence de température en degrés
#define TEMPERATURE_TOLERENCE 1

// Définition des valeurs de retour pour l'action des interrupteurs
#define BUTTON_COMPARE 512
#define ACTION_NOTHING 0
#define ACTION_LEFT 1
#define ACTION_RIGHT 2
#define ACTION_BOTH 3

// GPIOs utilisées pour commander les composantes Rouge, Vert et Bleu des rubans de LEDs
#define R_PIN 9
#define G_PIN 10
#define B_PIN 11

// GPIO utilisée pour commander la rotation du ventilateur
#define FAN_CMD 6

// GPIO utilisée pour lire la vitesse de rotation du ventilateur
#define FAN_READ 7

// GPIOs utilisées pour commander les prises de courant via les relais
#define RELAY_1_CMD 2
#define RELAY_2_CMD 12

// GPIO utilisée pour lire le capteur de température et humidité de l'air et définition du type de capteur (DHT11 ou DHT22)
#define DHT_PIN 5
#define DHT_TYPE DHT11

// GPIO utilisée pour lire le capteur de température de l'eau
#define DS18_PIN 8

// GPIOs utilisées pour commander l'écran LCD
#define LCD_RX_PIN 3
#define LCD_TX_PIN 4

// Entrées analogiques utilsées pour lire l'état des deux boutons du LCD
#define ANALOG_BUTTON_RIGHT 0
#define ANALOG_BUTTON_LEFT 1

// Prototypes des procédures et fonctions
//
void getWaterTemperature();																												// Procédure qui permet de relever la température de l'eau du bassin d'hydroculture
void getAirTemperature();																													// Procédure qui permet de relever la température de l'air dans l'unité hydroponique
void getAirHumidity();																														// Procédure qui permet de relever l'humidité de l'air dans l'unité hydroponique
void getProbesValues();																														// Procédure qui collecte les valeurs des sondes
void sendProbesValues();																													// Procédure qui envoie les valeurs des sondes au PC de surveillance
void provideFeedbacks();																													// Procédure qui prend les actions correctives si les valeurs sous contrôle dépassent les limites définies par le programme
void setFan(int speed);																														// Procédure qui ajuste la vitesse du ventilateur
void heatOn();																																		// Procédure qui démarre la résistance chauffante
void heatOff();																																		// Procédure qui arrête la résistance chauffante
void pumpOn();																																		// Procédure qui démarre la pompe d'arrosage
void pumpOff();																																		// Procédure qui arrête la pompe d'arrosage
void setLED(int r, int g, int b);																									// Procédure qui règle les différentes composantes de l'éclairage LED
void setInspect(bool state);																											// Procédure utilisée pour allumer la lumière verte
void checkLED();																																	// Procédure appelée chaque seconde qui vérifie si on doit allumer ou éteindre les lumières
void checkLCD();																																	// Procédure appelée chaque seconde qui vérifie si on doit afficher les paramètres de l'unité sur l'écran LCD et les fait défiler
void checkPump();																																	// Procédure appelée chaque seconde qui vérifie si on doit allumer ou éteindre la pompe d'arrosage
int getKeys();																																		// Fonction qui retourne la valeur correspondante aux touches enfoncées
int analogLevel(int percentage);																									// Fonction qui ajuste un pourcentage (0..100) vers une valeur analogWrite (0..255)
void readSerial();																																// Procédure appelée à chaque itération qui scrute le port USB
void sendUSBValue(const char* parameter, int value);															// Procédure qui envoie un nombre entier sur le port USB
void sendUSBValue(const char* parameter, float value, int width, int precision);	// Procédure qui envoie un nombre réel sur le port USB
void keepEventCounters();																													// Procédure qui dans la boucle principale vérifie si il est nécessaire d'activer un déclencheur

// Initialisation de l'écran LCD
LCD lcd(LCD_RX_PIN, LCD_TX_PIN);

// On initialise le capteur de température et humidité
DHT dht(DHT_PIN, DHT_TYPE);

// On initialise le capteur de température de l'eau
OneWire ds18(DS18_PIN);
DallasTemperature waterSensor(&ds18);

// On démarre le programme, on est donc dans la phase d'initialisation
bool initPhase = true;

// Au démarrage, la date de l'unité de germination n'est pas défini
bool isTimeSet = false;

// Etat des actions dans l'unité
int fanSpeed = 100;
bool isHeatOn = true;
bool isPumpOn = true;

// Définition des compteurs d'évènements avec action immédiate au démarrage
int secondDelay = SECOND_DELAY;
long minuteDelay = MINUTE_DELAY;
long quarterDelay = QUARTER_DELAY;

// Définition du compteur d'affichage des paramètres sur l'écran LCD
// Au démarrage, on n'affiche pas les paramètres
int lcdDisplay = -1;

// On prépare la variable de stockage du nom du programme
char programName[LCD_MAX_LENGTH];

// On prépare les variables d'éclairage
bool inspect = false;
bool isLightOn = false;
int redOn = -9999;
int greenOn = -9999;
int blueOn = -9999;
int lightOn = -9999;
int lightOff = -9999;

// On prépare les variables pour la régulation de l'eau
int flowCounter;
int flowOn = -9999;
int flowOff = -9999;
float waterLow = -9999;
float waterHigh = -9999;
float waterTemperature;

// On prépare les variables pour la régulation de l'air
float airLow = -9999;
float airHigh = -9999;
float airTemperature;
float airHumidity;

// Initialisation
//
void setup(){

	// Prépare la communication vers le Raspberry Pi via le bus USB
	Serial.begin(SERIAL_SPEED);

	// Démarre la lecture de la sonde de température de l'eau
	waterSensor.begin();

	// Prépare les GPIOs pour la commande de l'éclairage RGB
	// On éteint les LEDs dans tous les cas
	pinMode(R_PIN, OUTPUT);
	pinMode(G_PIN, OUTPUT);
	pinMode(B_PIN, OUTPUT);
	setLED(0, 0, 0);

	// Prépare les GPIOs pour la commande et la lecture de la vitesse du ventilateur
	// Arrête le ventilateur dans tous les cas
	pinMode(FAN_CMD, OUTPUT);
	pinMode(FAN_READ, INPUT);
	setFan(0);

	// Prépare les GPIOs pour la commande de la pompe d'arrosage et de la résistance chauffante
	// Arrête la pompe et la résistance chauffante dans tous les cas
	pinMode(RELAY_1_CMD, OUTPUT);
	pinMode(RELAY_2_CMD, OUTPUT);
	heatOff();
	pumpOff();

	// Au démarrage, on invite l'utilisateur à connecter le configurateur
	int waitingLoop = LCD_MAX_LENGTH;

	// On attend le chargement du programme de germination
	// Le délai de 2 secondes est nécessaire pour afficher la ligne sur le LCD
	delay(DISPLAY_TIME);
	lcd.displayCenter("INITIALISATION", LCD::DISPLAY_TOP);

	// Boucle d'attente de chargement du programme de germination
	while(initPhase){

		// On affiche une série de points sur la ligne du bas pour créer une animation
		// d'attente de chargement. A chaque itération de la boucle et si le compteur
		// n'a pas atteint la valeur maximale de l'écran LCD, on affiche un nouveau point à la fin de la ligne
		if(waitingLoop < LCD_MAX_LENGTH){
			waitingLoop++;
			delay(LOOP_DELAY);
			lcd.displayAfter(".");
		}

		// On a rempli la ligne. On affiche à nouveau l'invitation à l'utilisateur de
		// connecter le configurateur avant de recommencer une série de 16 points
		else{
			waitingLoop = 0;
			lcd.displayCenter("CONNECTER PC", LCD::DISPLAY_BOTTOM);
			delay(DISPLAY_TIME);
			lcd.displayAt(".", LCD::DISPLAY_BOTTOM, 0);
		}

		// On envoie une invitation de téléchargement sur le port USB
		Serial.println("INIT:GET_PROGRAM");
		readSerial();
	}

	// On est connecté au PC configurateur, on enclenche le téléchargement du programme
	while(!isTimeSet){
		Serial.println("INIT:GET_TIME");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(redOn == -9999){
		Serial.println("INIT:GET_RED_LEVEL");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(greenOn == -9999){
		Serial.println("INIT:GET_GREEN_LEVEL");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(blueOn == -9999){
		Serial.println("INIT:GET_BLUE_LEVEL");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(lightOn == -9999){
		Serial.println("INIT:GET_LIGHT_ON");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(lightOff == -9999){
		Serial.println("INIT:GET_LIGHT_OFF");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(flowOn == -9999){
		Serial.println("INIT:GET_FLOW_ON");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(flowOff == -9999){
		Serial.println("INIT:GET_FLOW_OFF");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(waterLow == -9999){
		Serial.println("INIT:GET_WATER_LOW");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(waterHigh == -9999){
		Serial.println("INIT:GET_WATER_HIGH");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(airLow == -9999){
		Serial.println("INIT:GET_AIR_LOW");
		delay(LOOP_DELAY);
		readSerial();
	}
	while(airHigh == -9999){
		Serial.println("INIT:GET_AIR_HIGH");
		delay(LOOP_DELAY);
		readSerial();
	}

	// Au démarrage, on allume la pompe
	flowCounter = -1;

	// Finallement, on affiche l'horloge
	lcd.setClock();
}

// Boucle principale
//
void loop(){

	//
	// A chaque itération de la boucle principale:
	//

	// On vérifie si une touche a été pressée
	int keys = getKeys();

	// Les deux boutons ont été pressés
	if(keys == ACTION_BOTH){

		// Si on avait d'abord pressé le bouton de gauche, il faut remettre la lumière dans son état initial
		if(inspect) setInspect(false);
	}

	// Le bouton de gauche a été pressé, on affiche en boucle les paramètres de l'unité de germination
	// après avoir rafraîchi les valeurs lues par les capteurs
	else if(keys == ACTION_LEFT && lcdDisplay == -1){
		getProbesValues();
		lcdDisplay = 0;
	}

	// Le bouton de droite a été pressé, on allume la lumière verte pour vérifier l'état des pousses
	else if(keys == ACTION_RIGHT && !inspect) setInspect(true);

	//Aucun bouton n'est pressé mais si cela privient du relâchement d'un bouton, il y a peut être une action à prendre
	else if(keys == ACTION_NOTHING){

		// On a relaché le bouton d'inspection, il faut remettre la lumière dans son état initial
		if(inspect) setInspect(false);
	}

	// On vérifie si on a un déclencheur d'évènement particulier à activer
	keepEventCounters();

	// On vérifie si un message est arrivé sur le port USB
	readSerial();

	// Et on patiente un tout petit peu...
	delay(LOOP_DELAY);
}

// Procédure qui ajuste la vitesse du ventilateur
// *speed* donne le pourcentage de la vitesse de rotation (0..100), 0 est éteint et 100 pleine vitesse
//
void setFan(int speed){
	if(fanSpeed != speed){
		fanSpeed = speed;
		analogWrite(FAN_CMD, analogLevel(speed));
		sendUSBValue("FAN", speed);
	}
}

// Procédure qui démarre la résistance chauffante si elle n'est pas encore allumée
//
void heatOn(){
	if(!isHeatOn){
		digitalWrite(RELAY_1_CMD, LOW);
		isHeatOn = true;
		Serial.println("INFO:HEAT=ON");
	}
}

// Procédure qui arrête la résistance chauffante si elle n'est pas encore éteinte
//
void heatOff(){
	if(isHeatOn){
		digitalWrite(RELAY_1_CMD, HIGH);
		isHeatOn = false;
		Serial.println("INFO:HEAT=OFF");
	}
}

// Procédure qui démarre la pompe d'arrosage si elle n'est pas encore allumée
//
void pumpOn(){
	if(!isPumpOn){
		digitalWrite(RELAY_2_CMD, LOW);
		isPumpOn = true;
		Serial.println("INFO:FLOW=ON");
	}
}

// Procédure qui arrête la pompe d'arrosage si elle n'est pas encore éteinte
//
void pumpOff(){
	if(isPumpOn){
		digitalWrite(RELAY_2_CMD, HIGH);
		isPumpOn = false;
		Serial.println("INFO:FLOW=OFF");
	}
}

// Procédure permettant de régler les différentes composantes de l'éclairage LED
// 	r donne le pourcentage de la composante ROUGE (0..100), 0 est éteint et 100 pleine illumination
// 	g donne le pourcentage de la composante VERTE (0..100), 0 est éteint et 100 pleine illumination
// 	b donne le pourcentage de la composante BLEUE (0..100), 0 est éteint et 100 pleine illumination
//
void setLED(int red, int green, int blue){

	// On allume les composantes correspondantes via les GPIO
	analogWrite(R_PIN, analogLevel(red));
	analogWrite(G_PIN, analogLevel(green));
	analogWrite(B_PIN, analogLevel(blue));

	// On considère la lumière verte comme invisible par les plantes
	if(red + blue == 0){
		isLightOn = false;
		Serial.println("INFO:LIGHT=OFF");
	}
	else{
		isLightOn = true;
		Serial.println("INFO:LIGHT=ON");
	}
}
// Procédure utilisée pour allumer ou éteindre la composante verte des LEDs
// Utilisée avec le bouton de droite afin de permettre l'inspection des semis
//

void setInspect(bool state){
	if(state)	analogWrite(G_PIN, 255);
	else analogWrite(G_PIN, 0);
	inspect = state; 
}

// Procédure appelée chaque minute et vérifiant si il faut allumer ou éteindre les LEDs
//
void checkLED(){

	// Si on est en mode inspection, pas besoin de vérifier
	if(!inspect){

		// Récupère le temps présent 
		int now = 60 * hour() + minute();

		// L'allumage et l'extinction sont dans la même journée
		if(lightOn < lightOff){
			if(now >= lightOn && now < lightOff){
				if(!isLightOn) setLED(redOn, greenOn, blueOn);
			}
			else{
				if(isLightOn) setLED(0, 0, 0);
			}
		}

		// L'allumage et l'extinction sont dans des journées différentes
		else{
			if(now >= lightOff && now < lightOn){
				if(isLightOn) setLED(0, 0, 0);
			}
			else{
				if(!isLightOn) setLED(redOn, greenOn, blueOn);
			}
		}
	}
}

// Procédure appelée chaque seconde qui vérifie si on doit afficher les paramètres de l'unité sur l'écran LCD et les fait défiler
//
void checkLCD(){

	// Chaînes utilisée pour convertir des réels en chaîne de caractère
	char string2Display[LCD_MAX_LENGTH] = "";
	char float2String[FLOAT_MAX_LENGTH] = "";

	// Si le compteur est plus grand ou égal à zéro, c'est que l'on doit afficher quelque chose
	if(lcdDisplay >= 0){
		lcdDisplay++;
		switch(lcdDisplay){
			case 1:
				lcd.displayCenter("PROGRAMME", LCD::DISPLAY_TOP);
				lcd.displayCenter(programName, LCD::DISPLAY_BOTTOM);
			break;
			case 3:
				dtostrf(airTemperature, 5, 1, float2String); 
				snprintf(string2Display, LCD_MAX_LENGTH, "%s%cC", float2String, LCD::SYMBOL_DEGREE);
				lcd.displayCenter("TEMP AIR", LCD::DISPLAY_TOP);
				lcd.displayCenter(string2Display, LCD::DISPLAY_BOTTOM);
			break;
			case 5:
				dtostrf(airHumidity, 5, 1, float2String); 
				snprintf(string2Display, LCD_MAX_LENGTH, "%s%c", float2String, 0x25);
				lcd.displayCenter("HUMIDITE AIR", LCD::DISPLAY_TOP);
				lcd.displayCenter(string2Display, LCD::DISPLAY_BOTTOM);
			break;
			case 7:
				dtostrf(waterTemperature, 5, 1, float2String); 
				snprintf(string2Display, LCD_MAX_LENGTH, "%s%cC", float2String, LCD::SYMBOL_DEGREE);
				lcd.displayCenter("TEMP EAU", LCD::DISPLAY_TOP);
				lcd.displayCenter(string2Display, LCD::DISPLAY_BOTTOM);
			break;
			case 9:
				lcd.displayClock();
				lcdDisplay = -1;
		}
	}
}

// Procédure appelée chaque seconde qui vérifie si on doit allumer ou éteindre la pompe d'arrosage
//
void checkPump(){
	if(flowCounter > 0){
		flowCounter--;
		if(flowCounter == 0){
			flowCounter = -flowOff;
			pumpOff();
		}
	}
	else{
		flowCounter++;
		if(flowCounter == 0){
			flowCounter = flowOn;
			pumpOn();
		}
	}
}

// Procédure qui permet de relever la température de l'air dans l'unité hydroponique
//
void getAirTemperature(){
	airTemperature = dht.readTemperature();
}

// Procédure qui permet de relever l'humidité de l'air dans l'unité hydroponique
//
void getAirHumidity(){
	airHumidity = dht.readHumidity();
}

// Procédure qui permet de relever la température de l'eau du bassin d'hydroculture
//
void getWaterTemperature(){
	waterSensor.requestTemperatures();
	waterTemperature = waterSensor.getTempCByIndex(0);
}

// Procédure qui collecte les valeurs des sondes
//
void getProbesValues(){
	getAirTemperature();
	getAirHumidity();
	getWaterTemperature();
}

// Procédure qui envoie les valeurs des sondes au PC de surveillance
//
void sendProbesValues(){
	sendUSBValue("AIR_TEMP", airTemperature, 5, 2);
	sendUSBValue("AIR_HUM", airHumidity, 5, 2);
	sendUSBValue("WATER_TEMP", waterTemperature, 5, 2);
}

// Procédure qui prend les actions correctives si les paramètres sous contrôles
// dépassent les valeurs limites définies par le programme
//
void provideFeedbacks(){

	// Correction de l'air
	if(airTemperature >= airHigh) setFan(100);
	if(airTemperature <= airHigh - TEMPERATURE_TOLERENCE) setFan(0);

	// Correction de l'eau
	if(waterTemperature <= waterLow) heatOn();
	if(waterTemperature >= waterHigh) heatOff();
}

// Fonction qui renvoie la valeur correspondante aux touches enfoncées
//
int getKeys(){
	int rV = ACTION_NOTHING; 
	if(analogRead(ANALOG_BUTTON_RIGHT) < BUTTON_COMPARE) rV += ACTION_RIGHT;
	if(analogRead(ANALOG_BUTTON_LEFT) < BUTTON_COMPARE) rV += ACTION_LEFT;
	return rV;
}

// Fonction qui adapte un pourcentage à un intervalle de valeurs de 0 à 255
// On vérifie la valeur en entrée et on la limite à l'intervalle 0..255
//
int analogLevel(int percentage){
	if(percentage > 100) return 255;
	if(percentage < 0) return 0;
	return percentage * 2.55;
}

// Ecoute le port série et analyse les messages reçus
//
void readSerial(){

	// Si il y a des données présentes sur le port série
	if(Serial.available() > 0){

		// On les lit (la fin est supposée toujours être délimitée par un "\n")
		String readData = Serial.readStringUntil('\n');

		// On extrait la position du séparateur afin d'isoler la commande de son paramètre
		int separator = readData.indexOf(':');

		// Si on a trouvé au moins une commande à analyser, on vérifie et on l'exécute si elle existe
		if(separator != -1){

			// On sépare la commande des données
			String command = readData.substring(0, separator);
			String param = readData.substring(separator + 1);

			/*
			** Dans la section qui suit, on déroule les actions correspondantes à la commande à exécuter
			*/

			if(command == "SET_PROGRAM"){
				param.toCharArray(programName, LCD_MAX_LENGTH);
				lcd.displayCenter("PROGRAMME", LCD::DISPLAY_TOP);
				lcd.displayCenter(programName, LCD::DISPLAY_BOTTOM);
				initPhase = false;
			}
			else if(command == "SET_TIME"){
				setTime(param.toInt());
				isTimeSet = true;
			}
			else if(command == "SET_RED_LEVEL"){
				redOn = param.toInt();
			}
			else if(command == "SET_GREEN_LEVEL"){
				greenOn = param.toInt();
			}
			else if(command == "SET_BLUE_LEVEL"){
				blueOn = param.toInt();
			}
			else if(command == "SET_LIGHT_ON"){
				int hours = param.substring(0, 2).toInt();
				int minutes = param.substring(3, 5).toInt();
				lightOn = 60 * hours + minutes;
			}
			else if(command == "SET_LIGHT_OFF"){
				int hours = param.substring(0, 2).toInt();
				int minutes = param.substring(3, 5).toInt();
				lightOff = 60 * hours + minutes;
			}
			else if(command == "SET_FLOW_ON"){
				flowOn = 60 * param.toInt();
			}
			else if(command == "SET_FLOW_OFF"){
				flowOff = 60 * param.toInt();
			}
			else if(command == "SET_WATER_LOW"){
				waterLow = param.toFloat();
			}
			else if(command == "SET_WATER_HIGH"){
				waterHigh = param.toFloat();
			}
			else if(command == "SET_AIR_LOW"){
				airLow = param.toFloat();
			}
			else if(command == "SET_AIR_HIGH"){
				airHigh = param.toFloat();
			}
		}
	}
}

// Procédure qui envoie un paramètre de type entier au Raspberry
// La phrase envoyée est du type ARDUINO_NAME:parameter:value
//
void sendUSBValue(const char* parameter, int value){
	char string2Send[SERIAL_MAX_LENGTH] = "";
	snprintf(string2Send, SERIAL_MAX_LENGTH, "INFO:%s=%d", parameter, value);
	Serial.println(string2Send);
}

// Procédure qui envoie un paramètre de type réel au Raspberry
// La conversion se fait par la fonction dtostfr car le format %f de la commande snprintf n'est pas supporté par l'Arduino
// Le paramètre width donne la longueur totale du float converti, en incluant le signe, la virgule et les décimaux
// Le paramètre precision donne le nombre de chiffres derrière la virgule
// La phrase envoyée est du type ARDUINO_NAME:parameter:value
//
void sendUSBValue(const char* parameter, float value, int width, int precision){
	char string2Send[SERIAL_MAX_LENGTH] = "";
	char float2String[FLOAT_MAX_LENGTH] = "";
	dtostrf(value, width, precision, float2String); 
	snprintf(string2Send, SERIAL_MAX_LENGTH, "INFO:%s=%s", parameter, float2String);
	Serial.println(string2Send);
}

// Procédure appelée à chaque itération de la boucle principale
// Vérifie l'état des compteurs d'événements par rapport à l'état du système
// Prend les mesures adéquates lors de transitions
//
void keepEventCounters(){

	// Décompte le temps qui s'écoule pour les différents déclencheurs
	// Si le temps est écoulé, pour un déclencheur, on effectue les opérations liées à ce déclencheur
	// Après avoir déroulé les opérations, on reprogramme le déclencheur
	secondDelay -= LOOP_DELAY;
	minuteDelay -= LOOP_DELAY;
	quarterDelay -= LOOP_DELAY;

	// Chaque seconde...
	if(secondDelay == 0){

		// On vérifie l'état de la pompe et on adapte si on change de plage
		checkPump();

		// On vérifie si on doit allumer ou éteindre les LEDs
		checkLED();

		// On vérifie si on doit afficher les paramètres de l'unité sur l'écran LCD
		checkLCD();

		// On reprogramme le déclencheur suivant
		secondDelay = SECOND_DELAY;
	}

	// Chaque minute...
	if(minuteDelay == 0){

		// On prend les mesures provenant des différentes sondes
		getProbesValues();

		// On prend une action corrective si une valeur dépasse les limites
		provideFeedbacks();

		// On envoie les mesures provenant des différentes sondes vers le PC de surveillance
		sendProbesValues();

		// On reprogramme le déclencheur suivant
		minuteDelay = MINUTE_DELAY;
	}

	// Chaque quart d'heure...
	if(quarterDelay == 0){

		// On reprogramme le déclencheur suivant
		quarterDelay = QUARTER_DELAY;
	}
}
