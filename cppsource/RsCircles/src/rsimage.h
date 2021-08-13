#ifndef rsimage_h
#define rsimage_h

#include <stdio.h>
#include <QtGlobal>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <iostream>
#include <fstream>

#include <QTextStream>
#include "rsparticle.h"

typedef std::vector<std::string> CsvRow;

class CsvFrame
{
private:
	char *buf;
	size_t len;
	char sc, qc, ec;

	CsvRow headers;
	std::vector<size_t> rowpos;

protected:
	size_t cpos;
	int readline(CsvRow &row);
public:
	CsvFrame(char sc=',', char qc='"', char ec='\\')
	{
		buf = NULL;
		len = cpos = 0;
		this->sc = sc;
		this->qc = qc;
		this->ec = ec;
	}
	virtual ~CsvFrame()
	{
		close();
	}
	void close() {
		if (buf) delete[] buf;
		buf = NULL;
		len = cpos = 0;
	}
	int open(const char *fname);
	CsvRow & getHeaders() { return headers; }
	int GetHeaderIndex(const char *hdr) {
		std::string h(hdr);
		for (size_t i = 0; i < headers.size(); i++) {
			if (headers[i] == h) return (int)i;
		}
		return -1;
	}
	char *GetDataPointer(size_t off) { return buf + off; }
};

class ParticleReader : public CsvFrame {
private:
	std::vector<int> hidx;
	double maxxm, maxym;
	size_t hdrlen;
public:
	ParticleReader() : CsvFrame() {}
	virtual ~ParticleReader() {}
	int ReadParticles(const char *fname, std::vector<rsParticle> & particles);
	double GetMaxXm() { return maxxm; }
	double GetMaxYm() { return maxym; }
	size_t GetHeadersLen() { return hdrlen; }
};

#endif
