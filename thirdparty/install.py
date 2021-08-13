#! /usr/bin/python
import os, sys, json, shutil, glob, subprocess
from pipes import quote as sh_quote

PACKAGES = [
    ('MatlabRT', 'mrt_inst.sh', '/opt/MatlabRT'),
    ('Fiji', 'fiji_inst.sh', '/opt/Fiji.app', ),
    ('CMake3', 'cmake_inst.sh', '/opt/CMake3', ),
    ('version_check', 'CMakeLists.txt', '/opt/RsVersionCheck', ),
    ('Qt5', 'qt_inst.sh', '/opt/Qt', ),
    ('VTK8', 'vtk_build.sh', '/opt/VTK8', ),
    ('ITK4', 'itk_build.sh', '/opt/ITK4', ),
]

MIN_QT_VER = '5.13.2'
MIN_VTK_VER = '8.0.0'
MIN_ITK_VER = '4.12.0'

VTK_ITK_KEYS = ('VTK_DIR', 'VTK_VER', 'ITK_DIR', 'ITK_VER', 'VTK_MODULES_DIR')

MRT_DOWNLOAD_URL = 'https://ssd.mathworks.com/supportfiles/downloads/R2019b/Release/4/deployment_files/installer/complete/glnxa64/MATLAB_Runtime_R2019b_Update_4_glnxa64.zip'
MRT_FILE = 'MATLAB_Runtime_R2019b_Update_4_glnxa64.zip'

FIJI_DOWNLOAD_URL = 'https://downloads.imagej.net/fiji/latest/fiji-linux64.zip'
FIJI_FILE = 'fiji-linux64.zip'

CMAKE_DOWNLOAD_URL = 'https://github.com/Kitware/CMake/releases/download/v3.16.5/cmake-3.16.5-Linux-x86_64.tar.gz'
CMAKE_FILE = 'cmake-3.16.5-Linux-x86_64.tar.gz'

VTK_DOWNLOAD_URL = 'https://www.vtk.org/files/release/8.2/VTK-8.2.0.zip'
VTK_FILE = 'VTK-8.2.0.zip'

ITK_DOWNLOAD_URL = 'https://github.com/InsightSoftwareConsortium/ITK/releases/download/v4.13.2/InsightToolkit-4.13.2.zip'
ITK_FILE = 'InsightToolkit-4.13.2.zip'

BASEDIR = os.path.abspath(os.path.dirname(__file__))
START_JSON_RESPONSE = '<<<========== START JSON RESPONSE ==========>>>'
END_JSON_RESPONSE = '>>>========== END JSON RESPONSE ==========<<<'

class InstallException(Exception):
    pass

def _download_from_web(url, fname):
    tempname = fname + '.part'
    try:
        os.remove(tempname)
    except Exception:
        pass
    cmd = ['wget', url, '-O', tempname]
    p = subprocess.Popen(cmd, stdout=sys.stderr)
    if p.wait() != 0:
        raise InstallException('Failed to download: '+url)
    try:
        os.remove(fname)
    except Exception:
        pass
    os.rename(tempname, fname)
    os.chmod(fname, 0666)

def version_less(nver, over):
    if over is None:
        return nver is None
    if nver is None:
        return True
    nparts = nver.split('.')
    oparts = over.split('.')
    for idx, np in enumerate(nparts):
        try:
            inp = int(np)
            onp = int(oparts[idx])
            if inp != onp:
                return inp < onp
        except Exception:
            return True
    return True

def check_integrity():
    os.chdir(BASEDIR)
    for item in PACKAGES:
        subdir, script = item[0:2]
        srcpath = os.path.join(BASEDIR, subdir, script)
        if not os.path.exists(srcpath):
            raise InstallException('Missing: '+srcpath)

def get_MRT_dir(resp, dest):
    try:
        logfn = os.path.join(dest, 'reshape_install.txt')
        hdr_found = False
        env = None
        with open(logfn, 'r') as fi:
            for _l in fi:
                if not hdr_found:
                    if 'your LD_LIBRARY_PATH environment variable' in _l:
                        hdr_found = True
                    continue
                l = _l.strip()
                if env is None and len(l) > 0:
                    env = l
                    break

        fullp = env.split(os.pathsep)[0]
        vdir = os.path.relpath(fullp, dest).split(os.sep)[0]
        mrt_dir = os.path.join(dest, vdir)
        resp['MRT_DIR'] = mrt_dir
        resp['MRT_ENV'] = env
        return True
    except Exception:
        pass
    return False
        
