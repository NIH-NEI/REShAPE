'''
Created on Sep 13, 2018

@author: Andrei I Volkov
'''

import sys, os, json, subprocess, time, datetime
import threading
from Tkinter import *
import tkMessageBox
from ScrolledText import ScrolledText
import tkFileDialog

import matconvnet_patch
from reshape_config import *
from reshape_csv import ReshapeCsv
from reshape_feature import ReshapeCellFeatureManager

import reshape_version

APP_NAME = reshape_version.RESHAPE_APP_NAME
APP_VERSION = '%s (%s)' % (reshape_version.RESHAPE_APP_VERSION, reshape_version.RESHAPE_APP_RELEASE)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

INTERNAL_SCALES = [('5x', 5.), ('2x', 2.), ('None', 1.), ('1/2', 0.5), ('1/5', 0.2)]
INTERNAL_SCALE_MAP = dict(INTERNAL_SCALES)
ARTI_FILTERS = [('None', 0.), ('Regular', 5.), ('Strong', 3.), ('Weak', 7.)]
ARTI_FILTER_MAP = dict(ARTI_FILTERS)

class ProcessedImage(object):
    def __init__(self, pathname, start_ts):
        self.pathname = pathname
        self.start_ts = start_ts
        #
        self.end_ts = None
        self.hist = []
        self.items = {}
    def add_item(self, ts, what, pathname):
        self.hist.append((ts, what, pathname))
        self.items[what] = pathname
    def get_item(self, what):
        return self.items.get(what)
    def __repr__(self):
        return 'ProcessedImage('+self.pathname+','+str(self.start_ts)+') hist='+str(self.hist)

class TiledImage(object):
    def __init__(self, jsonname):
        self.jsonname = jsonname
        #
        self.hist = []
        self.items = {}
        #
        self.tgt_dir = os.path.dirname(jsonname)
        self.data = {}
        self.tilemap = {}
        with open(jsonname, 'r') as fi:
            self.data = json.load(fi)
        self.source = self.data['source']
        self.basename = os.path.basename(self.source)
        idx = self.basename.rfind('.')
        if idx > 0:
            self.basename = self.basename[:idx]
        tiles = self.data['tiles']
        if isinstance(tiles,dict):
            self.data['tiles'] = [tiles]
        for tile in self.data['tiles']:
            name = tile['name']
            self.tilemap[name] = tile
    def save(self):
        with open(self.jsonname, 'w') as fo:
            json.dump(self.data, fo, indent=2)
    def add_item(self, ts, what, pathname):
        self.hist.append((ts, what, pathname))
        self.items[what] = pathname
    def get_item(self, what):
        return self.items.get(what)
      
class FijiProc(object):
    DEFAULT_FIJI_APP = '/Fiji.app/ImageJ-linux64'
    DEFAULT_FIJI_MACRO = 'REShAPE_Auto_0.0.2.ijm'
    DEFAULT_IMAGING_APP = os.path.join(BASE_DIR, 'Imaging', 'ReshapeImaging')
    def __init__(self, parent):
        self.parent = parent
        #
        self.limitspath = None
        self.cfg = parent.cfg
        self.fiji = self.cfg.get('fiji', self.DEFAULT_FIJI_APP)
        self.imaging = self.cfg.get('Imaging', self.DEFAULT_IMAGING_APP)
        
        self.running = False
        self.macro_dir = os.path.join(BASE_DIR, 'FijiProc')
        self.reshape = os.path.join(self.macro_dir, self.cfg.get('REShAPE', self.DEFAULT_FIJI_MACRO))
        #
        self.processed = []
        self.cur_params = []
        self.work_params = {}
        #
        self.tiled_images = []
        self.tilemap = {}
    def set_tiled_images(self, jsonlist):
        self.tiled_images = []
        self.tilemap = {}
        for fn in jsonlist:
            ti = TiledImage(fn)
            self.tiled_images.append(ti)
            self.tilemap.update(ti.tilemap)
    def gen_fiji_param(self):
        paramfn = os.path.join(self.parent.homedir, 'reshapeparam.txt')
        self.cur_params = params = [(k, str(v)) for k, v in self.parent.reshapeParams.iteritems()]
        with open(paramfn, 'w') as fo:
            fo.write('\n'.join(['='.join(pv) for pv in params]))
        return paramfn
    def update_status(self, txt):
        self.parent.update_status(txt)
    def run(self):
        self.running = True
        self.rc = -1
        self.processed = []
        self.work_params = {}
        rc = self.run_fiji()
        if rc:
            self.rc = rc        
            self.running = False
            return
        
        for ti in self.tiled_images:
            ti.save()
            
        rc = 0
        for ti in self.tiled_images:
            _rc = self.run_imaging(ti)
            if _rc != 0:
                self.update_status('Imaging exited (%d).' % (_rc,))
                rc = _rc
        if rc == 0:
            rc = self.run_combo()
        self.rc = rc        
        self.running = False
    #
    @staticmethod
    def parse_status(ln):
        if not ln.startswith('['):
            return None
        parts = ln[1:].split('] ', 1)
        try:
            ts = float(parts[0])
            ln = parts[1]
        except Exception, ex:
            return None
        parts = ln.split(': {', 1)
        try:
            what = parts[0].strip()
            pathname = parts[1].rsplit('}',1)[0]
        except Exception, ex:
            return None
        return ts, what, pathname
    #
    def run_imaging(self, tile):
        print 'Neighbors for:', tile.source
        origname = os.path.basename(tile.source)
        try:
            ts_start = datetime.datetime.now()
            cmd = [self.imaging,
                   'Jsonname=%s' % (tile.jsonname,),
                   'Basename=%s' % (tile.basename,),
                   'Origname=%s' % (origname,),
                   ]
            for key, val in self.work_params.iteritems():
                cmd.append('%s=%s' % (key, val))
            for key in ('graphic_choice', 'LUT_choice', 'graphic_format', 'reconstruct_tiled'):
                val = self.parent.reshapeParams.get(key, '')
                cmd.append('%s=%s' % (key, val))
            if not self.limitspath is None:
                cmd.append('feature_limits=%s' % (self.limitspath,))
            print cmd

            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.update_status('Neighbors for: '+tile.source)
            while True:
                ln = p.stdout.readline()
                if not ln: break
                ln = ln.strip()
                print 'IMAGING:', ln
                st = self.parse_status(ln)
                if st is None: continue
                ts, what, pathname = st
                tile.add_item(ts, what, pathname)
                if what in ('NeighborStats', 'ParticleData', 'Outlines', 'Neighbors', 'Area'):
                    self.update_status(os.path.basename(pathname))
            #
            rc = p.wait()
            elapsed = datetime.datetime.now() - ts_start
            # self.update_status('Imaging exited(%d) in %s' % (rc, str(elapsed)))
        except Exception, ex:
            self.update_status('Error executing Imaging: %s' % (ex,))
            rc = 3
        return rc    
    #    
    def run_fiji(self):
        curFile = None
        try:
            ts_start = datetime.datetime.now()
            fijip = self.gen_fiji_param()
            tilep = {'unit_pix':1, 'unit_real':1, 'unit_conv':0}
            params = dict(self.cur_params)
            if params.get('unit_conv') == 'Yes':
                tilep = {'unit_conv':1, 'unit_pix':float(params['unit_pix']), 'unit_real':float(params['unit_real'])}
            for ti in self.tiled_images:
                ti.data.update(tilep)
            cmd = [self.fiji, '--headless', '--console', '-macro', self.reshape, fijip]
            print cmd
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.update_status('Fiji started.')
            while True:
                ln = p.stdout.readline()
                if not ln: break
                ln = ln.strip()
                print 'FIJI:', ln
                st = self.parse_status(ln)
                if st is None: continue
                ts, what, pathname = st
                if what == 'Reading':
                    curFile = ProcessedImage(pathname, ts)
                    self.processed.append(curFile)
                    fn = os.path.basename(pathname)
                    self.update_status('%s' % (fn,))
                    if fn in self.tilemap:
                        tile = self.tilemap[fn]
                        tile['attr'] = curFile.items
                elif what == 'Done':
                    if curFile:
                        curFile.end_ts = ts
                        curFile = None
                elif curFile:
                    curFile.add_item(ts, what, pathname)
                else:
                    self.work_params[what] = pathname
            #       
            rc = p.wait()
            elapsed = datetime.datetime.now() - ts_start
            self.update_status('Fiji exited(%d) in %s' % (rc, str(elapsed)))
        except Exception, ex:
            self.update_status('Error executing Fiji: %s' % (ex,))
            rc = 3
        return rc
    #
    def run_combo(self):
        for ti in self.tiled_images:
            print ti.get_item('ParticleData')
        # Save combined CSV data
        combined_dir = self.work_params.get('CombinedDir')
        if combined_dir is None:
            return 0
        csvwr = ReshapeCsv(combined_dir)
        n_list = []
        a_list = []
        for imgp in self.tiled_images:
            nbr = imgp.get_item('NeighborStats')
            if nbr: n_list.append(nbr)
            ares = imgp.get_item('ParticleData')
            if ares: a_list.append(ares)
        if len(n_list) < 1 or len(a_list) < 1:
            return 0
        nfpath = csvwr.combineNeighbors(n_list)
        self.update_status(os.path.basename(nfpath));
        afpath = csvwr.combineData(a_list)
        self.update_status(os.path.basename(afpath));
        return 0
    #

