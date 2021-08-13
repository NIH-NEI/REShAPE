import os, time
from Tkinter import *
from ttk import Combobox

def center(win, fx=0.4, fy=0.45):
    win.update_idletasks()
    width = win.winfo_width()
    height = win.winfo_height()
    x = int((win.winfo_screenwidth() - width) * fx)
    y = int((win.winfo_screenheight() - height) * fy)
    win.geometry('{}x{}+{}+{}'.format(width, height, x, y))

class AttribValueDialog(Toplevel):
    def __init__(self, parent, title, all_list, excl_list=None):
        Toplevel.__init__(self, master=parent)
        self.all_list = all_list
        self.excl_list = excl_list or []
        #
        self.is_ok = False
        self.title(title)
        #
        nv = len(self.all_list)
        nrow = (nv + 3) / 4
        if nrow > 25:
            ncol = (nv + 24) / 25
            nrow = (nv + ncol - 1) / ncol
        elif nrow <= 1:
            nrow = 1
            ncol = 1
        else:
            ncol = (nv + nrow - 1) / nrow
        #
        self.frame = Frame(self, padx=2)
        self.frame.grid(column=0, row=0, columnspan=2, padx=15, pady=5)
        #
        excl_set = set(self.excl_list)
        self.v_map = {}
        self.cb_map = {}
        for i, nm in enumerate(self.all_list):
            col = i / nrow
            row = i % nrow
            var = BooleanVar()
            cb = Checkbutton(self.frame, text=nm, variable=var)
            cb.grid(column=col, row=row, padx=5, sticky=W)
            var.set(nm not in excl_set)
            self.cb_map[nm] = cb
            self.v_map[nm] = var
        #
        self.lbframe = Frame(self)
        self.lbframe.grid(column=0, row=1, sticky=W, pady=10, padx=20)
        self.checkBtn = Button(self.lbframe, text='Check All', padx=5, command=self.CheckBtnClicked)
        self.checkBtn.grid(column=0, row=0)
        self.unCheckBtn = Button(self.lbframe, text='Uncheck All', padx=5, command=self.UnCheckBtnClicked)
        self.unCheckBtn.grid(column=1, row=0, padx=5)

        self.rbframe = Frame(self)
        self.rbframe.grid(column=1, row=1, sticky=E, pady=10, padx=20)
        self.okBtn = Button(self.rbframe, text='OK', padx=20, command=self.OkBtnClicked)
        self.okBtn.grid(column=0, row=0)
        self.cancelBtn = Button(self.rbframe, text='Cancel', padx=5, command=self.CancelBtnClicked)
        self.cancelBtn.grid(column=1, row=0, padx=10)
        #
        center(self)
        self.focus_force()
        self.grab_set()
        self.wait_window()
    #
    def CheckBtnClicked(self):
        for var in self.v_map.itervalues():
            var.set(True)
    def UnCheckBtnClicked(self):
        for var in self.v_map.itervalues():
            var.set(False)
    #
    def OkBtnClicked(self):
        self.is_ok = True
        self.excl_list = sorted([nm for nm, var in self.v_map.iteritems() if not var.get()])
        self.destroy()
    def CancelBtnClicked(self):
        self.destroy()
    #

class FileSelectorDialog(Toplevel):
    FILENAME_ATTR = '-- file name --'
    ANY_VALUE = '-- Any value --'
    def __init__(self, parent, title, attr_list, sel_values=None):
        Toplevel.__init__(self, master=parent)
        self.attr_list = attr_list
        self.filter = sel_values or {}
        #
        self.is_ok = False
        self.title(title)
        #
        self.frame = Frame(self, padx=2)
        self.frame.grid(column=0, row=0, columnspan=2, padx=15, pady=5)
        #
        fnLab = Label(self.frame, text='File Name (contains):')
        fnLab.grid(column=0, row=0, padx=5, sticky=E)
        self.fnVar = StringVar()
        self.fnText = Entry(self.frame, width=48, textvariable=self.fnVar)
        self.fnText.grid(column=1, row=0, pady=5, sticky=W)
        self.fnVar.set(self.filter.get(self.FILENAME_ATTR, ''))
        #
        self.v_map = {}
        self.e_map = {}
        row = 1
        for a, vlist in self.attr_list:
            lab = Label(self.frame, text=a+' (equals):')
            lab.grid(column=0, row=row, padx=5, sticky=E)
            var = StringVar()
            txt = Combobox(self.frame, state='readonly', width=42, textvariable=var)
            txt.grid(column=1, row=row, pady=4, sticky=W)
            self.v_map[a] = var
            self.e_map[a] = txt
            txt['values'] = tuple([self.ANY_VALUE]+vlist)
            var.set(self.filter.get(a, self.ANY_VALUE))
            row += 1
        #
        self.bframe = Frame(self, padx=2)
        self.bframe.grid(column=0, row=1, columnspan=2, padx=15, pady=5, sticky=E)
        #
        self.okBtn = Button(self.bframe, text='OK', padx=20, command=self.OkBtnClicked)
        self.okBtn.grid(column=0, row=0)
        self.cancelBtn = Button(self.bframe, text='Cancel', padx=5, command=self.CancelBtnClicked)
        self.cancelBtn.grid(column=1, row=0, padx=10)
        #
        center(self)
        self.focus_force()
        self.grab_set()
        self.wait_window()
    #
    def OkBtnClicked(self):
        filt = {}
        fn = self.fnVar.get()
        if len(fn) > 0:
            filt[self.FILENAME_ATTR] = fn
        for a, var in self.v_map.iteritems():
            v = var.get()
            if not v or v == self.ANY_VALUE: continue
            filt[a] = v
        self.filter = filt
        self.is_ok = True
        self.destroy()
    #
    def CancelBtnClicked(self):
        self.destroy()
    #


