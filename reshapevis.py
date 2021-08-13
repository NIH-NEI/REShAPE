'''
Created on Jan 3, 2019

@author: Andrei I Volkov
'''

import sys, os, json, csv, math, subprocess, time, datetime
from collections import defaultdict
import shutil
import threading
import Tkinter as tk
from Tkinter import *
import tkMessageBox
import tkFileDialog
from PIL import ImageTk, Image

import visual
from visual.listentry import CellFeatureManager, ListFileEntry, FeatureEntry, listfiles, CELL_FEATURE_LIST
from visual.filegroup import FileGroupManager, FileGroupReader, dblquot, sanitizeR
from visual.listselect import ScrolledList, ListSelector, FileListPanel
from visual.templatemgr import TemplateManager
from visual.plotmgr import PlotManager
from visual.dialogs import AttribValueDialog

import reshape_version

APP_NAME = visual.APP_NAME = reshape_version.VISUAL_APP_NAME
APP_VERSION = visual.APP_VERSION = '%s (%s)' % (reshape_version.VISUAL_APP_VERSION, reshape_version.VISUAL_APP_RELEASE)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

class PlotProgressDialog(Toplevel):
    def __init__(self, parent, **options):
        Toplevel.__init__(self, master=parent)
        self.options = options
        #
        self.screenwidth = self.options.get('screenwidth', 1536)
        self.screenheight = self.options.get('screenheight', 864)
        self.is_ok = False
        self.title("Generating plots")
        self.frame = Frame(self, padx=2)
        self.frame.pack(fill=BOTH, expand=True, padx=40, pady=20)
        #
        self.msgLab = Label(self.frame, text="Generating plots", padx=5)
        self.msgLab.grid(column=0, row=0, padx=5, sticky=W)
        #
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.focus_force()
        self.grab_set()
    #
    def on_closing(self):
        pass
    #
    def message(self, msg):
        self.msgLab.configure(text=msg)
        w = self.screenwidth / 4
        h = self.screenheight / 10
        self.geometry('%dx%d+%d+%d' % (w, h, (self.screenwidth-w)/2, (self.screenheight-h)/2))
    #