def check_MatlabRT(resp, subdir, script, dest):
    print >> sys.stderr, 'Check Matlab Runtime...'
    if get_MRT_dir(resp, dest):
        return
    srcdir = os.path.join(BASEDIR, subdir)
    os.chdir(srcdir)
    if not os.path.exists(MRT_FILE):
        print >> sys.stderr, 'Downloading Matlab Runtime...'
        _download_from_web(MRT_DOWNLOAD_URL, MRT_FILE)
    print >> sys.stderr, 'Installing Matlab Runtime...'
    try:
        cmd = [os.path.join(srcdir, script), dest]
        p = subprocess.Popen(cmd, stdout=sys.stderr)
        p.wait()
    except Exception, ex:
        pass
    return get_MRT_dir(resp, dest)
        
def check_Fiji(resp, subdir, script, dest):
    print >> sys.stderr, 'Check Fiji...'
    fiji = os.path.join(dest, 'ImageJ-linux64')
    if not os.path.exists(fiji):
        srcdir = os.path.join(BASEDIR, subdir)
        os.chdir(srcdir)
        if not os.path.exists(FIJI_FILE):
            print >> sys.stderr, 'Downloading Fiji...'
            _download_from_web(FIJI_DOWNLOAD_URL, FIJI_FILE)
        print >> sys.stderr, 'Installing Fiji...'
        try:
            cmd = [os.path.join(srcdir, script), os.path.dirname(dest)]
            p = subprocess.Popen(cmd, stdout=sys.stderr)
            p.wait()
        except Exception:
            pass
    if not os.path.exists(fiji):
        raise RuntimeError('Failed to install Fiji')
    resp['FIJI'] = fiji
        
def get_cmake_ver():
    try:
        cmd = ['cmake', '--version']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        if p.returncode != 0:
            return None
        return out.split('\n')[0].split()[-1]
    except Exception:
        return None

def check_CMake3(resp, subdir, script, dest):
    print >> sys.stderr, 'Check CMake...'
    cmake_ver = get_cmake_ver()
    if cmake_ver is None:
        srcdir = os.path.join(BASEDIR, subdir)
        os.chdir(srcdir)
        if not os.path.exists(CMAKE_FILE):
            print >> sys.stderr, 'Downloading CMake...'
            _download_from_web(CMAKE_DOWNLOAD_URL, CMAKE_FILE)
        print >> sys.stderr, 'Installing CMake...'
        try:
            cmd = [os.path.join(srcdir, script), os.path.dirname(dest)]
            p = subprocess.Popen(cmd, stdout=sys.stderr)
            p.wait()
        except Exception:
            pass
        cmake_ver = get_cmake_ver()
        if cmake_ver:
            print >> sys.stderr, 'CMake', cmake_ver, 'successfully installed.'
    if cmake_ver is None:
        raise InstallException('Failed to install CMake')
    resp['CMAKE_VER'] = cmake_ver
        
def read_cmake_var(qtvf, varname):
    try:
        with open(qtvf, 'r') as fi:
            for _l in fi:
                l = _l.strip()
                if not l: continue
                try:
                    a, b = l.split('(')
                    b, c = b.split(')')[0].split()
                    if a.strip() == 'set' and b.strip() == varname:
                        c = c.strip()
                        if c.startswith('"'):
                            return json.loads(c)
                        return c
                except Exception:
                    pass
    except Exception:
        pass
    return None

def read_pkg_ver(qtvf):
    return read_cmake_var(qtvf, 'PACKAGE_VERSION')
def read_qt_dir(qtvf):
    return read_cmake_var(qtvf, 'Qt5_DIR')

