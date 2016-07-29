#! python
import unittest
import time
import curses
import sys
from daisyDelight import audio
prompt=False
def display(window, self):
  window.nodelay(True)
  while self.Output.isPlaying:
    window.addstr(0,0,"posisition %2d:%2d:%2d of %2d:%2d:%2d"%(
        self.AudioFile.pos/(60*60),
        (self.AudioFile.pos/60)%60,
        self.AudioFile.pos%60,
        self.AudioFile.length/(60*60),
        (self.AudioFile.length/60)%60,
	self.AudioFile.length%60
	)
      )
    try:
      key=window.getkey()
    except curses.error:
      key=None
    window.addstr(1,1,str(key))
    window.refresh()
    if(key=="KEY_RIGHT"):
      if((self.AudioFile.pos+10)>self.AudioFile.length):
	self.AudioFile.pos=self.AudioFile.length
      else:
	self.AudioFile.pos+=10
    elif(key=="KEY_LEFT"):
      if((self.AudioFile.pos-10)<0):
	self.AudioFile.pos=0
      else:
	self.AudioFile.pos-=10
    time.sleep(0.1)

    
class AudioTests(unittest.TestCase):

  def setUp(self):
    self.Output=audio.Output()
    if(len(sys.argv)<2):
      self.AudioFile=audio.AudioFile("/System/Library/Sounds/Submarine.aiff")
      #self.AudioFile=audio.AudioFile("/Users/samuelytterbrink/Documents/Python/Daisy Delight/Runing The Road.mp3")
    else:
      self.AudioFile=audio.AudioFile(sys.argv[1])
    self.Converter=audio.Converter(self.AudioFile,self.Output)

  def tearDown(self):
    self.Output.stop()
    #while self.Output.isPlaying:
    #  time.sleep(0.1)

  def testPlayback(self):
    if(prompt):
      self.Output.play(self.AudioFile,self.Converter)
      while 1:
	rInput=raw_input("did u here the sound [y/n]>")
	if(rInput=="y" or rInput=="n"):
	  self.assertEqual(rInput,"y")
	  break
    else:
      self.Output.play(self.AudioFile,self.Converter)
      time.sleep(2)
  
  def testPosAndLength(self):
    print
    
    print "posisition %f of %f seconds."% (self.AudioFile.pos,self.AudioFile.length)
    self.Output.play(self.AudioFile,self.Converter)
    #time.sleep(self.AudioFile.length)
    curses.wrapper(display,self)
        #print "posisition %d of %d seconds."% (self.AudioFile.pos,self.AudioFile.length)
    #self.AudioFile.pos=0
    #print "posisition %d of %d seconds."% (self.AudioFile.pos,self.AudioFile.length)
    #self.Output.play(self.AudioFile,self.Converter)
    #print "posisition %d of %d seconds."% (self.AudioFile.pos,self.AudioFile.length)

suite = unittest.TestLoader().loadTestsFromTestCase(AudioTests)
unittest.TextTestRunner(verbosity=2).run(suite)
