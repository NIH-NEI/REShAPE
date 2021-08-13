import platform
import sys, os
import glob, json

__all__ = ('iterenv', 'search_in_path', 'ReshapeConfig')

# Windows specific files and directories
WIN_ROOT = 'C:'+os.path.sep
PF = 'Program Files'
PFx86 = 'Program Files (x86)'
MSVC = 'Microsoft Visual Studio'
WKITS = 'Windows Kits'
WIN_HLIST = ('corecrt.h', 'windows.h', 'winapifamily.h')

FIJI_DIRS = ('Fiji', 'Fiji.app')

BASEDIR = os.path.abspath(os.path.dirname(__file__))

def get_os_info():
    name = platform.system().lower()
    os_dist = platform.dist()
    dist = os_dist[0].lower()
    try:
        maj = int(os_dist[1].split('.')[0])
    except Exception:
        maj = 0
    return name, dist, maj

def iterenv(var):
    val = os.getenv(var)
    if not val: return
    for p in val.split(os.path.pathsep):
        if not p.strip(): continue
        yield os.path.expandvars(p)

def search_in_path(what):
    for p in iterenv('PATH'):
        cand = os.path.join(p, what)
        if os.path.exists(cand):
            return cand
    return None

def _findcand(what, basep, res=None):
    if res is None:
        res = []
    try:
        _what = what.lower()
        for item in os.listdir(basep):
            fullp = os.path.join(basep, item)
            if item.lower() == _what:
                res.append(fullp)
            elif os.path.isdir(fullp):
                _findcand(what, fullp, res)
    except Exception:
         pass
    return res

def _get_bin_dir(fpath):
    while 'bin' in fpath:
        fdir, fname = os.path.split(fpath)
        if fname == 'bin':
            return fpath
        fpath = fdir
    return None

def _get_bver(fpath):
    binp = _get_bin_dir(fpath)
    if not binp is None:
        return os.path.basename(os.path.dirname(binp))
    return fpath

def _get_bver_f(fpath, prefix='v'):
    ver = _get_bver(fpath)
    if prefix and ver.startswith(prefix):
        ver = ver[len(prefix):]
    try:
        return float(ver)
    except Exception:
        return 0.

def _win_findcrtdefs(clcand):
    cpath = os.path.dirname(clcand)
    while 'bin' in cpath:
        cpath, cdir = os.path.split(cpath)
        if cdir == 'bin':
            incldir = os.path.join(cpath, 'include')
            if os.path.exists(os.path.join(incldir, 'crtdefs.h')):
                return incldir
            break
    return None

def _win_findclexe():
    for p in iterenv('PATH'):
        clp = os.path.join(p, 'cl.exe')
        if os.path.exists(clp):
            incldir = _win_findcrtdefs(clp)
            if not incldir is None:
                return p, incldir
    # Not found in PATH, let's try usual suspects
    cands = []
    basep = os.path.join(WIN_ROOT, PFx86)
    for item in os.listdir(basep):
        if not item.startswith(MSVC):
            continue
        fullp = os.path.join(basep, item)
        if not os.path.isdir(fullp):
            continue
        _findcand('cl.exe', fullp, cands)
    # Rank candidates
    ranked = []
    for clp in cands:
        rank = 0
        parts = clp.split(os.path.sep)
        if 'x64' in parts and 'Hostx64' in parts:
            rank = 3
        elif 'amd64' in parts:
            rank = 2
        elif 'x64' in parts:
            rank = 1
        else:
            continue
        incldir = _win_findcrtdefs(clp)
        if incldir is None: continue
        ver = 0
        for i, p in enumerate(parts):
            if p.startswith(MSVC):
                try:
                    ver = int(p[len(MSVC)+1:].split('.')[0])
                    break
                except Exception:
                    pass
            elif p == 'MSVC':
                try:
                    ver = int(parts[i+1].split('.')[0])
                    break
                except Exception:
                    pass
        if ver < 10: continue
        ranked.append((rank, ver, clp, incldir))
    if len(ranked) == 0:
        return None, None
    ranked.sort(reverse=True)
    rank, ver, clp, incldir = ranked[0]
    return clp, incldir

