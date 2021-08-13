#! /usr/bin/python
import platform
import os, sys, getpass, subprocess
import json
from pipes import quote as sh_quote

YUM_PACKAGES = [
    'epel-release',
    'python-pip',
    'tkinter',
    'python-devel',
    'freeglut-devel',
    'libXt-devel',
    'centos-release-scl',
    'devtoolset-7',
    'libjpeg-turbo-devel',
    'libcurl-devel',
    'openssl-devel',
    'libxml2-devel',
    'R',
]
YUM_GROUPS = [
    'development tools',
]
PIP_PACKAGES = [
    'Pillow',
    'Jinja2',
]

ALL_SETUP = (
    ('yum_packages', YUM_PACKAGES),
    ('yum_groups', YUM_GROUPS),
    ('pip_packages', PIP_PACKAGES),
    ('thirdparty', []),
    ('segmentation', ['Standalone', 'segmfunc']),
    ('cppsource', []),
    ('r_packages', []),
)
    

BASEDIR = os.path.abspath(os.path.dirname(__file__))
START_JSON_RESPONSE = '<<<========== START JSON RESPONSE ==========>>>'
END_JSON_RESPONSE = '>>>========== END JSON RESPONSE ==========<<<'
CPP_INSTALL_PREFIX = os.path.join(BASEDIR, 'Imaging')

class SudoInstaller(object):
    def __init__(self, sudopass):
        self.sudopass = sudopass
        #
        self.ctxfn = os.path.join(BASEDIR, 'reshape_setup.json')
        self.context = {}
    def save_context(self):
        with open(self.ctxfn, 'w') as fo:
            json.dump(self.context, fo, indent=2)
    def print_context(self):
        print '--- %s ---' % self.ctxfn
        print json.dumps(self.context, indent=2)
    def run_sudo(self, *cmd, **opts):
        self.out = self.err = None
        fullcmd = ['sudo', '-S'] + list(cmd)
        args = {
            'stdin': subprocess.PIPE,
            'stdout': subprocess.PIPE,
        }
        args.update(opts)
        p = subprocess.Popen(fullcmd, **args)
        self.out, self.err = p.communicate(self.sudopass+'\n')
        return p.wait()
    def command(self, *cmd, **opts):
        self.out = self.err = None
        fullcmd = list(cmd)
        args = {
            'stdout': sys.stderr,
        }
        args.update(opts)
        p = subprocess.Popen(fullcmd, **args)
        self.out, self.err = p.communicate()
        return p.wait()
    def yum_packages(self, *lst):
        for pkg in lst:
            print 'Installing package: '+pkg
            rc = self.run_sudo('yum', '-y', 'install', pkg)
            if rc != 0:
                print self.err
                return rc
            print self.out.split('\n')[-4:]
        return 0
    def yum_groups(self, *lst):
        for pkg in lst:
            print 'Installing package group: '+pkg
            rc = self.run_sudo('yum', '-y', 'groupinstall', pkg)
            if rc != 0:
                print self.err
                return rc
            print self.out.split('\n')[-4:]
        return 0
    def pip_packages(self, *lst):
        print 'Update PIP'
        rc = self.run_sudo(sys.executable, '-m', 'pip', 'install', '--upgrade', 'pip')
        if rc != 0:
            print self.err
            return rc
        for pkg in lst:
            print 'Installing Python package: '+pkg
            rc = self.run_sudo(sys.executable, '-m', 'pip', 'install', pkg)
            if rc != 0:
                print self.err
                return rc
            print self.out.split('\n')[-4:]
        return 0
    def thirdparty(self, *lst):
        os.chdir(os.path.join(BASEDIR, 'thirdparty'))
        rc = self.run_sudo(sys.executable, 'install.py', stderr=sys.stderr)
        try:
            jtext = self.out.split(START_JSON_RESPONSE)[1].split(END_JSON_RESPONSE)[0]
            jobj = json.loads(jtext)
            if 'error' in jobj:
                print jobj.pop('error')
                rc = 1
            self.context.update(jobj)
            self.save_context()
        except Exception:
            rc = 1
        return rc
    #
    def segmentation(self, segmdir, segmfn):
        if not 'MRT_ENV' in self.context:
            return 1
        mrt_env = self.context['MRT_ENV']
        try:
            segmpath = os.path.join(BASEDIR, segmdir)
            exe = os.path.join(segmpath, segmfn)
            if not os.path.exists(exe):
                print '%s not found (broken archive?);\nInstallation aborted.' % (exe,)
                return 1
            shname = os.path.join(segmpath, 'segmentation.sh')
            with open(shname, 'w') as fo:
                fo.write( \
'''#!/bin/sh
exe_name=$0
exe_dir=`dirname "$0"`
export LD_LIBRARY_PATH=.:%s
args=
while [ $# -gt 0 ]; do
    token=$1
    args="${args} \"${token}\"" 
    shift
done
eval "\"${exe_dir}/segmfunc\"" $args
''' % (mrt_env,)
            )
            os.chmod(shname, 0755)
            self.context['SEGMENTATION'] = shname
            self.save_context()
        except Exception:
            return 1
        return 0
    #
    def cppsource(self, *lst):
        cppdir = os.path.join(BASEDIR, 'cppsource')
        tgtdir = os.path.join(BASEDIR, 'Imaging')
        cmake_cmd = ['cmake',
            '--no-warn-unused-cli',
            '-DCMAKE_INSTALL_PREFIX:PATH=%s' % (CPP_INSTALL_PREFIX,),
        ]
        for pdir in ('Qt5_DIR', 'VTK_DIR', 'ITK_DIR'):
            if pdir in self.context:
                cmake_cmd.append('-D%s:PATH=%s' % (pdir, self.context[pdir]))
        cmake_cmd.append('..')
        cmake_cmdline = ' '.join(map(sh_quote, cmake_cmd))
        # print cmake_cmdline
        try:
            plist = []
            for proj in os.listdir(cppdir):
                projpath = os.path.join(cppdir, proj)
                if not os.path.isdir(projpath): continue
                cmf = os.path.join(projpath, 'CMakeLists.txt')
                if not os.path.exists(cmf): continue
                plist.append(proj)
                shname = os.path.join(projpath, 'build.sh')
                with open(shname, 'w') as fo:
                    fo.write( \
'''#!/bin/sh
cdir="$PWD"
mkdir build
cd build
rm -rf *
%s
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit $?
fi
gmake
if [ "$?" -ne "0" ]; then
  cd "$cdir"
  exit $?
fi
gmake install
cd "$cdir"
exit $?
''' % (cmake_cmdline,)
                    )
            os.chmod(shname, 0755)
            for proj in plist:
                print 'Building:', proj
                projpath = os.path.join(cppdir, proj)
                os.chdir(projpath)
                s = subprocess.Popen(['scl', 'enable', 'devtoolset-7', 'bash'], stdin=subprocess.PIPE, stdout=sys.stderr)
                s.communicate('./build.sh\nexit $?\n')
                if s.returncode != 0:
                    print 'Failed to build', proj
                    return 1
                tgtpath = os.path.join(tgtdir, proj)
                if os.path.exists(tgtpath):
                    self.context[proj] = tgtpath
            self.save_context()
        except Exception:
            return 1
        return 0
    #
    def r_packages(self, *lst):
        try:
            rspath = os.path.join(BASEDIR, 'templates', 'setup.R')
            self.run_sudo('Rscript', rspath, stderr=sys.stderr)
        except Exception:
            pass
        return 0
    #