class VisualizationPanel(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, relief=RAISED, borderwidth=1)
        self.columnconfigure(2, weight=1)
        self.rowconfigure(2, weight=1)
        # self.pack(fill=BOTH, expand=True)
        #
        self.options = options
        self.rscript = 'Rscript'
        self.homedir = options.get("homedir", ".")
        self.ftmgr = CellFeatureManager()
        self.gmgr = FileGroupManager(self.homedir)
        self.tmgr = TemplateManager(BASE_DIR)
        self.pmgr = PlotManager(self, APP_NAME, APP_VERSION, self.ftmgr, self.gmgr, self.tmgr, self.rscript)
        self.cache = {}
        self.ghist = []
        self.ghist_map = {}
        self.ghist_plain = []
        self.ymin = {}
        self.ymax = {}
        self.attr_values = defaultdict(set)
        self.excl_values = {}
        #
        self.statuslabel = self.options.get('statuslabel')
        self.selected_feature = None
        
        # Output directory
        tgtFrame = LabelFrame(self, text="Output Directory", padx=5, pady=5)
        tgtFrame.grid(column=0, row=0, columnspan=2, sticky=N)
        self.tgtDirVar = StringVar()
        self.tgtDirTxt = tgtDirTxt = Entry(tgtFrame, width=72, textvariable=self.tgtDirVar)
        tgtDirTxt.grid(column=1, row=0)
        tgtDirBtn = Button(tgtFrame, text="...", padx=5, command=self.tgtDirBtnClicked)
        tgtDirBtn.grid(column=2, row=0, padx=5, pady=5)
        
        # Group selector
        lfgroup = LabelFrame(self, text="Data File Group", padx=5, pady=5)
        lfgroup.grid(column=0, row=1, sticky=N)
        self.groupVar = StringVar()
        self.groupMenu = OptionMenu(lfgroup, self.groupVar,
                    "existing")
        self.groupMenu.config(width=32)
        self.groupMenu.grid(column=0, row=0, sticky=W)
        self.csvBtn = Button(self, text='Export CSV and R', padx=10, command=self.exportCsvClicked)
        self.csvBtn.grid(column=1, row=1, padx=32, pady=10, sticky=E)

        # Parameter selector
        lfparam = LabelFrame(self, text="Cell Feature", padx=5, pady=5)
        lfparam.grid(column=0, row=2, sticky=N)
        self.paramList = ScrolledList(lfparam,
            width=36,
            height=19,
            selectmode=EXTENDED,
            exportselection=False)
        self.paramList.grid(column=0, row=0, sticky=W)
        self.paramList.updateElements(CELL_FEATURE_LIST)
        #
        cfbtnfr = Frame(lfparam)
        cfbtnfr.grid(column=0, row=1, sticky=W+S)
        self.cfAddBtn = Button(cfbtnfr, text='Select All', padx=5, command=self.cfAddBtnClicked)
        self.cfAddBtn.grid(column=0, row=0, padx=5, pady=2, sticky=W)
        self.cfRemBtn = Button(cfbtnfr, text='Deselect All', padx=5, command=self.cfRemBtnClicked)
        self.cfRemBtn.grid(column=1, row=0, padx=5, pady=2, sticky=W)
        
        # Options
        lfopt = LabelFrame(self, text="Plot Options", padx=5, pady=5)
        lfopt.grid(column=1, row=2, sticky=N)
        
        labStyle = Label(lfopt, text='Plot Style:')
        labStyle.grid(column=0, row=0, padx=10, sticky=E)
        self.styleVar = StringVar()
        self.styleMenu = OptionMenu(lfopt, self.styleVar, 'boxplot', 'violin', 'density')
        self.styleMenu.config(width=20)
        self.styleMenu.grid(column=1, row=0, sticky=W)
        self.styleVar.set('boxplot')
        
        colfr = Frame(lfopt)
        colfr.grid(column=0, row=1, columnspan=2)
        
        lab1 = Label(colfr, text='Column 1:')
        lab1.grid(column=0, row=0, padx=10, sticky=E)
        self.pcol1Var = StringVar()
        self.pcol1Menu = OptionMenu(colfr, self.pcol1Var, 'None')
        self.pcol1Menu.config(width=18)
        self.pcol1Menu.grid(column=1, row=0, sticky=W)
        self.pcol1Btn = Button(colfr, text="...", padx=2, command=self.pcol1BtnClicked)
        self.pcol1Btn.grid(column=2, row=0, sticky=W)
        lab2 = Label(colfr, text='Column 2:')
        lab2.grid(column=0, row=1, padx=10, sticky=E)
        self.pcol2Var = StringVar()
        self.pcol2Menu = OptionMenu(colfr, self.pcol2Var, '-- None --')
        self.pcol2Menu.config(width=18)
        self.pcol2Menu.grid(column=1, row=1, sticky=W)
        self.pcol2Btn = Button(colfr, text="...", padx=2, command=self.pcol2BtnClicked)
        self.pcol2Btn.grid(column=2, row=1, sticky=W)
        
        self.log2Var = BooleanVar()
        self.log2Box = Checkbutton(lfopt, text='Log2', variable=self.log2Var)
        self.log2Var.set(False)
        self.log2Box.grid(column=0, row=2, padx=8, sticky=W)
        self.blackVar = BooleanVar()
        self.blackBox = Checkbutton(lfopt, text='Black Outline', variable=self.blackVar)
        self.blackVar.set(False)
        self.blackBox.grid(column=1, row=2, sticky=W)
        
        labX = Label(lfopt, text='X Axis Label:')
        labX.grid(column=0, row=3, padx=10, sticky=E)
        self.labXVar = StringVar()
        self.labXText = Entry(lfopt, width=26, textvariable=self.labXVar)
        self.labXText.grid(column=1, row=3, pady=2, sticky=W)
        labY = Label(lfopt, text='Y Axis Label:')
        labY.grid(column=0, row=4, padx=10, sticky=E)
        self.labYVar = StringVar()
        self.labYText = Entry(lfopt, width=26, textvariable=self.labYVar)
        self.labYText.grid(column=1, row=4, pady=2, sticky=W)
        
        lfopt2 = Frame(lfopt)
        lfopt2.grid(column=0, row=5, columnspan=2, pady=2)
        
        self.yMinVar = DoubleVar()
        self.yMinVar.set('')
        self.yMaxVar = DoubleVar()
        self.yMaxVar.set('')
        labYmin = Label(lfopt2, text='Y Limits:')
        labYmin.grid(column=0, row=0, padx=10, sticky=E)
        self.yMinText = Entry(lfopt2, width=12, textvariable=self.yMinVar)
        self.yMinText.grid(column=1, row=0, sticky=W)
        labYmax = Label(lfopt2, text='to')
        labYmax.grid(column=2, row=0, padx=5, sticky=E)
        self.yMaxText = Entry(lfopt2, width=12, textvariable=self.yMaxVar)
        self.yMaxText.grid(column=3, row=0, sticky=W)
        
        labW = Label(lfopt2, text='Size (in):')
        labW.grid(column=0, row=1, padx=10, sticky=E)
        self.labWVar = DoubleVar()
        self.labWText = Entry(lfopt2, width=12, textvariable=self.labWVar)
        self.labWText.grid(column=1, row=1, sticky=W)
        labH = Label(lfopt2, text='x')
        labH.grid(column=2, row=1, padx=5, sticky=W+E)
        self.labHVar = DoubleVar()
        self.labHText = Entry(lfopt2, width=12, textvariable=self.labHVar)
        self.labHText.grid(column=3, row=1, sticky=W)
        
        labSz = Label(lfopt, text='Dot Size:')
        labSz.grid(column=0, row=6, padx=10, sticky=E)
        self.dotSzVar = DoubleVar()
        self.dotSzText = Entry(lfopt, width=26, textvariable=self.dotSzVar)
        self.dotSzText.grid(column=1, row=6, pady=2, sticky=W)
        labDot = Label(lfopt, text='Dot Alpha:')
        labDot.grid(column=0, row=7, padx=10, sticky=E)
        self.dotAlphaVar = DoubleVar()
        self.dotAlphaText = Entry(lfopt, width=26, textvariable=self.dotAlphaVar)
        self.dotAlphaText.grid(column=1, row=7, pady=2, sticky=W)
        labBox = Label(lfopt, text='Box Alpha:')
        labBox.grid(column=0, row=8, padx=10, sticky=E)
        self.boxAlphaVar = DoubleVar()
        self.boxAlphaText = Entry(lfopt, width=26, textvariable=self.boxAlphaVar)
        self.boxAlphaText.grid(column=1, row=8, pady=2, sticky=W)
        
        self.annotVar = BooleanVar()
        self.annotBox = Checkbutton(lfopt, text='Draw Annotations', variable=self.annotVar)
        self.annotVar.set(False)
        self.annotBox.grid(column=0, row=11, columnspan=2, padx=10, sticky=W)
        #
        self.dfltBtn = Button(lfopt, text="Defaults", padx=10, command=self.loadDefaultOptions)
        self.dfltBtn.grid(column=0, row=12, pady=5, padx=5, sticky=W)
        self.plotBtn = Button(lfopt, text="Plot", padx=10, command=self.plotBtnClicked)
        self.plotBtn.grid(column=1, row=12, pady=5, padx=5, sticky=E)
        
        # Image Viewer
        self.cheight = self.screenheight*48/100
        self.cwidth = self.screenwidth*42/100
        
        self.imgFrame = LabelFrame(self, text="Plot")
        self.imgFrame.grid(column=2, row=0, rowspan=3, sticky=N)
        self.imgFrame.columnconfigure(0, weight=1)
        # History controls
        self.histFrame = Frame(self.imgFrame, padx=5, pady=2)
        self.histFrame.grid(column=0, row=0, sticky=N)
        self.pHistVar = StringVar()
        self.histMenu = OptionMenu(self.histFrame, self.pHistVar, 'None')
        self.histMenu.grid(column=0, row=0, sticky=W, padx=5)
        self.histMenu.config(width=30)
        self.pFtVar = StringVar()
        self.ftMenu = OptionMenu(self.histFrame, self.pFtVar, 'None')
        self.ftMenu.grid(column=1, row=0, sticky=W, padx=5)
        self.ftMenu.config(width=24)
        self.pLeftBtn = Button(self.histFrame, text="<", padx=5, command=self.pLeftBtnClicked)
        self.pLeftBtn.grid(column=2, row=0, padx=5)
        self.pRightBtn = Button(self.histFrame, text=">", padx=5, command=self.pRightBtnClicked)
        self.pRightBtn.grid(column=3, row=0, padx=5)
        self.pSaveThisBtn = Button(self.histFrame, text="Save", padx=5, command=self.saveThisClicked)
        self.pSaveThisBtn.grid(column=4, row=0, padx=25)
        self.pSaveAllBtn = Button(self.histFrame, text="Save All", padx=5, command=self.saveAllClicked)
        self.pSaveAllBtn.grid(column=5, row=0, padx=5)
        
        self.canvas = Canvas(self.imgFrame, width=self.cwidth, height=self.cheight)
        self.canvas.grid(column=0, row=1, sticky=N+E+W+S)
        #
        self.paramList.listbox.bind('<<ListboxSelect>>', self.onParamListSelect)
        self.groupVar.trace("w", self.groupVarChanged)
        self.pcol1Var.trace("w", self.pcolVarChanged)
        self.pcol2Var.trace("w", self.pcolVarChanged)
        self.pFtVar.trace("w", self.pFtVarChanged)
        self.pHistVar.trace("w", self.pHistVarChanged)
        self.labYVar.trace("w", self.labYVarChanged)
        self.yMinVar.trace("w", self.yMinVarChanged)
        self.yMaxVar.trace("w", self.yMaxVarChanged)
        self.styleVar.trace("w", self.styleVarChanged)
        #
        self.bind('<<PlotEvent>>', self.handle_plot_event)
    #
    def loadDefaultOptions(self):
        self.labWVar.set(8)
        self.labHVar.set(5)
        self.dotSzVar.set(0.5)
        self.dotAlphaVar.set(0.5)
        self.boxAlphaVar.set(0.2)
        self.paramList.selectAnItem('Cell Area')
        self.onParamListSelect(None)
        self.blackVar.set(False)
        self.annotVar.set(False)
        self.log2Var.set(False)
        self.ymin = {}
        self.ymax = {}
        self.excl_values = {}
        # self.pcol2Btn.configure(state='disabled')
    #
    def setStatus(self, text):
        if not self.statuslabel:
            return
        if text is None:
            text = ''
        self.statuslabel.configure(text=text)
    #
    def as_object(self):
        data = {
            'group': self.groupVar.get(),
            'features': [f.attr for f in self.paramList.getSelected()],
            'ymin': self.ymin.copy(),
            'ymax': self.ymax.copy(),
            'excl_values': self.excl_values.copy(),
            'primary': self.pcol1Var.get(),
            'secondary': self.getSecondary(),
            'plotstyle': self.styleVar.get(),
            'blackoutline': self.blackVar.get(),
            'xlabel': self.labXVar.get(),
            'width': self.labWVar.get(),
            'height': self.labHVar.get(),
            'dotsize': self.dotSzVar.get(),
            'dotalpha': self.dotAlphaVar.get(),
            'boxalpha': self.boxAlphaVar.get(),
            'annotations': self.annotVar.get(),
            'log2': self.log2Var.get(),
            }
        return data
    #
    def from_object(self, o):
        if not o: return
        features = set(o['features'])
        selected = [e for e in self.paramList.elements if e.attr in features]
        self.ymin = {}
        if 'ymin' in o:
            self.ymin = o['ymin'].copy()
        self.ymax = {}
        if 'ymax' in o:
            self.ymax = o['ymax'].copy()
        self.excl_values = {}
        if 'excl_values' in o:
            self.excl_values = o['excl_values'].copy()
        self.paramList.selectElements(selected)
        self.pcol1Var.set(o['primary'])
        secondary = o.get('secondary')
        if not secondary:
            secondary = '-- None --'
        self.pcol2Var.set(secondary)
        self.styleVar.set(o['plotstyle'])
        self.labXVar.set(o['xlabel'])
        self.labWVar.set(o['width'])
        self.labHVar.set(o['height'])
        self.dotSzVar.set(o['dotsize'])
        self.dotAlphaVar.set(o['dotalpha'])
        self.boxAlphaVar.set(o['boxalpha'])
        try:
            self.blackVar.set(o['blackoutline'])
            self.annotVar.set(o['annotations'])
            self.log2Var.set(o['log2'])
        except Exception:
            pass
    #
    def styleVarChanged(self, a, b, c):
        st = 'disabled' if self.styleVar.get() == 'density' else 'normal'
        self.dotSzText.configure(state=st)
        self.dotAlphaText.configure(state=st)
        self.annotBox.configure(state=st)
    #
    def onParamListSelect(self, event):
        self.selected_feature = None
        sel = self.paramList.getSelected()
        if len(sel) > 0:
            attr = sel[0].attr
            sf = str(sel[0])
            lab = self.ftmgr.getLabel(sf)
            self.labYVar.set(lab)
            self.yMinVar.set(self.ymin.get(attr, ''))
            self.yMaxVar.set(self.ymax.get(attr, ''))
        else:
            sf = None
            self.labYVar.set('')
            self.yMinVar.set('')
            self.yMaxVar.set('')
        st = "normal" if len(sel)==1 else "readonly"
        self.labYText.config(state=st)
        self.yMinText.config(state=st)
        self.yMaxText.config(state=st)
        self.selected_feature = sf
        self.pcol2Btn.configure(state='disabled' if self.getSecondary() is None else 'normal')
    #
    def pcol1BtnClicked(self):
        attr = self.pcol1Var.get()
        if attr in self.attr_values:
            attr_list = sorted(self.attr_values[attr])
        else:
            return
        dlg = AttribValueDialog(self, 'Select values to plot for '+dblquot(attr),
                    attr_list,
                    self.excl_values.get(attr))
        if dlg.is_ok:
            self.excl_values[attr] = dlg.excl_list
    #
    def pcol2BtnClicked(self):
        attr = self.getSecondary()
        if attr is None: return
        if attr in self.attr_values:
            attr_list = sorted(self.attr_values[attr])
        else:
            return
        dlg = AttribValueDialog(self, 'Select values to plot for '+dblquot(attr),
                    attr_list,
                    self.excl_values.get(attr))
        if dlg.is_ok:
            self.excl_values[attr] = dlg.excl_list
    #
    @property
    def selected_attr(self):
        try:
            return self.paramList.getSelected()[0].attr
        except Exception:
            return None
    def yMinVarChanged(self, a, b, c):
        if self.selected_attr is None: return
        try:
            self.ymin[self.selected_attr] = self.yMinVar.get()
        except Exception:
            if self.selected_attr in self.ymin:
                del self.ymin[self.selected_attr]
    def yMaxVarChanged(self, a, b, c):
        if self.selected_attr is None: return
        try:
            self.ymax[self.selected_attr] = self.yMaxVar.get()
        except Exception:
            if self.selected_attr in self.ymax:
                del self.ymax[self.selected_attr]
    #
    def labYVarChanged(self, a, b, c):
        if not self.selected_feature is None:
            self.ftmgr.setLabel(self.selected_feature, self.labYText.get())
    def cfAddBtnClicked(self):
        self.paramList.selectElements(self.paramList.elements)
    def cfRemBtnClicked(self):
        self.paramList.clearSelection()
    def pcolVarChanged(self, a, b, c):
        val = self.getSecondary()
        if val is None:
            val = self.pcol1Var.get()
            st = 'disabled'
        else:
            st = 'normal'
        self.pcol2Btn.configure(state=st)
        self.labXVar.set(val)
    def groupVarChanged(self, a, b, c):
        group_name = self.groupVar.get()
        grp = self.gmgr.loadGroup(group_name)
        self.attr_values = defaultdict(set)
        if 'filelist' in grp:
            for fentry in grp['filelist']:
                if 'attrib' in fentry:
                    attrib = fentry['attrib']
                    for k, v in attrib.iteritems():
                        self.attr_values[k].add(v)
        attrs = sorted(self.attr_values.keys())
        self.update_group_menu(self.pcol1Menu, self.pcol1Var, attrs)
        self.update_group_menu(self.pcol2Menu, self.pcol2Var, ['-- None --']+attrs)
        self.from_object(grp.get('settings'))
        self.updateGroupHistory(group_name)
        self.onParamListSelect(None)
    #
    def updateGroupHistory(self, grname):
        self.ghist = self.gmgr.loadHistory(grname)
        self.ghist_map = {}
        self.ghist_plain = []
        for h in self.ghist:
            hStr = str(h)
            self.ghist_map[hStr] = h
            for ft in h.features:
                self.ghist_plain.append((hStr, ft))
        self.update_group_menu(self.histMenu, self.pHistVar, [str(h) for h in self.ghist])
        if self.ghist:
            self.pHistVar.set(str(self.ghist[0]))
        else:
            self.pHistVar.set('')
    #
    def pHistVarChanged(self, a, b, c):
        h = self.ghist_map.get(self.pHistVar.get())
        if h is None or not h.features:
            self.update_group_menu(self.ftMenu, self.pFtVar, [])
            self.pFtVar.set('')
        else:
            ft = self.pFtVar.get()
            self.update_group_menu(self.ftMenu, self.pFtVar, h.features)
            if not ft in h.features:
                self.pFtVar.set(h.features[0])
        if not h is None:
            self.from_object(h.settings)
    def pFtVarChanged(self, a, b, c):
        h = self.ghist_map.get(self.pHistVar.get())
        ft = self.pFtVar.get()
        png_path = None
        if not h is None:
            png_path = h.filemap.get(ft)
        self.display_image(png_path)
    def pLeftBtnClicked(self):
        if len(self.ghist_plain) <= 1:
            return
        hStr = self.pHistVar.get()
        ft = self.pFtVar.get()
        idx = self.ghist_plain.index((hStr, ft))
        if idx < 0:
            return
        n = len(self.ghist_plain)
        idx = (idx + n - 1) % n
        _hStr, ft = self.ghist_plain[idx]
        if _hStr != hStr:
            self.pHistVar.set(_hStr)
        self.pFtVar.set(ft)
    def pRightBtnClicked(self):
        if len(self.ghist_plain) <= 1:
            return
        hStr = self.pHistVar.get()
        ft = self.pFtVar.get()
        idx = self.ghist_plain.index((hStr, ft))
        if idx < 0:
            return
        n = len(self.ghist_plain)
        idx = (idx + 1) % n
        _hStr, ft = self.ghist_plain[idx]
        if _hStr != hStr:
            self.pHistVar.set(_hStr)
        self.pFtVar.set(ft)
    #
    def saveAllClicked(self):
        self.setStatus('')
        h = self.ghist_map.get(self.pHistVar.get())
        if not h: return
        tgtDir = self.tgtDirVar.get()
        if not os.path.isdir(tgtDir): return
        cnt = 0
        try:
            for spath in h.filemap.itervalues():
                fn = os.path.basename(spath)
                if fn.startswith('_'):
                    fn = fn[1:]
                tpath = os.path.join(tgtDir, fn)
                shutil.copyfile(spath, tpath)
                cnt += 1
            if cnt > 0:
                self.setStatus('%d file(s) successfully copied to "%s"' % (cnt, tgtDir))
        except Exception, ex:
            tkMessageBox.showerror('Error', 'Error copying plot file(s):\n'+str(ex), parent=self)
    #
    def saveThisClicked(self):
        self.setStatus('')
        h = self.ghist_map.get(self.pHistVar.get())
        if not h: return
        tgtDir = self.tgtDirVar.get()
        if not os.path.isdir(tgtDir): return
        spath = h.filemap.get(self.pFtVar.get())
        fn = os.path.basename(spath)
        if fn.startswith('_'):
            fn = fn[1:]
        tpath = os.path.join(tgtDir, fn)
        try:
            shutil.copyfile(spath, tpath)
            self.setStatus('File "%s" successfully copied to "%s"' % (fn, tgtDir))
        except Exception, ex:
            tkMessageBox.showerror('Error', 'Error copying plot file:\n'+str(ex), parent=self)
    #
    def display_image(self, png_path):
        if png_path is None:
            self.cur_plot = None
            self.canvas.delete("IMG")
            self.imgFrame.configure(text='Plot')
            return
        img = Image.open(png_path)
        w, h = img.size
        try:
            sc = float(self.cwidth) / w
            sch = float(self.cheight) / h
            if sch < sc: sc = sch
            w = int(w*sc)
            h = int(h*sc)
            img = img.resize((w, h), Image.ANTIALIAS)
        except Exception:
            pass
        self.cur_plot = ImageTk.PhotoImage(img)
        self.canvas.delete("IMG")
        self.canvas.create_image(0, 0, image=self.cur_plot, anchor=NW, tags="IMG")
        self.imgFrame.configure(text=os.path.basename(png_path))
    #
    def handle_plot_event(self, evt):
        if self.pmgr.is_running:
            self.progr.message(self.pmgr.message)
        else:
            self.progr.destroy()
            self.plotBtn.configure(state=NORMAL)
            if self.pmgr.status != 0:
                tkMessageBox.showerror('Error', self.pmgr.message, parent=self)
                self.setStatus('')
            else:
                self.updateGroupHistory(self.pmgr.grname)
                self.setStatus('Plots for file group "%s" successfully generated.' % (self.pmgr.grname,))
    #
    def plotBtnClicked(self):
        grp = self.loadCurrentGroup()
        if not grp:
            return
        sel = self.getSelectedCellFeatures()
        if not sel:
            return
        o = self.as_object()
        
        self.plotBtn.configure(state=DISABLED)
        self.setStatus('Generating plots for file group "%s"...' % (o['group'],))
        self.pmgr.runit(grp, sel, o)
        self.progr = PlotProgressDialog(self, **self.options)
