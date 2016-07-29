#! /usr/bin/python
#coding: latin1
import ossaudiodev, mad
class MadPlayer:
	
	def __init__(self, state):
		self.output = ossaudiodev.open('w')
		self.Clip = None
		self.file = None
		self.state = state
		state.addObserver(self)
		self.isPlaying = False
		self.src = None
		self.t=None
	def update(self, observable, *args):
		if "clip" in observable.hasChanged:
			self.stop()
			self.Clip = observable.currClip
			self.loadAudioFile()

		if observable.isPaused and self.isPlaying:
			self.stop()

		if not observable.isPaused and not self.isPlaying:
			self.play()

	def play(self):
		def setOutParam(self):
			format=ossaudiodev.AFMT_S16_BE
			numberOfChannels=2#self.file.mode()
			samplerate=self.file.samplerate()
			(f,n,s)=self.output.setparameters(format,numberOfChannels,samplerate)
#			print "Frormat %d/%d Channels %d/%d samlplerate %d/%d"%(f,format,n,numberOfChannels,s,samplerate)
			if f!=format or s!=samplerate or n!=numberOfChannels:
				print "format not suported"
				import sys
				sys.exit(1)
			
		
		def playhelper(self):
			setOutParam(self)
			buff=self.file.read()
			while buff and self.isPlaying and self.file.current_time()<(self.Clip[1]*1000):
				self.output.write(buff)
				buff=self.file.read()
			if self.isPlaying:
				self.state.skip()
		import threading
		if self.t and self.t.isAlive and self.t!=threading.currentThread():
			self.t.join()	
		self.isPlaying=True
		self.t=threading.Thread(target=playhelper, args=(self,))
		self.t.start()

	def stop(self):
		self.isPlaying = False

	def loadAudioFile(self):
		if not self.src == self.Clip[2] or self.file.current_time()>self.Clip[0]*1000:
			self.file = mad.MadFile(self.Clip[2])
			self.src = self.Clip[2]
		while self.file.current_time()<int(self.Clip[0]*1000):
			self.buff=self.file.read()

