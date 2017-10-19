#include "formatter.h"
#include <algorithm>

Formatter::Formatter()
	: m_indent(0), m_tableDepth(0), m_outputParan(false), m_withinTable(false)
{}

Formatter & Formatter::getInstance()
{
	static Formatter instance;				 
	return instance;
}

void Formatter::reset()
{
	m_indent = 0;
	m_tableDepth = 0;
	m_outputParan = false;
	m_withinTable = false;
	m_formattedStr.clear();
}

void Formatter::comment(std::string text, bool multiLine)
{
	m_formattedStr.append(text);
}

void Formatter::string(std::string text)
{
	m_formattedStr.append(text);
}

void Formatter::comma(std::string text)
{
	m_formattedStr.append(text);
}

void Formatter::semicolon(std::string text)
{
	m_formattedStr.append(text);
}

void Formatter::functionStart(std::string text)
{
	increaseIndent();
	m_formattedStr.append(text);
}

void Formatter::conditionStart(std::string text)
{
	increaseIndent();
	m_formattedStr.append(text);
}

void Formatter::forLoopStart(std::string text)
{
	increaseIndent();
	m_formattedStr.append(text);
}

void Formatter::blockEnd(std::string text)
{
	decreaseIndent();
	removeLastChar();
	m_formattedStr.append(text).append("\n");
}

void Formatter::tableStart(std::string text)
{
	m_formattedStr.append("\n" + m_currIndent + text);
	increaseIndent();
	increaseTableDepth();
}

void Formatter::increaseIndent()
{
	++m_indent;
	m_currIndent = "";
	m_currIndent.insert(0, m_indent, '\t');
}

void Formatter::decreaseIndent()
{
	--m_indent;
	m_currIndent = "";
	m_currIndent.insert(0, m_indent, '\t');
}

void Formatter::increaseTableDepth()
{
	++m_tableDepth;
	setWithinTable(true);
}

void Formatter::decreaseTableDepth()
{
	--m_tableDepth;
	if (m_tableDepth == 0)
		setWithinTable(false);
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

void Formatter::tableEnd(std::string text)
{
	decreaseIndent();
	decreaseTableDepth();
	std::string resultStr(text);

	// check if we have an extra paran. if so, erase one to simplify the syntax
	size_t numParans = std::count(resultStr.begin(), resultStr.end(), ')');
	if (numParans > 1)
	{
		resultStr.erase(1, 1);
		// also append a newline if the str has a period ','
		if (resultStr.find(',') != std::string::npos)
			resultStr.append("\n");
	}

	// remove any whitespace
	resultStr.erase(std::remove(resultStr.begin(), resultStr.end(), ' '), resultStr.end());

	m_formattedStr.append("\n" + m_currIndent + resultStr + m_currIndent);
}

void Formatter::newLine(std::string text)
{
	m_formattedStr.append("\n").append(m_currIndent);
}

void Formatter::anyChar(std::string text)
{
	m_formattedStr.append(text);
}

bool Formatter::isWithinTable() const
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
