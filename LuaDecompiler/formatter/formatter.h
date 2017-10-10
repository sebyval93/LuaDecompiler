#pragma once
#include <string>

class Formatter
{
public:
	// singleton getInstance
	static Formatter& getInstance();

	int m_indent;
	int m_paran;
	bool m_outputParan;
	bool m_withinTable;
	std::string m_formattedStr;

	// prevent copying
	Formatter(Formatter const&) = delete;
	void operator=(Formatter const&) = delete;

	void reset();

	void increaseIndent();
	void decreaseIndent();
	void increaseParan();
	void decreaseParan();
	void setOutputParan(bool outputParan);
	void setWithinTable(bool withinTable);

	void removeLastChar();

	bool outputParan();
	bool isWithinTable();

	std::string& appendStr(std::string& str);
	std::string& appendStr(const char* str);
	std::string generateIndent();
	std::string& getFormattedStr();

private:
	// prevent outside instantiation
	Formatter();


};