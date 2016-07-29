#! /usr/bin/python
# coding: latin1

import cmd, sys, toolKit
from state import DDBook


# import readline
class DDTextUserInterface(cmd.Cmd):
    def __init__(self, state):
        cmd.Cmd.__init__(self)
        self.state = state
        self.state.addObserver(self)
        self.prompt = 'DaisyDelight>\n'

    def update(self, observable):
        if "text" in observable.hasChanged:
            print "Now playing: %s" % (self.state.getText())
        if "msg" in observable.hasChanged:
            print "Message for you:", self.state.msg

    """def do_eject(self,arg):
        self.state.eject()"""

    def help_quit(self):
        print """Exits the program"""

    def do_quit(self, arg):
        print "Bye...!"
        self.state.pause()
        if self.state.md5 and self.state.name and self.state.id:
            toolKit.setBookmark(self.state.md5, self.state.name, self.state.id)
        sys.exit(0)

    def help_load(self):
        print """
		Loads a Daisy book. The first argument can be the path where the Daisy book is to be found. If there is no argument the program will search for folders with a ncc.html in it. More than  one alternative accessable causes promptedoptions."""

    def do_load(self, arg):
        path = toolKit.findAllPaths()
        if arg:
            if arg.isdigit():
                dBook = DDBook(path[int(arg) - 1])
                self.state.setDDBook(dBook)
            else:
                dBook = DDBook(arg)
                self.state.setDDBook(dBook)
            # if self.state.md5 and self.state.name and self.state.id:
            #	toolKit.setBookmark(self.state.md5, self.state.name,self.state.id)

        else:
            print "Paths found on your computer:"
            for i, path in zip(range(1, len(path) + 1), path):
                print "\t %d:%s" % (i, path)

    def help_next(self):
        print """Jumps to the next item at the present level and starts playing it."""

    def do_next(self, arg):
        self.state.next()

    def help_prew(self):
        print """Jumps to the prewious item at the present level and starts playing it."""

    def do_prew(self, arg):
        self.state.prew()

    def help_up(self):
        print """Increases the navigation level with one. If allreaady at the highest level nothing happends."""

    def do_up(self, arg):
        self.state.levelUp()

    def help_down(self):
        print """Decreases the navigation level with one. If allreaady at the lowest level nothing happends."""

    def do_down(self, arg):
        self.state.levelDown()

    def help_play(self):
        print """Starts playeing att curent position. If a argument iss passed to it it will start att that point"""

    def do_play(self, arg):
        self.state.play()

    def help_skip(self):
        print """Skip this phrase and continue with the next one"""

    def do_skip(self, arg):
        self.state.skip()

    def help_pause(self):
        print """Pauses the player. Type play to start again. """

    def do_pause(self, arg):
        self.state.pause()

    def do_EOF(self, arg):
        self.do_quit(arg)

    # Shortcuts and aliases
    do_EOF = do_quit
    do_q, help_q = do_quit, help_quit
    do_n, help_n = do_next, help_next
    do_p, help_p = do_prew, help_prew
    do_u, help_u = do_up, help_up
    do_d, help_d = do_down, help_down
    do_s, help_s = do_skip, help_skip


import Tkinter


