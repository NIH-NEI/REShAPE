
import os, datetime, csv, json
import shutil
import math
import re
from .listentry import ListFileEntry, CELL_ATTR_MAP
from .cellfeature import CELL_FEATURE_REMAP

__Rbad__ = re.compile('[^a-zA-Z0-9_\.]|^(?=\d|\.)')
def sanitizeR(v):
    if v is None:
        return v
    return re.sub(__Rbad__, '_', v)

def dblquot(s):
    return json.dumps(str(s))

class GroupHistoryEntry(object):
    def __init__(self, grname, ts, plotstyle, settings, histdir):
        self.grname = grname
        self.ts = ts
        self.sts = self.ts.strftime('%Y-%m-%d %H:%M:%S')
        self.plotstyle = plotstyle
        self.settings = settings
        self.histdir = histdir
        #
        self.plots = {}
        if 'plots' in self.settings:
            self.plots = self.settings['plots']
        self.features = []
        self.filemap = {}
        for attr in self.settings['features']:
            fn = self.plots.get(attr)
            if not fn:
                fn = self.plots.get(sanitizeR(attr))
                if not fn: continue
            fpath = os.path.join(self.histdir, fn)
            if not os.path.exists(fpath): continue
            feature = CELL_ATTR_MAP.get(attr, attr)
            self.features.append(feature)
            self.filemap[feature] = fpath
    #
    def __unicode__(self):
        return self.sts + ' (' + self.plotstyle + ')'
    def __str__(self):
        return self.__unicode__()
    def __repr__(self):
        return self.__unicode__()

