
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "csv.h"

void parse_csv_string(CsvRow &crow, const char *sbuf, char delim, char quote, char esc)
{
	const char *p = sbuf;
	crow.resize(crow.size()+1);
	std::string *t = & crow[crow.size()-1];
	bool in_quote = false;
	bool in_esc = false;
	while(*p) {
		char c = *p++;
		if (in_esc) {
			t->append(1, c);
			in_esc = false;
			continue;
		}
		if (c == esc) {
			in_esc = true;
			continue;
		}
		if (in_quote) {
			if (c == quote) {
				in_quote = false;
				continue;
			}
			t->append(1, c);
			continue;
		}
		if (c == quote) {
			in_quote = true;
			continue;
		}
		if (c == delim) {
			crow.resize(crow.size()+1);
			t = & crow[crow.size()-1];
			continue;
		}
		t->append(1, c);
	}
}

std::string csv_quote(const std::string &src, char sc, char qc, char ec)
{
	std::string res;
	res.append(1, qc);
	int n = 0;
	for (char c : src) {
		if (c == sc) {
			++n;
		} else if (c == qc || c == ec) {
			res.append(1, ec);
			++n;
		}
		res.append(1, c);
	}
	res.append(1, qc);
	if (n>0)
		return res;
	return src;
}

//-------------------------------- CsvReader --------------------------------

bool CsvReader::next()
{
	crow.clear();
	if (at_eof) return false;
	fin.getline(sbuf, bufsize);
	if (fin.fail()) {
		at_eof = true;
		return false;
	}
	parse_csv_string(crow, sbuf, delim, quote, esc);
	++line;
	if (fin.eof()) at_eof = true;
	return true;
}

//-------------------------------- CsvWriter --------------------------------

bool CsvWriter::WriteRow(const CsvRow& row)
{
	if (!pfos) return false;
	std::ostream& fos = *pfos;
	bool first = true;
	for (const std::string& s : row) {
		if (first) first = false;
		else fos << delim;
		fos << s;
	}
	fos << std::endl;
	++line;
	return true;
}

bool CsvWriter::next()
{
	if (!modified) return is_open();
	modified = false;
	if (!pfos) {
		crow.clear();
		return false;
	}
	std::ostream & fos = *pfos;
	bool first = true;
	for (std::string &s : crow) {
		if (first) first = false;
		else fos << delim;
		fos << s;
	}
	fos << std::endl;
	++line;
	crow.clear();
	return true;
}