def find_vtk_dir(resp, prefix):
    try:
        cfgpatt = os.path.join(prefix, 'lib64', 'cmake', '*', 'VTKConfig.cmake')
        cfg_list = glob.glob(cfgpatt)
    except Exception:
        return False
    for cfg in cfg_list:
        try:
            vtk_dir = os.path.dirname(cfg)
            cfgver = os.path.join(vtk_dir, 'VTKConfigVersion.cmake')
            vtk_ver = read_pkg_ver(cfgver)
            if version_less(vtk_ver, MIN_VTK_VER):
                continue
            resp['VTK_VER'] = vtk_ver
            resp['VTK_DIR'] = vtk_dir
            return True
        except Exception:
            pass
    return False
    
def find_vtk(resp):
    prefix = cfg_list = None
    if 'VTK_DIR' in resp:
        cfg = os.path.join(resp['VTK_DIR'], 'VTKConfig.cmake')
        parts = cfg.split('/lib64/cmake/')
        if len(parts) == 2:
            prefix = parts[0]
        cfg_list = [cfg]
    
    if cfg_list is None:
        for item in PACKAGES:
            nm = item[0]
            if nm != 'VTK8': continue
            prefix = item[2]
        if prefix is None:
            return False
        try:
            cfgpatt = os.path.join(prefix, 'lib64', 'cmake', '*', 'VTKConfig.cmake')
            cfg_list = glob.glob(cfgpatt)
        except Exception:
            return False
    
    for cfg in cfg_list:
        try:
            vtk_dir = os.path.dirname(cfg)
            cfgver = os.path.join(vtk_dir, 'VTKConfigVersion.cmake')
            vtk_ver = read_pkg_ver(cfgver)
            if version_less(vtk_ver, MIN_VTK_VER):
                continue
            vtkmod = read_cmake_var(cfg, 'VTK_MODULES_DIR')
            if not prefix is None:
                vtkmod = vtkmod.replace('${VTK_INSTALL_PREFIX}', prefix)
            modcfg = os.path.join(vtkmod, 'vtkViewsQt.cmake')
            qt_dir = read_qt_dir(modcfg)
            qt_ver = read_pkg_ver(os.path.join(qt_dir, 'Qt5ConfigVersion.cmake'))
            if version_less(qt_ver, MIN_QT_VER):
                continue
            resp['Qt5_VER'] = qt_ver
            resp['Qt5_DIR'] = qt_dir
            resp['VTK_VER'] = vtk_ver
            resp['VTK_DIR'] = vtk_dir
            return True
        except Exception:
            pass
    return False

def find_itk_dir(resp, prefix):
    try:
        cfgpatt = os.path.join(prefix, 'lib', 'cmake', '*', 'ITKConfig.cmake')
        cfg_list = glob.glob(cfgpatt)
    except Exception:
        return False
    for cfg in cfg_list:
        try:
            itk_dir = os.path.dirname(cfg)
            cfgver = os.path.join(itk_dir, 'ITKConfigVersion.cmake')
            itk_ver = read_pkg_ver(cfgver)
            if version_less(itk_ver, MIN_ITK_VER):
                continue
            resp['ITK_VER'] = itk_ver
            resp['ITK_DIR'] = itk_dir
            return True
        except Exception:
            pass
    return False

def find_itk(resp):
    prefix = None
    for item in PACKAGES:
        nm = item[0]
        if nm != 'ITK4': continue
        prefix = item[2]
    if prefix is None:
        return False
    try:
        cfgpatt = os.path.join(prefix, 'lib', 'cmake', '*', 'ITKConfig.cmake')
        cfg_list = glob.glob(cfgpatt)
    except Exception:
        return False
    for cfg in cfg_list:
        try:
            itk_dir = os.path.dirname(cfg)
            cfgver = os.path.join(itk_dir, 'ITKConfigVersion.cmake')
            itk_ver = read_pkg_ver(cfgver)
            if version_less(itk_ver, MIN_ITK_VER):
                continue
            itkmod = read_cmake_var(cfg, 'ITK_MODULES_DIR').replace('${ITK_INSTALL_PREFIX}', prefix)
            modcfg = os.path.join(itkmod, 'ITKVtkGlue.cmake')
            vtk_dir = read_cmake_var(modcfg, 'VTK_DIR')
            vtk_ver = read_pkg_ver(os.path.join(vtk_dir, 'VTKConfigVersion.cmake'))
            if version_less(vtk_ver, MIN_VTK_VER):
                continue
            resp['VTK_VER'] = vtk_ver
            resp['VTK_DIR'] = vtk_dir
            resp['ITK_VER'] = itk_ver
            resp['ITK_DIR'] = itk_dir
            return True
        except Exception:
            pass
    return False

