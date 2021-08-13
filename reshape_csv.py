
import os, sys
import math, csv

CELL_FEATURE_REMAP = [
    ('Cell_Centroid_XCoord', 'X'), ('Cell_Centroid_YCoord', 'Y'), ('Cell_Area', 'Area'),
    ('Cell_Perimeter', 'Perim.'), ('Cell_Neighbors', 'Neighbors'), ('Area_Perimeter', 'Area/Perim.'),
    ('Perimeter_Neighbors', 'PoN'), ('Polygonality_Side_Ratio', 'PSR'),
    ('Polygonality_Area_Ratio', 'PAR'), ('Polygonality_Score', 'Poly_Ave'),
    ('Hexagonality_Side_Ratio', 'HSR'), ('Hexagonality_Area_Ratio', 'HAR'),
    ('Hexagonality_Score', 'Hex_Ave'), ('Major_Cell_Axis', 'Major'), ('Minor_Cell_Axis', 'Minor'),
    ('Aspect_Ratio', 'AR'), ('Angle_Major_Axis', 'Angle'), ('Ferets_Max_Diameter', 'Feret'),
    ('Ferets_Min_Diameter', 'MinFeret'), ('Ferets_Aspect_Ratio', 'Ferets AR'),
    ('Angle_Feret_Max', 'FeretAngle'), ('Cell_Bounding_Box_Width', 'Width'),
    ('Cell_Bounding_Box_Height', 'Height'), ('Cell_Circularity', 'Circ.'),
    ('Cell_Solidity', 'Solidity'), ('Cell_Compactness', 'Compactness'), ('Cell_Extent', 'Extent')
]

class Summarizer(object):
    def __init__(self):
        self.data = [[] for x in CELL_FEATURE_REMAP]
        self.nanset = set()
    #
    def dec3val(self, v, i):
        try:
            x = float(v)
            if math.isnan(x):
                self.nanset.add(i)
                return v
            self.data[i].append(x)
            return '%3.3f' % x
        except Exception:
            return v
    #
    @staticmethod
    def mean_stdev(lst):
        if not lst:
            return 'NaN', 'NaN'
        mean = sum(lst) / len(lst)
        if len(lst) > 1:
            stdev = math.sqrt(sum((x-mean)*(x-mean) for x in lst) / (len(lst) - 1))
            _stdev = '%3.3f' % stdev
        else:
            _stdev = 'NaN'
        return '%3.3f' % mean, _stdev
    #
    @staticmethod
    def min_val(lst):
        if not lst:
            return '1.798E308'
        return '%3.3f' % min(lst)
    #
    @staticmethod
    def max_val(lst):
        if not lst:
            return '-1.798E308'
        return '%3.3f' % max(lst)
    #
    def __iter__(self):
        mean_row = [None, 'Mean']
        stdev_row = [None, 'SD']
        min_row = [None, 'Min']
        max_row = [None, 'Max']
        for i, lst in enumerate(self.data):
            if i in self.nanset:
                mean = stdev = 'NaN'
            else:
                mean, stdev = self.mean_stdev(lst)
            mean_row.append(mean)
            stdev_row.append(stdev)
            min_row.append(self.min_val(lst))
            max_row.append(self.max_val(lst))
        mean_row.append('NaN')
        stdev_row.append('NaN')
        min_row.append('1.798E308')
        max_row.append('-1.798E308')
        yield mean_row
        yield stdev_row
        yield min_row
        yield max_row
#

class CellDataReader(object):
    @staticmethod
    def dec3val(v):
        try:
            x = float(v)
            return '%3.3f' % x
        except Exception:
            return v
    def __init__(self, csvname, summ):
        self.csvname = csvname
        self.summ = summ
        #
        self.fn = os.path.basename(csvname)
        self.headers = None
        self.header_map = {}
        self.remap = []
        self.csvfile = None
        self.rdr = None
    #
    def __enter__(self):
        try:
            self.csvfile = open(self.csvname, 'rb')
            self.rdr = csv.reader(self.csvfile)
            self.headers = headers = self.rdr.next()
            for idx, hdr in enumerate(headers):
                self.header_map[hdr] = idx
            self.remap = [self.header_map[src] for tgt,src in CELL_FEATURE_REMAP]
        except Exception:
            self.csvfile = None
    #
    def __exit__(self, exc_type, exc_value, traceback):
        if self.csvfile:
            self.csvfile.close()
            self.csvfile = None
    #
    def __iter__(self):
        if self.csvfile is None:
            return
        for src_row in self.rdr:
            try:
                tgt_row = [None, ""] + [self.summ.dec3val(src_row[idx], i) for i,idx in enumerate(self.remap)] + [self.fn]
            except Exception, ex:
                print ex
                print 'idx:', idx
                print src_row
            yield tgt_row