def get_os_info():
    name = platform.system().lower()
    os_dist = platform.dist()
    dist = os_dist[0].lower()
    try:
        maj = int(os_dist[1].split('.')[0])
    except Exception:
        maj = 0
    return name, dist, maj
    #

def set_exec_permissions():
    dirs = []
    for _cdir in ('Standalone', 'Imaging'):
        cdir = os.path.join(BASEDIR, _cdir)
        if os.path.isdir(cdir):
            dirs.append(cdir)
    for _tdir in ('thirdparty', 'cppsource'):
        tdir = os.path.join(BASEDIR, _tdir)
        for fn in os.listdir(tdir):
            cdir = os.path.join(tdir, fn)
            if os.path.isdir(cdir):
                dirs.append(cdir)
    for cdir in dirs:
        for fn in os.listdir(cdir):
            _, ext = os.path.splitext(fn)
            if not ext in ('.sh', '.run', ''): continue
            fpath = os.path.join(cdir, fn)
            os.chmod(fpath, 0755)
    #
        
if __name__ == '__main__':
    
    name, dist, maj = get_os_info()
    if name != 'linux' or dist != 'centos' or maj < 7:
        print 'This setup program can only run on Centos 7 Linux platform.'
        sys.exit(1)
    if maj > 7:
        print >> sys.stderr, 'WARNING: This setup program has not been tested on Centos %d;' % (maj,)
        print >> sys.stderr, '         Continue at your own risk.'
    
    cdir = os.getcwd()
    
    try:
        set_exec_permissions()
    except Exception, ex:
        print 'WARNING: Permissions not set:', str(ex)
    
    sudopass = getpass.getpass("Enter sudo password for '%s': " % (getpass.getuser(),))
    # print json.dumps(sudopass)
    
    inst = SudoInstaller(sudopass)
    for fncname, params in ALL_SETUP:
        os.chdir(BASEDIR)
        rc = getattr(inst, fncname)(*params)
        if rc != 0: break
    if rc == 0:
        inst.print_context()
    
    os.chdir(cdir)
    status = 'SUCCESS' if rc == 0 else 'FAILED'
    print 'Setup status: %s.' % (status,)
    sys.exit(rc)