def get_qt_ver(dest):
    cqt_ver = cqt_dir = None
    pattern = os.path.join(dest, '*/gcc_64/lib/cmake/Qt5/Qt5ConfigVersion.cmake')
    for qtvf in glob.glob(pattern):
        qt_ver = read_pkg_ver(qtvf)
        if version_less(qt_ver, cqt_ver): continue
        cqt_ver = qt_ver
        cqt_dir = os.path.dirname(qtvf)
    return cqt_ver, cqt_dir

def check_Qt5(resp, subdir, script, dest):
    print >> sys.stderr, 'Check Qt5...'
    srcdir = os.path.join(BASEDIR, subdir)
    os.chdir(srcdir)
    #
    qt_ver, qt_dir = get_qt_ver(dest)
    if version_less(qt_ver, MIN_QT_VER):
        print >> sys.stderr, 'Installing Qt5...'
        mtpath = os.path.join(dest, 'MaintenanceTool')
        if os.path.exists(mtpath):
            raise InstallException('''An earlier Qt installation is found.
Please run "%s" manually (as root)
and install component Qt/Qt %s/Desktop gcc 64bit (or higher).''' % (mtpath, MIN_QT_VER))
        qt_ver = qt_dir = None
        shutil.rmtree(dest, ignore_errors=True)
        try:
            cmd = [os.path.join(srcdir, script)]
            p = subprocess.Popen(cmd, shell=True, stdout=sys.stderr)
            rc = p.wait()
#             if os.path.exists(mtpath):
#                 # Try running MaintenanceTool directly if it exists
#                 print >> sys.stderr, 'Running Qt5 Maintenance tool directly...'
#                 cmd = [mtpath, '--verbose', '--script', 'non-interactive.qs']
#                 p = subprocess.Popen(cmd, stdout=sys.stderr)
#                 rc = p.wait()
            if rc != 0:
                raise RuntimeError('Qt5')
            qt_ver, qt_dir = get_qt_ver(dest)
            if not qt_ver is None:
                print >> sys.stderr, 'Qt', qt_ver, 'successfully installed.'
                resp['Qt5_Installed'] = True
        except Exception:
            pass
    #
    if qt_ver is None:
        raise InstallException('Failed to install Qt5')
    else:
        resp['Qt5_VER'] = qt_ver
        resp['Qt5_DIR'] = qt_dir

def check_version_check(resp, subdir, script, dest):
    print >> sys.stderr, 'Version check...'
    os.chdir(BASEDIR)
    shutil.rmtree(dest, ignore_errors=True)
    srcdir = os.path.join(BASEDIR, subdir)
    try:
        shutil.copytree(srcdir, dest)
        tgtdir = os.path.join(dest, 'build')
        os.mkdir(tgtdir)
        os.chdir(tgtdir)
        cmd = ['cmake']
        if 'Qt5_DIR' in resp:
            cmd.append('-DQt5_DIR:PATH='+resp['Qt5_DIR'])
        if 'VTK_DIR' in resp:
            cmd.append('-DVTK_DIR:PATH='+resp['VTK_DIR'])
        if 'ITK_DIR' in resp:
            cmd.append('-DITK_DIR:PATH='+resp['ITK_DIR'])
        cmd.append('..')
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        for _l in out.split('\n'):
            if _l.startswith('--'):
                _l = _l[2:]
            parts = _l.split(':')
            if len(parts) != 2: continue
            k = parts[0].strip()
            if k in VTK_ITK_KEYS:
                resp[k] = parts[1].strip()
    except Exception:
        pass
    shutil.rmtree(dest, ignore_errors=True)

