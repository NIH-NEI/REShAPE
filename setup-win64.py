
if __name__ == '__main__':

    try:
        import pip
    except ImportError:
        print 'Installing pip...'
        try:    
            get_pip = __import__('get-pip')
            get_pip.main()
        except SystemExit, ex:
            print 'Ignored sys.exit()'

    print 'Installing dependencies...'
    import os, sys, subprocess
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'Jinja2', 'Pillow'])
    
    sys.exit(0)
        
    