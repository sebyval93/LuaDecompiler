#include "decompiler.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stack>
#include "lex.yy.h"
#include "luac\luac.h"

// function that loads binary lua scripts
extern "C" Proto* loadproto(const char* filename);

// TODO: test settable and getindexed extensively

Decompiler::Decompiler()
	: m_format(Formatter::getInstance()), m_success(true)
{}

std::string Decompiler::decompileFunction()
{
	FuncInfo &funcInfo = m_funcInfos.back();
	const Instruction* code = funcInfo.tf->code;
	const Instruction* p = code;

	funcInfo.nForLoops = 0;
	funcInfo.nForLoopLevel = 0;
	funcInfo.nLocals = 0;

	std::string funcStr;
	funcInfo.codeStack.clear();

	if (!funcInfo.isMain)
	{
		funcStr = "function (";
		// add arguments if we have any
		for (int i = 0; i < funcInfo.tf->numparams; ++i)
		{
			std::string argName = "arg" + std::to_string(i + 1);
			funcInfo.locals.insert(std::make_pair(i, argName));
			funcStr += funcInfo.locals.at(i);

			StackValue local;
			local.type = ValueType::STRING_LOCAL;
			local.str = argName;
			funcInfo.codeStack.push_back(local);
			
			// if this is not the last elem, put a delimiter
			if (i != (funcInfo.tf->numparams - 1))
				funcStr += ", ";
		}
		funcStr += ")\n";

	}

	for (;;)
	{
		int line = p - code + 1;
		Instruction instr = *p;
		std::string tempStr;

		FuncInfo &currInfo = m_funcInfos.back();

		if (!currInfo.context.empty() && currInfo.context.back().dest == line)
		{
			Context cont = currInfo.context.back();
			// eval conditions
			// this is currently a test
			std::string condition;

			condition = "testCOND";

			std::string condPrologue, condEpilogue;
			if (cont.type == Context::IF)
			{
				condPrologue = "if ";
				condEpilogue = " then\n";
			}
			else
			{
				condPrologue = "while ";
				condEpilogue = " do\n";
			}

			// assemble and insert cond
			std::string finalCond = condPrologue + condition + condEpilogue;
			funcStr.insert(cont.strIndex, finalCond);
			funcStr += "end\n";

			currInfo.context.pop_back();
		}

		switch (GET_OPCODE(instr))
		{
		case OP_END:
			break;

		case OP_RETURN:
			funcStr += opReturn(GETARG_U(instr));
			break;

		case OP_CALL:
			funcStr += opCall(GETARG_A(instr), GETARG_B(instr), false);
			break;

		case OP_TAILCALL:
			funcStr += opTailCall(GETARG_A(instr), GETARG_B(instr));
			break;

		case OP_PUSHNIL:
			opPushNil(GETARG_U(instr));
			break;

		case OP_POP:
			opPop(GETARG_U(instr));
			break;

		case OP_PUSHINT:
			opPushInt(GETARG_S(instr));
			break;

		case OP_PUSHSTRING:
			opPushString(currInfo.tf->kstr[GETARG_U(instr)]->str);
			break;

		case OP_PUSHNUM:
			opPushNum(std::to_string(currInfo.tf->knum[GETARG_U(instr)]));
			break;

		case OP_PUSHNEGNUM:
			opPushNegNum(std::to_string(currInfo.tf->knum[GETARG_U(instr)]));
			break;

		case OP_PUSHUPVALUE:
			opPushUpvalue(GETARG_U(instr));
			break;

		case OP_GETLOCAL:
			opGetLocal(GETARG_U(instr));
			break;

		case OP_GETGLOBAL:
			opGetGlobal(GETARG_U(instr));
			break;

		case OP_GETTABLE:
			opGetTable();
			break;

		case OP_GETDOTTED:
			opGetDotted(GETARG_U(instr));
			break;

		case OP_GETINDEXED:
			opGetIndexed(GETARG_U(instr));
			break;

		case OP_PUSHSELF:
			opPushSelf(GETARG_U(instr));
			break;

		case OP_CREATETABLE:
			opCreateTable(GETARG_U(instr));
			break;

		case OP_SETLOCAL:
			funcStr += opSetLocal(GETARG_U(instr));
			break;

		case OP_SETGLOBAL:
			funcStr += opSetGlobal(GETARG_U(instr));
			break;

		case OP_SETTABLE:
			funcStr += opSetTable(GETARG_A(instr), GETARG_B(instr));
			break;

		case OP_SETLIST:
			opSetList(GETARG_A(instr), GETARG_B(instr));
			break;

		case OP_SETMAP:
			opSetMap(GETARG_U(instr));
			break;

		case OP_ADD:
			opAdd();
			break;

		case OP_ADDI:
			opAddI(GETARG_S(instr));
			break;

		case OP_SUB:
			opSub();
			break;

		case OP_MULT:
			opMult();
			break;

		case OP_DIV:
			opDiv();
			break;

		case OP_POW:
			opPow();
			break;

		case OP_CONCAT:
			opConcat(GETARG_U(instr));
			break;

		case OP_MINUS:
			opMinus();
			break;

		case OP_NOT:
			opNot();
			break;

		case OP_JMPNE:
			{
				//showErrorMessage("Unimplemented opcode JMPNE! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPNE;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPEQ:
			{
				//showErrorMessage("Unimplemented opcode JMPEQ! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPEQ;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}

			}
			break;

		case OP_JMPLT:
			{
				//showErrorMessage("Unimplemented opcode JMPLT! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPLT;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPLE:
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPLE;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPGT:
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPGT;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPGE:
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPGE;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPT:
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPT;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPF:
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPF;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPONT:
			{
				// IDK YET
				//showErrorMessage("Unimplemented opcode JMPONT! exiting!", true);
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPONT;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMPONF:
			{
				// IDK YET
				//showErrorMessage("Unimplemented opcode JMPONF! exiting!", true);
				std::string arg0 = currInfo.codeStack.back().str;
				currInfo.codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPONF;
				elem.nextCond = CondElem::NONE;

				if (currInfo.context.empty() || currInfo.context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					currInfo.context.push_back(cont);
				}
				else
				{
					currInfo.context.back().conds.push_back(elem);
					currInfo.context.back().dest = elem.dest;
				}
			}
			break;

		case OP_JMP:
			opJmp(GETARG_S(instr));
			break;

		case OP_PUSHNILJMP:
			opPushNilJmp();
			break;

		case OP_FORPREP:
			funcStr += opForPrep();
			break;

		case OP_FORLOOP:
			funcStr += opForLoop();
			break;

		case OP_LFORPREP:
			funcStr += opLForPrep();
			break;

		case OP_LFORLOOP:
			funcStr += opLForLoop();
			break;

		case OP_CLOSURE:
			opClosure(GETARG_A(instr), GETARG_B(instr));
			break;

		}

		if (instr == OP_END)
		{
			if (!currInfo.isMain)
				funcStr += "end\n";

			break;
		}
		p++;
	}

	return funcStr;
}

void Decompiler::processPath(std::string pathStr)
{
	// update to c++17 soon, microsoft??
	using namespace std::experimental;
	
	filesystem::path path(pathStr);

	std::string sourceStr;

	if (!filesystem::exists(path))
	{
		std::cerr << "Path " << pathStr << " does not exist!" << '\n';
	}

	if (filesystem::is_regular_file(path))
	{
		sourceStr = decompileFile(path.string().c_str());
		if (!sourceStr.empty())
		{
			saveFile(sourceStr, path.parent_path().string() + "\\" + path.stem().string() + "_d" + path.extension().string());

			if (m_success)
				std::cout << "File " << path.filename() << " successfully decompiled!\n";
			else
				std::cout << "File " << path.filename() << " decompiled with errors!\n";

			sourceStr.clear();
			m_format.reset();
			m_success = true;
		}
	}
	else
	{
		filesystem::recursive_directory_iterator dir(path), end;
		filesystem::path rootOutputPath = path.parent_path();
		rootOutputPath.append(path.filename().string() + "_d");

		filesystem::create_directory(rootOutputPath);

		while (dir != end)
		{
			if (filesystem::is_regular_file(dir->path()))
			{
				sourceStr = decompileFile(dir->path().string().c_str());
				if (!sourceStr.empty())
				{
					filesystem::path newPath = rootOutputPath / dir->path().string().substr(rootOutputPath.string().length() - 2);

					if (!filesystem::exists(newPath.parent_path()))
						filesystem::create_directory(newPath.parent_path());

					saveFile(sourceStr, newPath.string());

					if (m_success)
						std::cout << "File " << dir->path().filename() << " successfully decompiled!\n";
					else
						std::cout << "File " << dir->path().filename() << " decompiled with errors!\n";

					m_format.reset();
					sourceStr.clear();
					m_success = true;
				}
			}

			++dir;
		}
	}

}

std::string Decompiler::evalCondition(CondElem currentCond)
{
	return "";
}

int Decompiler::invertCond(int cnd)
{
	OpCode cond = (OpCode)cnd;

	switch (cond)
	{
	case OP_JMPNE:
		return OP_JMPEQ;
	case OP_JMPEQ:
		return OP_JMPNE;
	case OP_JMPLT:
		return OP_JMPGE;
	case OP_JMPLE:
		return OP_JMPGT;
	case OP_JMPGT:
		return OP_JMPLE;
	case OP_JMPGE:
		return OP_JMPLT;
	case OP_JMPT:
		return OP_JMPF;
	case OP_JMPF:
		return OP_JMPT;
	case OP_JMPONT:
		return OP_JMPONF;
	case OP_JMPONF:
		return OP_JMPONT;

	default:
		return -1;
	}
}

std::string Decompiler::decompileFile(const char* fileName)
{
	Proto* tf = loadLuaStructure(fileName);
	std::string sourceStr;

	std::experimental::filesystem::path path(fileName);

	if (tf == NULL)
	{
		std::cout << "Error: file " << path.filename() << " is not a compiled lua file!\n";
		return sourceStr;
	}

	//std::cout << "File " << path.filename() << " opened successfully!\n";


	FuncInfo mainInfo;
	mainInfo.isMain = true;
	mainInfo.tf = tf;
	m_funcInfos.push_back(mainInfo);
	sourceStr = decompileFunction();
	m_funcInfos.pop_back();

	return formatCode(sourceStr);
}

std::string Decompiler::formatCode(std::string &sourceStr)
{
	const reflex::Input strInput(sourceStr);
	yyFlexLexer lexer(strInput, &std::cout);
	lexer.yylex();
	//std::cout << *m_formattedStr;

	return m_format.getFormattedStr();
}

void Decompiler::saveFile(const std::string &src, const std::string &path)
{
	std::ofstream file;
	file.open(path, std::ios::trunc);

	file << src;
}

Proto* Decompiler::loadLuaStructure(const char* fileName)
{ 
	return loadproto(fileName);
}

void Decompiler::showErrorMessage(std::string message, bool exitError)
{
	std::cerr << "Error: " << message << '\n';
	m_success = false;

	if (exitError)
	{
		// pause
		char f;
		std::cin >> f;
		std::exit(1);
	}
}

void Decompiler::opEnd()
{
}

std::string Decompiler::opReturn(int returnBase)
{
	//int returnBase = GETARG_U(instr);
	std::vector<StackValue> args;
	std::string tempStr;

	FuncInfo &currInfo = m_funcInfos.back();

	// pop size - base
	int items = currInfo.codeStack.size() - returnBase;
	for (int i = 0; i < items; ++i)
	{
		args.push_back(currInfo.codeStack.back());
		currInfo.codeStack.pop_back();
	}

	tempStr = "return ";

	// insert arguments in the right order
	for (auto it = args.rbegin(); it != args.rend(); ++it)
	{
		tempStr += (*it).str;
		if (--args.rend() != it)
			tempStr += ", ";
	}

	return (tempStr + '\n');
}

std::string Decompiler::opCall(int callBase, int numResults, bool isTailCall)
{
	std::vector<StackValue> args;
	std::string funcName;
	std::string tempStr;

	FuncInfo &currInfo = m_funcInfos.back();

	funcName = currInfo.codeStack[callBase].str;

	int items = currInfo.codeStack.size() - callBase;
	for (int i = 0; i < items - 1; ++i)
	{
		if (currInfo.codeStack.back().type == ValueType::STRING_PUSHSELF)
		{
			std::string str = currInfo.codeStack.back().str;
			currInfo.codeStack.pop_back();
			str = currInfo.codeStack.back().str + str;
			currInfo.codeStack.pop_back();

			StackValue result;
			result.str = str;
			result.type = ValueType::STRING_GLOBAL;
			currInfo.codeStack.push_back(result);
		}
		else
		{
			args.push_back(currInfo.codeStack.back());
			currInfo.codeStack.pop_back();
		}
	}

	// get funcName
	funcName = currInfo.codeStack.back().str;
	currInfo.codeStack.pop_back();

	tempStr = funcName + "(";

	// insert arguments in the right order
	for (auto it = args.rbegin(); it != args.rend(); ++it)
	{
		tempStr += (*it).str;
		if (--args.rend() != it)
			tempStr += ", ";
	}

	tempStr += ")";

	if (numResults > 0)
	{
		StackValue result;
		result.str = tempStr;
		result.type = ValueType::STRING;

		if (isTailCall)
			result.str = "return " + result.str;

		if (numResults != 255)
		{
			for (int i = 0; i < numResults; ++i)
			{
				currInfo.codeStack.push_back(result);
			}
		}
		else
			currInfo.codeStack.push_back(result);

		if (isTailCall)
		{
			// assume argb to be 1??
			// HACK maybe working
			currInfo.codeStack.pop_back();
			return (result.str + '\n');
			//funcStr += (result.str + '\n');
		}

		return "";
	}
	else
	{
		//funcStr += (tempStr + '\n');
		return (tempStr + '\n');
	}
}

std::string Decompiler::opTailCall(int callBase, int numResults)
{
	return opCall(callBase, numResults, true);
}

void Decompiler::opPushNil(int numNil)
{
	StackValue result;
	result.str = "nil";
	result.type = ValueType::NIL;

	for (int i = 0; i < numNil; ++i)
		m_funcInfos.back().codeStack.push_back(result);
}

void Decompiler::opPop(int numPop)
{
	for (int i = 0; i < numPop; ++i)
	{
		m_funcInfos.back().codeStack.pop_back();
	}
}

void Decompiler::opPushInt(int num)
{
	StackValue stackValue;
	stackValue.str = std::to_string(num);
	stackValue.type = ValueType::INT;
	m_funcInfos.back().codeStack.push_back(stackValue);
}

void Decompiler::opPushString(std::string str)
{
	StackValue result;

	if (str.find('\n') != std::string::npos || str.find('\t') != std::string::npos)
	{
		str.insert(0, "[[");
		str.insert(str.size(), "]]");
	}
	else
	{
		str.insert(0, "\"");
		str.insert(str.size(), "\"");
	}
	result.str = str;
	result.type = ValueType::STRING;
	m_funcInfos.back().codeStack.push_back(result);
}

void Decompiler::opPushNum(std::string numStr)
{
	StackValue stackValue;
	stackValue.str = numStr;

	// trim trailing zeros, if is a fp number
	// don't have to check for .0 or 1.0, thx lua!
	// it is guaranteed to have something beyond the . if fp
	size_t dotPos = stackValue.str.find('.');
	if (dotPos != std::string::npos)
	{
		for (size_t i = stackValue.str.size() - 1; i > dotPos; --i)
		{
			if (stackValue.str[i] == '0')
				stackValue.str.erase(i);
			else
				break;
		}
	}

	stackValue.type = ValueType::INT;
	m_funcInfos.back().codeStack.push_back(stackValue);
}

void Decompiler::opPushNegNum(std::string numStr)
{
	StackValue stackValue;
	stackValue.str = numStr;

	// trim trailing zeros, if is a fp number
	// don't have to check for .0 or 1.0, thx lua!
	// it is guaranteed to have something beyond the . if fp
	size_t dotPos = stackValue.str.find('.');
	if (dotPos != std::string::npos)
	{
		for (size_t i = stackValue.str.size() - 1; i > dotPos; --i)
		{
			if (stackValue.str[i] == '0')
				stackValue.str.erase(i);
			else
				break;
		}
	}

	stackValue.str.insert(0, "-");

	stackValue.type = ValueType::INT;
	m_funcInfos.back().codeStack.push_back(stackValue);
}

void Decompiler::opPushUpvalue(int upvalueIndex)
{
	StackValue result;

	FuncInfo &currInfo = m_funcInfos.back();

	result.str = '%' + currInfo.upvalues.at(upvalueIndex);
	result.type = ValueType::STRING;

	currInfo.codeStack.push_back(result);
}

std::string Decompiler::opGetLocal(int localIndex)
{
	StackValue stackValue;
	std::string tempStr;

	FuncInfo &currInfo = m_funcInfos.back();

	if (currInfo.locals.find(localIndex) == currInfo.locals.end())
	{
		// local is not present in the list
		// name it.
		std::string localName = "loc" + std::to_string(localIndex - currInfo.tf->numparams + 1);
		currInfo.locals.insert(std::make_pair(localIndex, localName));
		++currInfo.nLocals;

		tempStr += "local " + localName + " = " + currInfo.codeStack[localIndex].str + "\n";
	}

	stackValue.str = currInfo.locals.find(localIndex)->second;
	stackValue.type = ValueType::STRING_LOCAL;
	m_funcInfos.back().codeStack.push_back(stackValue);

	return tempStr;
}

void Decompiler::opGetGlobal(int globalIndex)
{
	StackValue stackValue;
	FuncInfo &currInfo = m_funcInfos.back();

	stackValue.index = globalIndex;
	stackValue.str = std::string(currInfo.tf->kstr[stackValue.index]->str);
	stackValue.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(stackValue);
}

void Decompiler::opGetTable()
{
	std::vector<StackValue> args;
	StackValue result;
	FuncInfo &currInfo = m_funcInfos.back();

	args.push_back(currInfo.codeStack.back());
	currInfo.codeStack.pop_back();
	args.push_back(currInfo.codeStack.back());
	currInfo.codeStack.pop_back();

	result.str = args[1].str + '[' + args[0].str + ']';

	currInfo.codeStack.push_back(result);
}

void Decompiler::opGetDotted(int stringIndex)
{
	FuncInfo &currInfo = m_funcInfos.back();

	std::string str = currInfo.tf->kstr[stringIndex]->str;
	StackValue target, result;

	target = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	result.str = target.str + "." + str;
	result.type = ValueType::STRING;

	currInfo.codeStack.push_back(result);
}

void Decompiler::opGetIndexed(int localIndex)
{
	FuncInfo &currInfo = m_funcInfos.back();

	std::string local = currInfo.locals.at(localIndex);
	StackValue target, result;

	target = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = target.str + "[" + local + "]";
	result.type = target.type;

	currInfo.codeStack.push_back(result);
}

void Decompiler::opPushSelf(int stringIndex)
{
	FuncInfo currInfo = m_funcInfos.back();

	std::string str = currInfo.tf->kstr[stringIndex]->str;
	StackValue target, result;

	result.str = ":" + str;
	result.type = ValueType::STRING_PUSHSELF;

	currInfo.codeStack.push_back(result);
}

void Decompiler::opCreateTable(int numElems)
{
	StackValue result;
	if (numElems > 0)
	{
		result.str += "{ ";
		result.type = ValueType::TABLE_BRACE;
		result.index = numElems;
	}
	else
	{
		result.str = "{}";
		result.type = ValueType::STRING_GLOBAL;
	}

	m_funcInfos.back().codeStack.push_back(result);
}

std::string Decompiler::opSetLocal(int localIndex)
{
	StackValue val;
	std::string local, result;
	FuncInfo &currInfo = m_funcInfos.back();

	val = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	if (currInfo.locals.size() <= localIndex)
	{
		std::cout << "WARNING!! SETLOCAL out of bounds!!! ignoring";
		return result;
	}
	local = currInfo.locals.at(localIndex);

	result = local + " = " + val.str + "\n";

	return result;
}

std::string Decompiler::opSetGlobal(int globalIndex)
{
	StackValue val;
	std::string global, result;
	FuncInfo &currInfo = m_funcInfos.back();

	val = currInfo.codeStack.back();
	global = currInfo.tf->kstr[globalIndex]->str;

	if (val.type == ValueType::CLOSURE_STRING)
	{
		// we have a closure on the stack
		// insert after "function ", which is 9 chars
		currInfo.codeStack.pop_back();
		val.str.insert(9, global);
		return val.str;

	}
	else
	{
		currInfo.codeStack.pop_back();
		result = global + " = " + val.str + '\n';

		return result;
	}
}

std::string Decompiler::opSetTable(int targetIndex, int numElems)
{
	std::vector<StackValue> args;
	std::string result;
	FuncInfo &currInfo = m_funcInfos.back();

	if (targetIndex == numElems && numElems == 3)
	{
		for (int i = 0; i < numElems; ++i)
		{
			args.push_back(currInfo.codeStack.back());
			currInfo.codeStack.pop_back();
		}

		if (args[1].type == ValueType::STRING_GLOBAL)
		{
			args[1].str.erase(0, 1);
			args[1].str.erase(args[1].str.size() - 1);
		}

		result = args[2].str + "[" + args[1].str + "] = " + args[0].str;

		return (result + '\n');
	}
	else
	{
		// unimplemented yet
		showErrorMessage("SETTABLE " + std::to_string(targetIndex) + " " + std::to_string(numElems) + " not implemented!!!", false);
		return result;
	}
}

void Decompiler::opSetList(int targetIndex, int numElems)
{
	std::vector<StackValue> args;
	StackValue target, tableBrace, result;
	FuncInfo &currInfo = m_funcInfos.back();

	if (targetIndex != 0)
	{
		showErrorMessage("SETLIST not fully implemented!, first arg is nonzero!", false);
	}

	for (int i = 0; i < numElems; ++i)
	{
		args.push_back(currInfo.codeStack.back());
		currInfo.codeStack.pop_back();
	}

	tableBrace = currInfo.codeStack.back();
	if (currInfo.codeStack.back().type == ValueType::TABLE_BRACE)
	{
		if (tableBrace.index > numElems)
		{

			currInfo.codeStack.pop_back();

			for (auto it = args.rbegin(); it != args.rend(); ++it)
			{
				result.str += (*it).str;

				if (--args.rend() != it)
					result.str += ", ";
			}

			result.str += ";";

			tableBrace.str += result.str;
			tableBrace.index -= numElems;

			currInfo.codeStack.push_back(tableBrace);

			return;
		}

		currInfo.codeStack.pop_back();
	}

	result.str = "{ ";

	for (auto it = args.rbegin(); it != args.rend(); ++it)
	{
		result.str += (*it).str;

		if (--args.rend() != it)
			result.str += ", ";
	}

	result.str += " }";
	result.type = ValueType::STRING;

	currInfo.codeStack.push_back(result);
}

void Decompiler::opSetMap(int numElems)
{
	StackValue identifier, mapValue, tableBrace, result;
	std::vector<std::string> args;
	FuncInfo &currInfo = m_funcInfos.back();

	// TODO: nicer name
	bool hasRemainingElems = false;

	for (int i = 0; i < numElems; ++i)
	{
		mapValue = currInfo.codeStack.back();
		currInfo.codeStack.pop_back();
		identifier = currInfo.codeStack.back();
		currInfo.codeStack.pop_back();

		//remove quotes from identifier
		if (identifier.type == ValueType::STRING)
		{
			identifier.str.erase(0, 1);
			identifier.str.erase(identifier.str.size() - 1);
		}
		else if (identifier.type == ValueType::INT)
		{
			identifier.str.insert(0, "[");
			identifier.str.insert(identifier.str.size(), "]");
		}

		args.push_back(identifier.str + " = " + mapValue.str);
	}

	// pop until we find a brace
	while (currInfo.codeStack.back().type != ValueType::TABLE_BRACE)
	{
		args.push_back(currInfo.codeStack.back().str);
		currInfo.codeStack.pop_back();
	}

	tableBrace = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	tableBrace.index -= numElems;

	if (tableBrace.index > 0)
		hasRemainingElems = true;

	result.type = ValueType::STRING_GLOBAL;
	for (int i = args.size() - 1; i >= 0; --i)
	{
		result.str += args[i];
		if ((i - 1) >= 0)
			result.str += ", ";
	}

	if (hasRemainingElems)
	{
		currInfo.codeStack.push_back(tableBrace);
	}
	else
	{
		result.str.insert(0, tableBrace.str);
		result.str += " }";
	}

	currInfo.codeStack.push_back(result);
}

void Decompiler::opConcat(int numElems)
{
	StackValue result;
	std::vector<StackValue> args;
	FuncInfo &currInfo = m_funcInfos.back();

	for (int i = 0; i < numElems; ++i)
	{
		args.push_back(currInfo.codeStack.back());
		currInfo.codeStack.pop_back();
	}
	for (int i = numElems - 1; i >= 0; --i)
	{
		result.str += args[i].str;
		if ((i - 1) >= 0)
			result.str += "..";
	}

	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opAdd()
{
	StackValue y, x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	y = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = x.str + " + " + y.str;
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opAddI(int value)
{
	StackValue stackValue;
	FuncInfo &currInfo = m_funcInfos.back();

	stackValue = currInfo.codeStack.back();
	StackValue newValue;
	currInfo.codeStack.pop_back();

	std::string op;
	if (value >= 0)
		op = " + ";
	else
		op = "";

	newValue.str = stackValue.str + op + std::to_string(value);
	newValue.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(newValue);
}

void Decompiler::opSub()
{
	StackValue y, x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	y = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = x.str + " - " + y.str;
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opMult()
{
	StackValue y, x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	y = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = "( " + x.str + " * " + y.str + " )";
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opDiv()
{
	StackValue y, x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	y = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = "( " + x.str + " / " + y.str + " )";
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opPow()
{
	StackValue y, x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	y = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();
	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = "( " + x.str + " ^ " + y.str + " )";
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opMinus()
{
	StackValue x, result;
	FuncInfo &currInfo = m_funcInfos.back();

	x = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	result.str = "-" + x.str;
	result.type = ValueType::STRING_GLOBAL;
	currInfo.codeStack.push_back(result);
}

void Decompiler::opNot()
{
	// showErrorMessage("Unimplemented opcode NOT! exiting!", true);
	FuncInfo &currInfo = m_funcInfos.back();
	std::string arg = "not " + currInfo.codeStack.back().str;
	currInfo.codeStack.pop_back();

	StackValue result;
	result.type = ValueType::STRING_GLOBAL;
	result.str = arg;

	currInfo.codeStack.push_back(result);
}

std::string Decompiler::opForPrep()
{
	StackValue val1, val2, val3;
	std::string result;
	FuncInfo &currInfo = m_funcInfos.back();

	val3 = currInfo.codeStack[currInfo.codeStack.size() - 1];
	val2 = currInfo.codeStack[currInfo.codeStack.size() - 2];
	val1 = currInfo.codeStack[currInfo.codeStack.size() - 3];



	std::string locName = "for" + std::to_string(currInfo.nForLoops);
	++currInfo.nForLoops;
	++currInfo.nForLoopLevel;
	int locIndex = currInfo.nLocals;
	++currInfo.nLocals;
	currInfo.locals.insert(std::make_pair(locIndex, locName));
	result += "for " + locName + " = " + val1.str + ", " + val2.str + ", " +
		val3.str + "\ndo\n";

	return result;
}

std::string Decompiler::opForLoop()
{
	std::string result;
	FuncInfo &currInfo = m_funcInfos.back();

	// the last local should be the forloop var
	--currInfo.nLocals;
	--currInfo.nForLoopLevel;
	currInfo.locals.erase(currInfo.nLocals);

	// clear control vars
	currInfo.codeStack.pop_back();
	currInfo.codeStack.pop_back();
	currInfo.codeStack.pop_back();

	result = "end\n";

	return result;
}

std::string Decompiler::opLForPrep()
{
	//showErrorMessage("Unimplemented opcode LFORPREP! exiting!", true);
	FuncInfo &currInfo = m_funcInfos.back();
	StackValue tableName = currInfo.codeStack.back();
	currInfo.codeStack.pop_back();

	StackValue invisTable, index, value;
	invisTable.type = ValueType::STRING_LOCAL;
	invisTable.str = "_t";
	index.type = ValueType::STRING_LOCAL;
	index.str = "index";
	value.type = ValueType::STRING_LOCAL;
	value.str = "value";

	currInfo.codeStack.push_back(invisTable);
	currInfo.codeStack.push_back(index);
	currInfo.codeStack.push_back(value);

	currInfo.locals.insert(std::make_pair(currInfo.nLocals++, "_t"));
	currInfo.locals.insert(std::make_pair(currInfo.nLocals++, "index"));
	currInfo.locals.insert(std::make_pair(currInfo.nLocals++, "value"));

	return ("for index, value in " + tableName.str + "\ndo\n");
}

std::string Decompiler::opLForLoop()
{
	std::string result;
	FuncInfo &currInfo = m_funcInfos.back();

	//showErrorMessage("Unimplemented opcode LFORLOOP! exiting!", true);
	currInfo.locals.erase(--currInfo.nLocals);
	currInfo.locals.erase(--currInfo.nLocals);
	currInfo.locals.erase(--currInfo.nLocals);

	currInfo.codeStack.pop_back();
	currInfo.codeStack.pop_back();
	currInfo.codeStack.pop_back();

	result = "end\n";

	return result;
}

void Decompiler::opClosure(int closureIndex, int numUpvalues)
{
	FuncInfo &currInfo = m_funcInfos.back();
	FuncInfo funcInfo;
	StackValue stackValue;
	std::string closureSrc;

	// pop all upvalues if any are found
	// and register them into funcInfo
	for (int i = 0; i < numUpvalues; ++i)
	{
		StackValue upvalue;
		upvalue = currInfo.codeStack.back();
		currInfo.codeStack.pop_back();

		currInfo.upvalues.insert(std::make_pair(numUpvalues - (i + 1), upvalue.str));
	}

	// decompile closure
	funcInfo.isMain = false;
	funcInfo.tf = currInfo.tf->kproto[closureIndex];

	m_funcInfos.push_back(funcInfo);
	closureSrc = decompileFunction();
	m_funcInfos.pop_back();

	stackValue.str = closureSrc;
	stackValue.type = ValueType::CLOSURE_STRING;

	// closureSrc += "end\n";

	m_funcInfos.back().codeStack.push_back(stackValue);
}

void Decompiler::opJmpne(int destLine, int currLine)
{
}

void Decompiler::opJmpeq(int destLine, int currLine)
{
}

void Decompiler::opJmplt(int destLine, int currLine)
{
}

void Decompiler::opJmple(int destLine, int currLine)
{
}

void Decompiler::opJmpgt(int destLine, int currLine)
{
}

void Decompiler::opJmpge(int destLine, int currLine)
{
}

void Decompiler::opJmpt(int destLine, int currLine)
{
}

void Decompiler::opJmpf(int destLine, int currLine)
{
}

void Decompiler::opJmpont(int destLine, int currLine)
{
}

void Decompiler::opJmponf(int destLine, int currLine)
{
}

void Decompiler::opJmp(int destLine)
{
	showErrorMessage("Unimplemented opcode JMP! continuing!", false);
	FuncInfo &currInfo = m_funcInfos.back();

	if (!currInfo.context.empty() && destLine < 0)
	{
		currInfo.context.back().type = Context::WHILE;
	}
}

void Decompiler::opPushNilJmp()
{
	//showErrorMessage("Unimplemented opcode PUSHNILJMP! exiting!", true);
	StackValue result;
	FuncInfo &currInfo = m_funcInfos.back();

	result.str = "nil";
	result.type = ValueType::NIL;

	currInfo.codeStack.push_back(result);
}
