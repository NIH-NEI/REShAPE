
import sys, os, json

MY_NAME = os.path.basename(__file__)

def patch(vl_compilenn):
    backup = vl_compilenn+'.bak'
    temp = vl_compilenn+'.tmp'
    lines = []
    try:
        with open(vl_compilenn, 'r') as fi:
            ln = fi.readline()
            while ln:
                lines.append(ln)
                ln = fi.readline()
    except Exception, ex:
        return 1, 'Error reading %s: %s' % (vl_compilenn, str(ex))
    lastln = -1
    for i, _ln in enumerate(lines):
        ln = _ln.strip().replace(' ', '').replace('\t', '')
        if ln.startswith('opts.mexFlags='):
            return 0, 'Seems to be patched already: %s' % (vl_compilenn,)
        if ln.startswith('opts=vl_argparse('):
            lastln = i
            break
    if lastln < 0:
        return 1, 'Seems to be invalid:', vl_compilenn
    lines.insert(i, "\n")
    lines.insert(i, "opts.mexFlags = {'-largeArrayDims'};\n")
    lines.insert(i, "% <### Inserted by "+MY_NAME+" ###>\n")
    lines.insert(i, "\n")
    
    i += 4
    while i < len(lines):
        ln = lines[i].strip().replace(' ', '').replace('\t', '')
        if ln.startswith('flags.mex') and "'-largeArrayDims'" in ln:
            try:
                sarr = ln.split('{')[1].split('}')[0].replace("'", '"')
                arr = json.loads('['+sarr+']')
                del arr[arr.index('-largeArrayDims')]
                ln = lines[i]
                lines[i] = '% ' + ln
                newln = ln.split('=')[0]
                if not arr:
                    newln = newln + '= opts.mexFlags;\n'
                else:
                    arr = ["'"+p+"'" for p in arr]
                    newln = newln + '= horzcat(opts.mexFlags, {' + ', '.join(arr) + '});\n'
                lines.insert(i+1, '\n')
                lines.insert(i+1, newln)
                lines.insert(i, "% <### Changed by "+MY_NAME+" ###>\n")
                lines.insert(i, '\n')
                i += 5
            except Exception, ex:
                i += 1
        else:
            i += 1
    
    try:
        with open(temp, 'w') as fo:
            for ln in lines:
                fo.write(ln)
    except Exception, ex:
        return 1, "Can't write temp file %s: %s" % (temp, str(ex))
    
    if os.path.exists(backup):
        try:
            os.remove(backup)
        except Exception, ex:
            return 1, "Can't delete backup file %s: %s" % (backup, str(ex))
    try:
        os.rename(vl_compilenn, backup)
        os.rename(temp, vl_compilenn)
    except Exception, ex:
        return 1, "Can't rename %s -> %s: %s" (os.path.basename(temp), vl_compilenn, str(ex))
    
    return 0, 'OK'    

if __name__ == '__main__':
    vl_compilenn = 'C:\\AVProjects\\MCN\\Segmentation\\matconvnet\\matlab\\vl_compilenn.m'
    c, msg = patch(vl_compilenn)
    if c:
        print 'ERROR:', msg
    else:
        print msg
    sys.exit(c)
