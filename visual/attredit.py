import sys, os, json, csv, math, subprocess, time, datetime
import threading
import Tkinter as tk
from Tkinter import *
import tkMessageBox
from ScrolledText import ScrolledText
import tkFileDialog
from ttk import Combobox

from . import APP_NAME

class AttributeEntry(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, master=parent)
        self.pack(fill=X, expand=True)
        #
        self.options = options
        #
        self.nameVar = StringVar()
        if 'name' in options:
            nm = options['name']
            self.nameVar.set(nm)
            self.nameElem = Label(self, text=nm)
        else:   
            self.nameElem = Combobox(self, textvariable=self.nameVar)
        
        self.eqLab = Label(self, text='=')
        
        self.valueVar = StringVar()
        self.valueElem = Combobox(self, textvariable=self.valueVar)
        if 'value' in options:
            self.valueVar.set(options['value'])
            self._validate_value()
        #
        self.showChildren()
        self.nameVar.trace("w", self.nameVarChanged)
        self.valueVar.trace("w", self.valueVarChanged)
        #
        if 'list' in options:
            self.valueElem['values'] = tuple(options['list'])
    def hideChildren(self):
        for widget in self.winfo_children():
            widget.grid_forget()
    def showChildren(self):
        self.nameElem.grid(column=0, row=0, padx=5, pady=2)
        self.eqLab.grid(column=1, row=0, padx=5)
        self.valueElem.grid(column=2, row=0, padx=5, pady=2)
    def nameVarChanged(self, a, b, c):
        if 'addbutton' in self.options:
            btn = self.options['addbutton']
            if self.name:
                btn['state'] = 'normal'
            else:
                btn['state'] = 'disabled'
        self._validate_name()
    def setValueMap(self, valuemap):
        if not valuemap:
            valuemap = {}
        self.options['valuemap'] = valuemap
        self._validate_name()
    def _validate_name(self):
        if 'valuemap' in self.options:
            valueList = self.options['valuemap'].get(self.name)
            if not valueList:
                valueList = []
            else:
                valueList = sorted(valueList)
            self.setValueList(valueList)
    def _validate_value(self):
        v = self.value
        if v == '-- no change --':
            self.valueElem['foreground'] = 'gray'
        else:
            self.valueElem['foreground'] = 'black'
    def valueVarChanged(self, a, b, c):
        self._validate_value()
        self.fireValueChanged()
    def fireValueChanged(self):
        fn = self.options.get('valueChanged')
        if not fn: return
        fn(self.name, self.value)

    @property
    def name(self):
        return self.nameVar.get()

    @property
    def value(self):
        return self.valueVar.get()
        
    def setName(self, vl):
        self.nameVar.set(vl)
    def setValue(self, vl):
        self.valueVar.set(vl)
    def commit(self):
        if isinstance(self.nameElem, Combobox):
            self.nameElem.destroy()
            self.nameElem = Label(self, text=self.name)
            self.nameElem.grid(column=0, row=0, padx=5, pady=2, sticky=EW)
            self.fireValueChanged()
    def setNameList(self, namelist):
        if not isinstance(self.nameElem, Combobox):
            return
        self.nameElem['values'] = tuple(namelist)
    def setValueList(self, valueList):
        self.valueElem['values'] = tuple(valueList)
    def getNameList(self, excl=True):
        res = set()
        if isinstance(self.nameElem, Combobox):
            res.update(self.nameElem['values'])
        if self.name:
            if excl:
                res.discard(self.name)
        return sorted(res)
    def addName(self, nm):
        if isinstance(self.nameElem, Combobox):
            names = set()
            names.update(self.nameElem['values'])
            names.add(nm)
            self.nameElem['values'] = tuple(sorted(names))

class AttributeEditor(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, master=parent)
        self.pack(fill=BOTH, expand=True)
        #
        self.options = options
        #
        self.committed = []
        self.addBtn = Button(self, text=' + ', command=self.addAttrClicked)
        self.addBtn['state'] = 'disabled'
        self.uncommitted = AttributeEntry(self, addbutton=self.addBtn)
        self.updateChildren()
        #
    @property
    def names(self):
        res = [elem.name for elem in self.committed]
        res.sort()
        return res
    def setData(self, attributes, available):
        self.committed = []
        for data in attributes:
            data['valueChanged'] = self.valueChanged
            self.committed.append(AttributeEntry(self, **data))
        nameList = sorted(available.keys())
        self.uncommitted.setNameList(nameList)
        self.uncommitted.setValueMap(available)
        if len(nameList) > 0:
            self.uncommitted.setName(nameList[0])
        self.updateChildren()
    def setNewValue(self, txt):
        self.uncommitted.setValue(txt)
    def hideChildren(self):
        for widget in self.winfo_children():
            widget.grid_forget()
    def updateChildren(self):
        for widget in self.winfo_children():
            widget.grid_forget()
        for row, elem in enumerate(self.committed):
            elem.grid(column=0, row=row, sticky=EW)
            def callback(id=row):
                self.removeAttr(id)
            btn = Button(self, text=' - ', command=callback)
            btn.grid(column=1, row=row)
        row = len(self.committed)+1
        self.uncommitted.grid(column=0, row=row)
        if row > self.options.get('maxrows', 5):
            self.uncommitted.hideChildren()
            return
        self.uncommitted.showChildren()
        self.addBtn.grid(column=1, row=row)
    def removeAttr(self, id):
        elem = self.committed[id]
        result = tkMessageBox.askyesno(APP_NAME, 'Remove attribute "%s"\nAre you sure?' % (elem.name,))
        if not result:
            return
        self.uncommitted.addName(elem.name)
        del self.committed[id]
        self.updateChildren()
        fn = self.options.get('attributeRemoved')
        if not fn: return
        fn(elem.name)
    def addAttrClicked(self):
        elem = self.uncommitted
        if elem.name in self.names:
            tkMessageBox.showerror(APP_NAME, 'Attribute "%s" already exists' % (elem.name,))
            return
        if 'addbutton' in elem.options:
            del elem.options['addbutton']
        nameList = elem.getNameList()
        elem.options['valueChanged'] = self.valueChanged
        elem.commit()
        self.committed.append(elem)
        self.addBtn['state'] = 'disabled'
        self.uncommitted = AttributeEntry(self, addbutton=self.addBtn)
        self.uncommitted.setNameList(nameList)
        self.uncommitted.focus_set()
        self.updateChildren()
    def valueChanged(self, name, value):
        fn = self.options.get('attributeChanged')
        if not fn: return
        fn(name, value)
