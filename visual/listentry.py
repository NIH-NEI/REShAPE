import os, json
from .cellfeature import CELL_FEATURE_REMAP

CELL_FEATURE_MAP = dict([(n.replace('_', ' '), a) for n, a in CELL_FEATURE_REMAP])
CELL_ATTR_MAP = dict([(a, n.replace('_', ' ')) for n, a in CELL_FEATURE_REMAP])

def dblquot(s):
    return json.dumps(str(s))

class CellFeatureManager(object):
    EXPR_CHARS = '[]{}~^'
    exprMap = {}
    #
    @classmethod
    def to_expr(cls, val):
        if val is None:
            val = ''
        for c in cls.EXPR_CHARS:
            if c in val:
                return 'expression('+val+')'
        return dblquot(val)
    #
    def getLabel(self, feature):
        try:
            attr = CELL_FEATURE_MAP[feature]
            return self.exprMap[attr]
        except Exception:
            pass
        return feature
    #
    def getExpression(self, feature):
        try:
            attr = CELL_FEATURE_MAP[feature]
            return self.to_expr(self.exprMap[attr])
        except Exception:
            pass
        return dblquot(feature)
    #
    def setLabel(self, feature, expr):
        try:
            attr = CELL_FEATURE_MAP[feature]
            if not expr or expr == feature:
                del self.exprMap[attr]
            else:
                self.exprMap[attr] = expr
        except Exception:
            pass
    #
    
class ListFileEntry(object):
    def __init__(self, fullp, basename=None, attrib=None):
        self.fullpath = os.path.abspath(fullp)
        #
        self.attrib = {}
        if attrib:
            self.attrib.update(attrib)
        self.basename = os.path.basename(self.fullpath) if basename is None else basename
    def sortkey(self):
        return (self.basename, self.fullpath)
    def __eq__(self, other):
        return self.fullpath == other.fullpath
    def __ne__(self, other):
        return self.fullpath != other.fullpath
    def __hash__(self):
        return self.fullpath.__hash__()
    def __unicode__(self):
        return self.basename
    def __str__(self):
        return self.__unicode__()
    def __repr__(self):
        return self.__unicode__()
    
class FeatureEntry(object):
    def __init__(self, attr, name):
        self.attr = attr
        self.name = name
    def __unicode__(self):
        return self.name
    def __str__(self):
        return self.__unicode__()
    def __repr__(self):
        return self.__unicode__()
    
CELL_FEATURE_LIST = [FeatureEntry(v, n.replace('_', ' ')) for n,v in CELL_FEATURE_REMAP]

def listfiles(basedir, subdirs, filters, res=None):
    if res is None:
        res = []
    def accept(fn, filters):
        for n, v in filters.iteritems():
            if n == 'endswith' and not fn.endswith(v):
                return False
            elif n == 'contains' and not v in fn:
                return False
        return True
    try:
        for fn in os.listdir(basedir):
            fullp = os.path.join(basedir, fn)
            if os.path.isdir(fullp):
                if subdirs:
                    listfiles(fullp, subdirs, filters, res)
                continue
            if accept(fn, filters):
                res.append(ListFileEntry(fullp))
    except Exception:
        pass
    return res
