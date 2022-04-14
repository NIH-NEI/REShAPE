#ifndef csv_h
#define csv_h

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

typedef std::vector<std::string> CsvRow;
const double _NaN_ = std::nan(" ");

void parse_csv_string(CsvRow &crow, const char *sbuf, char delim=',', char quote='\"', char esc='\\');
std::string csv_quote(const std::string &src, char delim=',', char quote='\"', char esc='\\');

inline std::string csv_double(double v, const char *dformat="%lg")
{
	std::string s;
	if (std::isnan(v))
		s = "NaN";
	else {
		s.resize(256);
		auto len = std::snprintf(&s[0], s.size(), dformat, v);
		s.resize(len);
	}
	return s;
}

class CsvReader
{
protected:
	std::ifstream fin;
	bool at_eof;
	char *sbuf = NULL;
	size_t bufsize;
	int line;
	CsvRow headers;
	bool headers_set;
	//
	CsvRow crow;
public:
	char delim;
	char esc;
	char quote;
	
	CsvReader(const char *filename, char _delim=',', char _esc='\\', char _quote='"', size_t _bufsize=4096) :
		fin(filename, std::ios::in), bufsize(_bufsize), delim(_delim), esc(_esc), quote(_quote)
	{
		line = 0;
		at_eof = true;
		headers_set = false;
		if (fin.is_open()) {
			sbuf = new char[bufsize];
			at_eof = false;
		}
	}
	virtual ~CsvReader() { fin.close(); if (sbuf) delete [] sbuf; }
	void setHeaders(CsvRow &_headers) {
		headers.clear();
		for (auto &h : _headers) headers.push_back(h);
		headers_set = true;
	}
	int header_index(const char *hdr) {
		if (!headers_set) return -1;
		for (int i=0; size_t(i)<headers.size(); i++) {
			if (!strcmp(headers[i].c_str(), hdr)) return i;
		}
		return -1;
	}
	CsvRow GetRow() {
		CsvRow res;
		for (auto &s : crow) res.push_back(s);
		return res;
	}
	int GetLine() { return line; }
	const char *GetValue(int idx, const char *dflt=NULL) {
		if (idx<0 || size_t(idx)>=crow.size()) return dflt;
		return crow[idx].c_str();
	}
	int GetInt(int idx, int dflt=0) {
		if (idx<0 || size_t(idx)>=crow.size()) return dflt;
		int res;
		if (sscanf(crow[idx].c_str(), "%d", &res) == 1) return res;
		return dflt;
	}
	double GetDouble(int idx, double dflt=_NaN_) {
		if (idx<0 || size_t(idx)>=crow.size()) return dflt;
		double res;
		if (sscanf(crow[idx].c_str(), "%lf", &res) == 1) return res;
		return dflt;
	}
	//
	bool next();
};

class CsvWriter
{
protected:
	std::filebuf fb;
	std::ostream * pfos;
	//
	std::string sdfmt;
	CsvRow crow;
	bool modified = false;
	int line;
public:
	char delim;
	char esc;
	char quote;
	
	CsvWriter(const char *filename, char _delim=',', char _esc='\\', char _quote='"') :
		pfos(NULL), sdfmt("%lg"), delim(_delim), esc(_esc), quote(_quote)
	{
		line = 0;
		fb.open(filename, std::ios::out);
		if (fb.is_open()) {
			pfos = new std::ostream(&fb);
		}
	}
	virtual ~CsvWriter() { close(); }
	bool is_open() { return pfos != NULL; }
	void close() {
		if (pfos) {
			next();
			fb.close();
			delete pfos;
			pfos = NULL;
		}
	}
	int GetLine() { return line; }
	void SetDoubleFormat(const char *dfmt) {
		sdfmt = dfmt;
	}
	void SetRow(const CsvRow& row) {
		crow.resize(row.size());
		for (int i = 0; size_t(i) < crow.size(); i++)
			crow[i] = csv_quote(row[i], delim, quote, esc);
		modified = true;
	}
	void append(const char *v) {
		std::string s = v;
		crow.push_back(csv_quote(s, delim, quote, esc));
		modified = true;
	}
	void append(std::string &s) {
		crow.push_back(csv_quote(s, delim, quote, esc));
		modified = true;
	}
	void append(int v) {
		crow.push_back(std::to_string(v));
		modified = true;
	}
	void append(double v, const char *dfmt=NULL) {
		if (!dfmt) dfmt = sdfmt.c_str();
		crow.push_back(csv_double(v, dfmt));
		modified = true;
	}
	bool WriteRow(const CsvRow& row);
	bool next();
};

#endif