# example_config = {'Imaging': u'/home/vmshare/REShAPE/Imaging/ReshapeImaging',
#          'SEGMENTATION': u'/home/vmshare/REShAPE/Standalone/segmentation.sh',
#          'cuda_ver': None,
#          'MRT_DIR': u'/opt/MatlabRT/v97',
#          'matlab_opts': ['-nodisplay', '-nosplash'],
#          'fiji': u'/opt/Fiji.app/ImageJ-linux64',
#          'mex': {'enableCudnn': False,
#                  'verbose': 1,
#                  'EnableImreadJpeg': True,
#                  'cudaMethod': 'mex',
#                  'enableGpu': True,
#                  'cudaRoot': None},
#          'matlab_ver': None,
#          'REShAPE': 'REShAPE_Auto_0.0.1.ijm',
#          'USE_STANDALONE': True,
#          'environ': {},
#          'matlab': None,
#          'cuda': None}

class MatlabSegmentation(object):
    def __init__(self, parent):
        self.parent = parent
        #
        self.cfg = parent.cfg
        self.matlab_standalone = None
        self.use_standalone = False
        self.matlab = self.cfg.get('matlab', 'matlab')
        self.matlab_opts = self.cfg.get('matlab_opts') or []
        if 'SEGMENTATION' in self.cfg:
            self.matlab_standalone = self.cfg['SEGMENTATION']
            self.use_standalone = self.cfg.get('USE_STANDALONE', False)
            if self.use_standalone:
                print 'Using Matlab standalone script:', self.matlab_standalone
        #
        self.running = False
        self.segm_dir = os.path.join(BASE_DIR, 'Segmentation')
        self.logfile = os.path.join(parent.homedir, 'segmfunc.log')
        #
        self.outputs = []
    # Writing metadata <IP-Create Image Subset-01.czi> -> C:\AVProjects\CziData\Processed\IP-Create Image Subset-01.json
    def read_log(self):
        fi = None
        while self.running:
            if fi is None:
                try:
                    fi = open(self.logfile, 'r')
                except Exception:
                    fi = None
                if fi is None:
                    time.sleep(0.5)
                    continue
            for ln in fi.readlines():
                ln = ln.strip()
                print 'MATLAB:', ln
                fn = None
                if ln.startswith('Already processed image:'):
                    fn = ln[25:] + ' (already done)'
                elif ln.startswith('Processing '):
                    fn = ln
                elif ln.startswith('Writing segmented image '):
                    parts = ln.split(' -> ')
                    if len(parts) > 1:
                        fn = '-> '+parts[1].strip()
                elif ln.startswith('Segmenting image:'):
                    fn = '<- '+ln[18:].split('<-')[0].strip()
                elif ln.startswith('Writing metadata '):
                    fn = ln[18:].split('->')[1].strip()
                    self.outputs.append(fn)
                    fn = '-> '+fn
                elif ln.startswith('MatConvNet is not compiled,'):
                    fn = 'Compiling MatConvNet...'
                elif ln.startswith('MatConvNet compiled in:'):
                    fn = ln
                elif ln.startswith('Found ') and 'image(s) in directory:' in ln:
                    fn = ln
                elif ln.startswith('Error:'):
                    fn = ln
                if not fn is None:
                    self.update_status(fn)
            time.sleep(0.5)
        if fi:
            fi.close()
    #        
    def genparam(self):
        paramfn = os.path.join(self.parent.homedir, 'segmparam.json')
        params = {
            'cwd': self.segm_dir,
            'logfile': self.logfile,
            'batchSize': self.parent.batchSize,
            'tilepad': self.parent.tilepad,
            'srcDir': self.parent.srcDir,
            'tgtDir': self.parent.tgtDir,
            'useGpu': self.parent.useGpu == 'Yes',
            'iscale': INTERNAL_SCALE_MAP.get(self.parent.internalScale, 1.),
            'arti_filter': ARTI_FILTER_MAP.get(self.parent.artiFilter, 0.),
            'cziOptions': self.parent.cziOptions,
            'mex': self.cfg.get('mex', {}),
        }
        with open(paramfn, 'w') as fo:
            json.dump(params, fo, indent=2)
        return paramfn
    #   
    def run(self):
        self.rc = -1
        self.running = True
        self.outputs = []
        if os.path.exists(self.logfile):
            os.remove(self.logfile)

        rc, msg = matconvnet_patch.patch(os.path.join(self.segm_dir, 'matconvnet', 'matlab', 'vl_compilenn.m'))
        if rc:
            print msg
            
        thread = threading.Thread(target=self.read_log)
        thread.setDaemon(True)
        thread.start()
        # cur_dir = os.getcwd()
        try:
            start_ts = datetime.datetime.now()
            # os.chdir(self.segm_dir)
            if self.use_standalone:
                cmd = [self.matlab_standalone, self.genparam()]
            else:
                arg = "try; segmfunc('%s'); catch ex; disp(strrep(ex.message,sprintf('\\n'),': ')); quit(1); end; quit(0);" % (self.genparam())
                cmd = [self.matlab]+self.matlab_opts+['-sd', self.segm_dir, '-logfile', self.logfile, '-r', arg]
            print cmd
            p = subprocess.Popen(cmd)
            self.parent.statusText.insert(END, 'Matlab started.\n')
            self.parent.statusText.see(END)
            self.rc = p.wait()
            elapsed = datetime.datetime.now() - start_ts
            self.parent.statusText.insert(END, 'Matlab exited(%d) in %s\n' % (self.rc, str(elapsed)))
            self.parent.statusText.see(END)
            
        except Exception, ex:
            self.parent.statusText.insert(END, 'Error executing Matlab: %s\n' % (ex,))
            self.parent.statusText.see(END)
        # os.chdir(cur_dir)
        self.running = False
        thread.join()
    #
    def update_status(self, txt):
        self.parent.update_status(txt)
    #

