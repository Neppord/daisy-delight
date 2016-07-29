#! /usr/bin/python
#coding: latin1
from daisyDelight.audio import Output, AudioFile, Converter
from threading import Timer

class Player:

	def __init__(self, state):
		state.addObserver(self)
		self.currClip= None
		self.timer = None
		self.output = Output()
		self.state = state
		self.audioFile = None
		self.converter = None
		self.mp3file = None

	def update(self, observable, *args):
		#print "player: updates"
			
		if observable.currClip != self.currClip:
			self.stop()
			self.currClip = observable.currClip
			self.loadAudioFile()

		if observable.isPaused and self.output.isPlaying:
			self.stop()

		if not observable.isPaused and not self.output.isPlaying:
			self.play()

	def play(self):
		#print "is playing"	
		self.timer=Timer(self.currClip[1]-float(self.audioFile.pos),self.atEndOfFile)
		self.output.play(self.audioFile,self.converter)
		self.timer.start()

	def stop(self):
		if self.timer and self.timer.isAlive():
			self.timer.cancel()
		self.output.stop()

	def loadAudioFile(self):
		if self.mp3file != self.currClip[2]:
			self.mp3file = self.currClip[2]
			self.audioFile = AudioFile(self.mp3file)
		self.audioFile.pos = self.currClip[0]
		self.converter = Converter(self.audioFile,
		self.output)	

	def atEndOfFile(self):
		self.output.stop()
		self.state.skip()


