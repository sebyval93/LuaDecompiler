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
		std::vector<StackValue> codeStack;
		std::vector<Context> context;
		Proto* tf;
	};

	std::vector<FuncInfo> m_funcInfos;

	Proto* loadLuaStructure(const char* fileName);
	// returns end offs
	std::string evalCondition(CondElem currentCond);
	int invertCond(int cnd);
	std::string decompileFile(const char* fileName);
	std::string decompileFunction();
	std::string formatCode(std::string &funcStr);
	void saveFile(const std::string &src, const std::string &path);
	void showErrorMessage(std::string, bool exitError);

	// Opcodes

	void opEnd();

	std::string opReturn(int returnBase);
	std::string opCall(int callBase, int numResults, bool isTailCall);
	std::string opTailCall(int callBase, int numResults);

	void opPushNil(int numNil);
	void opPop(int numPop);
	void opPushInt(int num);
	void opPushString(std::string str);
	void opPushNum(std::string numStr);
	void opPushNegNum(std::string numStr);
	void opPushUpvalue(int upvalueIndex);

	std::string opGetLocal(int localIndex);
	void opGetGlobal(int globalIndex);
	void opGetTable();
	void opGetDotted(int stringIndex);
	void opGetIndexed(int localIndex);

	void opPushSelf(int stringIndex);
	void opCreateTable(int numElems);

	std::string opSetLocal(int localIndex);
	std::string opSetGlobal(int globalIndex);
	std::string opSetTable(int targetIndex, int numElems);
	void opSetList(int targetIndex, int numElems);
	void opSetMap(int numElems);

	void opConcat(int numElems);

	void opAdd();
	void opAddI(int value);
	void opSub();
	void opMult();
	void opDiv();
	void opPow();
	void opMinus();
	void opNot();

	std::string opForPrep();
	std::string opForLoop();
	std::string opLForPrep();
	std::string opLForLoop();

	void opClosure(int closureIndex, int numUpvalues);

	void opJmpne(int destLine, int currLine);
	void opJmpeq(int destLine, int currLine);
	void opJmplt(int destLine, int currLine);
	void opJmple(int destLine, int currLine);
	void opJmpgt(int destLine, int currLine);
	void opJmpge(int destLine, int currLine);
	void opJmpt(int destLine, int currLine);
	void opJmpf(int destLine, int currLine);
	void opJmpont(int destLine, int currLine);
	void opJmponf(int destLine, int currLine);
	void opJmp(int destLine);

	void opPushNilJmp();


};