class EntryFloatAuto(Entry):
    def __init__(self, parent, **options):
        Entry.__init__(self, parent)
        self.options = options
        #
        self.auto = options.get('auto', 'Auto')
        self.auto_color = options.get('color', 'gray')
        self.fg_color = self['fg']
        self.is_auto = False
        self.has_focus = False
        self.bind("<FocusIn>", self.onFocusIn)
        self.bind("<FocusOut>", self.onFocusOut)
        self.put_auto()
    def put_auto(self):
        self.insert(0, self.auto)
        self['fg'] = self.auto_color
        self.is_auto = True
    def onFocusIn(self, evt):
        self.has_focus = True
        if self.is_auto:
            self.delete('0', 'end')
            self['fg'] = self.fg_color
            self.is_auto = False
    def onFocusOut(self, evt):
        self.has_focus = False
        try:
            v = float(self.get())
        except Exception:
            self.delete('0', 'end')
            self.put_auto()
    def setFloat(self, v):
        self.delete('0', 'end')
        if v is None:
            if not self.has_focus:
                self.put_auto()
            return
        self.insert(0, str(v))
        self['fg'] = self.fg_color
        self.is_auto = False
    def getFloat(self):
        try:
            v = float(self.get())
            return v
        except Exception:
            pass
        return None
    #
class ManualLimitsDialog(Toplevel):
    def __init__(self, parent, ftmgr, **options):
        Toplevel.__init__(self, master=parent)
        self.ftmgr = ftmgr
        #
        self.is_ok = False
        self.title("Cell Feature Limits")
        self.frame = Frame(self, padx=2)
        self.frame.grid(column=0, row=0, padx=15, pady=5)
        self.grid_columnconfigure(0, weight=1)
        #
        lbl = Label(self.frame, text="Cell Feature", padx=5)
        lbl.grid(column=0, row=0, padx=5, sticky=E)
        lbl = Label(self.frame, text="Min. Value", padx=5)
        lbl.grid(column=1, row=0, padx=5, sticky=W)
        lbl = Label(self.frame, text="Max. Value", padx=5)
        lbl.grid(column=2, row=0, padx=5, sticky=W)
        #
        self.ft_map = {}
        row = 1
        for ft in self.ftmgr.features:
            lbl = Label(self.frame, text=ft.label+':', padx=5)
            lbl.grid(column=0, row=row, padx=5, sticky=E)
            txt = EntryFloatAuto(self.frame, width=16)
            txt.grid(column=1, row=row, padx=5, sticky=W)
            self.ft_map[ft.minName] = txt
            txt.setFloat(ft.minValue)
            txt = EntryFloatAuto(self.frame, width=16)
            txt.grid(column=2, row=row, padx=5, sticky=W)
            self.ft_map[ft.maxName] = txt
            txt.setFloat(ft.maxValue)
            row += 1
        #
        self.btnframe = Frame(self)
        self.btnframe.grid(column=0, row=1, padx=15, pady=5, sticky=E)
        
        btnDflt = Button(self.btnframe, text="Load Defaults", padx=10, command=self.onloaddefaults)
        btnDflt.grid(column=0, row=0, padx=5, sticky=W)
        btnClear = Button(self.btnframe, text="Clear All", padx=10, command=self.onclearall)
        btnClear.grid(column=1, row=0, padx=5, sticky=W)
        
        lbl = Label(self.btnframe, text="", padx=5)
        lbl.grid(column=2, row=0, padx=25)
        
        btnSave = Button(self.btnframe, text="Save", padx=10, command=self.onsave)
        btnSave.grid(column=3, row=0, padx=5, sticky=E)
        btnCancel = Button(self.btnframe, text="Cancel", padx=10, command=self.oncancel)
        btnCancel.grid(column=4, row=0, padx=5, sticky=E)
        #
        self.focus_force()
        self.grab_set()
        self.wait_window()
    def onloaddefaults(self):
        data = self.ftmgr.loadDefaults()
        for ft in self.ftmgr.features:
            txt = self.ft_map[ft.minName]
            txt.setFloat(data.get(ft.minName))
            txt = self.ft_map[ft.maxName]
            txt.setFloat(data.get(ft.maxName))
    def onclearall(self):
        for txt in self.ft_map.itervalues():
            txt.setFloat(None)
    def onsave(self):
        for ft in self.ftmgr.features:
            txt = self.ft_map[ft.minName]
            ft.minValue = txt.getFloat()
            txt = self.ft_map[ft.maxName]
            ft.maxValue = txt.getFloat()
        self.is_ok = True
        self.destroy()
    def oncancel(self):
        self.destroy()
    #
