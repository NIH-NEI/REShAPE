import os, sys

VISUAL_CELL_FEATURES = [
    ("Area", "Area", 5., 2000.),
    ("Peri", "Perimeter", 5., 250.),
    ("AoP", "Area/Perimeter", 0.25, 10.),
    ("PoN", "Perimeter/Neighbors", 1., 200.),
    ("EllipMaj", "Ellipse Major Axis", 1., 100.),
    ("ElipMin", "Ellipse Minor Axis", 1., 100.),
    ("AR", "Ellipse Aspect Ratio", 1., 5.),
    ("Angle", "Ellipse Angle", 0., 180.),
    ("Feret", "Feret Major", 1., 100.),
    ("MinFeret", "Feret Minor", 1., 100.),
    ("FeretAR", "Feret Aspect Ratio", 1., 5.),
    ("FeretAngle", "Feret Angle", 0., 180.),
    ("Circ", "Circularity", 0., 1.),
    ("Solidity", "Solidity", 0., 1.),
    ("Compactness", "Compactness", 0., 1.),
    ("Extent", "Extent", 0., 1.),
    ("Poly_SR", "Polygon Side Ratio", 0., 1.),
    ("Poly_AR", "Polygon Area Ratio", 0., 1.),
    ("Poly_Ave", "Polygonality Score", 0., 10.),
    ("Hex_SR", "Hexagon Side Ratio", 0., 1.),
    ("Hex_AR", "Hexagon Area Ratio", 0., 1.),
    ("Hex_Ave", "Hexagonality Score", 0., 10.),
]  

class ReshapeCellFeature(object):
    def __init__(self, name, label):
        self.name = name
        self.label = label
        #
        self.defMin = self.defMax = None
        self.minName = 'Min.'+self.name
        self.minValue = None
        self.maxName = 'Max.'+self.name
        self.maxValue = None
    #
    def store(self, data):
        if isinstance(self.minValue, (int, long, float)):
            data[self.minName] = self.minValue
        if isinstance(self.maxValue, (int, long, float)):
            data[self.maxName] = self.maxValue
    def load(self, data):
        try:
            self.minValue = float(data[self.minName])
        except Exception:
            self.minValue = None
        try:
            self.maxValue = float(data[self.maxName])
        except Exception:
            self.maxValue = None
    #
class ReshapeCellFeatureManager(object):
    def __init__(self):
        self.features = []
        self.feature_map = {}
        for name, label, _min, _max in VISUAL_CELL_FEATURES:
            f = ReshapeCellFeature(name, label)
            f.defMin = _min
            f.defMax = _max
            self.features.append(f)
            self.feature_map[name] = f
    #
    def load(self, data):
        for f in self.features:
            f.load(data)
    def store(self):
        data = {}
        for f in self.features:
            f.store(data)
        return data
    def loadDefaults(self):
        data = {}
        for f in self.features:
            if isinstance(f.defMin, (int, long, float)):
                data[f.minName] = f.defMin
            if isinstance(f.defMax, (int, long, float)):
                data[f.maxName] = f.defMax
        return data
    #
        