class TkGUI(Tkinter.Frame):
    def __init__(self, state, master=None):
        Tkinter.Frame.__init__(self, master)
        self.state = state
        self.toc_var = Tkinter.StringVar(self)
        self.radio_level = Tkinter.IntVar(self)
        state.addObserver(self)
        import tkFileDialog
        self.filedialog = tkFileDialog.Open(
            filetypes=(("Ncc File", ".html", ".htm"),))
        self.grid(sticky="nsew")
        self.winfo_toplevel().columnconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.navigateButtons = []
        self.createwidgets()

    def updateTOC(self, (name, id, level)):
        # print "updateToc"
        # print "adding:",name
        self.tocmenu.delete(0, Tkinter.END)
        # print "cleard content"
        # print "adding",len(name),"new items."
        lvl = ["H1", "H2", "H3", "H4", "H5", "H6", "pages"]
        menus = [self.tocmenu]
        levels = [0]
        for n, i, l in zip(name, id, level):
            if (l > levels[-1]):  # make sub menue
                menus.append(Tkinter.Menu(menus[-1]))

                menus[-2].add_cascade(label=lvl[l], menu=menus[-1],
                                      command=lambda x=i: self.state.setId(x))
                levels.append(l)
            elif (l < levels[-1]):  # go to a super menue
                while l < levels[-1]:
                    menus.pop()
                    levels.pop()
                menus[-1].add_separator()
            else:  # continue whith the old menu
                pass

            menus[-1].add_command(label=n,
                                  command=lambda x=i: self.state.setId(x))
        # print n,l,levels

    def update(self, observable):
        # if self.level == self.radio_level and self.level!= self.state.level:
        #	self.level = self.state.level
        #	self.radio_level = self.state.level
        # print self.state.level
        self.level_radio[self.state.level].select()
        if self.state.dDBook:
            self.book_label["text"] = self.state.name
            self.chapter_label["text"] = self.state.getText()
            for x in self.navigateButtons:
                x.config(state='normal')
        else:
            for x in self.navigateButtons:
                x.config(state='disabled')
        if "msg" in observable.hasChanged:
            self.msg_label["text"] = self.state.msg

    def load_command(self):
        fileref = self.filedialog.show()
        if fileref:
            self.state.setDDBook(DDBook(fileref[:-8]))
            self.play_button.config(state='normal')
            self.updateTOC(self.state.getTOC(6))

    def createwidgets(self):
        self.book_label = Tkinter.Label(self, text="...", width=40,
                                        font=("Monaco", 14), wraplength=300)
        self.book_label.grid(column=3, row=0, columnspan=7, sticky="we")
        self.chapter_label = Tkinter.Label(self, text="...", width=40,
                                           wraplength=300, font=("Monaco", 14))
        self.chapter_label.grid(column=2, row=2, columnspan=7, rowspan=3,
                                sticky="we")
        self.msg_label = Tkinter.Label(self, text="Welcome to Daisy Delight...",
                                       font=("Monaco", 14), wraplength=300,
                                       width=40)
        self.msg_label.grid(column=2, row=6, columnspan=7, sticky="we")
        self.load_button = Tkinter.Button(self, text="Load",
                                          command=self.load_command)
        self.load_button.grid(column=0, row=0, columnspan=1, sticky="we")
        self.stop_button = Tkinter.Button(self, text="Stop",
                                          command=self.stop_command,
                                          state='disabled')
        self.stop_button.grid(column=1, row=3, columnspan=1, sticky="we")
        self.prew_button = Tkinter.Button(self, text="Prew",
                                          command=self.state.prew,
                                          state='disabled')
        self.prew_button.grid(column=0, row=2, columnspan=1)
        self.navigateButtons.append(self.prew_button)
        self.next_button = Tkinter.Button(self, text="Next",
                                          command=self.state.next,
                                          state='disabled')
        self.next_button.grid(column=1, row=2, columnspan=1)
        self.navigateButtons.append(self.next_button)
        self.down_button = Tkinter.Button(self, text="down",
                                          command=self.state.levelDown,
                                          state='disabled')
        self.down_button.grid(column=0, row=1, columnspan=1)
        self.navigateButtons.append(self.down_button)
        self.up_button = Tkinter.Button(self, text="up",
                                        command=self.state.levelUp,
                                        state='disabled')
        self.up_button.grid(column=1, row=1, columnspan=1, sticky="ew")
        self.navigateButtons.append(self.up_button)
        # self.skip_prew_button=Tkinter.Button(self,text="P skip")
        # self.skip_prew_button.grid(column=3,row=1,columnspan=1)
        self.skip_next_button = Tkinter.Button(self, text="skip",
                                               state='disabled',
                                               command=self.state.skip)
        self.skip_next_button.grid(column=0, row=4, columnspan=2, sticky="we")
        self.navigateButtons.append(self.skip_next_button)
        self.play_button = Tkinter.Button(self, text="Play",
                                          command=self.play_command,
                                          state='disabled')
        self.play_button.grid(column=0, row=3, columnspan=1, sticky="we")
        self.quit_button = Tkinter.Button(self, text="Quit",
                                          command=self.quit_command)
        self.quit_button.grid(column=1, row=0, columnspan=1, sticky="we")
        self.toc = Tkinter.Menubutton(
            self)  # OptionMenu(self,self.toc_var,"TOC")
        self.toc["text"] = "TOC"
        self.toc.grid(column=0, row=6, columnspan=2, sticky="e")
        self.tocmenu = Tkinter.Menu(self.toc)
        self.toc['menu'] = self.tocmenu

        modes = [('h1', 0), ('h2', 1), ('h3', 2), ('h4', 3), ('h5', 4),
                 ('h6', 5), ('pg', 6)]
        self.level_radio = []
        for text, value in modes:
            self.level_radio.append(
                Tkinter.Radiobutton(self, text=text, variable=self.radio_level,
                                    value=value, command=self.set_level))
            self.level_radio[-1].grid(column=(value + 2), row=1, columnspan=1,
                                      sticky="we")

    def set_level(self):
        self.state.setLevel(self.radio_level.get())

    # print "set",self.radio_level.get()
    # self.level_radio[self.radio_level].select()

    def quit_command(self):
        self.state.pause()
        if self.state.md5 and self.state.name and self.state.id:
            toolKit.setBookmark(self.state.md5, self.state.name, self.state.id)
        import sys
        sys.exit()

    def play_command(self):
        self.play_button.config(state='disabled')
        self.stop_button.config(state='normal')
        self.state.play()

    def stop_command(self):
        self.stop_button.config(state='disabled')
        self.play_button.config(state='normal')
        self.state.pause()


if __name__ == "__main__":
    # p=DaisyPrompt(None)
    # p.cmdloop()
    g = TkGUI()
    g.mainloop()
