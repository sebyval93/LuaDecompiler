#pragma once
#include <unordered_map>
#include <string>
#include "formatter.h"
#include "llimits.h"

struct Proto;


class Decompiler
{
public:
	Decompiler();
	void processPath(std::string path);

private:
	enum ValueType { NONE, INT, STRING, STRING_PUSHSELF, STRING_GLOBAL, STRING_LOCAL, NIL, CLOSURE_STRING, TABLE_BRACE };

	Formatter& m_format;
	bool m_success;

	struct StackValue
	{
		std::string str;
		unsigned int index;
		int type;
	};

	struct FuncInfo
	{
		//std::string name;
		int index;
		int nLocals;
		int nForLoops;
		int nForLoopLevel;
		bool isMain;
		std::unordered_map<int, std::string> locals;
		std::unordered_map<int, std::string> upvalues;
	};

	struct CondElem
	{
		enum CondType
		{
			OR, AND, NONE
		};
		std::vector<std::string> args;
		int jmpType;
		int lineNum;
		int dest;
		CondType nextCond;
	};

	struct Context
	{
		enum ContextType
		{
			IF, WHILE
		};
		int dest;
		int strIndex;
		std::vector<CondElem> conds;
		ContextType type;
	};

	Proto* loadLuaStructure(const char* fileName);
	// returns end offs
	int evalCondition(std::string &funcStr, Instruction* p, Instruction* code, std::vector<Decompiler::StackValue> &codeStack, CondElem currentCond);
	int invertCond(int cnd);
	std::string decompileFile(const char* fileName);
	std::string decompileFunction(Proto* tf, FuncInfo &funcInfo);
	std::string formatCode(std::string &funcStr);
	void findClosureArgs(const Proto *tf, FuncInfo &funcInfo);
	void saveFile(const std::string &src, const std::string &path);
	void showErrorMessage(std::string, bool exitError);
};