def check_vtk_install(resp):
    try:
        if 'Qt5_Installed' in resp:
            del resp['Qt5_Installed']
            raise RuntimeError('Rebuild VTK')
        vtkdir = resp['VTK_DIR']
        vtkmod = resp['VTK_MODULES_DIR']
        vtk_qt_dir = read_qt_dir(os.path.join(vtkmod, 'vtkRenderingQt.cmake'))
        if not os.path.samefile(resp['Qt5_DIR'], vtk_qt_dir):
            # resp['vtk_qt_dir'] = vtk_qt_dir
            raise RuntimeError('Rebuild VTK')
        return True
    except Exception:
        pass
    for k in VTK_ITK_KEYS:
        if k in resp: del resp[k]
    return False

def check_VTK8(resp, subdir, script, dest):
    print >> sys.stderr, 'Check VTK...'
    if check_vtk_install(resp):
        return
    srcdir = os.path.join(BASEDIR, subdir)
    os.chdir(srcdir)
    if not os.path.exists(VTK_FILE):
        print >> sys.stderr, 'Downloading VTK...'
        _download_from_web(VTK_DOWNLOAD_URL, VTK_FILE)
     
    print >> sys.stderr, 'Installing VTK...'
    shutil.rmtree(dest, ignore_errors=True)
    
    cmd = [os.path.join(srcdir, script), os.path.dirname(dest)]
    if 'Qt5_DIR' in resp:
        cmd.append('-DQt5_DIR:PATH='+resp['Qt5_DIR'])
    s = subprocess.Popen(['scl', 'enable', 'devtoolset-7', 'bash'], stdin=subprocess.PIPE, stdout=sys.stderr)
    s.communicate(' '.join(map(sh_quote, cmd)) + '\nexit $?\n')
    if s.returncode != 0 or not find_vtk_dir(resp, dest):
        raise RuntimeError('Failed to install VTK')
    print >> sys.stderr, 'VTK successfully installed.'

def check_ITK4(resp, subdir, script, dest):
    print >> sys.stderr, 'Check ITK...'
    if 'ITK_DIR' in resp:
        return
    srcdir = os.path.join(BASEDIR, subdir)
    os.chdir(srcdir)
    if not os.path.exists(ITK_FILE):
        print >> sys.stderr, 'Downloading ITK...'
        _download_from_web(ITK_DOWNLOAD_URL, ITK_FILE)
     
    print >> sys.stderr, 'Installing ITK...'
    shutil.rmtree(dest, ignore_errors=True)
    cmake_pars = []
    if 'Qt5_DIR' in resp:
        cmake_pars.append('-DQt5_DIR:PATH='+resp['Qt5_DIR'])
    if 'VTK_DIR' in resp:
        cmake_pars.append('-DVTK_DIR:PATH='+resp['VTK_DIR'])
    cmd = [os.path.join(srcdir, script), os.path.dirname(dest)]
    if len(cmake_pars) > 0:
        cmd.append(' '.join(map(sh_quote, cmake_pars)))
    s = subprocess.Popen(['scl', 'enable', 'devtoolset-7', 'bash'], stdin=subprocess.PIPE, stdout=sys.stderr)
    s.communicate(' '.join(map(sh_quote, cmd)) + '\nexit $?\n')
    if s.returncode != 0 or not find_itk_dir(resp, dest):
        raise RuntimeError('Failed to install ITK')
    print >> sys.stderr, 'ITK successfully installed.'

if __name__ == '__main__':
    import datetime
    
    if os.geteuid() != 0:
        print >> sys.stderr, 'This script must be run as root.'
        sys.exit(1)
        
    start_ts = datetime.datetime.now()
    print >> sys.stderr, 'Installing Third Party products'
    
    cdir = os.getcwd()
    resp = {}
    rc = 0
    try:
        check_integrity()
        find_itk(resp)
        find_vtk(resp)
        for item in PACKAGES:
            fnc = globals()['check_'+item[0]]
            fnc(resp, *item)
    except Exception, ex:
        err = str(ex)
        print >> sys.stderr, 'ERROR:', err
        resp['error'] = err
        rc = 1

    os.chdir(cdir)
    print START_JSON_RESPONSE
    print json.dumps(resp, indent=2)
    print END_JSON_RESPONSE
    
    elapsed = datetime.datetime.now() - start_ts
    print >> sys.stderr, 'Third Party done in:', str(elapsed)
    sys.exit(rc)
