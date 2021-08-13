import os, time, csv
from collections import defaultdict
import Tkinter as tk
from Tkinter import *
import tkSimpleDialog
import tkMessageBox
import tkFileDialog
from ScrolledText import ScrolledText
from .listentry import ListFileEntry, listfiles
from .filegroup import FileGroupManager
from .attredit import AttributeEditor
from .dialogs import FileSelectorDialog

from . import APP_NAME

class ScrolledList(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, parent)
        self.options = options
        
        self.elements = []
        
        self.yScroll = Scrollbar(self, orient=VERTICAL)
        self.yScroll.grid(row=0, column=1, sticky=N+S)
        
        self.listbox = Listbox(self,
            width=options.get('width', 66),
            height=options.get('height', 18),
            activestyle="none",
            selectmode=options.get('selectmode', EXTENDED),
            exportselection=False,
            yscrollcommand=self.yScroll.set)
        self.listbox.grid(row=0, column=0, sticky=N+S+W+E)
        self.yScroll['command'] = self.listbox.yview

        self.pack(fill=BOTH, expand=True)
    def clear(self):
        if self.listbox.size() > 0:
            self.listbox.delete(0, self.listbox.size()-1)
        self.elements = []
    def _clearSelection(self):
        self.listbox.selection_clear(0, self.listbox.size())
    def clearSelection(self):
        self._clearSelection()
        self.listbox.event_generate('<<ListboxSelect>>')
    def addElements(self, elements):
        eset = self.elementSet()
        eset.update(elements)
        self.elements = list(eset)
        self._push()
    def _push(self):
        self.elements.sort(key=lambda x: x.sortkey() if hasattr(x, 'sortkey') else str(x))
        if self.listbox.size() > 0:
            self.listbox.delete(0, self.listbox.size()-1)
        for j in range(0, len(self.elements), 200):
            sublist = self.elements[j:j+200]
            self.listbox.insert(END, *sublist)
        self.listbox.see(0)
    def updateElements(self, elements):
        self.elements = []
        self.addElements(elements)
    def popSelected(self):
        poplist = []
        staylist = []
        selidx = [int(x) for x in self.listbox.curselection()]
        for i, e in enumerate(self.elements):
            if i in selidx:
                poplist.append(e)
            else:
                staylist.append(e)
        self._clearSelection()
        self.elements = staylist
        self._push()
        self.listbox.event_generate('<<ListboxSelect>>')
        return poplist
    def selectElements(self, elements):
        self._clearSelection()
        sel = set(elements)
        first = None
        last = None
        for i, e in enumerate(self.elements):
            if e in sel:
                self.listbox.selection_set(i)
                if first is None:
                    first = i
                last = i
        if not first is None:
            self.listbox.see(last)
            self.listbox.see(first)
        self.listbox.event_generate('<<ListboxSelect>>')
    def selectAnItem(self, text):
        self._clearSelection()
        for i, e in enumerate(self.elements):
            if text in str(e):
                self.listbox.selection_set(i)
        self.listbox.event_generate('<<ListboxSelect>>')        
    def getSelected(self):
        selected = []
        for i in self.listbox.curselection():
            i = int(i)
            selected.append(self.elements[i])
        return selected
    def elementSet(self):
        return set(self.elements)
    def toggleSelection(self):
        all_elem = self.elementSet()
        for elem in self.getSelected():
            all_elem.discard(elem)
        self.selectElements(all_elem)
    def selectElementsContaining(self, txt):
        sel_elem = set(self.getSelected())
        for elem in self.elements:
            if txt in str(elem):
                sel_elem.add(elem)
        self.selectElements(sel_elem)
    def deselectElementsContaining(self, txt):
        sel_elem = set(self.getSelected())
        for elem in self.elements:
            if txt in str(elem):
                sel_elem.discard(elem)
        self.selectElements(sel_elem)
    #
    def selectMatchingElements(self, filt):
        sel_elem = set(self.getSelected())
        for elem in self.elements:
            if self.match_element(elem, filt):
                sel_elem.add(elem)
        self.selectElements(sel_elem)
    def deselectMatchingElements(self, filt):
        sel_elem = set()
        for elem in self.getSelected():
            if not self.match_element(elem, filt):
                sel_elem.add(elem)
        self.selectElements(sel_elem)
    #
    @staticmethod
    def match_element(elem, filt):
        if FileSelectorDialog.FILENAME_ATTR in filt:
            fn = filt[FileSelectorDialog.FILENAME_ATTR]
            if not fn in str(elem):
                return False
        for a, v in filt.iteritems():
            if a == FileSelectorDialog.FILENAME_ATTR:
                continue
            if not a in elem.attrib:
                return False
            if elem.attrib[a] != v:
                return False
        return True
    #

