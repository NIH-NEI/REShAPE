#ifndef rscsvio_h
#define rscsvio_h

#include <iostream>
#include <itksys/SystemTools.hxx>
#include <itkCSVFileReaderBase.h>

#include "rsparticle.h"

typedef std::vector<std::string> rsCsvRow;
typedef std::vector<rsCsvRow> rsCsvData;

class rsCsvIO : public itk::CSVFileReaderBase
{
private:
	itk::SizeValueType rows;
	itk::SizeValueType columns;
	std::string error;
	rsCsvRow headers;
	rsCsvData data;
	rsCsvRow *currentRow;
	std::string dformat;

	void ReadNextRow(rsCsvRow & row);
	void PrintARow(rsCsvRow & row, std::ostream & out);
protected:
	rsCsvIO();
	virtual ~rsCsvIO() ITK_OVERRIDE {}

public:
	typedef rsCsvIO		Self;
	typedef itk::SmartPointer<Self>		Pointer;
	typedef itk::SmartPointer<const Self> ConstPointer;

	/** Standard New method. */
	itkNewMacro(Self);

	/** Run-time type information (and related methods) */
	itkTypeMacro(rsCsvIO, CSVFileReaderBase);

	virtual void Parse() ITK_OVERRIDE;

	int Read(const char *file_name) { SetFileName(file_name); Parse(); return Fail(); }
	int Read(std::string & file_name) { return Read(file_name.c_str()); }
	int Write(const char *file_name);
	int Write(std::string & file_name) { return Write(file_name.c_str()); }
	int Fail() { return ! this->error.empty(); }
	std::string & GetErrorMessage() { return error; }
	std::string & GetFileName() { return this->m_FileName; }
	uint64 GetRowCount() { return (uint64)rows; }
	uint64 GetColumnCount() { return (uint64)columns; }
	rsCsvRow & GetHeaders() { return headers; }
	int GetHeaderIndex(std::string & label);
	int GetHeaderIndex(const char *label) { std::string tmp(label); return GetHeaderIndex(tmp); }
	rsCsvRow & GetRow(uint64 iRow);
	void PrintHeaders(std::ostream & out) { PrintARow(headers, out); }
	void PrintCurrentRow(std::ostream & out) { if (currentRow) PrintARow(*currentRow, out);	}
	std::string & GetValueAt(int idx);
	int GetIntAt(int idx, int dflt=0);
	double GetDoubleAt(int idx);
	std::string & GetDoubleFormat() { return dformat; }
	void SetDoubleFormat(std::string & dformat) { this->dformat = dformat; }
	void SetDoubleFormat(const char *dformat) { this->dformat = dformat; }
	void AppendHeader(std::string & label) { headers.push_back(label); }
	void AppendHeader(const char *label) { headers.push_back(label); }
	rsCsvRow & AppendRow(rsCsvRow & row);
	void AppendValue(std::string & val) { if (currentRow) currentRow->push_back(val); }
	void AppendValue(const char *val) { if (currentRow) currentRow->push_back(val); }
	void AppendInt(int v) { if (currentRow) currentRow->push_back(std::to_string(v)); }
	void AppendDouble(double v);
	void SetDoubleAt(int idx, double v);
	void SetIntAt(int idx, int v);
};

std::string csv_quote(std::string &src, char sc=',', char qc = '\"', char ec = '\\');

#endif
