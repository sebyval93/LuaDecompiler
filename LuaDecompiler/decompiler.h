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
	std::string evalCondition(CondElem currentCond);
	int invertCond(int cnd);
	std::string decompileFile(const char* fileName);
	std::string decompileFunction(Proto* tf, FuncInfo &funcInfo);
	std::string formatCode(std::string &funcStr);
	void findClosureArgs(const Proto *tf, FuncInfo &funcInfo);
	void saveFile(const std::string &src, const std::string &path);
	void showErrorMessage(std::string, bool exitError);

	// Opcodes

	void opEnd();

	std::string opReturn(int returnBase, std::vector<StackValue> &codeStack);
	std::string opCall(int callBase, int numResults, bool isTailCall, std::vector<StackValue> &codeStack);
	std::string opTailCall(int callBase, int numResults, std::vector<StackValue> &codeStack);

	void opPushNil(int numNil, std::vector<StackValue> &codeStack);
	void opPop(int numPop, std::vector<StackValue> &codeStack);
	void opPushInt(int num, std::vector<StackValue> &codeStack);
	void opPushString(std::string str, std::vector<StackValue> &codeStack);
	void opPushNum(std::string numStr, std::vector<StackValue> &codeStack);
	void opPushNegNum(std::string numStr, std::vector<StackValue> &codeStack);
	void opPushUpvalue(int upvalueIndex, FuncInfo &funcInfo, std::vector<StackValue> &codeStack);

	std::string opGetLocal(int localIndex, FuncInfo &funcInfo, Proto* tf, std::vector<StackValue> &codeStack);
	void opGetGlobal(int globalIndex, Proto* tf, std::vector<StackValue> &codeStack);
	void opGetTable(std::vector<StackValue> &codeStack);
	void opGetDotted(int stringIndex, Proto* tf, std::vector<StackValue> &codeStack);
	void opGetIndexed(int localIndex, FuncInfo &funcInfo, std::vector<StackValue> &codeStack);

	void opPushSelf(int stringIndex, Proto* tf, std::vector<StackValue> &codeStack);
	void opCreateTable(int numElems, std::vector<StackValue> &codeStack);

	std::string opSetLocal(int localIndex, FuncInfo &funcInfo, std::vector<StackValue> &codeStack);
	std::string opSetGlobal(int globalIndex, Proto* tf, std::vector<StackValue> &codeStack);
	std::string opSetTable(int targetIndex, int numElems, std::vector<StackValue> &codeStack);
	void opSetList(int targetIndex, int numElems, std::vector<StackValue> &codeStack);
	void opSetMap(int numElems, std::vector<StackValue> &codeStack);

	void opConcat(int numElems, std::vector<StackValue> &codeStack);

	void opAdd(std::vector<StackValue> &codeStack);
	void opAddI(int value, std::vector<StackValue> &codeStack);
	void opSub(std::vector<StackValue> &codeStack);
	void opMult(std::vector<StackValue> &codeStack);
	void opDiv(std::vector<StackValue> &codeStack);
	void opPow(std::vector<StackValue> &codeStack);
	void opMinus(std::vector<StackValue> &codeStack);
	void opNot(std::vector<StackValue> &codeStack);

	std::string opForPrep(FuncInfo &funcinfo, std::vector<StackValue> &codeStack);
	std::string opForLoop(FuncInfo &funcinfo, std::vector<StackValue> &codeStack);
	std::string opLForPrep(FuncInfo &funcinfo, std::vector<StackValue> &codeStack);
	std::string opLForLoop(FuncInfo &funcinfo, std::vector<StackValue> &codeStack);

	void opClosure(int closureIndex, int numUpvalues, Proto *tf, std::vector<StackValue> &codeStack);

	void opJmpne(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpeq(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmplt(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmple(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpgt(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpge(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpt(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpf(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmpont(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmponf(int destLine, int currLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);
	void opJmp(int destLine, std::vector<Context> &context, std::vector<StackValue> &codeStack);

	void opPushNilJmp(std::vector<Context> &context, std::vector<StackValue> &codeStack);


};