class ListSelector(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, parent)
        self.options = options
        
        self.grname = 'Group'
        self.grdir = None
        self.sel_filter = {}
        
        self.grid_rowconfigure(1, weight=1)
        
        self.exList = ScrolledList(self)
        self.exList.grid(column=0, row=0, rowspan=2, sticky=N+S)
        
        mf = Frame(self)
        mf.grid(column=1, row=0)
        self.addBtn = Button(mf, text="->", padx=5, command=self.addButtonClicked)
        self.addBtn.grid(column=0, row=0, padx=5, sticky=W+E)
        self.remBtn = Button(mf, text="<-", padx=5, command=self.removeButtonClicked)
        self.remBtn.grid(column=0, row=1, padx=5, sticky=W+E)

        infr = Frame(self)
        infr.grid(column=2, row=0, sticky=N+S)
        self.inList = ScrolledList(infr, height=10)
        self.inList.grid(column=0, row=0, sticky=N+S)
        self.inList.listbox.bind('<<ListboxSelect>>', self.onInListSelect)
        
        self.inStatVar = StringVar()
        self.inStatLab = Label(infr, textvariable=self.inStatVar)
        self.inStatLab.grid(column=0, row=1, padx=5, pady=1, sticky=W)
        self.inStatLab.configure(foreground='#555599')
        
        sf = Frame(self)
        sf.grid(column=3, row=0, sticky=N)
        self.selAddBtn = Button(sf, text='+', padx=5, command=self.onAddSelect)
        self.selAddBtn.grid(column=0, row=0, sticky=W+E, padx=5)
        self.selSubBtn = Button(sf, text='-', padx=5, command=self.onSubSelect)
        self.selSubBtn.grid(column=0, row=1, sticky=W+E, padx=5)
        self.selToggleBtn = Button(sf, text='*', padx=5, command=self.onToggleSelect)
        self.selToggleBtn.grid(column=0, row=2, sticky=W+E, padx=5)
        
        self.attrFrame = LabelFrame(self, text="Pseudocolumns", padx=5, pady=5)
        self.attrFrame.grid(column=2, row=1, sticky=N+S+W+E)
        
        self.attrEdit = AttributeEditor(self.attrFrame,
            attributeChanged=self.attributeChanged, attributeRemoved=self.attributeRemoved)
        self.attrEdit.grid(column=0, row=0)
        self.attrEdit.hideChildren()
        
        eif = Frame(self)
        eif.grid(column=3, row=1, sticky=N, pady=10)
        self.expAttrBtn = Button(eif, text='Exp', command=self.onExportAttr)
        self.expAttrBtn.grid(column=0, row=0, sticky=W+E, padx=5)
        self.impAttrBtn = Button(eif, text='Imp', command=self.onImportAttr)
        self.impAttrBtn.grid(column=0, row=1, sticky=W+E, padx=5)
        
        self.pack(fill=BOTH, expand=True)
        #
    def _get_attr_list(self):
        attr_map = defaultdict(set)
        for e in self.inList.elements:
            for k, v in e.attrib.iteritems():
                if v:
                    attr_map[k].add(v)
        attr_list = []
        for a in sorted(attr_map.keys()):
            attr_list.append((a, sorted(attr_map[a])))
        return attr_list
    def onAddSelect(self):
        dlg = FileSelectorDialog(self, 'Select Files', self._get_attr_list(), self.sel_filter)
        if not dlg.is_ok:
            return
        self.sel_filter = dlg.filter
        res = self.sel_filter.get(FileSelectorDialog.FILENAME_ATTR, '')
        self.inList.selectMatchingElements(self.sel_filter)
        self.exList.selectElementsContaining(res)
        self.attrEdit.setNewValue(res.strip())
    def onSubSelect(self):
        dlg = FileSelectorDialog(self, 'De-Select Files', self._get_attr_list(), self.sel_filter)
        if not dlg.is_ok:
            return
        self.sel_filter = dlg.filter
        res = self.sel_filter.get(FileSelectorDialog.FILENAME_ATTR, '')
        self.inList.deselectMatchingElements(self.sel_filter)
        self.exList.deselectElementsContaining(res)
    def onToggleSelect(self):
        self.inList.toggleSelection()
        self.exList.toggleSelection()
    def _update_sel_count(self):
        ntotal = len(self.inList.elements)
        nsel = len(self.inList.getSelected())
        self.inStatVar.set('%d of %d selected' % (nsel, ntotal))
    def onInListSelect(self, event):
        self._update_sel_count()
        fn = self.options.get('inSelect')
        if not fn: return
        fn(self.inList, self.inList.getSelected())
    def setAttribData(self, data):
        if data is None:
            self.attrEdit.hideChildren()
            return
        self.attrEdit.setData(data['attributes'], data['available'])
    def attributeChanged(self, name, value):
        for fentry in self.inList.getSelected():
            fentry.attrib[name] = value
    def attributeRemoved(self, name):
        for fentry in self.inList.getSelected():
            if name in fentry.attrib:
                del fentry.attrib[name]
    def updateLeft(self, flist):
        fset = set(flist) - self.inList.elementSet()
        self.exList.updateElements(list(fset))
    def updateRight(self, flist, grname=None):
        self.grname = grname or 'Group'
        self.grdir = None
        self.inList.updateElements(flist)
        self._update_sel_count()
    def addButtonClicked(self):
        selements = self.exList.popSelected()
        self.inList.addElements(selements)
        self.inList.selectElements(selements)
    def removeButtonClicked(self):
        selements = self.inList.popSelected()
        self.exList.addElements(selements)
        self.exList.selectElements(selements)
    def groupFileList(self):
        res = []
        for e in self.inList.elements:
            res.append({'name':e.fullpath, 'attrib':e.attrib.copy()})
        return res
    #
    def _export_file_attrib(self, fpath):
        all_attr = set()
        for e in self.inList.elements:
            all_attr.update(e.attrib.keys())
        attr_list = sorted(all_attr)
        headers = ['file'] + attr_list
        with open(fpath, 'wb') as fo:
            wr = csv.writer(fo, dialect='excel')
            wr.writerow(headers)
            for e in self.inList.elements:
                row = [e.basename]
                for attr in attr_list:
                    row.append(e.attrib.get(attr, ''))
                wr.writerow(row)
    def onExportAttr(self):
        try:
            cdir = os.path.dirname(self.inList.elements[0].fullpath)
        except Exception:
            self.grdir = None
            return
        if self.grdir is None:
            self.grdir = cdir
        fn = self.grname + '_pseudocolumns.csv'
        fpath = tkFileDialog.asksaveasfilename(
            initialdir=self.grdir,
            initialfile=fn,
            filetypes = (('CSV files','*.csv'),),
            title='Export File names and Pseudocolumns',
            defaultextension='.csv')
        if not fpath is None:
            try:
                self._export_file_attrib(fpath)
            except Exception:
                pass
    #
    def _import_file_attrib(self, fpath):
        exMap = dict([(e.basename,e) for e in self.exList.elements])
        inMap = dict([(e.basename,e) for e in self.inList.elements])
        with open(fpath, 'r') as fi:
            rd = csv.DictReader(fi, dialect='excel')
            for row in rd:
                if not 'file' in row: continue
                fn = row.pop('file')
                if fn in inMap:
                    e = inMap[fn]
                elif fn in exMap:
                    e = exMap.pop(fn)
                    inMap[fn] = e
                else:
                    continue
                e.attrib.update(row)
            self.updateRight(sorted(inMap.values(), key=lambda x:x.basename), self.grname)
            self.updateLeft(sorted(exMap.values(), key=lambda x:x.basename))
    def onImportAttr(self):
        cdir = None
        try:
            cdir = os.path.dirname(self.inList.elements[0].fullpath)
        except Exception:
            try:
                cdir = os.path.dirname(self.exList.elements[0].fullpath)
            except Exception:
                return
        fpath = tkFileDialog.askopenfilename(
            initialdir=cdir,
            filetypes = (('CSV files','*.csv'),),
            title='Import File names and Pseudocolumns',
            defaultextension='.csv')
        if not fpath is None:
            try:
                self._import_file_attrib(fpath)
            except Exception:
                pass
    #
        