class NeighborReader(object):
    def __init__(self, csvname):
        self.csvname = csvname
        #
        self.headers = []
        self.idxn = 1
        self.idxs = 2
        self.imgname = None
        self.data = {}
        self.max_n = 0
        #
        self.read()
    #
    def read(self):
        with open(self.csvname, 'rb') as csvfile:
            rdr = csv.reader(csvfile)
            for row in rdr:
                if self.imgname is None:
                    self.headers = row
                    self.imgname = row[self.idxs]
                    continue
                try:
                    n = int(row[self.idxn])
                    s = int(row[self.idxs])
                    self.data[n] = s
                    if self.max_n < n:
                        self.max_n = n
                except Exception:
                    pass
    #
    def get(self, n):
        return self.data.get(n, 0)
    
# class HeaderedCsvReader(object):
#     def __init__(self, csv_name):
#         self.csv_name = csv_name
#         #
#         self.csvfile = None
#         self.rdr = None
#         self.headers = []
#         self.header_map = {}
#     def __enter__(self):
#         self.csvfile = open(self.csvname, 'rb')
#         self.rdr = csv.reader(self.csvfile)
#         self.headers = headers = self.rdr.next()
#         self.header_map.clear()
#         for idx, hdr in enumerate(headers):
#             self.header_map[hdr] = idx
#     def __exit__(self, exc_type, exc_value, traceback):
#         if self.csvfile:
#             self.csvfile.close()
#             self.csvfile = None
#     def getval(self, row, hdname):
#         if hdname in self.header_map:
#             return row[self.header_map[hdname]]
#         return None
#     def getint(self, row, hdname):
#         try:
#             return int(self.getval(row, hdname))
#         except Exception:
#             return None
#     def getfloat(self, row, hdname):
#         try:
#             return float(self.getval(row, hdname))
#         except Exception:
#             return None

class ReshapeCsv(object):
    NAME_NEIGHBORS = 'All_Neighbor_Values.csv'
    NAME_DATA = 'All Cell Metric Values.csv'
    #
    def __init__(self, tgt_dir):
        self.tgt_dir = tgt_dir
    #
    def combineNeighbors(self, src_list):
        nb_data = [NeighborReader(fn) for fn in src_list]
        max_n = max([nb.max_n for nb in nb_data]) + 1
        headers = [" ", "Neighbors"] + [nb.imgname for nb in nb_data] + ["Sum of Frequencies"]
        tgtfile = os.path.join(self.tgt_dir, self.NAME_NEIGHBORS)
        with open(tgtfile, "wb") as csvfile:
            wr = csv.writer(csvfile)
            wr.writerow(headers)
            for n in range(max_n):
                id = n+1
                row = [id, n]
                total = 0
                for nb in nb_data:
                    cnt = nb.get(n)
                    total += cnt
                    row.append(cnt)
                row.append(total)
                wr.writerow(row)
        return tgtfile
    #
    def combineData(self, src_list):
        summ = Summarizer()
        headers = [" ", "Label"] + [tgt for tgt, src in CELL_FEATURE_REMAP] + ["File_Name"]
        tgtfile = os.path.join(self.tgt_dir, self.NAME_DATA)
        with open(tgtfile, "wb") as csvfile:
            wr = csv.writer(csvfile)
            wr.writerow(headers)
            id = 1
            for csvname in src_list:
                rows = CellDataReader(csvname, summ)
                with rows:
                    for row in rows:
                        row[0] = id
                        id += 1
                        wr.writerow(row)
            # Write summary
            for row in summ:
                row[0] = id
                id += 1
                wr.writerow(row)
        return tgtfile

if __name__ == '__main__':
    tgt_dir = 'C:\\AVProjects\\TestData\\Processed\\Combined Files'
    src_dir = 'C:\\AVProjects\\TestData\\Processed\\Analysis'

    src_n_list = []
    src_d_list = []
    for fn in os.listdir(src_dir):
        if fn.endswith('_Neighbors.csv'):
            src_n_list.append(os.path.join(src_dir, fn))
        elif fn.endswith('_Data.csv'):
            src_d_list.append(os.path.join(src_dir, fn))
            
    cvt = ReshapeCsv(tgt_dir)
    print cvt.combineNeighbors(src_n_list)
    print cvt.combineData(src_d_list)
    
    sys.exit(0)

