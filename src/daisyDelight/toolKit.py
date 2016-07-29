#! /usr/bin/python
#coding: latin1
from glob import glob
from os import getenv


def findAllPaths():
	toReturn = []

	for ncc in ("ncc.html","ncc.htm","Ncc.html","Ncc.htm","NCC.htm","NCC.html","NCC.HTML","NCC.HTM"):
		for x in glob("/Volumes/*/"+ncc):
			toReturn.append(x[:-8])
		for x in glob("/media/*/"+ncc):
			toReturn.append(x[:-8])
		for x in glob("/mnt/*/"+ncc):
			toReturn.append(x[:-8])
	return toReturn

def setBookmark(bookref, bookname, id):
	newline = "<book> <title>" + bookname + "</title> <md5>" + bookref + "</md5> <id>" + id + "</id> </book> \n"
	path = getenv("HOME") + "/.daisyDelight"
	try:	
		conffile = open( path, 'r+')
	except IOError:
		conffile = open( path, 'w')
		conffile.close()
		conffile = open(path, 'r+')

	conf = conffile.readlines()
	conffile.close()
	for line in conf:
		if bookref in line:
			conf[conf.index(line)] = newline
			break
	else:
		conf.append(newline)
	conffile = open(path, 'w')
	conffile.writelines(conf)
	conffile.close()

def getBookmark(md5):
	id = ''
	path = getenv("HOME") + "/.daisyDelight"
	import os.path
	if os.path.exists(path):
		conffile = open(path, 'r')
		for line in conffile.readlines():
			if md5 in line:
				id = line[(line.rfind('<id>')+4):line.find('</id>')]
	return id

if __name__ == "__main__":	
	pass	

