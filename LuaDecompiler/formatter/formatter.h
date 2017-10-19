#pragma once
#include <string>

class Formatter
{
public:
	// singleton accessor
	static Formatter& getInstance();

	// prevent copying
	Formatter(Formatter const&) = delete;
	void operator=(Formatter const&) = delete;

	// reset internal state, used when we finished
	//  formatting a file
	void reset();

	// write as-is
	void comment(std::string text, bool multiLine);
	void string(std::string text);
	void comma(std::string text);
	void semicolon(std::string text);

	// write as-is, then increase indent
	void functionStart(std::string text);
	void conditionStart(std::string text);
	void forLoopStart(std::string text);

	// remove tab from input, write, reduce indent
	void blockEnd(std::string text);
	
	// increase indent
	void tableStart(std::string text);

	// decrease indent
	// additionally, if we have multiple closing parans in input,
	//  write them as-is
	void tableEnd(std::string text);

	void newLine(std::string text);

	// write as-is
	void anyChar(std::string text);

	// the scanner needs to know
	bool isWithinTable() const;

	std::string& getFormattedStr();

private:
	// prevent outside instantiation
	Formatter();

	bool outputParan();

	void increaseIndent();
	void decreaseIndent();
	void increaseTableDepth();
	void decreaseTableDepth();
	void setWithinTable(bool withinTable);

	void removeLastChar();

	std::string& appendStr(std::string& str);
	std::string& appendStr(const char* str);
	std::string generateIndent();

	int m_indent;
	int m_tableDepth;
	bool m_outputParan;
	bool m_withinTable;
	std::string m_formattedStr;
	std::string m_currIndent;

};