class ManualLimitsFrame(Frame):
    def __init__(self, parent, ftmgr, **options):
        Frame.__init__(self, parent)
        self.ftmgr = ftmgr
        self.options = options
        #
        self.grid_columnconfigure(0, weight=0)
        self.grid_columnconfigure(1, weight=0)
        self.grid_columnconfigure(2, weight=1)
        #
        self.useVar = useVar = StringVar()
        
        self.useYes = Radiobutton(self, padx=5,
                    text="Yes", variable=useVar, value="Yes")
        self.useYes.grid(column=0, row=0, sticky=W)
        self.useNo = Radiobutton(self, padx=5,
                    text="No", variable=useVar, value="No")
        self.useNo.grid(column=1, row=0, sticky=W)

        self.limitsBtn = Button(self, text="Set Limits", padx=10, command=self.limitsBtnClicked)
        self.limitsBtn.grid(column=2, row=0, padx=5, pady=5, sticky=W+E)
        #
        self.useVar.trace("w", self.useVarChanged)
        useVar.set("Yes")
        #
    def limitsBtnClicked(self):
        dlg = ManualLimitsDialog(self, self.ftmgr)
        if dlg.is_ok and 'onsetlimits' in self.options:
            self.options['onsetlimits']()
    def useVarChanged(self, a, b, c):
        st = NORMAL if self.isUseLImits() else DISABLED
        self.limitsBtn.configure(state=st)
    def isUseLImits(self):
        return self.useVar.get() == 'Yes'
    def setUseLimits(self, st):
        self.useVar.set('Yes' if st else 'No')
    #
class ReshapeCtlMainWindow(object):
    PERSISTENT_PROPERTIES = ('srcDir', 'tgtDir', 'batchSize', 'tilepad', 'useGpu', 'cziOptions',
            'reshapeParams', 'useManualLimits', 'internalScale', 'artiFilter')
    DEFAULT_BATCH_SIZE = 20
    DEFAULT_TILEPAD = 100
    def __init__(self, homedir):
        self.homedir = homedir
        #
        self.statefile = os.path.join(self.homedir, 'state.json')
        self.configfile = os.path.join(self.homedir, 'config.json')
        self.configurator = ReshapeConfig()
        self.cfg = self._loadConfig()
        self._checkConfig()
        self.startdir = os.path.expanduser('~')
        self.matl = MatlabSegmentation(self)
        self.fiji = FijiProc(self)
        self.ftmgr = ReshapeCellFeatureManager()
        #
        self.window = window  = Tk()

#         screen_width = window.winfo_screenwidth()
#         screen_height = window.winfo_screenheight()
#         geom = '%2.0fx%2.0f' % (screen_width*0.55, screen_height*0.5)
#         window.geometry(geom)
        
        window.title(APP_NAME+' version '+APP_VERSION)
        
        self.frame = frame = Frame(window, relief=RAISED, borderwidth=1)
        frame.pack(fill=BOTH, expand=True)
        
# GroupBox - input/output folders
        self.lfdir = lfdir = LabelFrame(frame, text="Directories")
        lfdir.grid(column=0, row=0, columnspan=3)
        
        srcDirLbl = Label(lfdir, text="Input directory:", padx=10)
        srcDirLbl.grid(column=0, row=0, sticky=E)
        
        self.srcDirTxt = srcDirTxt = Entry(lfdir, width=72)
        srcDirTxt.grid(column=1, row=0)
        
        def srcDirBtnClicked():
            srcDir = tkFileDialog.askdirectory(initialdir = self.srcDir,
                    title="Select directory containing input files (.tif)",
                    mustexist=True)
            if not srcDir: return
            self.srcDir = srcDir
            self.tgtDir = os.path.join(srcDir, 'Processed').replace('\\', '/')
            self.saveState()
        
        srcDirBtn = Button(lfdir, text="...", padx=5, command=srcDirBtnClicked)
        srcDirBtn.grid(column=2, row=0, padx=5, pady=5)
        
        tgtDirLbl = Label(lfdir, text="Output directory:", padx=10)
        tgtDirLbl.grid(column=0, row=1, sticky=E)
        
        self.tgtDirTxt = tgtDirTxt = Entry(lfdir, width=72)
        tgtDirTxt.grid(column=1, row=1)
        
        def tgtDirBtnClicked():
            tgtDir = tkFileDialog.askdirectory(initialdir = self.tgtDir,
                    title="Select directory for output files")
            if not tgtDir: return
            self.tgtDir = tgtDir
            self.saveState()
        
        tgtDirBtn = Button(lfdir, text="...", padx=5, command=tgtDirBtnClicked)
        tgtDirBtn.grid(column=2, row=1, padx=5, pady=5)
        
# GroupBox - NN options
        self.lfnn = lfnn = LabelFrame(frame, text="NN Segmentation Options", padx=5, pady=10)
        lfnn.grid(column=0, row=1, sticky=W, padx=5)
        
        batchSzLbl = Label(lfnn, text="Batch Size:", padx=5)
        batchSzLbl.grid(column=0, row=0, sticky=E)
        self.batchSzTxt = batchSzTxt = Entry(lfnn, width=12)
        batchSzTxt.grid(column=1, row=0, columnspan=2, sticky=W, padx=5)
        batchSzTxt.bind('<FocusOut>', lambda x: self.saveState())

        gpuLbl = Label(lfnn, text="Use GPU?", padx=2)
        gpuLbl.grid(column=0, row=1, sticky=E, padx=5)

        self.gpuVar = gpuVar = StringVar()
        gpuVar.set("Yes")
        
        self.gpuYes = gpuYes = Radiobutton(lfnn, padx=2,
                    text="Yes", variable=gpuVar, value="Yes")
        gpuYes.grid(column=1, row=1, sticky=N+W)
        self.gpuNo = gpuNo = Radiobutton(lfnn, padx=2,
                    text="No", variable=gpuVar, value="No")
        gpuNo.grid(column=2, row=1, sticky=N+W)
        
        iscaleLbl = Label(lfnn, text="Internal Scale:", padx=5)
        iscaleLbl.grid(column=0, row=2, sticky=E)
        self.iscaleVar = StringVar()
        self.iscaleVar.set('None')
        self.iscaleMenu = iscaleMenu = OptionMenu(lfnn, self.iscaleVar, *[n for n,_ in INTERNAL_SCALES])
        iscaleMenu.config(width=8)
        iscaleMenu.grid(column=1, row=2, columnspan=2, sticky=W)
        
        artiLbl = Label(lfnn, text="Arti Filter:")
        artiLbl.grid(column=0, row=3, sticky=E, padx=5)
        self.artiVar = StringVar()
        self.artiVar.set('None')
        self.artiMenu = artiMenu = OptionMenu(lfnn, self.artiVar, *[n for n,_ in ARTI_FILTERS])
        artiMenu.config(width=10)
        artiMenu.grid(column=1, row=3, columnspan=2, sticky=W)
      