class FileListPanel(Frame):
    def __init__(self, parent, **options):
        Frame.__init__(self, relief=RAISED, borderwidth=1)
        self.pack(fill=BOTH, expand=True)
        #
        self.options = options
        self.homedir = options.get("homedir", ".")
        self.gmgr = FileGroupManager(self.homedir)
        
        # Source directory
        lfdir = LabelFrame(self, text="Source Directory and Filters", padx=5, pady=5)
        lfdir.grid(column=0, row=0, sticky=W+N)
        
        srcDirLbl = Label(lfdir, text="Data directory:", padx=10)
        srcDirLbl.grid(column=0, row=0, sticky=E)
        
        self.srcDirVar = StringVar()
        self.srcDirTxt = srcDirTxt = Entry(lfdir, width=72, textvariable=self.srcDirVar)
        srcDirTxt.grid(column=1, row=0)
        srcDirBtn = Button(lfdir, text="...", padx=5, command=self.srcDirBtnClicked)
        srcDirBtn.grid(column=2, row=0, padx=5, pady=5)
        
        subDirLbl = Label(lfdir, text="Include Subdirectories:", padx=10)
        subDirLbl.grid(column=0, row=1, sticky=E)
        self.subDirVar = StringVar()
        self.subDirVar.set("Yes")
        subDirYesNo = Frame(lfdir)
        subDirYesNo.grid(column=1, row=1, pady=5, sticky=W)
        subDirYes = Radiobutton(subDirYesNo, padx=10,
                    text="Yes", variable=self.subDirVar, value="Yes")
        subDirYes.grid(column=0, row=0, sticky=W)
        subDirNo = Radiobutton(subDirYesNo, padx=10,
                    text="No", variable=self.subDirVar, value="No")
        subDirNo.grid(column=1, row=0, sticky=W)
        
        endsWithLabel = Label(lfdir, text="Name Ends with:", padx=10)
        endsWithLabel.grid(column=0, row=2, sticky=E)
        self.endsWithVar = StringVar()
        endsWithTxt = Entry(lfdir, width=72, textvariable=self.endsWithVar)
        endsWithTxt.grid(column=1, row=2, pady=5, sticky=W)
        
        containsLabel = Label(lfdir, text="Name Contains:", padx=10)
        containsLabel.grid(column=0, row=3, sticky=E)
        self.containsVar = StringVar()
        containsTxt = Entry(lfdir, width=72, textvariable=self.containsVar)
        containsTxt.grid(column=1, row=3, pady=5, sticky=W)
        
        # Open/Save File Lists
        lflists = LabelFrame(self, text="Groups", padx=5, pady=5)
        lflists.grid(column=1, row=0, sticky=W+N)
        
        self.namedListVar = StringVar()
        self.namedListMenu = OptionMenu(lflists, self.namedListVar,
                    "-- NEW --", "existing")
        self.namedListMenu.config(width=24)
        self.namedListMenu.grid(column=0, row=0, columnspan=2)
        self.loadListBtn = Button(lflists, text="Load", padx=5, command=self.loadBtnClicked)
        self.loadListBtn.grid(column=2, row=0)
        self.delListBtn = Button(lflists, text="Del", padx=5, command=self.delBtnClicked)
        self.delListBtn.grid(column=3, row=0)
        
        nameLabel = Label(lflists, text="Name:")
        nameLabel.grid(column=0, row=1)
        self.curListVar = StringVar()
        curListTxt = Entry(lflists, width=30, textvariable=self.curListVar)
        curListTxt.grid(column=1, row=1, columnspan=2)
        self.saveListBtn = Button(lflists, text="Save", padx=5, command=self.saveBtnClicked)
        self.saveListBtn.grid(column=3, row=1)
        
        descrLabel = Label(lflists, text="Description:")
        descrLabel.grid(column=0, row=3, columnspan=4, sticky=W)
        
        self.descrText = ScrolledText(lflists, height=3, width=32)
        self.descrText.grid(column=0, row=4, columnspan=4)
        
        # Source file lists
        lffiles = LabelFrame(self, text="Files")
        lffiles.grid(column=0, row=1, columnspan=2, sticky=W+E+N+S)
        
        self.lst = lst = ListSelector(lffiles, inSelect=self.selectionChanged)
        lst.grid(column=0, row=0, sticky=W+E+N+S)
        
        self.srcDirVar.trace("w", self.dirVarChanged)
        self.subDirVar.trace("w", self.dirVarChanged)
        self.endsWithVar.trace("w", self.dirVarChanged)
        self.containsVar.trace("w", self.dirVarChanged)
        
        self.namedListVar.trace("w", self.nameVarChanged)
        self.curListVar.trace("w", self.curListVarChanged)
        self.namedListVar.set("-- NEW --")
        self.curListVar.set("")
        self.updateFileGroups()
        
    def updateFileGroups(self):
        groups = self.gmgr.listGroups()
        menu = self.namedListMenu["menu"]
        menu.delete(1, END)
        for g in groups:
            nm = str(g)
            menu.add_command(label=nm, 
                command=tk._setit(self.namedListVar, nm))
    def selectionChanged(self, inList, selected):
        if len(selected) == 0:
            self.lst.setAttribData(None)
            return
        all_attr = defaultdict(set)
        for fentry in inList.elements:
            for n, v in fentry.attrib.iteritems():
                all_attr[n].add(v)
        sel_attr = set()
        for fentry in selected:
            sel_attr.update(fentry.attrib.keys())
        attribs = []
        for attr in sorted(sel_attr):
            values = set()
            for fentry in selected:
                values.add(fentry.attrib.get(attr, ''))
            value = '-- no change --'
            if len(values) == 1:
                value = values.pop()
            attribs.append({'name':attr, 'value':value, 'list':sorted(all_attr[attr])})
            del all_attr[attr]
        self.lst.setAttribData({'attributes':attribs, 'available':all_attr})
    def saveBtnClicked(self):
        grname = self.curListVar.get()
        if self.gmgr.groupExists(grname):
            result = tkMessageBox.askyesno(APP_NAME,
                'File Group "%s" already exists;\nDo you want to write over?' % (grname,))
            if not result: return
            grdata = self.gmgr.loadGroup(grname)
        else:
            grdata = {}
        grdata['description'] = self.descrText.get(1.0, END).rstrip()
        grdata['filelist'] = self.lst.groupFileList()
        grdata['ts'] = int(time.time())
        self.gmgr.deleteHistory(grdata)
        self.gmgr.saveGroup(grname, grdata)
        self.updateFileGroups()
        self.namedListVar.set(grname)
        self.lst.grname = grname
    def delBtnClicked(self):
        grname = self.namedListVar.get()
        if self.gmgr.groupExists(grname):
            result = tkMessageBox.askyesno(APP_NAME,
                'Delete File Group "%s"?' % (grname,))
            if not result: return
        self.gmgr.deleteGroup(grname)
        self.updateFileGroups()
        self.namedListVar.set("-- NEW --")
    def loadBtnClicked(self):
        grname = self.namedListVar.get()
        gr = self.gmgr.loadGroup(grname)
        flist = [ListFileEntry(fp['name'], attrib=fp['attrib']) for fp in gr.get('filelist', [])]
        self.lst.updateRight(flist, grname)
        self.descrText.delete(1.0,END)
        self.descrText.insert(END,gr.get('description', ''))
        self.curListVar.set(grname)
        self.updateDir()
    def srcDirBtnClicked(self):
        srcDir = tkFileDialog.askdirectory(initialdir = self.srcDirVar.get(),
                title="Select directory containing REShAPE files (.csv)",
                mustexist=True)
        if not srcDir: return
        self.srcDirVar.set(srcDir)
    def nameVarChanged(self, a, b, c):
        st = DISABLED if self.namedListVar.get() == '-- NEW --' else NORMAL
        if st == DISABLED:
            self.lst.updateRight([])
            self.updateDir()
            self.curListVar.set('')
            self.descrText.delete(1.0,END)
        self.loadListBtn.configure(state=st)
        self.delListBtn.configure(state=st)
    def curListVarChanged(self, a, b, c):
        st = DISABLED if not self.curListVar.get() else NORMAL
        self.saveListBtn.configure(state=st)
    def dirVarChanged(self, a, b, c):
        self.updateDir()
        self.saveState()
    def getFilters(self):
        filters = {}
        endswith = self.endsWithVar.get()
        if endswith:
            filters['endswith'] = endswith
        contains = self.containsVar.get()
        if contains:
            filters['contains'] = contains
        return filters
    def updateDir(self):
        srcDir = self.srcDirVar.get()
        if not srcDir:
            return
        subdirs = self.subDirVar.get() == 'Yes'
        flist = listfiles(srcDir, subdirs, self.getFilters())
        self.lst.updateLeft(flist)
    def setOption(self, name, value):
        self.options[name] = value
    def saveState(self):
        if 'savestate' in self.options:
            self.options['savestate']()
