import os, sys, json, threading, subprocess
import time
from .filegroup import FileGroupManager, FileGroupReader, dblquot, sanitizeR

class PlotManager(object):
    def __init__(self, parent, app_name, app_ver, ftmgr, gmgr, tmgr, rscript):
        self.parent = parent
        self.app_name = app_name
        self.app_ver = app_ver
        self.ftmgr = ftmgr
        self.gmgr = gmgr
        self.tmgr = tmgr
        self.rscript = rscript
        #
        self.is_running = False
        self.thread = None
        self.message = None
        self.status = 0
        self.history_entry = None
    #
    def runit(self, group_data, selected_features, settings):
        if self.is_running:
            return False
        self.is_running = True
        self.group_data = group_data
        self.selected_features = selected_features
        self.settings = settings
        self.grname = self.settings['group']
        self.thread = threading.Thread(target=self._runit)
        self.thread.start()
    #
    def notify(self):
        self.parent.event_generate('<<PlotEvent>>', when='tail')
    #
    def _runit(self):
        time.sleep(0.1)
        self.status = 0
        self.history_entry = None
        try:
            self.do_runit()
        except Exception, ex:
            self.status = 2
            self.message = 'Exception: '+str(ex)
        self.is_running = False
        self.thread = None
        self.notify()
    #
    def _get_value_list(self, grp, o, attrk='primary'):
        try:
            attr = o[attrk]
            flist = grp['filelist']
            excl_set = set()
            if 'excl_values' in o:
                excl_list = o['excl_values'].get(attr)
                if excl_list:
                    excl_set.update(excl_list)
            incl_set = set()
            for e in flist:
                if not 'attrib' in e: continue
                v = e['attrib'].get(attr)
                if v is None or v in excl_set: continue
                incl_set.add(v)
            return sorted(incl_set), len(excl_set) > 0
        except Exception:
            pass
        return [], False
    def generate_context(self, grp, sel, o, csv_file):
        grname = o['group']
        s_grname = sanitizeR(grname)
        rdr = FileGroupReader(grp)
        csv_headers = rdr.headers()
        
        plotstyle = o['plotstyle']
        features = [x.attr for x in sel]
        ylabels = [x.name for x in sel]
        png_files = ['%s-%s-%s.png' % (s_grname, sanitizeR(feature), plotstyle) for feature in features]

        pri_values, filt_pri = self._get_value_list(grp, o, 'primary')
        sec_values, filt_sec = self._get_value_list(grp, o, 'secondary')
        try:
            textsize = o['width']*0.1
            if textsize < 0.25: textsize = 0.25
            elif textsize > 2.5: textsize = 2.5
            titlesize = textsize*1.5
            legendsize = textsize*0.8
        except Exception:
            textsize = 1.
            titlesize = 1.5
            legendsize = 0.8

        ymin = o.get('ymin', {})
        ymax = o.get('ymax', {})
        limlist = [str(ymin.get(x.attr, 'NA')) for x in sel] + [str(ymax.get(x.attr, 'NA')) for x in sel]

        boxborder = ''
        if o['boxalpha'] < 0.5:
            boxborder = 'color=%s, ' % (sanitizeR(o['primary']),)
        data = {
            'grname': s_grname,
            'csv_file': dblquot(csv_file),
            'csv_headers': 'c('+','.join([dblquot(h) for h in csv_headers])+')',
            'features': 'c(' + ','.join([dblquot(sanitizeR(f)) for f in features]) + ')',
            'ylimits': 'c(' + ','.join(limlist) + ')',
            'ylabels': 'c(' + ','.join([self.ftmgr.getExpression(l) for l in ylabels]) + ')',
            'png_files': 'c(' + ','.join([dblquot(f) for f in png_files]) + ')',
            'plot_style': plotstyle,
            'blackoutline': o['blackoutline'],
            'log2': o['log2'],
            'primary': sanitizeR(o['primary']),
            'primary_quot': dblquot(o['primary'].replace('_', ' ')),
            'secondary': sanitizeR(o['secondary']),
            'x_label': self.ftmgr.to_expr(o['xlabel']),
            'width': str(o['width']),
            'height': str(o['height']),
            'dotsize': str(o['dotsize']),
            'dotalpha': str(o['dotalpha']),
            'boxalpha': str(o['boxalpha']),
            'textsize': '%2.2f' % (textsize,),
            'titlesize': '%2.2f' % (titlesize,),
            'legendsize': '%2.2f' % (legendsize,),
            'boxborder': boxborder,
            'APP_NAME': self.app_name,
            'APP_VERSION': self.app_ver,
            'annotations': o['annotations'],
            'need_filtering': filt_pri or filt_sec,
        }
        if len(pri_values) > 0:
            data['pri_values'] = 'c(' + ','.join([dblquot(v) for v in pri_values]) + ')'
        if len(sec_values) > 0:
            data['sec_values'] = 'c(' + ','.join([dblquot(v) for v in sec_values]) + ')'
        return data
    #
    def runRscript(self, r_path):
        curdir = os.getcwd()
        data = None
        try:
            r_dir, r_file = os.path.split(r_path)
            os.chdir(r_dir)
            cmd = [self.rscript, r_file]
            print os.getcwd(), '>', cmd
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            r_out, r_err = p.communicate()
            rc = p.returncode
            if rc != 0:
                self.status = rc
                self.message = r_err
            else:
                data = {}
                for _l in r_out.split('\n'):
                    l = _l.strip()
                    if l.startswith('[1] '):
                        l = l[4:]
                        idx = l.find('|')
                        if idx < 0: continue
                        data[l[:idx]] = l[idx+1:]
                self.message = 'OK'
        except Exception, ex:
            self.status = 2
            self.message = 'Exception: '+str(ex)
        os.chdir(curdir)
        return data
    #        
    def do_runit(self):
        grp = self.group_data
        sel = self.selected_features
        o = self.settings

        grname = o['group']
        s_grname = sanitizeR(grname)
        self.message = 'Generating combined CSV for '+grname
        self.notify()
        grp['settings'] = o
        self.gmgr.saveGroup(grname, grp, delhistory=False)
        
        plotstyle = o['plotstyle']
        templ_fn = 'density.R' if plotstyle == 'density' else 'base.R'

        csv_path = self.gmgr.cacheGroupCsv(grname, grp)
        if csv_path is None:
            self.status = 1
            self.message = self.gmgr.errmsg
            self.notify()
            return
        hist_dir = self.gmgr.cacheGroupHistoryEntry(grname, grp, plotstyle)
        csv_file = os.path.relpath(csv_path, hist_dir)
        
        data = self.generate_context(grp, sel, o, csv_file)
        rcode = self.tmgr.render(templ_fn, data)
        r_file = s_grname+'.R'
        try:
            r_path = os.path.join(hist_dir, r_file)
            with open(r_path, "wb") as fo:
                fo.write(rcode)
        except Exception, ex:
            self.status = 1
            self.message = 'Failed to generate R: ' + str(ex)
            return
        self.message = 'Running Rscript for '+grname
        self.notify()
        plots = self.runRscript(r_path)
        if not plots is None:
            o['plots'] = plots      
            self.history_entry = spath = os.path.join(hist_dir, FileGroupManager.SETTINGS_FILE)
            with open(spath, 'w') as fo:
                json.dump(o, fo, indent=2)
    #

