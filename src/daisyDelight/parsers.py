#! /usr/bin/python
#coding: latin1
from HTMLParser import HTMLParser
class XmlReader:
	def __init__(self):
		import xml.parsers.expat
		self.expat=xml.parsers.expat.ParserCreate()
		self.expat.StartElementHandler = self.startElemetHandler
		self.expat.EndElementHandler = self.endElementHandler 
	
	def startElemetHandler(self, name, attrs):
		if "start_"+name in self.__class__.__dict__ and callable(self.__class__.__dict__["start_"+name]):
			self.__class__.__dict__["start_"+name](self,attrs)
		else:
			self.start_default(name,attrs)
	
	def endElementHandler(self, name):
		if "end_"+name in self.__class__.__dict__ and callable(self.__class__.__dict__["end_"+name]):
			self.__class__.__dict__["end_"+name](self)
		else:
			self.end_default(name)

	def end_default(self, name):		
		pass
	
	def start_default(self,name, attrs):
		pass
class SmilReader(XmlReader):
	
	def __init__(self):
		XmlReader.__init__(self)
		self.id_found = None
		self.clips = []
		self.finished = False

	def start_text(self, attrs):
		#print "text hittad"
		if "id" in attrs and attrs['id'] == self.bookmark:
			self.id_found = "text"
			#print "id:", attrs['id']
		elif self.id_found == "text":
			self.id_found = None
			self.finished = True	

	def start_par(self, attrs):
		if "id" in attrs and attrs['id'] == self.bookmark:
			self.id_found = "par"

	def start_audio(self, attrs):
		#print "audio found"
		if self.id_found :
			begin=float(attrs["clip-begin"].split("=")[1].split("s")[0])
			end=float(attrs["clip-end"].split("=")[1].split("s")[0])
			self.clips.append((begin, end, self.path+attrs["src"]))	


	def end_par(self):		
		if  self.id_found == "par":
			self.id_found = None
			self.finished = True	

	def getAudioFromFile(self, path, href):
		import xml.parsers.expat
		href, self.bookmark = href.split('#')
		self.path=path
		f = open(path+href,"r")
		for line in f:
			try:
				self.expat.Parse(line)
				if self.finished:
					#print "break"
					break
			except xml.parsers.expat.ExpatError:
				#print "encoding:",f.encoding
				print "error" + line	

	
		self.finished = False
		toReturn =  self.clips
		f.close()
		return toReturn
		
levels = ["h1", "h2", "h3", "h4", "h5", "h6", "pg"]

class NccParser(HTMLParser):
	def __init__(self):
		HTMLParser.__init__(self)
		self.toc = {}
		self.chron=[{"prew":[None]*len(levels)}]
		self.name = None
	def handle_starttag(self, tag, attr):
		if tag.lower() == 'span':
			tag = 'pg'
		if tag.lower() in levels:
			for name, value in attr:
				if name == "id":
					self.chron[-1]["id"]=value	
					self.chron[-1]["level"]=tag.lower()
		elif (tag.lower() == 'meta'):
			#print attr
			if attr[0][1] == 'dc:title':
				self.name = attr[1][1]
					
		elif (tag == 'a'):
			for name, value in attr:
				if name == "href":
					self.chron[-1]["href"] = value
		# print "<"+tag+">"

	def handle_data(self, text):
		if self.lasttag and self.lasttag.lower()=='a':
			#print "text: \n\t"+text
			self.chron[-1]['text'] = self.chron[-1].get('text',"")+text
			
	def handle_endtag(self,tag):
		self.lasttag="/"+tag
		#print self.lasttag
		if tag == 'span': 
			tag = 'pg'
		if tag.lower() in levels:
			try:
				nextPrew = getDirectionDict(self.chron[-1]['level'], self.chron[-1]['prew'], self.chron[-1]["id"])
			except KeyError:
				print "skriver ut" , self.lineno
				raise
			#print "len:",len(nextPrew)#"list:",nextPrew
			self.chron.append({"prew": nextPrew})
			#		print "</"+tag+">"

	def getTOC(self):
		""" goes through the list and fixes prew and return the first item"""
		#self.chron=self.chron[:-1]
		try:
			self.chron[-2]['next'] = [None] * len(levels)
		except IndexError:
			#print self.chron
			raise
		for now,prew in zip(self.chron[-2:0:-1],self.chron[-3::-1]):
			prew['next'] = getDirectionDict(now['level'],now['next'], now['id'])		  
			#print now['id'], "ger next till", prew['id']
		for node in self.chron[:-1]:
			id = node["id"]
			self.toc[id]=node
		return self.toc, self.chron[:-1]

def getDirectionDict( level, list, default):
	head = list[:max(levels.index(level), 0)]
	tail = [default]*(len(levels)-levels.index(level))
	return head + tail

if __name__=="__main__":
	import glob
	file = glob.glob('/Volumes/*/ncc.html').pop()
	toRead = open(file)
	nccparser = NccParser()
	print "feeding..."
	lines=toRead.readlines()
	for line in lines:
		print "feeding line:",lines.index(line),"of",len(lines)
		nccparser.feed(line)
	print "feeded!"
	toRead.close();
	root = nccparser.getTOC()
	print " "
	print root
