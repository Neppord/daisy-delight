#! /usr/bin/python
#coding: latin1

class Observable:
	def __init__(self):
		self.observers = []
		self.hasChanged = []

	def addObserver(self, observer):
		self.observers.append(observer)

	def notifyAll(self):
		if self.hasChanged:
			for x in self.observers:
				x.update(self)
			self.hasChanged = []
	
	def setChanged(self, *value):
		if not value: self.hasChanged.append(None) 
		for v in value:
			self.hasChanged.append(v)
		