class FileGroupManager(object):
    RETENTION = datetime.timedelta(days=14)
    SETTINGS_FILE = 'settings.json'
    def __init__(self, homedir):
        self.homedir = homedir
        #
        self.errmsg = None
        #
        self.groupdir = os.path.join(self.homedir, 'filegroups')
        if not os.path.exists(self.groupdir):
            try:
                os.makedirs(self.groupdir)
            except Exception:
                self.groupdir = self.homedir
        self.historydir = os.path.join(self.homedir, 'History')
        if not os.path.exists(self.historydir):
            try:
                os.makedirs(self.historydir)
            except Exception:
                self.historydir = self.homedir
        #
    def listGroups(self):
        res = []
        for fn in os.listdir(self.groupdir):
            if fn.startswith('group__') and fn.endswith('.json'):
                fullp = os.path.join(self.groupdir, fn)
                res.append(ListFileEntry(fullp, fn[7:-5]))
        return res
    def purgeHistory(self):
        tocheck = [self.historydir]
        retain = set()
        todo = []
        tsnow = datetime.datetime.now()
        for fe in self.listGroups():
            grname = fe.basename
            ghdir = self._grHist(grname)
            if not os.path.isdir(ghdir):
                continue
            retain.add(ghdir)
            tocheck.append(ghdir)
            for h in self.loadHistory(grname):
                howold = tsnow - h.ts
                if howold <= self.RETENTION:
                    retain.add(h.histdir)
        for cdir in tocheck:
            for fn in os.listdir(cdir):
                fpath = os.path.join(cdir, fn)
                if not os.path.isdir(fpath):
                    continue
                if not fpath in retain:
                    todo.append(fpath)
        for fpath in todo:
            try:
                shutil.rmtree(fpath, True)
            except Exception:
                pass   
    #        
    def _grFullp(self, grname):
        return os.path.join(self.groupdir, 'group__%s.json' % (grname,))
    def _grHist(self, grname):
        return os.path.join(self.historydir, sanitizeR(grname))
    def groupExists(self, grname):
        fullp = self._grFullp(grname)
        return os.path.exists(fullp)
    def deleteGroup(self, grname):
        self.deleteHistory(grname)
        fullp = self._grFullp(grname)
        try:
            os.remove(fullp)
            return True
        except Exception:
            return False
    def loadGroup(self, grname):
        fullp = self._grFullp(grname)
        try:
            with open(fullp, 'r') as fi:
                res = json.load(fi)
            _filelist = res['filelist']
            filelist = []
            for fentry in _filelist:
                if isinstance(fentry, (str,unicode)):
                    fentry = {'name':fentry, 'attrib': {}}
                filelist.append(fentry)
            res['filelist'] = filelist
        except Exception:
            res = {}
        return res
    def saveGroup(self, grname, data, delhistory=True):
        self.errmsg = None
        if delhistory:
            self.deleteHistory(grname)

        fullp = self._grFullp(grname)
        try:
            histDir = self._mkhistory(grname, data)
            if not histDir is None:
                with open(fullp, 'w') as fo:
                    json.dump(data, fo)
        except Exception, ex:
            self.errmsg = str(ex)
        return self.errmsg
    def cacheGroupHistoryEntry(self, grname, data, plot_type):
        csv_path = self.cacheGroupCsv(grname, data)
        ts = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
        hist_dir = os.path.join(data['history'], '%s-%s' % (ts, plot_type))
        self.errmsg = None
        try:
            if not os.path.isdir(hist_dir):
                os.mkdir(hist_dir)
            return hist_dir
        except Exception, ex:
            self.errmsg = str(ex)
        return None
    def _mkhistory(self, grname, data):    
        if 'history' in data:
            histDir = data['history']
        else:
            histDir = self._grHist(grname)
            data['history'] = histDir
        try:
            if not os.path.isdir(histDir):
                os.mkdir(histDir)
        except Exception, ex:
            self.errmsg = str(ex)
            return None
        return histDir
    def cacheGroupCsv(self, grname, data):
        histDir = self._mkhistory(grname, data)
        if histDir is None:
            return None
        self.errmsg = None
        csv_path = None
        try:
            _grname = sanitizeR(grname)
            csv_path = os.path.join(histDir, _grname+'.csv')
            if os.path.exists(csv_path):
                return csv_path
            rdr = FileGroupReader(data)
            with open(csv_path, 'wb') as fo:
                wr = csv.writer(fo, quoting=csv.QUOTE_MINIMAL)
                wr.writerow(rdr.headers(False))
                for row in rdr.iterrows():
                    wr.writerow(row)
            data['csv_path'] = csv_path
            self.saveGroup(grname, data, delhistory=False)
        except Exception, ex:
            self.errmsg = str(ex)
            csv_path = None
        return csv_path
    def deleteHistory(self, grname):
        try:
            shutil.rmtree(self._grHist(grname), True)
        except Exception:
            pass
    def loadHistory(self, grname):
        hist = []
        histDir = self._grHist(grname)
        if not os.path.isdir(histDir):
            return hist
        for fn in os.listdir(histDir):
            hpath = os.path.join(histDir, fn)
            if not os.path.isdir(hpath):
                continue
            sfile = os.path.join(hpath, self.SETTINGS_FILE)
            try:
                sts, pstyle = fn.split('-')
                ts = datetime.datetime.strptime(sts, '%Y%m%d%H%M%S')
                with open(sfile, 'r') as fi:
                    settings = json.load(fi)
            except Exception:
                continue
            hist.append(GroupHistoryEntry(grname, ts, pstyle, settings, hpath))
        hist.sort(key=lambda x: x.ts, reverse=True)
        return hist
    #
    @staticmethod
    def _readCsvFileAttr(filename, attr, res):
        headers = None
        idx = -1
        with open(filename, 'rb') as fi:
            rd = csv.reader(fi)
            for row in rd:
                if headers is None:
                    headers = row
                    idx = headers.index(attr)
                    continue
                try:
                    v = float(row[idx])
                    if not math.isnan(v):
                        res.append(v)
                except Exception:
                    pass
    #
    def readFeatureData(self, grname, attr):
        res = []
        gr = self.loadGroup(grname)
        filelist = gr.get('filelist', [])
        for fentry in filelist:
            self._readCsvFileAttr(fentry['name'], attr, res)
        return res

class FileGroupReader(object):
    def __init__(self, group_data):
        self.group_data = group_data    
        #
        self.columns = [v for _, v in CELL_FEATURE_REMAP] + ['XM', 'YM']
        if not 'filelist' in self.group_data:
            self.group_data['filelist'] = []
        attr_set = set()
        for fentry in self.group_data['filelist']:
            if 'attrib' in fentry:
                attr_set.update(fentry['attrib'].keys())
        self.pseudos = sorted(attr_set)
    def headers(self, sanitize=True):
        if sanitize:
            return ['id'] + [sanitizeR(c) for c in self.columns] + [sanitizeR(str(c)) for c in self.pseudos]
        return ['id'] + self.columns + self.pseudos
    def read_csv(self, filename):
        try:
            headers = None
            remap = None
            with open(filename, 'rb') as fi:
                rd = csv.reader(fi)
                for _row in rd:
                    if headers is None:
                        headers = _row
                        remap = []
                        for nm in self.columns:
                            idx = headers.index(nm)
                            if idx < 0: return
                            remap.append(idx)
                        continue
                    row = [_row[idx] for idx in remap]
                    yield row
        except Exception:
            return
    def iterrows(self):
        for fentry in self.group_data['filelist']:
            attrib = fentry.get('attrib', {})
            pvalues = [attrib.get(n, '') for n in self.pseudos]
            id = 0
            for row in self.read_csv(fentry.get('name')):
                id += 1
                yield [str(id)] + row + pvalues
    #
            
        
        
         