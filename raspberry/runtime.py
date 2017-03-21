#!/usr/bin/python
# -*- coding: utf-8 -*-

# Import des librairies
import os
import logging
import logging.config
from time import time, sleep, strftime
import serial
from usb import USBDaemon
from loadprogram import LoadProgram
import requests
from requests.auth import HTTPBasicAuth

# Définition du programme de germination
PROGRAM = 'BASILIC'

# Définition du fuseau horaire pour l'adaptation de l'heure UTC
GMT = 1

# Définition du logger
LOGGING_CONF = 'logging.conf'
LOGGER = 'vertx.logger'

# Définition des propriétés de communication série avec l'unité de germination
ARDUINO_PORT = '/dev/ttyUSB0'
ARDUINO_SPEED = 115200
ARDUINO_CONNECT_WAIT = 2;	
ARDUINO_CONNECT_RETRY = 10;	

# Définition des paramètres de configuration pour les services internet
HTTP_BASE_URL = 'https://vertx.zetof.net'
HTTP_USER = 'vertx'
HTTP_PWD = 'LeVertCaGere'
HTTP_TIMEOUT = 2

# On n'est pas en phase de quitter le programme
running = True

# Lecture du programme de germination à dérouler
program = LoadProgram(PROGRAM)

# Méthode permettant d'enregistrer une valeur de capteur dans la base de données
#
def dbStore(table, value):
	url = HTTP_BASE_URL + '/insert'
	data = {'table_name':table, 'param_value':value}
	r = requests.post(url, auth=HTTPBasicAuth(HTTP_USER, HTTP_PWD), data=data, timeout=HTTP_TIMEOUT)

# Fonction qui imprime à l'écran les données du programme
#
def printProgram():
	os.system('clear')
	print '                        *** PROGRAM: ' + program.programName + ' ***\n'
	print '         --> Paramètres d\'éclairage:\n'
	print '                    Eclairage Rouge: ' + program.getParameter('light.red') + '%'
	print '                     Eclairage Vert: ' + program.getParameter('light.green') + '%'
	print '                     Eclairage Bleu: ' + program.getParameter('light.blue') + '%'
	print '         Heure de début d\'éclairage: ' + program.getParameter('light.on')
	print '           Heure de fin d\'éclairage: ' + program.getParameter('light.off') + '\n'
	print '          --> Paramètres d\'arrosage:\n'
	print 'Plage de fonctionnement de la pompe: ' + program.getParameter('water.flow.on') + 'min'
	print '         Plage de repos de la pompe: ' + program.getParameter('water.flow.off') + 'min\n'
	print '            --> Paramètres de l\'air:\n'
	print '         Température basse de l\'air: ' + program.getParameter('air.temperature.low') + '°C'
	print '         Température haute de l\'air: ' + program.getParameter('air.temperature.high') + '°C\n'
	print '            --> Paramètres de l\'eau:\n'
	print '         Température basse de l\'eau: ' + program.getParameter('water.temperature.low') + '°C'
	print '         Température haute de l\'eau: ' + program.getParameter('water.temperature.high') + '°C\n'

# Fonction qui traite des problèmes de lecture sur le port USB d'un Arduino
#
def arduinoReadProblem(data):

	# Traitement des différents cas d'erreurs / redémarrage
	if data == 'WARNING':
		logger.warning('Erreur ponctuelle à la lecture des données de l\'unité de germination')
	elif data == 'ERROR':
		logger.error('Erreur récurrente à la lecture des données de l\'unité de germination')
	elif data == 'OK':
		logger.info('Reprise de la lecture des données de l\'unité de germination')

# Fonction qui initialise l'unité de germination
#
def initArduino(data):

	# Traitement des différents cas d'initialisation / envoi des paramètres du programma
	#
	if data == 'GET_PROGRAM':
		arduino.sendCommand('SET_PROGRAM:' + program.programName)
	elif data == 'GET_TIME':
		arduino.sendCommand('SET_TIME:' + str(int(time()) + 3600 * GMT))
	elif data == 'GET_RED_LEVEL':
		arduino.sendCommand('SET_RED_LEVEL:' + program.getParameter('light.red'))
	elif data == 'GET_GREEN_LEVEL':
		arduino.sendCommand('SET_GREEN_LEVEL:' + program.getParameter('light.green'))
	elif data == 'GET_BLUE_LEVEL':
		arduino.sendCommand('SET_BLUE_LEVEL:' + program.getParameter('light.blue'))
	elif data == 'GET_LIGHT_ON':
		arduino.sendCommand('SET_LIGHT_ON:' + program.getParameter('light.on'))
	elif data == 'GET_LIGHT_OFF':
		arduino.sendCommand('SET_LIGHT_OFF:' + program.getParameter('light.off'))
	elif data == 'GET_FLOW_ON':
		arduino.sendCommand('SET_FLOW_ON:' + program.getParameter('water.flow.on'))
	elif data == 'GET_FLOW_OFF':
		arduino.sendCommand('SET_FLOW_OFF:' + program.getParameter('water.flow.off'))
	elif data == 'GET_WATER_LOW':
		arduino.sendCommand('SET_WATER_LOW:' + program.getParameter('water.temperature.low'))
	elif data == 'GET_WATER_HIGH':
		arduino.sendCommand('SET_WATER_HIGH:' + program.getParameter('water.temperature.high'))
	elif data == 'GET_AIR_LOW':
		arduino.sendCommand('SET_AIR_LOW:' + program.getParameter('air.temperature.low'))
	elif data == 'GET_AIR_HIGH':
		arduino.sendCommand('SET_AIR_HIGH:' + program.getParameter('air.temperature.high'))

