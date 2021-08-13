#! /usr/bin/python
import platform
import os, sys, json, subprocess

from visual.templatemgr import TemplateManager
import reshape_version

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DESKTOP_TEMPL_DIR = os.path.join(BASE_DIR, 'Icons', 'desktop')
MATLAB_DIR = os.path.join(BASE_DIR, 'matlab')

def generate_context():
    ctxfn = os.path.join(BASE_DIR, 'reshape_setup.json')
    with open(ctxfn, 'r') as fi:
        setup = json.load(fi)
    ctx = {
        'PLATFORM_RELEASE': platform.release(),
        'BASE_DIR': BASE_DIR,
        'FIJI': setup['FIJI'],
        'FIJI_DIR': os.path.dirname(setup['FIJI']),
    }
    for key in dir(reshape_version):
        if not key.startswith('_'):
            ctx[key] = getattr(reshape_version, key)
    return ctx

def trust_desktop_icon(fpath):
    # gio set Your_desktop_file.desktop "metadata::trusted" yes
    p = subprocess.Popen(['gio', 'set', fpath, 'metadata::trusted', 'yes'])
    return p.wait()

def refresh_gnome():
    # gnome-shell --replace & disown
    return subprocess.call('gnome-shell --replace & disown', shell=True)

def create_desktop_icons():
    desktop_dir = os.path.join(os.path.expanduser('~'), 'Desktop')
    ctx = generate_context()
    tm = TemplateManager(None, templdir=DESKTOP_TEMPL_DIR)
    for fn in os.listdir(DESKTOP_TEMPL_DIR):
        if not fn.endswith('.desktop'): continue
        print >> sys.stderr, fn, '->', desktop_dir
        data = tm.render(fn, ctx)
        fpath = os.path.join(desktop_dir, fn)
        with open(fpath, 'w') as fo:
            fo.write(data)
        os.chmod(fpath, 0755)
        trust_desktop_icon(fpath)
    #refresh_gnome()
    
def generate_matlab_projects():
    ctx = generate_context()
    tm = TemplateManager(None, templdir=MATLAB_DIR)
    for templ_fn in os.listdir(MATLAB_DIR):
        fn, ext = os.path.splitext(templ_fn)
        if ext != '.templ': continue
        fpath = os.path.join(MATLAB_DIR, fn)
        print >> sys.stderr, templ_fn, '->', fpath
        data = tm.render(templ_fn, ctx)
        with open(fpath, 'w') as fo:
            fo.write(data)
        
if __name__ == '__main__':
    create_desktop_icons()
    # generate_matlab_projects()
    rc = 0
    sys.exit(rc)
