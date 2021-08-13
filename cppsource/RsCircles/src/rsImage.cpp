#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "rsimage.h"
// #include "rscsvio.h"

#define IABS(a) ((a)<0 ? (-(a)) : (a))

struct CSVSM {
	char sc, qc, ec;
	CsvRow *prow;
	std::string *cstr;
	char wt;
	bool at_start;
	bool at_eol;

	int st;
	CSVSM(CsvRow &_row, char sc, char qc, char ec) {
		prow = &_row;
		this->sc = sc;
		this->qc = qc;
		this->ec = ec;
		flush();
		st = 0;
	}
	void flush() {
		prow->push_back(std::string(""));
		cstr = &(prow->at(prow->size() - 1));
		wt = 0;
		at_start = true;
		at_eol = false;
		st = 0;
	}
	void putc(char c) {
		if (st == 0) csv_plain(c);
		else csv_quoted(c);
	}
	void csv_plain(char c) {
		if (wt != 0) {
			if (wt != '\r' && c != '\n')
				cstr->push_back(wt);
			wt = 0;
			at_start = false;
		}
		if (c == '\r') {
			wt = c;
			return;
		}
		else if (c == '\n') {
			at_eol = true;
			return;
		}
		else if (c == sc) {
			flush();
			return;
		}
		else if (c == qc && at_start) {
			st = 1;
			at_start = false;
			return;
		}
		cstr->push_back(c);
		at_start = false;
	}
	void csv_quoted(char c) {
		if (wt != 0) {
			if (wt == ec) {
				if (!(c == ec || c == qc || c == sc))
					cstr->push_back(wt);
				wt = 0;
				cstr->push_back(c);
				return;
			}
			else if (wt == qc) {
				if (c == sc) {
					flush();
					return;
				}
				if (c != qc)
					cstr->push_back(wt);
				wt = 0;
				cstr->push_back(c);
				return;
			}
			else {
				cstr->push_back(wt);
				wt = 0;
			}
		}
		if (c == qc || c == ec) wt = c;
		else cstr->push_back(c);
	}
};

int CsvFrame::readline(CsvRow &row)
{
	if (cpos >= len) return 0;
	row.resize(0);
	rowpos.push_back(cpos);
	CSVSM sm(row, sc, qc, ec);
	while (cpos < len) {
		sm.putc(buf[cpos]);
		++cpos;
		if (sm.at_eol) break;
	}
	return 1;
}

int CsvFrame::open(const char *fname)
{
	std::ifstream fi(fname, std::ios::in | std::ios::binary | std::ios::ate);
	if (fi.is_open()) {
		close();
		len = (size_t)fi.tellg();
		buf = new char[len];
		fi.seekg(0, std::ios::beg);
		fi.read(buf, len);
		fi.close();
		if (!readline(headers)) return 0;
		return 1;
	}
	return 0;
}

static inline int ParseInt(std::string s) {
	int res = 0;
	sscanf(s.c_str(), "%d", &res);
	return res;
}

static inline double ParseDouble(std::string s) {
	double res = NaN;
	sscanf(s.c_str(), "%lf", &res);
	return res;
}

int ParticleReader::ReadParticles(const char *fname, std::vector<rsParticle> & particles)
{
	if (!open(fname)) return 0;
	particles.resize(0);
	maxxm = maxym = 0.;
	hdrlen = cpos;

	int iXM = GetHeaderIndex("XM");
	int iYM = GetHeaderIndex("YM");
	int iNeighbors = GetHeaderIndex("Neighbors");
	if (iXM < 0 || iYM < 0 || iNeighbors < 0) {
		close();
		return 0;
	}
	int maxI = 0;
	if (iXM > maxI) maxI = iXM;
	if (iYM > maxI) maxI = iYM;
	if (iNeighbors > maxI) maxI = iNeighbors;
	size_t _numVisualParams = numVisualParams - 1;
	hidx.resize(_numVisualParams);
	for (size_t i = 0; i < _numVisualParams; i++) {
		int idx = GetHeaderIndex(visualParams[i].header);
		if (idx < 0) {
			close();
			return 0;
		}
		if (idx > maxI) maxI = idx;
		hidx[i] = idx;
	}

	CsvRow row;
	size_t startpos = cpos;
	while (readline(row)) {
		if (maxI >= row.size()) {
			startpos = cpos;
			continue;
		}
		size_t pidx = particles.size();
		particles.resize(pidx + 1);
		rsParticle & pt = particles[pidx];
		pt.p.selected = 0.;

		pt.startpos = startpos;
		startpos = pt.endpos = cpos;
		pt.p.xm = ParseDouble(row[iXM]);
		if (pt.p.xm > maxxm) maxxm = pt.p.xm;
		pt.p.ym = ParseDouble(row[iYM]);
		if (pt.p.ym > maxym) maxym = pt.p.ym;
		pt.p.neighbors = ParseInt(row[iNeighbors]);
		for (size_t i = 0; i < _numVisualParams; i++) {
			int ipos = hidx[i];
			size_t poff = visualParams[i].off;
			*(pt.parPtr(poff)) = ParseDouble(row[ipos]);
		}
	}
	return 1;
}