# Fonction qui enregistre une action ou une valeur de paramètre provenant de l'unité de germination
#
def logInfo(data):

	# Si on a une valeur de paramètre, on la retrouve dans les données passées
	splitData = data.split('=')
	action = splitData[0]
	value = splitData[1]

	# On vérifie le type d'action à enregistrer et on effectue l'enregistrement correspondant
	if action == 'FLOW':
		if value == 'ON':
			logger.info('Allumage de la pompe d\'arrosage')
			# dbStore('fan_state', '1')
		elif value == 'OFF':
			logger.info('Arrêt de la pompe d\'arrosage')
			# dbStore('fan_state', '0')
	elif action == 'HEAT':
		if value == 'ON':
			# dbStore('heat_state', '1')
			logger.info('Allumage de la résistance chauffante')
		elif value == 'OFF':
			# dbStore('heat_state', '0')
			logger.info('Arrêt de la résistance chauffante')
	elif action == 'LIGHT':
		if value == 'ON':
			# dbStore('light_state', '1')
			logger.info('Allumage de l\'éclairage horticole')
		elif value == 'OFF':
			logger.info('Extinction de l\'éclairage horticole')
			# dbStore('light_state', '0')
	elif action == 'FAN':
		logger.info('Réglage du ventilateur à ' + value + '% de la puissance')
		# dbStore('fan_state', value)
	elif action == 'AIR_TEMP':
		logger.info('Température de l\'air: ' + value + '°C')
		# dbStore('air_temp', value)
	elif action == 'AIR_HUM':
		logger.info('Humidité de l\'air: ' + value + '%')
		# dbStore('air_hum', value)
	elif action == 'WATER_TEMP':
		logger.info('Température de l\'eau: ' + value + '°C')
		# dbStore('water_temp', value)

# Définition du callback lors de la réception d'un message venant d'un Arduino
# Ce callback prend en compte l'analyse des messages venant d'un Arduino et
# l'action associée
#
def processDataFromArduino(receivedData):

	# On sépare les parties du message reçu
	splitData = receivedData.split(':')

	# On définit un dictionnaire pour le traitement des différents messages
	action = {'ARDUINO_READ': arduinoReadProblem,
						'INIT': initArduino,
						'INFO': logInfo}
	
	# Finalement, on appelle la fonction correspondante à la commande sur base du dictionnaire
	# Si la clé n'existe pas, il est nécessaire d'intercepter l'erreur pour éviter tout problème
	# En cas de problème, on le reporte dans les logs
	try:
		action[splitData[0]](splitData[1])

	except KeyError as e:
		logger.error('La commande ' + splitData[0] + ' n\'existe pas')

# Initialisation du logging
logging.config.fileConfig(LOGGING_CONF)
logger = logging.getLogger(LOGGER)

# Tant qu'on n'a pas réussi une connection vers l'unité de germination, on réessaie
connect_attempt = 0
arduino = None
while arduino == None:

	# On tente une connection vers l'unité
	try:
		arduino = USBDaemon(ARDUINO_PORT, ARDUINO_SPEED, processDataFromArduino)

	# Si on y arrive pas, on rapporte l'erreur et on boucle
	except serial.SerialException as e:
				
		# On incrémente le compteur d'essai de reconnexion
		connect_attempt += 1;

		# Si on a dépassé le nombre limite, on le signale dans les logs et on remet le compteur à zéro
		if connect_attempt > ARDUINO_CONNECT_RETRY:
			logger.error('Problème à la connexion de l\'unité de germination')
			connect_attempt = 0;

		# On attend un instant avant de retenter
		sleep(ARDUINO_CONNECT_WAIT)

# On enregistre le démarrage de l'unité
logger.info('Démarrage du programme de contrôle\n***\n*** Démarrage de l\'unité de germination ***\n***')
# dbStore('app_start', program.programName)

# On attend la fin du programme demandée par l'utilisateur
while running == True:

	# On imprime les données du programme en cours
	printProgram()

	# A tout moment, l'utilisateur peut quitter le programme en entrant le mot 'exit'
	userInput = raw_input('Pour terminer le PGM, entrer \'exit\': ')
	if userInput.strip() == 'exit':
		running = False

# On fait le ménage à la sortie de la boucle principale, avant d'aller faire dodo
arduino.stop()