# GroupBox - Tiled Image options
        self.lftiled = lftiled = LabelFrame(frame, text="Tiled Image Options", padx=5, pady=10)
        lftiled.grid(column=1, row=1, sticky=N+W+E, padx=0)
        padLbl = Label(lftiled, text="Pad Tiles By:", padx=5)
        padLbl.grid(column=0, row=0, sticky=E)
        self.padTxt = padTxt = Entry(lftiled, width=12)
        padTxt.grid(column=1, row=0, sticky=W)
        padTxt.bind('<FocusOut>', lambda x: self.saveState())
        
# GroupBox - CZI options
        self.lfczi = lfczi = LabelFrame(frame, text="CZI options", padx=0, pady=2)
        lfczi.grid(column=2, row=1, sticky=N+W+E, padx=5)
        
        self.saveSrcVar = saveSrcVar = StringVar()
        saveSrcVar.set("No")
        self.saveSrcChk = Checkbutton(lfczi, text="Save TIFF",
                variable=saveSrcVar, onvalue="Yes", offvalue="No")
        self.saveSrcChk.grid(column=0, row=0, sticky=W, padx=5)
        
        lbl = Label(lfczi, text="Suffix")
        lbl.grid(column=1, row=0, padx=5)
        lbl = Label(lfczi, text="Main")
        lbl.grid(column=2, row=0)
        
        self.czioptctl = []
        self.mainVar = mainVar = IntVar()
        mainVar.set(1)
        for i in range(4):
            row = i + 1
            var = BooleanVar()
            check = Checkbutton(lfczi, text="Plane "+str(i+1), command=self.onChannelCheck, var=var)
            check.grid(column=0, row=row, sticky=E, padx=5)
            lab = Entry(lfczi, width=12)
            lab.grid(column=1, row=row, sticky=W, padx=5)
            radio = Radiobutton(lfczi, padx=2, variable=mainVar, value=i+1)
            radio.grid(column=2, row=row, sticky=W, padx=5)
            self.czioptctl.append((check, lab, radio, var))
        
# GroupBox - Output Graphics Options
        self.lfgo = lfgo = LabelFrame(frame, text="Output Graphics Options", pady=5)
        lfgo.grid(column=3, row=0, rowspan=2)
        
        reconstrLabel = Label(lfgo, text="Reconstruct Tiled Images?", padx=10)
        reconstrLabel.grid(column=0, row=0, sticky=E)
        reconstrPane = Frame(lfgo)
        reconstrPane.grid(column=1, row=0, sticky=W)
        
        self.reconstrVar = reconstrVar = StringVar()
        reconstrVar.set("Yes")
        
        self.reconstrYes = reconstrYes = Radiobutton(reconstrPane, padx=5,
                    text="Yes", variable=reconstrVar, value="Yes")
        reconstrYes.grid(column=0, row=0, sticky=W)
        self.reconstrNo = reconstrNo = Radiobutton(reconstrPane, padx=5,
                    text="No", variable=reconstrVar, value="No")
        reconstrNo.grid(column=1, row=0, sticky=W)
        
        graphChoiceLbl = Label(lfgo, text="Create Colored Images:", padx=10)
        graphChoiceLbl.grid(column=0, row=1, sticky=E)
        
        self.graphChoiceVar = graphChoiceVar = StringVar()
        graphChoiceVar.set("All")
        # "Just Neighbors", "All But Neighbors", 
        self.outImgMenu = outImgMenu = OptionMenu(lfgo, graphChoiceVar,
                    "All", "None")
        outImgMenu.grid(column=1, row=1, sticky=EW)
        
        coloringLbl = Label(lfgo, text="Coloring:", padx=10)
        coloringLbl.grid(column=0, row=2, sticky=E)
        
        self.coloringVar = coloringVar = StringVar()
        coloringVar.set("Thermal")
        self.coloringMenu = coloringMenu = OptionMenu(lfgo, coloringVar,
                    "Thermal", "Green", "mpl-magma","phase", "Fire", "Jet", "Cyan Hot")
        coloringMenu.grid(column=1, row=2, sticky=EW)
        
        imgFormatLbl = Label(lfgo, text="Image Format:", padx=10)
        imgFormatLbl.grid(column=0, row=3, sticky=E)
        
        self.imgFormatVar = imgFormatVar = StringVar()
#        imgFormatVar.set("Tiff Stack+PNG Montage")
        imgFormatVar.set("Tiff Separate Images")
        self.imgFormatMenu = imgFormatMenu = OptionMenu(lfgo, imgFormatVar,
                    "PNG Separate Images", "Tiff Separate Images")
                    # "Both Tiff", "Tiff Stack+PNG Montage", "Tiff Stack","Tiff Montage",
                    # "PNG Montage", "Tiff Separate Images", "Tiff Stack")
        imgFormatMenu.config(width=24)
        imgFormatMenu.grid(column=1, row=3, sticky=EW)
        
        limLab = Label(lfgo, text="Use Manual Limits?", padx=10)
        limLab.grid(column=0, row=4, sticky=E)
        self.limBox = ManualLimitsFrame(lfgo, self.ftmgr, onsetlimits=self.saveState)
        self.limBox.grid(column=1, row=4, sticky=W)
        
        #goNoteLbl1 = Label(lfgo, text="*Note: Image generation takes the majority of the processing time;")
        #goNoteLbl1.grid(column=0, row=4, columnspan=2, sticky=W)
        #goNoteLbl2 = Label(lfgo, text="If Images are large Tif Stacks, it may fail.")
        #goNoteLbl2.grid(column=0, row=5, columnspan=2, sticky=W)

