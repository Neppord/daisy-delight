#! /usr/bin/python
# coding: latin1
from parsers import NccParser, SmilReader
from observable import Observable
import md5


class DDBook:
    def __init__(self, fileref):
        self.path = fileref
        self.fillDict()

    def fillDict(self):
        import os.path
        path = None
        for ncc in (
        "ncc.html", "ncc.htm", "Ncc.html", "Ncc.htm", "NCC.htm", "NCC.html",
        "NCC.HTML", "NCC.HTM"):
            if os.path.exists(os.path.join(self.path, ncc)):
                path = os.path.join(self.path, ncc)
                break
        if path:
            f = file(path)
            self.md5 = md5.new(f.read()).hexdigest()
            f.close()
            f = file(path)
            nccparser = NccParser()
            nccparser.feed(f.read())
            f.close()
            self.toc, self.chron = nccparser.getTOC()
            self.name = nccparser.name
            return
        raise IOError, "Path " + self.path + " did not contain a ncc file."

    def getFirstId(self):
        return self.chron[0]["id"]

    def getLastId(self):
        return self.chron[-1]["id"]

    def next(self, id, level):
        # print self.toc[id]
        return self.toc[id]["next"][level]

    def prew(self, id, level):
        return self.toc[id]["prew"][level]

    def getAudio(self, id):
        # print id
        if not "audio" in self.toc[id]:
            smilparser = SmilReader()
            self.toc[id]["audio"] = smilparser.getAudioFromFile(self.path,
                                                                self.toc[id][
                                                                    "href"])
        return self.toc[id]["audio"]

    def getText(self, id):
        # print "getText", id
        return self.toc[id]["text"]


class DDState(Observable):
    def __init__(self):
        Observable.__init__(self)
        self.level = 0
        self.isPaused = True
        self.id = None
        self.currClip = None
        self.currClipIndex = None
        self.name = None
        self.md5 = None
        self.dDBook = None
        self.msg = "Welcome to Daisy Delight"

    """def eject(self):
        self.level = 0
        self.isPaused = True
        self.id = None
        self.currClip = None
        self.currClipIndex= None
        self.name = None
        self.md5 = None
        self.dDBook = None
        self.msg = "Book ejected no book loaded. """

    def setDDBook(self, dDBook):
        self.dDBook = dDBook
        self.md5 = dDBook.md5
        import toolKit
        if len(toolKit.getBookmark(self.md5)) > 0:
            self.id = toolKit.getBookmark(self.md5)
        else:
            self.id = dDBook.getFirstId()
        self.name = dDBook.name
        self.currClipIndex = 0
        self.currClip = dDBook.getAudio(self.id)[0]
        self.msg = self.name + " is now loaded and ready to be played"
        self.setChanged("clip", "book", "msg")
        self.notifyAll()

    def pause(self):
        self.isPaused = True
        self.setChanged()
        self.notifyAll()

    def play(self):
        if self.dDBook:
            self.isPaused = False
            self.msg = "Now playing: " + self.name
        else:
            self.msg = "Could not start playing,there is no DaisyBook loaded"
        self.setChanged("msg")
        self.notifyAll()

    def next(self):
        self.setId(self.dDBook.next(self.id, self.level))

    def prew(self):
        self.setId(self.dDBook.prew(self.id, self.level))

    def skip(self):
        if (len(self.dDBook.getAudio(self.id)) - 1) <= self.currClipIndex:
            self.next()
        else:
            self.currClipIndex += 1
            self.currClip = self.dDBook.getAudio(self.id)[self.currClipIndex]
            # self.msg = "skipping to: clip %d of %d. "%(self.currClipIndex+1,len(self.dDBook.getAudio(self.id)))
            self.setChanged("clip")
            self.notifyAll()

    def levelUp(self):
        if self.level > 0:
            self.level -= 1
            self.setChanged()
            self.notifyAll()

    def levelDown(self):
        if self.level < 6:
            self.level += 1
            self.setChanged()
            self.notifyAll()

    def setLevel(self, level):
        self.level = level
        self.setChanged()
        self.notifyAll()

    def getText(self, id=None):
        if not id: id = self.id
        if self.dDBook:
            return self.dDBook.getText(id)
        else:
            return None

    def getTOC(self, level):
        levels = ["h1", "h2", "h3", "h4", "h5", "h6", "pg"]
        lv = []
        id = []
        name = []
        # print "getToc(",levels[level],")"

        # print "chron",self.dDBook.chron
        for x in self.dDBook.chron:
            if level >= levels.index(x['level']):
                id.append(x['id'])
                name.append(x['text'])
                lv.append(levels.index(x['level']))
        return (name, id, lv)

    def setId(self, id):
        # print "old",self.id,"new",id
        self.id = id
        self.currClipIndex = 0
        self.currClip = self.dDBook.getAudio(self.id)[self.currClipIndex]
        self.msg = "Moved to: " + self.getText()
        self.setChanged("clip", "text", "msg")
        self.notifyAll()
