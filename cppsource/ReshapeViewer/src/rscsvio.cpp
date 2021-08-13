
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include "rscsvio.h"

rsCsvIO::rsCsvIO()
{
	this->SetFieldDelimiterCharacter(',');
	this->SetStringDelimiterCharacter('"');
	this->SetHasColumnHeaders(true);
	this->SetHasRowHeaders(false);
	this->UseStringDelimiterCharacterOff();
	this->currentRow = NULL;
	this->dformat = "%lf";
}

int rsCsvIO::GetHeaderIndex(std::string & label)
{
	for (size_t i = 0; i < headers.size(); i++)
		if (headers[i] == label) return (int)i;
	return -1;
}

void rsCsvIO::ReadNextRow(rsCsvRow & row)
{
	std::string entry;
	row.clear();
	for (uint64 i = 0; i < columns; i++) {
		this->GetNextField(entry);
		row.push_back(entry);
	}
}

rsCsvRow & rsCsvIO::GetRow(uint64 iRow)
{
	rsCsvRow & cRow = data[iRow];
	currentRow = &cRow;
	return cRow;
}

void rsCsvIO::PrintARow(rsCsvRow & row, std::ostream & out)
{
	int first = 1;
	for (auto &x : row) {
		if (first) first = 0;
		else out << this->GetFieldDelimiterCharacter();
		out << x;
	}
	out << std::endl;
}

std::string & rsCsvIO::GetValueAt(int idx)
{
	static std::string empty;
	if (currentRow == NULL || idx < 0 || (uint64)idx >= currentRow->size())
		return empty;
	return currentRow->at((size_t)idx);
}

int rsCsvIO::GetIntAt(int idx, int dflt)
{
	int res;
	if (sscanf(GetValueAt(idx).c_str(), "%d", &res) == 1)
		return res;
	return dflt;
}

double rsCsvIO::GetDoubleAt(int idx)
{
	double res;
	if (sscanf(GetValueAt(idx).c_str(), "%lf", &res) != 1)
		res = NaN;
	return res;
}

static void double_to_string(double v, const char *dformat, std::string & s)
{
	if (std::isnan(v))
		s = "NaN";
	else {
		s.resize(64);
		auto len = std::snprintf(&s[0], s.size(), dformat, v);
		s.resize(len);
	}
}

rsCsvRow & rsCsvIO::AppendRow(rsCsvRow & row)
{
	data.push_back(row);
	return GetRow(data.size() - 1);
}

void rsCsvIO::AppendDouble(double v)
{
	if (!currentRow) return;
	std::string tmp;
	double_to_string(v, dformat.c_str(), tmp);
	currentRow->push_back(tmp);
}

void rsCsvIO::SetDoubleAt(int idx, double v)
{
	if (!currentRow) return;
	std::string & tmp = currentRow->at(idx);
	tmp.resize(0);
	double_to_string(v, dformat.c_str(), tmp);
}

void rsCsvIO::SetIntAt(int idx, int v)
{
	if (!currentRow) return;
	currentRow->at(idx) = std::to_string(v);
}

void rsCsvIO::Parse()
{
	rows = 0;
	columns = 0;
	error.clear();
	data.clear();

	this->PrepareForParsing();

	this->m_InputStream.clear();
	this->m_InputStream.open(this->m_FileName.c_str());
	if (this->m_InputStream.fail())
	{
		error = itksys::SystemTools::GetLastSystemError();
//		std::cout << "ERROR reading <<" << this->m_FileName << ">> : " << error << std::endl;
	}

	// Get the data dimension and set the matrix size
	this->GetDataDimension(rows, columns);

	if (this->m_HasRowHeaders)
		++columns;

	if (this->m_HasColumnHeaders)
		ReadNextRow(headers);

	rsCsvRow row;
	for (uint64 i = 0; i < rows; i++)
	{
		ReadNextRow(row);
		data.push_back(row);
	}

	this->m_InputStream.close();
}

int rsCsvIO::Write(const char *file_name)
{
	SetFileName(file_name);
	std::ofstream fout(file_name);
	if (fout.is_open()) {
		if (this->m_HasColumnHeaders) {
			PrintHeaders(fout);
		}
		for (uint64 iRow = 0; iRow < data.size(); iRow++) {
			PrintARow(data[iRow], fout);
		}
	}
	else {
		error = "write error";
//		std::cout << "ERROR writing <<" << file_name << ">>" << std::endl;
		return 1;
	}
	return 0;
}

std::string csv_quote(std::string &src, char sc, char qc, char ec)
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