# GroupBox - Cell Size Restrictions Options
        self.lfcellsz = lfcellsz = LabelFrame(frame, text="Cell Size Restrictions for Analysis", pady=5)
        lfcellsz.grid(column=3, row=2, sticky=EW)

        poreLowerLbl = Label(lfcellsz, text="Lower Cell Size:", padx=10)
        poreLowerLbl.grid(column=0, row=0, sticky=E)
        
        self.poreLowerTxt = poreLowerTxt = Entry(lfcellsz, width=12)
        poreLowerTxt.grid(column=1, row=0)
        
        self.poreLowerUnits = Label(lfcellsz, text="Pixels")
        self.poreLowerUnits.grid(column=2, row=0, sticky=W, padx=5)
        
        poreUpperLbl = Label(lfcellsz, text="Upper Cell Size:", padx=10)
        poreUpperLbl.grid(column=0, row=1, sticky=E)
        
        self.poreUpperUnits = Label(lfcellsz, text="Pixels")
        self.poreUpperUnits.grid(column=2, row=1, sticky=W, padx=5)
        
        self.poreUpperTxt = poreUpperTxt = Entry(lfcellsz, width=12)
        poreUpperTxt.grid(column=1, row=1)

# GroupBox - Automated Unit Conversion
        self.lfunitcvt = lfunitcvt = LabelFrame(frame, text="Automated Unit Conversion", pady=5)
        lfunitcvt.grid(column=3, row=3, sticky=EW)
        
        unitCvtLbl = Label(lfunitcvt, text="Convert pixels to real units?", padx=10)
        unitCvtLbl.grid(column=0, row=0, sticky=E)

        self.unitCvtVar = unitCvtVar = StringVar()
        unitCvtVar.set("Yes")
        
        self.unitCvtYes = unitCvtYes = Radiobutton(lfunitcvt, padx=5,
                    text="Yes", variable=unitCvtVar, value="Yes")
        unitCvtYes.grid(column=1, row=0, sticky=W)
        self.unitCvtNo = unitCvtNo = Radiobutton(lfunitcvt, padx=5,
                    text="No", variable=unitCvtVar, value="No")
        unitCvtNo.grid(column=2, row=0, sticky=W)
      
        unitPixLbl = Label(lfunitcvt, text="Length of scale bar:", padx=10)
        unitPixLbl.grid(column=0, row=1, sticky=E)
        
        self.unitPixTxt = unitPixTxt = Entry(lfunitcvt, width=12)
        unitPixTxt.grid(column=1, row=1)

        Label(lfunitcvt, text="Pixels", padx=5).grid(column=2, row=1, sticky=W)

        unitMicronLbl = Label(lfunitcvt, text="Length of scale bar:", padx=10)
        unitMicronLbl.grid(column=0, row=2, sticky=E)
        
        self.unitMicronTxt = unitMicronTxt = Entry(lfunitcvt, width=12)
        unitMicronTxt.grid(column=1, row=2)

        Label(lfunitcvt, text="Microns", padx=5).grid(column=2, row=2, sticky=W)