#        self.progr.wait_window()
    #
            
    @staticmethod
    def update_group_menu(optMenu, grpVar, attrs):
        menu = optMenu["menu"]
        cval = grpVar.get()
        menu.delete(0, END)
        nval = None
        for nm in attrs:
            if nval is None or nm == cval:
                nval = nm
            menu.add_command(label=nm, 
                command=tk._setit(grpVar, nm))
        grpVar.set(nval or '')
    
    @property
    def screenwidth(self):
        return self.options.get('screenwidth', 1536)
    @property
    def screenheight(self):
        return self.options.get('screenheight', 864)
    #
    def tgtDirBtnClicked(self):
        tgtDir = tkFileDialog.askdirectory(initialdir = self.tgtDirVar.get(),
                title="Select directory for output files",
                mustexist=True)
        if not tgtDir: return
        self.tgtDirVar.set(tgtDir)
    #
    def updateFileGroups(self):
        groups = self.gmgr.listGroups()
        menu = self.groupMenu["menu"]
        cval = self.groupVar.get()
        menu.delete(0, END)
        nval = None
        for g in groups:
            nm = str(g)
            if nval is None or nm == cval:
                nval = nm
            menu.add_command(label=nm, 
                command=tk._setit(self.groupVar, nm))
        self.groupVar.set(nval)
    #
    def exportCsvClicked(self):
        if not self.checkTgtDir(): return
        grdata = self.loadCurrentGroup()
        if not grdata: return
        tgtDir = self.tgtDirVar.get()
        grname = self.groupVar.get()
        csv_path = self.gmgr.cacheGroupCsv(grname, grdata)
        errmsg = None
        if not csv_path is None:
            _csvname = grname+'.csv'
            csvname = os.path.join(tgtDir, _csvname)
            _rname = grname+'.R'
            rname = os.path.join(tgtDir, _rname)
            try:
                # Copy cached CSV to the target directory
                shutil.copyfile(csv_path, csvname)
                # Generate R script
                o = self.as_object()
                context = self.pmgr.generate_context(
                    grdata,
                    self.getSelectedCellFeatures(),
                    o,
                    os.path.basename(csvname))
                templ_fn = 'density.R' if o['plotstyle'] == 'density' else 'base.R'
                rcode = self.tmgr.render(templ_fn, context)
                with open(rname, "wb") as fo:
                    fo.write(rcode)
            except Exception, ex:
                errmsg = str(ex)
        else:
            errmsg = self.gmgr.errmsg
        if errmsg is None:
            self.setStatus('Files "%s" and "%s" successfully written to %s' % (_csvname, _rname, tgtDir))
        else:
            tkMessageBox.showerror('Error',
                'Error writing\n'+csvname+'\n'+errmsg,
                parent=self)
    #
    def checkTgtDir(self):
        tgtDir = self.tgtDirVar.get()
        if os.path.isdir(tgtDir):
            return True
        res = tkMessageBox.askyesno(APP_NAME,
            'Directory does not exist:\n%s\nCreate?' % (tgtDir,),
            parent=self)
        if res:
            try:
                os.makedirs(tgtDir)
            except Exception, ex:
                tkMessageBox.showerror('Error', str(ex), parent=self)
                return False
        return res
    #
    def loadCurrentGroup(self):
        grname = self.groupVar.get()
        grp = self.gmgr.loadGroup(grname)
        if not 'filelist' in grp:
            tkMessageBox.showerror('Error', 'No File Group selected', parent=self)
            return None
        return grp
    #
    def getCurrentCellFeature(self):
        sel = self.paramList.getSelected()
        if len(sel) == 0:
            tkMessageBox.showerror('Error', 'No Cell Feature selected', parent=self)
            return None
        return sel[0].attr
    #
    def getSelectedCellFeatures(self):
        sel = self.paramList.getSelected()
        if len(sel) == 0:
            tkMessageBox.showerror('Error', 'No Cell Feature selected', parent=self)
            return None
        return sel
    #
    def getSecondary(self):
        secondary = self.pcol2Var.get()
        if not secondary or secondary == '-- None --':
            secondary = None
        return secondary

