#!/usr/bin/python
# -*- coding: utf-8 -*-

from time import sleep
import threading
import serial
import logging

# Classe implémentant la communication série entre le Raspberry et les Arduinos sur un port USB
#
class USBDaemon:


	# Liste des constantes
	#
	READ_TIME = 0.1						# Durée entre deux lectures du port USB en secondes
	RECONNECT_TIME = 5				# Durée entre deux tentatives de reconnexion
	NBR_OF_RECONNECTIONS = 3	# Nombre de tentatives de reconnexion avant la levée d'une alarme

	# Méthode qui tourne constamment en thread, lancée par le constructeur
	# Ecoute le port USB en provenance de l'Arduino
	# Si une communication est détectée, les données sont dispatchées aux observateurs de la classe
	#
	def __listenUSB(self):

		# On ne reprogramme l'écoute du port USB que si le programme principal n'a pas demandé un arrêt général
		if self.running == True:

			# On ne lit que si des données sont présentes
			try:
				if self.arduino.inWaiting() > 0:
					self.messageReceived = self.arduino.readline().strip()

					# On appelle le callback pour traitement du message
					self.callback(self.messageReceived)

			# On a un incident à la lecture du port USB
			except IOError as e:

				# On notifie par le callback qu'une erreur de lecture est survenue
				self.callback('ARDUINO_READ:WARNING')
				
				# On on initialise la reconnexion
				reconnectCounter = 0
				reconnected = False

				# Tant qu'on n'a pas réussi une reconnexion, on essaie, à condition que le programme est toujours en cours
				# Chaque fois que l'on est arrivé au nombre maximum d'essais, on remonte une ALERT
				while reconnected == False and self.running == True:

					# Tentative de reconnexion - On ferme d'abord le port USB
					# Si on y arrive, on le signale afin de sortir de la boucle
					# Et on notifie le callback
					try:
						self.arduino.close()
						sleep(self.RECONNECT_TIME)
						self.__openUSB()
						reconnected = True
						self.callback(self.machineName + 'ARDUINO_READ:OK')

					# Echec à la tentative de reconnexion - On incrémente le compteur des essais
					except serial.SerialException as e:
						reconnectCounter += 1

						# Si le nombre maximum de reconnexion est atteint, on rapporte l'incident en ALERT
						if reconnectCounter == self.NBR_OF_RECONNECTIONS:

							# On fait passer l'alarme au niveau ALERT en notifiant le callback 
							self.callback(self.machineName + ':ARDUINO_READ:ERROR')

			# Finalement, on reboucle pour continuer d'écouter le port USB jusqu'à la fin du programme
			readTimer = threading.Timer(self.READ_TIME, self.__listenUSB)
			readTimer.start()

	# Méthode permettant d'ouvrir la connexion au port USB
	#
	def __openUSB(self):

		# On essaie de créer l'objet de communication avec les valeurs d'initialisation
		try:
			self.arduino = serial.Serial(self.serialName, self.serialSpeed)

			# On supprime tout buffer pour être sûr de ne pas avoir de débris de communication
			self.arduino.flushInput()
			self.arduino.flushOutput()

		# Si on n'y arrive pas, on remonte une exception
		except serial.SerialException as e:
			raise e

	# Méthode permettant d'envoyer une commande à l'Arduino
	#
	def sendCommand(self, commandString):

		# On essaie d'envoyer une commande via le port USB et on temporise un tout petit peu pour éviter d'en envoyer une autre trop vite
		try:
			self.arduino.write(commandString + '\n')

		# Si on n'y arrive pas, on lève une exception
		except serial.SerialException as e:
			raise e

	# Méthode permettant d'arrêter de façon propre la classe par arrêt programmé des threads
	#
	def stop(self):
		self.arduino.close()
		self.running = False

	# Constructeur de la classe. Ce constructeur prend en entrée les paramètres suivants:
	#		* serialName: le nom du port USB à utiliser pour la communication (ex: /dev/ttyACM0)
	#		* serialSpeed: la vitesse de communication en bauds
	#		* warningCallback: la fonction à exécuter en cas d'alarme de type WARNING
	#		* alertCallback: la fonction à exécuter en cas d'alarme de type ALERT
	#		* clearAlarmCallback: la fonction à exécuter en cas de suppression d'une alarme en cours
	#
	def __init__(self, serialName, serialSpeed, callback):

		try:

			# On sauvegarde les valeurs d'initialisation si on doit redémarrer la communication
			# après une erreur survenue sur le port USB
			self.serialName = serialName
			self.serialSpeed = serialSpeed
			self.callback = callback

			# On essaie de créer l'objet de communication avec les valeurs d'initialisation
			self.arduino = None
			self.__openUSB();

			# Création des variables de contôle de la classe
			self.messageReceived = None
			self.running = True

			# Démarre l'écoute du port série
			self.__listenUSB()

		# Si on n'y arrive pas, on remonte une exception
		except serial.SerialException as e:
			raise e
