#!/usr/bin/python
# -*- coding: utf-8 -*-

from time import time, strftime

# Classe permettant contenant toutes les valeurs sous contrôle de l'unité VERT-X
#
class Params:

	# Liste des constantes
	#
	AIR_TEMPERATURE = 'AIR_TEMP'
	AIR_HUMIDITY = 'AIR_HUM'

	# Méthode permettant d'assigner une valeur à un paramètre
	# Comme les données viennent des Arduinos, elles sont toujours représentées sous forme d'une chaine de caractères
	# Elles sont stockées telles quelles et seront traduites si besoin suivant le type défini par le dictionnaire
	# Si la clé a été trouvée et la valeur insérée, on retourne True, sinon False
	# A noter que l'on insère également le moment auquel la valeur a été modifiée ce qui nous permet de vérifier la frîcheur des données
	#
	def setParameterValue(self, pName, pValue):
		if self.params[pName] != None:
			self.params[pName][0] = pValue
			self.params[pName][1] = time();
			return True
		else:
			return False

	# Méthode permettant d'obtenir la valeur d'un paramètre et de la retourner en tant que chaîne de caractères
	# Si la clé a été trouvée on renvoie la valeur correspondante, sinon None
	#
	def getParameterAsString(self, pName):
		if self.params[pName] != None:
			return self.params[pName][0]
		else:
			return None

	# Méthode permettant d'obtenir la valeur d'un paramètre, traduite suivant son type défini dans le dictionnaire
	# Si la clé a été trouvée on renvoie la valeur correspondante, sinon None
	#
	def getParameterAsValue(self, pName):
		if self.params[pName] != None:
			return self.params[pName][2](self.params[pName][0])
		else:
			return None

	# Méthode permettant d'obtenir la fraîcheur d'un parammètre
	# Si la clé a été trouvée on renvoie le temps écoulé depuis la dernière mise à jour, sinon None
	#
	def getParameterFreshness(self, pName):
		if self.params[pName] != None:
			return time() - self.params[pName][1]
		else:
			return None

	# Constructeur de la classe
	#
	def __init__(self):

		# Définition du dictionnaire des paramètres
		initTime = time()
		self.params = {self.AIR_TEMPERATURE: [None, initTime, float],
									 self.AIR_HUMIDITY: [None, initTime, float] }