class ReshapeVisMainWindow(object):
    PERSISTENT_PROPERTIES = ('srcDir', 'tgtDir', 'rscript', 'currentGroup',
        'includeSubdirs', 'endsWithFilter', 'containsFilter', 'cellFeatureLabels')
    def __init__(self, homedir):
        self.homedir = homedir
        #
        self.statefile = os.path.join(self.homedir, 'vstate.json')
        
        self.window = window  = Tk()
        window.title(APP_NAME+' version '+APP_VERSION)
        window.rowconfigure(0, weight=0)
        window.rowconfigure(1, weight=1)
        window.columnconfigure(0, weight=1)
        
        self.screenwidth = window.winfo_screenwidth()
        self.screenheight = window.winfo_screenheight()
        
        self.mainframe = Frame(window)
        self.mainframe.grid(column=1, row=1, sticky=N+S+W+E)
        self.mainframe.columnconfigure(0, weight=0)
        self.mainframe.columnconfigure(1, weight=1)
        self.mainframe.rowconfigure(0, weight=0)
        self.mainframe.rowconfigure(1, weight=1)
        
        # Status line
        self.statLabel = Label(window, text="", padx=5, pady=2)
        self.statLabel.grid(column=0, row=2, columnspan=2, sticky=W)

        btnframe = Frame(window)
        btnframe.grid(column=0, row=0, pady=10, sticky=N+W)
        self.paneVar = StringVar()
        self.paneVar.set("Visualizations")
        btnVis = Radiobutton(btnframe, padx=10, indicatoron=0,
                    text="Visualizations", variable=self.paneVar, value="Visualizations")
        btnVis.grid(column=0, row=0, sticky=W)
        btnPaneMgr = Radiobutton(btnframe, padx=10, indicatoron=0,
                    text="Group Manager", variable=self.paneVar, value="GroupManager")
        btnPaneMgr.grid(column=1, row=0, sticky=W)
        
        self.gmgrframe = FileListPanel(self.mainframe, homedir=self.homedir)
        self.gmgrframe.grid(column=0, row=1, sticky=N+W)
        
        self.visframe = VisualizationPanel(self.mainframe, homedir=self.homedir,
                screenwidth=self.screenwidth,
                screenheight=self.screenheight,
                statuslabel=self.statLabel)
        self.visframe.grid(column=0, row=1, sticky=N+W)
        self.endsWithFilter = '_Data.csv'
        #
        self.loadState()
        self.visframe.loadDefaultOptions()
        self.visframe.updateFileGroups()
        #
        if not self.srcDir:
            self.srcDir = homedir
        self.gmgrframe.setOption('savestate', self.saveState)
        
        self.gmgrframe.grid_remove()
        self.curframe = self.visframe

        self.paneVar.trace("w", self.paneVarChanged)
        #
    def paneVarChanged(self, a, b, c):
        if self.curframe:
            self.curframe.grid_remove()
        if self.paneVar.get() == "Visualizations":
            self.visframe.grid(column=0, row=1, sticky=N+W)
            self.curframe = self.visframe
            self.visframe.cache.clear()
            self.visframe.updateFileGroups()
        else:
            self.gmgrframe.grid(column=0, row=1, sticky=N+W)
            self.curframe = self.gmgrframe
        self.statLabel.configure(text='')
    @property
    def srcDir(self):
        return self.gmgrframe.srcDirVar.get()
    @srcDir.setter
    def srcDir(self, text):
        self.gmgrframe.srcDirVar.set(text)
    @property
    def tgtDir(self):
        return self.visframe.tgtDirVar.get()
    @tgtDir.setter
    def tgtDir(self, text):
        self.visframe.tgtDirVar.set(text)
    @property
    def currentGroup(self):
        return self.visframe.groupVar.get()
    @currentGroup.setter
    def currentGroup(self, groupName):
        grp = self.visframe.gmgr.loadGroup(groupName)
        if not grp:
            return
        self.visframe.groupVar.set(groupName)
    @property
    def includeSubdirs(self):
        return self.gmgrframe.subDirVar.get()
    @includeSubdirs.setter
    def includeSubdirs(self, text):
        self.gmgrframe.subDirVar.set(text)
    @property
    def endsWithFilter(self):
        return self.gmgrframe.endsWithVar.get()
    @endsWithFilter.setter
    def endsWithFilter(self, text):
        self.gmgrframe.endsWithVar.set(text)
    @property
    def containsFilter(self):
        return self.gmgrframe.containsVar.get()
    @containsFilter.setter
    def containsFilter(self, text):
        self.gmgrframe.containsVar.set(text)
    @property
    def rscript(self):
        return self.visframe.rscript
    @rscript.setter
    def rscript(self, text):
        self.visframe.rscript = text
    @property
    def cellFeatureLabels(self):
        return self.visframe.ftmgr.exprMap.copy()
    @cellFeatureLabels.setter
    def cellFeatureLabels(self, val):
        self.visframe.ftmgr.exprMap.update(val)
    #
    def loadState(self):
        self.reshapeParams = None
        try:
            with open(self.statefile, 'r') as fi:
                params = json.load(fi)
            for prop in self.PERSISTENT_PROPERTIES:
                if not prop in params: continue
                setattr(self, prop, params[prop])
        except Exception, ex:
            print 'loadState() exception:', ex
            pass
    def saveState(self):
        try:
            params = dict((prop, getattr(self, prop)) for prop in self.PERSISTENT_PROPERTIES)
            with open(self.statefile, 'w') as fo:
                json.dump(params, fo, indent=2)
        except Exception, ex:
            print 'saveState() exception:', ex
            pass
    def run(self):
        self.window.mainloop();
        self.saveState()
        #
        print 'Purge history:'
        self.gmgrframe.gmgr.purgeHistory()

if __name__ == '__main__':
    
    myhome = os.path.join(os.path.expanduser('~'), '.reshape')
    if not os.path.isdir(myhome):
        try:
            os.mkdir(myhome)
        except Exception:
            myhome = '.'
    
    proc = ReshapeVisMainWindow(myhome)
    proc.run()

    sys.exit(0)