# GroupBox - Progress
        self.lfprogr = lfprogr = LabelFrame(frame, text="Progress")
        lfprogr.grid(column=0, row=2, columnspan=3, rowspan=2, padx=5)

        self.statusText = statusText = ScrolledText(lfprogr, height=12, width=72)
        statusText.grid(column=0, row=0)
        
        def closeButtonClicked():
            self.saveState()
            self.window.quit()
        
        self.closeButton = closeButton = Button(window, text="Close", command=closeButtonClicked)
        closeButton.pack(side=RIGHT, padx=5, pady=5, ipadx=15)
        
        def matlButtonClicked():
            if self.validate_input():
                self.saveState()
                thread = threading.Thread(target=self.go_for_it)
                thread.setDaemon(True)
                thread.start()
        
        self.matlabButton = matlabButton = Button(window, text="Go For It", command=matlButtonClicked)
        matlabButton.pack(side=RIGHT, padx=5, pady=5, ipadx=15)
        #
        self.shutBoxVar = IntVar()
        self.shutBoxVar.set(0)
        self.shutBox = Checkbutton(window, text="Shutdown the system when finished",
                variable=self.shutBoxVar, onvalue=1, offvalue=0)
        self.shutBox.pack(side=RIGHT, padx=5, pady=5, ipadx=15)
        #
        self.batchSize = self.DEFAULT_BATCH_SIZE
        self.tilepad = self.DEFAULT_TILEPAD
        self.useGpu = 'Yes'
        self.srcDir = self.startdir
        self.tgtDir = self.startdir
        
        self.reconstrVar.trace("w", self.reconstrVarChanged)
        self.unitCvtVar.trace("w", self.unitCvtVarChanged)
        self.saveSrcVar.trace("w", self.saveSrcVarChanged)
        self.mainVar.trace("w", self.mainVarChanged)
        self.shutBoxVar.trace("w", self.shutBoxVarChanged)
        self.cziOptionsChanged()
        self.loadState()
    #
    def onChannelCheck(self):
        if self.saveSrcVar.get() == 'No':
            return
        imain = self.mainVar.get() - 1
        for i, (c, l, r, v) in enumerate(self.czioptctl):
            if i==imain:
                v.set(True)
            st = NORMAL if v.get() else DISABLED
            l.configure(state=st)
            r.configure(state=st)
            st = DISABLED if i==imain else NORMAL
            c.configure(state=st)
    def cziOptionsChanged(self):
        if self.saveSrcVar.get() == 'No':
            for ctls in self.czioptctl:
                for c in ctls[0:3]:
                    c.configure(state=DISABLED)
        else:
            self.onChannelCheck()
    def mainVarChanged(self, a, b, c):
        self.onChannelCheck()
    def saveSrcVarChanged(self, a, b, c):
        self.cziOptionsChanged()
    #
    def shutBoxVarChanged(self, a, b, c):
        shutStatus = 'ON' if self.shutBoxVar.get() != 0 else 'OFF'
        print 'Automatic system shutdown is', shutStatus
    #
    def reconstrVarChanged(self, a, b, c):
        st = DISABLED if self.reconstrVar.get() == 'No' else NORMAL
        self.outImgMenu.configure(state=st)
        self.coloringMenu.configure(state=st)
        self.imgFormatMenu.configure(state=st)
    #
    def unitCvtVarChanged(self, a, b, c):
        if self.unitCvtVar.get() == 'Yes':
            self.unitPixTxt.config(state='normal')
            self.unitMicronTxt.config(state='normal')
            self.poreLowerUnits.config(text='Sq.Microns')
            self.poreUpperUnits.config(text='Sq.Microns')
            pore_lower, pore_upper = self._get_cell_limit_pixels('No')
        else:
            self.unitPixTxt.config(state='disabled')
            self.unitMicronTxt.config(state='disabled')
            self.poreLowerUnits.config(text='Pixels')
            self.poreUpperUnits.config(text='Pixels')
            pore_lower, pore_upper = self._get_cell_limit_pixels('Yes')
        self._set_cell_limit_pixels(pore_lower, pore_upper)
    #
    def create_tgt_dir(self, tgtDir):
        try:
            os.mkdir(tgtDir)
            self.tgtDir = tgtDir.replace('\\', '/')
            return True
        except Exception:
            tkMessageBox.showerror(APP_NAME, 'Error creating directory\n"%s"' % (tgtDir,))
            return False
        
    def validate_input(self):
        srcDir = os.path.abspath(self.srcDir)
        tgtDir = os.path.abspath(self.tgtDir)
        if srcDir == tgtDir:
            result = tkMessageBox.askyesno(APP_NAME,
                'Input and Output directories must be different.\n'+
                'Would you like to change the Output directory to "./Processed"?')
            if not result:
                return False
            tgtDir = os.path.join(tgtDir, 'Processed')
            if not os.path.isdir(tgtDir):
                return self.create_tgt_dir(tgtDir)
            else:
                self.tgtDir = tgtDir.replace('\\', '/')
        if not os.path.exists(tgtDir):
            result = tkMessageBox.askyesno(APP_NAME,
                'The Output directory does not exist.\n'+
                'Do you want to create it?')
            if not result:
                return False
            return self.create_tgt_dir(tgtDir)
        if len(os.listdir(tgtDir)) > 0:
            result = tkMessageBox.askyesno(APP_NAME,
                'The Output directory already contains some data.\n'+
                'Writing results may override some existing data.\n'+
                'Continue anyway?')
            return result
        return True
    
    def generate_limits_file(self):
        self.limitspath = None
        if self.limBox.isUseLImits():
            try:
                self.limitspath = os.path.join(self.homedir, 'feature_limits.json')
                with open(self.limitspath, 'w') as fo:
                    json.dump(self.ftmgr.store(), fo)
            except Exception:
                self.limitspath = None
        return self.limitspath
        
    def go_for_it(self):
        start_ts = datetime.datetime.now()
        self.matlabButton.configure(state=DISABLED)
        self.fiji.limitspath = self.generate_limits_file()
        self.matl.run()
        rc = self.matl.rc
        if 0 == rc:
            print self.matl.outputs
            self.fiji.set_tiled_images(self.matl.outputs)
            self.fiji.run()
            rc = self.fiji.rc
        self.matlabButton.configure(state=NORMAL)
        elapsed = datetime.datetime.now() - start_ts
        self.statusText.insert(END, '\nDONE in '+str(elapsed)+'.\n')
        self.statusText.see(END)
        print ''
        print 'Done (%d).' % (rc,)
        if self.shutBoxVar.get() != 0:
            self.autoShutDown()
        return rc
    
    def autoShutDown(self):
        print('Trying to shut down the system...')
        try:
            if sys.platform == 'win32':
                os.system('shutdown /s /f')
            else:
                os.system('sudo shutdown now')
        except Exception, ex:
            print('Failed to shutdown the system, reason:', str(ex))
    
    @property
    def srcDir(self):
        return self.srcDirTxt.get()
    @srcDir.setter
    def srcDir(self, text):
        self.srcDirTxt.delete(0, END)
        self.srcDirTxt.insert(0, text)
    @property
    def tgtDir(self):
        return self.tgtDirTxt.get()
    @tgtDir.setter
    def tgtDir(self, text):
        self.tgtDirTxt.delete(0, END)
        self.tgtDirTxt.insert(0, text)
    @property
    def batchSize(self):
        return self.getEntryAsInt(self.batchSzTxt, self.DEFAULT_BATCH_SIZE)
    @batchSize.setter
    def batchSize(self, batchSz):
        self.setEntryAsInt(self.batchSzTxt, batchSz)
    @property
    def tilepad(self):
        return self.getEntryAsInt(self.padTxt, self.DEFAULT_TILEPAD)
    @tilepad.setter
    def tilepad(self, pad):
        self.setEntryAsInt(self.padTxt, pad)
    @property
    def useGpu(self):
        return self.gpuVar.get()
    @useGpu.setter
    def useGpu(self, v):
        self.gpuVar.set(v)
    @property
    def internalScale(self):
        return self.iscaleVar.get()
    @internalScale.setter
    def internalScale(self, v):
        if not v in INTERNAL_SCALE_MAP:
            v = 'None'
        self.iscaleVar.set(v)
    @property
    def artiFilter(self):
        return self.artiVar.get()
    @artiFilter.setter
    def artiFilter(self, v):
        if not v in ARTI_FILTER_MAP:
            v = 'None'
        self.artiVar.set(v)
    @property
    def useManualLimits(self):
        return self.limBox.isUseLImits()
    @useManualLimits.setter
    def useManualLimits(self, st):
        self.limBox.setUseLimits(st)
    @property
    def cziOptions(self):
        save_source = self.saveSrcVar.get() == 'Yes'
        source_rel = 'Source'
        source_dir = os.path.abspath(os.path.join(self.tgtDir, source_rel))
        planes = []
        mainpl = self.mainVar.get()
        imain = mainpl - 1
        suff = self.czioptctl[imain][1].get()
        planes.append({'plane': mainpl, 'suff': suff})
        for i, (c, l, r, v) in enumerate(self.czioptctl):
            if i==imain or not v.get(): continue
            suff = l.get().strip()
            if not suff:
                suff = 'Plane%d' % (i+1,)
            planes.append({'plane': i+1, 'suff': l.get()})
        return {
            'save_source': save_source,
            'source_rel': source_rel,
            'source_dir': source_dir,
            'planes': planes,
        }
    @cziOptions.setter
    def cziOptions(self, opts):
        try:
            for c, l, r, v in self.czioptctl:
                v.set(False)
            for i, pobj in enumerate(opts['planes']):
                plane = pobj['plane']
                suff = str(pobj['suff'])
                c, l, r, v = self.czioptctl[plane - 1]
                l.configure(state=NORMAL)
                l.delete(0, END)
                l.insert(0, suff)
                v.set(True)
                if i == 0:
                    self.mainVar.set(plane)
            self.saveSrcVar.set('Yes' if opts['save_source'] else 'No')
        except Exception, ex:
            print ex
            self.saveSrcVar.set('No')
        self.cziOptionsChanged()    
    #
        
    @staticmethod
    def setEntryAsInt(ent, val):
        ent.delete(0, END)
        ent.insert(0, str(val))
    @staticmethod
    def getEntryAsInt(ent, dflt=None):
        try:
            val = int(ent.get())
            return val
        except Exception:
            if not dflt is None:
                ReshapeCtlMainWindow.setEntryAsInt(ent, val)
            pass
        return dflt
        
    @staticmethod
    def setEntryAsFloat(ent, val):
        ent.delete(0, END)
        ent.insert(0, str(val))
    @staticmethod
    def getEntryAsFloat(ent, dflt=None):
        try:
            val = float(ent.get())
            return val
        except Exception:
            if not dflt is None:
                ReshapeCtlMainWindow.setEntryAsInt(ent, val)
            pass
        return dflt
    
    def _get_cell_limit_pixels(self, cvt=None):
        if cvt is None:
            cvt = self.unitCvtVar.get()
        try:
            if cvt == 'Yes':
                um_to_pix = self.getEntryAsFloat(self.unitPixTxt, 1.) / self.getEntryAsFloat(self.unitMicronTxt, 1.)
                um_to_pix *= um_to_pix
                pore_lower = int(self.getEntryAsFloat(self.poreLowerTxt, 5.) * um_to_pix + 0.5)
                pore_upper = int(self.getEntryAsFloat(self.poreUpperTxt, 1300.) * um_to_pix + 0.5)
            else:
                pore_lower = self.getEntryAsInt(self.poreLowerTxt, 50)
                pore_upper = self.getEntryAsInt(self.poreUpperTxt, 12000)
        except Exception:
            pore_lower = 50
            pore_upper = 12000
        return pore_lower, pore_upper
    #
    def _set_cell_limit_pixels(self, pore_lower, pore_upper, cvt=None):
        if cvt is None:
            cvt = self.unitCvtVar.get()
        if cvt == 'Yes':
            self.poreLowerTxt.delete(0, END)
            self.poreUpperTxt.delete(0, END)
            try:
                um_to_pix = self.getEntryAsFloat(self.unitPixTxt, 1.) / self.getEntryAsFloat(self.unitMicronTxt, 1.)
                um_to_pix *= um_to_pix
                self.poreLowerTxt.insert(0, '%2.2f' % (pore_lower / um_to_pix,))
                self.poreUpperTxt.insert(0, '%2.2f' % (pore_upper / um_to_pix,))
            except Exception:
                self.setEntryAsInt(self.poreLowerTxt, 5)
                self.setEntryAsInt(self.poreUpperTxt, 1300)
        else:
            self.setEntryAsInt(self.poreLowerTxt, pore_lower)
            self.setEntryAsInt(self.poreUpperTxt, pore_upper)
    #
    @property
    def reshapeParams(self):
        pore_lower, pore_upper = self._get_cell_limit_pixels()
        return {
            'srcDir': self.tgtDir.replace('/', os.path.sep),
            'graphic_choice': self.graphChoiceVar.get(),
            'LUT_choice': self.coloringVar.get(),
            'graphic_format': self.imgFormatVar.get(),
            'unit_conv': self.unitCvtVar.get(),
            'pore_lower': pore_lower,
            'pore_upper': pore_upper,
            'unit_conv': self.unitCvtVar.get(),
            'unit_pix': self.getEntryAsFloat(self.unitPixTxt, 1.),
            'unit_real': self.getEntryAsFloat(self.unitMicronTxt, 1.),
            'reconstruct_tiled': self.reconstrVar.get(),
        }
    @reshapeParams.setter
    def reshapeParams(self, par):
        if par is None:
            par = {
                'pore_lower': 50,
                'pore_upper': 12000,
                'unit_pix': 1.,
                'unit_real': 1.,
            }
        if 'graphic_choice' in par:
            self.graphChoiceVar.set(par['graphic_choice'])
        if 'LUT_choice' in par:
            self.coloringVar.set(par['LUT_choice'])
        if 'graphic_format' in par:
            self.imgFormatVar.set(par['graphic_format'])
        if 'unit_conv' in par:
            self.unitCvtVar.set(par['unit_conv'])
        if 'reconstruct_tiled' in par:
            self.reconstrVar.set(par['reconstruct_tiled'])
        self.setEntryAsFloat(self.unitPixTxt, par.get('unit_pix'))
        self.setEntryAsFloat(self.unitMicronTxt, par.get('unit_real'))
        self._set_cell_limit_pixels(par.get('pore_lower'), par.get('pore_upper'))
    #
    def _check_matlab(self):
        if self.cfg.get('SEGMENTATION') and self.cfg.get('USE_STANDALONE'):
            if os.path.exists(self.cfg['SEGMENTATION']):
                return True
        if self.cfg.get('matlab'):
            return os.path.exists(self.cfg['matlab'])
        return False
    def _check_fiji(self):
        if self.cfg.get('fiji') and os.path.exists(self.cfg['fiji']):
            return True
        return False   
    def _checkConfig(self):
        if not self._check_matlab() or not self._check_fiji():
            print 'self.configurator.autoconfig()'
            self.cfg = self.configurator.autoconfig()
            self._saveConfig(self.cfg)
        self.configurator.update_env(self.cfg.get('environ', {}))
    def _loadConfig(self):
        try:
            with open(self.configfile, 'r') as fi:
                cfg = json.load(fi)
        except Exception:
            cfg = {}
        return cfg
    def _saveConfig(self, cfg):
        try:
            with open(self.configfile, 'w') as fo:
                json.dump(cfg, fo, indent=2)
        except Exception:
            pass
    def loadState(self):
        self.reshapeParams = None
        try:
            with open(self.statefile, 'r') as fi:
                params = json.load(fi)
            for prop in self.PERSISTENT_PROPERTIES:
                if not prop in params: continue
                setattr(self, prop, params[prop])
            if 'feature_limits' in params:
                self.ftmgr.load(params['feature_limits'])
            if 'porelwrtxt' in params:
                self.setEntryAsFloat(self.poreLowerTxt, params['porelwrtxt'])
            if 'poreuprtxt' in params:
                self.setEntryAsFloat(self.poreUpperTxt, params['poreuprtxt'])
        except Exception:
            pass
    def saveState(self):
        xtra = {
            'porelwrtxt': self.poreLowerTxt.get(),
            'poreuprtxt': self.poreUpperTxt.get(),
        }
        try:
            params = dict((prop, getattr(self, prop)) for prop in self.PERSISTENT_PROPERTIES)
            params['feature_limits'] = self.ftmgr.store()
            params.update(xtra)
            with open(self.statefile, 'w') as fo:
                json.dump(params, fo, indent=2)
        except Exception:
            pass
    def run(self):
        self.window.mainloop()
    def update_status(self, txt):
        self.statusText.insert(END, txt+'\n')
        self.statusText.see(END)

if __name__ == '__main__':
    
    myhome = os.path.join(os.path.expanduser('~'), '.reshape')
    if not os.path.isdir(myhome):
        try:
            os.mkdir(myhome)
        except Exception:
            myhome = '.'
    
    proc = ReshapeCtlMainWindow(myhome)
    proc.run()

    sys.exit(0)
