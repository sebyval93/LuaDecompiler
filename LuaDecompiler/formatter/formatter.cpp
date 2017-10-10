#include "formatter.h"

Formatter::Formatter()
	: m_indent(0), m_paran(0), m_outputParan(false), m_withinTable(false)
{}

Formatter & Formatter::getInstance()
{
	static Formatter instance;				 
	return instance;
}

void Formatter::reset()
{
	m_indent = 0;
	m_paran = 0;
	m_outputParan = false;
	m_withinTable = false;
	m_formattedStr.clear();
}

void Formatter::increaseIndent()
{
	++m_indent;
}

void Formatter::decreaseIndent()
{
	--m_indent;
}

void Formatter::increaseParan()
{
	++m_paran;
}

void Formatter::decreaseParan()
{
	--m_paran;
}

void Formatter::setOutputParan(bool outputParan)
{
	m_outputParan = outputParan;
}

void Formatter::setWithinTable(bool withinTable)
{
	m_withinTable = withinTable;
}

void Formatter::removeLastChar()
{
	m_formattedStr.pop_back();
}

bool Formatter::outputParan()
{
	return m_outputParan;
}

bool Formatter::isWithinTable()
{
	return m_withinTable;
}

std::string& Formatter::appendStr(std::string& str)
{
	return m_formattedStr.append(str);
}

std::string& Formatter::appendStr(const char* str)
{
	return m_formattedStr.append(str);
}

std::string Formatter::generateIndent()
{
	std::string result;
	if (m_indent > 0)
		result.insert(0, m_indent, '\t');

	return result;
}

std::string& Formatter::getFormattedStr()
{
	return m_formattedStr;
}
