#!/usr/bin/python
# -*- coding: utf-8 -*-

import ConfigParser

# Classe permettant de charger les programmes de germination
#
class LoadProgram:

	# Liste des constantes
	#
	PROGRAMS_FILE = 'programs.cfg'

	# Méthode récupérant la valeur d'un paramètre du programme
	#
	def getParameter(self, parameter):

		# La valeur du paramètre n'a pas encore été trouvée
		value = None

		# Cherche la valeur du paramètre pour le programme chargé à partir du fichier
		if parameter in self.program:
			value = self.program[parameter]
		return value

	# Constructeur de la classe
	# Ce constructeur prend un paramètre en entrée:
	#		* locale: la locale utilisée dans le programme
	#
	def __init__(self, programName):

		# Sauve le nom du programme
		self.programName = programName

		# Prépare le parser, lit le fichier et stocke les traductions dans une structure
		configFile = ConfigParser.ConfigParser()
		configFile.read([self.PROGRAMS_FILE])
		self.program = dict(configFile.items(programName))
