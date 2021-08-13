
import os
from jinja2 import Template

class TemplateManager(object):
    def __init__(self, appdir, templdir=None):
        self.appdir = appdir
        self.templdir = os.path.join(appdir, 'templates') if templdir is None else templdir
        #
        self.msg = None
    def loadTemplate(self, name):
        self.msg = None
        fpath = os.path.join(self.templdir, name)
        try:
            with open(fpath, 'rb') as fi:
                res = fi.read()
            return res
        except Exception, ex:
            self.msg = str(ex)
        return None
    def render(self, name, data):
        templ = Template(self.loadTemplate(name))
        return templ.render(data)
#         if templ is None:
#             return None
#         _parts = templ.split('{{')
#         parts = None
#         for _p in _parts:
#             if parts is None:
#                 parts = [_p]
#                 continue
#             p = _p.split('}}')
#             if len(p) == 2:
#                 var = p[0].strip()
#                 parts.append(data.get(var, ''))
#                 parts.append(p[1])
#             else:
#                 parts.append(''.join(p))
#         return ''.join(parts)
            