def _win_findkits(hnames):
    cands = []
    basep = os.path.join('C:'+os.path.sep, PFx86, WKITS)
    _findcand(hnames[0], basep, cands)
    ranked = []
    for hp in cands:
        dir, name = os.path.split(hp)
        verdir = None
        ver = None
        while True:
            dir, name = os.path.split(dir)
            if not name: break
            try:
                ver = tuple(int(p) for p in name.split('.'))
                verdir = os.path.join(dir, name)
                break
            except Exception:
                pass
        if verdir is None: continue
        res = [os.path.dirname(hp)]
        for hpn in hnames[1:]:
            hpncands = _findcand(hpn, verdir)
            if not hpncands:
                res = None
                break
            res.append(os.path.dirname(hpncands[0]))
        if res is None: continue
        res.append(ver)
        ranked.append(res)
    ranked.sort(reverse=True)    
    return ranked[0][:-1] if ranked else None

class ReshapeConfig(object):
    def __init__(self):
        self.osname, self.osdist, self.osmajver = get_os_info()
        #
        self.ctxfn = os.path.join(BASEDIR, 'reshape_setup.json')
        self.matlab_options = []
        self.backenv = {}
        self.setup = {}
        #
        if self.osname == 'windows':
            self.find_matlab = self._find_matlab_windows
            self.find_cuda = self._find_cuda_windows
            self.find_fiji = self._find_fiji_windows
            self.config_mex = self._config_mex_windows
            self.config_env = self._config_env_windows
            self.matlab_options = ['-wait', '-automation']
        elif self.osname == 'linux':
            self.find_matlab = self._find_matlab_linux
            self.find_cuda = self._find_cuda_linux
            self.find_fiji = self._find_fiji_linux
            self.config_mex = self._config_mex_linux
            self.config_env = self._config_env_linux
            self.matlab_options = ['-nodisplay', '-nosplash']
            self._read_setup()
        else:
            raise RuntimeError('Unsupported OS: '+platform.system())
    #
    def _read_setup(self):
        if self.osname != 'linux':
            return
        try:
            with open(self.ctxfn, 'r') as fi:
                self.setup = json.load(fi)
        except Exception:
            pass
            
    #
    def _find_matlab_linux(self):
        what = 'matlab'
        matlab = search_in_path(what)
        if matlab is None:
            
            patt = os.path.join('/usr/local/MATLAB/*/bin', what)
            try:
                bver = None
                for fpath in glob.glob(patt):
                    _bver = _get_bver(fpath)
                    if _bver is None: continue
                    if bver is None or _bver > bver:
                        matlab = fpath
                        bver = _bver
            except Exception:
                pass
            
            return bver, matlab
        matlab = os.path.realpath(matlab)
        return _get_bver(matlab), matlab
    #
    def _find_matlab_windows(self):
        what = 'matlab.exe'
        matlab = search_in_path(what)
        if not matlab is None:
            return _get_bver(matlab), matlab
        cands = _findcand(what, os.path.join(WIN_ROOT, PF, 'MATLAB'))
        if not cands:
            return None, None
        return sorted([(_get_bver(p), p) for p in cands], reverse=True)[0]
    #
    def _find_cuda_linux(self):
        what = 'nvcc'
        nvcc = search_in_path(what)
        if nvcc is None:
            patt = os.path.join('/usr/local/cuda-*/bin', what)
            try:
                bver = None
                for fpath in glob.glob(patt):
                    _bver = _get_bver_f(fpath, prefix='cuda-')
                    if _bver == 0.: continue
                    if bver is None or _bver > bver:
                        nvcc = fpath
                        bver = _bver
            except Exception:
                pass
            if nvcc is None:
                return None, None
        nvcc = os.path.realpath(nvcc)
        return _get_bver_f(nvcc, prefix='cuda-'), os.path.dirname(os.path.dirname(nvcc))
    #
    def _find_cuda_windows(self):
        what = 'nvcc.exe'
        nvcc = search_in_path(what)
        if not nvcc is None:
            return _get_bver_f(nvcc), os.path.dirname(os.path.dirname(nvcc))
        cands = _findcand(what, os.path.join(WIN_ROOT, PF, 'NVIDIA GPU Computing Toolkit'))
        if not cands:
            return None, None
        ver, p = sorted([(_get_bver_f(p), p) for p in cands], reverse=True)[0]
        return ver, os.path.dirname(os.path.dirname(p))
    #
    def _find_fiji_linux(self):
        if 'FIJI' in self.setup:
            return self.setup['FIJI']
        what = 'ImageJ-linux64'
        fiji = search_in_path(what)
        if not fiji is None:
            return fiji
        for root in ('/', '/usr/local', '/opt'):
            for susp in FIJI_DIRS:
                fiji = os.path.join(root, susp, what)
                if os.path.exists(fiji):
                    return fiji
        return None
    #
    def _find_fiji_windows(self):
        what = 'ImageJ-win64.exe'
        fiji = search_in_path(what)
        if not fiji is None:
            return fiji
        for susp in FIJI_DIRS:
            fiji = os.path.join(WIN_ROOT, susp, what)
            if os.path.exists(fiji):
                return fiji
        return None
    #
    def _config_mex_linux(self, cfg):
        if not 'cuda' in cfg:
            cfg['mex'] = {}
            return
        cfg['mex'] = {
                'enableGpu': True,
                'enableCudnn': False,
                'EnableImreadJpeg': True,
                'cudaMethod': 'mex',
                'cudaRoot': cfg['cuda'],
                'verbose': 1,
            }
    #
    def _config_mex_windows(self, cfg):
        try:
            cuda = cfg['cuda']
            matlab_ver = cfg['matlab_ver']
        except Exception:
            cfg['mex'] = {}
            return
        cfg['mex'] = mex = {
                'enableGpu': True,
                'enableCudnn': False,
                'EnableImreadJpeg': True,
                'cudaMethod': 'nvcc',
                'cudaRoot': cuda,
                'verbose': 1,
            }
        if matlab_ver >= 'R2018a':
            mex['mexFlags'] = ['-R2018a']
    #
    def _config_env_linux(self):
        return {}
    #
    def _config_env_windows(self):
        newenv = {}
        # Handle SDK include directories
        includes = _win_findkits(WIN_HLIST)
        if includes:
            newenv['+INCLUDE'] = os.path.pathsep.join(includes)
        # Handle cl.exe
        clp, incldir = _win_findclexe()
        if not clp is None:
            bindir = os.path.dirname(clp)
            newenv['+PATH'] = bindir
        return newenv
    #
    def update_env(self, newenv):
        for key, val in newenv.iteritems():
            if key.startswith('+'):
                key = key[1:]
                if key in self.backenv:
                    oldval = self.backenv[key]
                else:
                    self.backenv[key] = oldval = os.getenv(key)
                if oldval:
                    val = val + os.path.pathsep + oldval
            else:
                if not key in self.backenv:
                    self.backenv[key] = os.getenv(key)
            os.environ[key] = val
    #
    def restore_env(self):
        for key, val in self.backenv.iteritems():
            if val is None:
                if key in os.environ:
                    del os.environ[key]
            else:
                os.environ[key] = val
    #
    def autoconfig(self):
        matlab_ver, matlab = self.find_matlab()
        cuda_ver, cuda = self.find_cuda()
        fiji = self.find_fiji()
        environ = self.config_env()
        cfg = {
                'matlab_ver': matlab_ver,
                'matlab': matlab,
                'matlab_opts': self.matlab_options,
                'cuda_ver': cuda_ver,
                'cuda': cuda,
                'fiji': fiji,
                'environ': environ,
                'REShAPE': 'REShAPE_Auto_0.0.1.ijm',
            }
        self.config_mex(cfg)
        if 'SEGMENTATION' in self.setup and 'MRT_DIR' in self.setup:
            cfg['SEGMENTATION'] = self.setup['SEGMENTATION']
            cfg['MRT_DIR'] = self.setup['MRT_DIR']
            cfg['USE_STANDALONE'] = True
        if 'ReshapeImaging' in self.setup:
            cfg['Imaging'] = self.setup['ReshapeImaging']
        return cfg
    
if __name__ == '__main__':
    
    acfg = ReshapeConfig()
    cfg = acfg.autoconfig()
    print cfg
#     acfg.update_env(cfg['environ'])
#     print '--- New env ---'
#     print 'PATH=', os.getenv('PATH')
#     print 'INCLUDE=', os.getenv('INCLUDE')
#     print '--- Backup env ---'
#     print acfg.backenv
    
    sys.exit(0)


