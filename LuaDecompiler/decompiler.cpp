#include "decompiler.h"
//#include "luacdefs.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stack>
#include "lex.yy.h"
#include "luac\luac.h"

extern "C" Proto* loadproto(const char* filename);

#define P_OP(x)	//printf("%-11s\t\n",x);
#define P_NONE
#define P_AB	//GETARG_A(instr);GETARG_B(instr);
#define P_F	//GETARG_A(instr);GETARG_B(instr);
#define P_J	//GETARG_S(instr);
#define P_Q	//GETARG_U(instr);
#define P_K	//GETARG_U(instr);
#define P_L	//GETARG_U(instr);
#define P_N	//GETARG_U(instr);
#define P_S	//printf("ARGS: %d\n", GETARG_S(instr));
#define P_U	//GETARG_U(instr);


// TODO: test settable and getindexed extensively

Decompiler::Decompiler()
	: m_format(Formatter::getInstance())
{}

std::string Decompiler::decompileFunction(Proto* tf, FuncInfo &funcInfo)
{
	const Instruction* code = tf->code;
	const Instruction* p = code;

	funcInfo.nForLoops = 0;
	funcInfo.nForLoopLevel = 0;
	funcInfo.nLocals = 0;

	std::string funcStr;
	std::vector<StackValue> codeStack;
	std::vector<Context> context;

	if (!funcInfo.isMain)
	{
		funcStr = "function (";
		// add arguments if we have any
		for (int i = 0; i < tf->numparams; ++i)
		{
			std::string argName = "arg" + std::to_string(i + 1);
			funcInfo.locals.insert(std::make_pair(i, argName));
			funcStr += funcInfo.locals.at(i);

			StackValue local;
			local.type = ValueType::STRING_LOCAL;
			local.str = argName;
			codeStack.push_back(local);
			
			// if this is not the last elem, put a delimiter
			if (i != (tf->numparams - 1))
				funcStr += ", ";
		}
		funcStr += ")\n";

	}

	for (;;)
	{
		int line = p - code + 1;
		Instruction instr = *p;
		std::string tempStr;

		if (!context.empty() && context.back().dest == line)
		{
			Context cont = context.back();
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

			context.pop_back();
		}

		//std::cout << line << "\n";

		switch (GET_OPCODE(instr))
		{
		case OP_END:
			P_OP("END");
			P_NONE;
			break;

		case OP_RETURN:
			P_OP("RETURN");
			{
				int returnBase = GETARG_U(instr);
				std::vector<StackValue> args;
				std::string tempStr;

				// pop size - base
				int items = codeStack.size() - returnBase;
				for (int i = 0; i < items; ++i)
				{
					args.push_back(codeStack.back());
					codeStack.pop_back();
				}

				tempStr = "return ";

				// insert arguments in the right order
				for (auto it = args.rbegin(); it != args.rend(); ++it)
				{
					tempStr += (*it).str;
					if (--args.rend() != it)
						tempStr += ", ";
				}

				funcStr += (tempStr + '\n');
			}
			//P_U;
			break;

		case OP_CALL:
		case OP_TAILCALL:
			P_OP("CALL");
			{

				int callBase = GETARG_A(instr);
				int nResults = GETARG_B(instr);
				std::vector<StackValue> args;
				std::string funcName;
				std::string tempStr;

				//callBase -= (funcInfo.nForLoopLevel * 3);

				funcName = codeStack[callBase].str;

				int items = codeStack.size() - callBase;
				for (int i = 0; i < items - 1; ++i)
				{
					if (codeStack.back().type == ValueType::STRING_PUSHSELF)
					{
						std::string str = codeStack.back().str;
						codeStack.pop_back();
						str = codeStack.back().str + str;
						codeStack.pop_back();

						StackValue result;
						result.str = str;
						result.type = ValueType::STRING_GLOBAL;
						codeStack.push_back(result);
					}
					else
					{
						args.push_back(codeStack.back());
						codeStack.pop_back();
					}
				}

				// get funcName
				funcName = codeStack.back().str;
				codeStack.pop_back();

				tempStr = funcName + "(";

				// insert arguments in the right order
				for (auto it = args.rbegin(); it != args.rend(); ++it)
				{
					tempStr += (*it).str;
					if (--args.rend() != it)
						tempStr += ", ";
				}

				tempStr += ")";

				if (nResults > 0)
				{
					StackValue result;
					result.str = tempStr;
					result.type = ValueType::STRING;

					if (GET_OPCODE(instr) == OP_TAILCALL)
						result.str = "return " + result.str;

					if (nResults != 255)
					{
						for (int i = 0; i < nResults; ++i)
						{
							codeStack.push_back(result);
						}
					}
					else
						codeStack.push_back(result);

					if (GET_OPCODE(instr) == OP_TAILCALL)
					{
						// assume argb to be 1??
						// HACK maybe working
						codeStack.pop_back();
						funcStr += (result.str + '\n');
					}
				}
				else
				{
					funcStr += (tempStr + '\n');
				};

			}
			//P_AB;
			break;
			/*
		case OP_TAILCALL:
			P_OP("UNIMPLEMENTED TAILCALL")
			{
				showErrorMessage("Unimplemented opcode TAILCALL! exiting!", true);
			}
			P_AB;
			break;
			*/
		case OP_PUSHNIL:
			P_OP("UNIMPLEMENTED PUSHNIL");
			{
				int numNils = GETARG_U(instr);
				StackValue result;
				result.str = "nil";
				result.type = ValueType::NIL;

				for (int i = 0; i < numNils; ++i)
					codeStack.push_back(result);
			}
			//P_U;
			break;

		case OP_POP:
			P_OP("POP");
			{
				int numItemsToPop = GETARG_U(instr);

				for (int i = 0; i < numItemsToPop; ++i)
				{
					codeStack.pop_back();
					//funcInfo.locals.erase(funcInfo.nLocals - 1);
					//--funcInfo.nLocals;
				}
			}
			//P_U;
			break;

		case OP_PUSHINT:
			P_OP("PUSHINT");
			{
				StackValue stackValue;
				stackValue.str = std::to_string(GETARG_S(instr));
				stackValue.type = ValueType::INT;
				codeStack.push_back(stackValue);
			}
			//P_S;
			break;

		case OP_PUSHSTRING:
			P_OP("PUSHSTRING");
			{
				std::string str;
				StackValue result;

				str = std::string(tf->kstr[GETARG_U(instr)]->str);

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
				codeStack.push_back(result);
			}
			//codeStack.push_back(std::string(tf->kstr[GETARG_U(instr)]->str));
			//P_Q;
			break;

		case OP_PUSHNUM:
			P_OP("PUSHNUM");
			{
				StackValue stackValue;
				stackValue.str = std::to_string(tf->knum[GETARG_U(instr)]);

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
				codeStack.push_back(stackValue);
				//funcStack.push(obj);
			}
			//P_N;
			break;

		case OP_PUSHNEGNUM:
			P_OP("PUSHNEGNUM");
			{
				StackValue stackValue;
				stackValue.str = std::to_string(tf->knum[GETARG_U(instr)]);

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
				codeStack.push_back(stackValue);
			}
			//P_N;
			break;

		case OP_PUSHUPVALUE:
			P_OP("PUSHUPVALUE");
			{
				int upvalueIndex = GETARG_U(instr);
				StackValue result;

				result.str = '%' + funcInfo.upvalues.at(upvalueIndex);
				result.type = ValueType::STRING;

				codeStack.push_back(result);
			}
			//P_U;
			break;

		case OP_GETLOCAL:
			P_OP("GETLOCAL");
			{
				StackValue stackValue;
				int localIndex = GETARG_U(instr);

				if (funcInfo.locals.find(localIndex) == funcInfo.locals.end())
				{
					// local is not present in the list
					// name it.
					std::string localName = "loc" + std::to_string(localIndex - tf->numparams + 1);
					funcInfo.locals.insert(std::make_pair(localIndex, localName));
					++funcInfo.nLocals;

					funcStr += "local " + localName + " = " + codeStack[localIndex].str + "\n";
				}

				stackValue.str = funcInfo.locals.find(GETARG_U(instr))->second;
				stackValue.type = ValueType::STRING_LOCAL;
				codeStack.push_back(stackValue);
			}
			//P_L;
			break;

		case OP_GETGLOBAL:
			P_OP("GETGLOBAL");
			{
				StackValue stackValue;
				stackValue.index = GETARG_U(instr);
				stackValue.str = std::string(tf->kstr[stackValue.index]->str);
				stackValue.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(stackValue);
			}
			//P_K;
			break;

		case OP_GETTABLE:
			P_OP("GETTABLE");
			{
				std::vector<StackValue> args;
				StackValue result;

				args.push_back(codeStack.back());
				codeStack.pop_back();
				args.push_back(codeStack.back());
				codeStack.pop_back();

				result.str = args[1].str + '[' + args[0].str + ']';

				codeStack.push_back(result);
			}
			//P_NONE;
			break;

		case OP_GETDOTTED:
			P_OP("GETDOTTED");
			{
				int stringIndex = GETARG_U(instr);
				std::string str = tf->kstr[stringIndex]->str;
				StackValue target, result;

				target = codeStack.back();
				codeStack.pop_back();
				result.str = target.str + "." + str;
				result.type = ValueType::STRING;

				codeStack.push_back(result);
			}
			break;

		case OP_GETINDEXED:
			P_OP("UNIMPLEMENTED GETINDEXED");
			{
				//showErrorMessage("Unimplemented opcode GETINDEXED! exiting!", true);

				int localIndex = GETARG_U(instr);
				std::string local = funcInfo.locals.at(localIndex);
				StackValue target, result;

				target = codeStack.back();
				codeStack.pop_back();

				result.str = target.str + "[" + local + "]";
				result.type = target.type;

				codeStack.push_back(result);
			}
			P_L;
			break;

		case OP_PUSHSELF:
			P_OP("PUSHSELF");
			{
				int stringIndex = GETARG_U(instr);
				std::string str = tf->kstr[stringIndex]->str;
				StackValue target, result;

				/*target = codeStack.back();
				codeStack.pop_back();*/
				//result.str = target.str + ":" + str;
				result.str =  ":" + str;
				result.type = ValueType::STRING_PUSHSELF;

				codeStack.push_back(result);
			}
			//P_K;
			break;

		case OP_CREATETABLE:
			P_OP("CREATETABLE");
			{
				int numElems = GETARG_U(instr);
				StackValue result;
				if (numElems > 0)
				{
					/*if (codeStack.size() > 0 && codeStack.back().type == ValueType::STRING)
					result.str += ", ";*/
					result.str += "{ ";
					result.type = ValueType::TABLE_BRACE;
					result.index = numElems;
				}
				else
				{
					result.str = "{}";
					result.type = ValueType::STRING_GLOBAL;
				}

				codeStack.push_back(result);
			}
			//P_U;
			break;

		case OP_SETLOCAL:
			P_OP("SETLOCAL");
			{
				StackValue val;
				std::string local, result;
				val = codeStack.back();
				codeStack.pop_back();
				int localIndex = GETARG_U(instr);

				if (funcInfo.locals.size() <= localIndex)
				{
					std::cout << "WARNING!! SETLOCAL out of bounds!!! ignoring";
					break;
				}
				local = funcInfo.locals.at(localIndex);

				result = local + " = " + val.str + "\n";

				funcStr += result;
			}
			//P_L;
			break;

		case OP_SETGLOBAL:
			P_OP("SETGLOBAL");
			{
				StackValue val;
				std::string global, result;
				val = codeStack.back();
				global = tf->kstr[GETARG_U(instr)]->str;
				//codeStack.pop_back();
				if (val.type == ValueType::CLOSURE_STRING)
				{
					// we have a closure on the stack
					// insert after "function ", which is 9 chars
					codeStack.pop_back();
					val.str.insert(9, global);
					funcStr += val.str;

				}
				else
				{
					codeStack.pop_back();
					result = global + " = " + val.str + '\n';

					funcStr += result;
				}

			}
			//P_K;
			break;

		case OP_SETTABLE:
			P_OP("SETTABLE");
			{
				int targetIndex = GETARG_A(instr);
				int numElems = GETARG_B(instr);
				std::vector<StackValue> args;
				std::string result;

				if (targetIndex == numElems && numElems == 3)
				{
					for (int i = 0; i < numElems; ++i)
					{
						args.push_back(codeStack.back());
						codeStack.pop_back();
					}

					if (args[1].type == ValueType::STRING_GLOBAL)
					{
						args[1].str.erase(0, 1);
						args[1].str.erase(args[1].str.size() - 1);
					}

					result = args[2].str + "[" + args[1].str + "] = " + args[0].str;

					funcStr += result + '\n';
				}
				else
				{
					// unimplemented yet
					showErrorMessage("SETTABLE " + std::to_string(targetIndex) + " " + std::to_string(numElems) + " not implemented!!!", false);
				}
			}
			//P_AB;
			break;

		case OP_SETLIST:
			P_OP("SETLIST");
			{
				int targetIndex = GETARG_A(instr);
				int numItems = GETARG_B(instr);
				std::vector<StackValue> args;
				StackValue target, tableBrace, result;

				if (targetIndex != 0)
				{
					showErrorMessage("SETLIST not fully implemented!, first arg is nonzero!", false);
				}

				for (int i = 0; i < numItems; ++i)
				{
					args.push_back(codeStack.back());
					codeStack.pop_back();
				}

				tableBrace = codeStack.back();
				if (codeStack.back().type == ValueType::TABLE_BRACE)
				{
					if (tableBrace.index > numItems)
					{

						codeStack.pop_back();

						for (auto it = args.rbegin(); it != args.rend(); ++it)
						{
							result.str += (*it).str;

							if (--args.rend() != it)
								result.str += ", ";
						}

						result.str += ";";

						tableBrace.str += result.str;
						tableBrace.index -= numItems;

						codeStack.push_back(tableBrace);

						break;
					}

					codeStack.pop_back();
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

				codeStack.push_back(result);

			}
			//P_AB;
			break;

		case OP_SETMAP:
			P_OP("SETMAP");
			{
				int numElems = GETARG_U(instr);
				StackValue identifier, mapValue, tableBrace, result;
				std::vector<std::string> args;

				// TODO: nicer name
				bool hasRemainingElems = false;

				for (int i = 0; i < numElems; ++i)
				{
					mapValue = codeStack.back();
					codeStack.pop_back();
					identifier = codeStack.back();
					codeStack.pop_back();

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
				while (codeStack.back().type != ValueType::TABLE_BRACE)
				{
					args.push_back(codeStack.back().str);
					codeStack.pop_back();
				}

				tableBrace = codeStack.back();
				codeStack.pop_back();
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
					codeStack.push_back(tableBrace);
				}
				else
				{
					result.str.insert(0, tableBrace.str);
					result.str += " }";
				}

				codeStack.push_back(result);
			}
			//P_U;
			break;

		case OP_ADD:
			P_OP("ADD");
			{
				StackValue y, x, result;
				y = codeStack.back();
				codeStack.pop_back();
				x = codeStack.back();
				codeStack.pop_back();

				result.str = x.str + " + " + y.str;
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			break;

		case OP_ADDI:
			P_OP("ADDI");
			{
				int value;
				StackValue stackValue;
				value = GETARG_S(instr);
				stackValue = codeStack.back();
				StackValue newValue;
				codeStack.pop_back();

				std::string op;
				if (value >= 0)
					op = " + ";
				else
					op = "";

				newValue.str = stackValue.str + op + std::to_string(GETARG_S(instr));
				newValue.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(newValue);
			}
			//P_S;
			break;

		case OP_SUB:
			P_OP("SUB");
			{
				StackValue y, x, result;
				y = codeStack.back();
				codeStack.pop_back();
				x = codeStack.back();
				codeStack.pop_back();

				result.str = x.str + " - " + y.str;
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			break;

		case OP_MULT:
			P_OP("MULT");
			{
				StackValue y, x, result;
				y = codeStack.back();
				codeStack.pop_back();
				x = codeStack.back();
				codeStack.pop_back();

				result.str = "( " + x.str + " * " + y.str + " )";
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			break;

		case OP_DIV:
			P_OP("DIV");
			{
				StackValue y, x, result;
				y = codeStack.back();
				codeStack.pop_back();
				x = codeStack.back();
				codeStack.pop_back();

				result.str = "( " + x.str + " / " + y.str + " )";
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			break;

		case OP_POW:
			P_OP("POW");
			{
				StackValue y, x, result;
				y = codeStack.back();
				codeStack.pop_back();
				x = codeStack.back();
				codeStack.pop_back();

				result.str = "( " + x.str + " ^ " + y.str + " )";
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			break;

		case OP_CONCAT:
			P_OP("CONCAT");
			{
				int numArgs = GETARG_U(instr);
				StackValue result;
				std::vector<StackValue> args;
				for (int i = 0; i < numArgs; ++i)
				{
					args.push_back(codeStack.back());
					codeStack.pop_back();
				}
				for (int i = numArgs - 1; i >= 0; --i)
				{
					result.str += args[i].str;
					if ((i - 1) >= 0)
						result.str += "..";
				}

				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			//P_U;
			break;

		case OP_MINUS:
			P_OP("MINUS");
			{
				StackValue x, result;
				x = codeStack.back();
				codeStack.pop_back();

				result.str = "-" + x.str;
				result.type = ValueType::STRING_GLOBAL;
				codeStack.push_back(result);
			}
			P_NONE;
			break;

		case OP_NOT:
			P_OP("UNIMPLEMENTED NOT");
			{
				// showErrorMessage("Unimplemented opcode NOT! exiting!", true);
				std::string arg = "not " + codeStack.back().str;
				codeStack.pop_back();

				StackValue result;
				result.type = ValueType::STRING_GLOBAL;
				result.str = arg;

				codeStack.push_back(result);
			}
			P_NONE;
			break;

		case OP_JMPNE:
			P_OP("UNIMPLEMENTED JMPNE");
			{
				//showErrorMessage("Unimplemented opcode JMPNE! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPNE;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPEQ:
			P_OP("UNIMPLEMENTED JMPEQ");
			{
				//showErrorMessage("Unimplemented opcode JMPEQ! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPEQ;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}

			}
			P_J;
			break;

		case OP_JMPLT:
			P_OP("UNIMPLEMENTED JMPLT");
			{
				//showErrorMessage("Unimplemented opcode JMPLT! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPLT;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPLE:
			P_OP("UNIMPLEMENTED JMPLE");
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPLE;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPGT:
			P_OP("UNIMPLEMENTED JMPGT");
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPGT;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPGE:
			P_OP("UNIMPLEMENTED JMPGE");
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg1 = codeStack.back().str;
				codeStack.pop_back();
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.args.push_back(arg1);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPGE;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPT:
			P_OP("UNIMPLEMENTED JMPT");
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPT;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPF:
			P_OP("UNIMPLEMENTED JMPF");
			{
				//showErrorMessage("Unimplemented opcode JMPLE! exiting!", true);
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPF;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPONT:
			P_OP("UNIMPLEMENTED JMPONT");
			{
				// IDK YET
				//showErrorMessage("Unimplemented opcode JMPONT! exiting!", true);
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPONT;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMPONF:
			P_OP("UNIMPLEMENTED JMPONF");
			{
				// IDK YET
				//showErrorMessage("Unimplemented opcode JMPONF! exiting!", true);
				std::string arg0 = codeStack.back().str;
				codeStack.pop_back();
				int destLine = GETARG_S(instr) + line + 1;

				CondElem elem;
				elem.args.push_back(arg0);
				elem.dest = destLine;
				elem.lineNum = line;
				elem.jmpType = OP_JMPONF;
				elem.nextCond = CondElem::NONE;

				if (context.empty() || context.back().dest > destLine)
				{
					Context cont;
					cont.conds.push_back(elem);
					cont.dest = destLine;
					cont.type = Context::IF;
					cont.strIndex = funcStr.size();
					context.push_back(cont);
				}
				else
				{
					context.back().conds.push_back(elem);
					context.back().dest = elem.dest;
				}
			}
			P_J;
			break;

		case OP_JMP:
			P_OP("UNIMPLEMENTED JMP");
			{
				showErrorMessage("Unimplemented opcode JMP! continuing!", false);
				int dest = GETARG_S(instr);
				if (!context.empty() && dest < 0)
				{
					context.back().type = Context::WHILE;
				}
			}
			P_J;
			break;

		case OP_PUSHNILJMP:
			P_OP("UNIMPLEMENTED PUSHNILJMP");
			{
				//showErrorMessage("Unimplemented opcode PUSHNILJMP! exiting!", true);
				StackValue result;
				result.str = "nil";
				result.type = ValueType::NIL;

				codeStack.push_back(result);
			}
			P_NONE;
			break;

		case OP_FORPREP:
			P_OP("FORPREP");
			{
				StackValue val1, val2, val3;
				/*
				val3 = codeStack.back();
				codeStack.pop_back();
				val2 = codeStack.back();
				codeStack.pop_back();
				val1 = codeStack.back();
				codeStack.pop_back();*/
				val3 = codeStack[codeStack.size() - 1];
				val2 = codeStack[codeStack.size() - 2];
				val1 = codeStack[codeStack.size() - 3];



				std::string locName = "for" + std::to_string(funcInfo.nForLoops);
				++funcInfo.nForLoops;
				++funcInfo.nForLoopLevel;
				int locIndex = funcInfo.nLocals;
				++funcInfo.nLocals;
				funcInfo.locals.insert(std::make_pair(locIndex, locName));
				funcStr += "for " + locName + " = " + val1.str + ", " + val2.str + ", " +
					val3.str + "\ndo\n";
			}
			P_J;
			break;

		case OP_FORLOOP:
			P_OP("FORLOOP");
			{
				// the last local should be the forloop var
				--funcInfo.nLocals;
				--funcInfo.nForLoopLevel;
				funcInfo.locals.erase(funcInfo.nLocals);

				// clear control vars
				codeStack.pop_back();
				codeStack.pop_back();
				codeStack.pop_back();

				funcStr += "end\n";
			}
			P_J;
			break;

		case OP_LFORPREP:
			P_OP("LFORPREP");
			{
				//showErrorMessage("Unimplemented opcode LFORPREP! exiting!", true);
				StackValue tableName = codeStack.back();
				codeStack.pop_back();

				StackValue invisTable, index, value;
				invisTable.type = ValueType::STRING_LOCAL;
				invisTable.str = "_t";
				index.type = ValueType::STRING_LOCAL;
				index.str = "index";
				value.type = ValueType::STRING_LOCAL;
				value.str = "value";

				codeStack.push_back(invisTable);
				codeStack.push_back(index);
				codeStack.push_back(value);

				funcInfo.locals.insert(std::make_pair(funcInfo.nLocals++, "_t"));
				funcInfo.locals.insert(std::make_pair(funcInfo.nLocals++, "index"));
				funcInfo.locals.insert(std::make_pair(funcInfo.nLocals++, "value"));

				funcStr += "for index, value in " + tableName.str + "\ndo\n";
			}
			P_J;
			break;

		case OP_LFORLOOP:
			P_OP("UNIMPLEMENTED LFORLOOP");
			{
				//showErrorMessage("Unimplemented opcode LFORLOOP! exiting!", true);
				funcInfo.locals.erase(--funcInfo.nLocals);
				funcInfo.locals.erase(--funcInfo.nLocals);
				funcInfo.locals.erase(--funcInfo.nLocals);

				codeStack.pop_back();
				codeStack.pop_back();
				codeStack.pop_back();
				
				funcStr += "end\n";
			}
			P_J;
			break;

		case OP_CLOSURE:
			P_OP("CLOSURE");
			{
				int closureIndex = GETARG_A(instr);
				int numUpvalues = GETARG_B(instr);
				FuncInfo funcInfo;
				StackValue stackValue;
				std::string closureSrc;

				// pop all upvalues if any are found
				// and register them into funcInfo
				for (int i = 0; i < numUpvalues; ++i)
				{
					StackValue upvalue;
					upvalue = codeStack.back();
					codeStack.pop_back();

					funcInfo.upvalues.insert(std::make_pair(numUpvalues - (i + 1), upvalue.str));
				}

				// TODO: increase identation level
				//funcStr += "function ";

				//stackValue.index = funcStr.size();
				//stackValue.type = ValueType::CLOSURE_STR_INDEX;

				//findClosureArgs(tf->kproto[closureIndex], funcInfo);

				//int numArgs = funcInfo.locals.size();
				/*
				funcStr += '(';
				for (int i = 0; i < numArgs; ++i)
				{
					funcStr += ("arg" + std::to_string(i + 1));

					if (i != (numArgs - 1))
						funcStr += ", ";
				}
				funcStr += ")\n";
				*/

				// decompile closure
				funcInfo.isMain = false;
				closureSrc = decompileFunction(tf->kproto[closureIndex], funcInfo);

				stackValue.str = closureSrc;
				stackValue.type = ValueType::CLOSURE_STRING;

				// closureSrc += "end\n";

				codeStack.push_back(stackValue);
			}
			//P_F;
			break;

		}

		if (instr == OP_END)
		{
			//sourceStr += funcStr;
			if (!funcInfo.isMain)
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
			std::cout << "File " << path.filename() << " successfully decompiled!\n";
			sourceStr.clear();
			m_format.reset();
		}
	}
	else
	{
		filesystem::recursive_directory_iterator dir(path), end;

		while (dir != end)
		{
			if (filesystem::is_regular_file(dir->path()))
			{
				sourceStr = decompileFile(dir->path().string().c_str());
				if (!sourceStr.empty())
				{
					saveFile(sourceStr, dir->path().parent_path().string() + "\\" + dir->path().stem().string() + "_d" + dir->path().extension().string());
					std::cout << "File " << dir->path().filename() << " successfully decompiled!\n";
					m_format.reset();
					sourceStr.clear();
				}
			}

			sourceStr.clear();
			++dir;
		}
	}

}

int Decompiler::evalCondition(std::string &funcStr, Instruction* p, Instruction* code, std::vector<Decompiler::StackValue> &codeStack, CondElem currentCond)
{

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
	sourceStr = decompileFunction(tf, mainInfo);

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

void Decompiler::findClosureArgs(const Proto *tf, FuncInfo &funcInfo)
{
	const Instruction* code = tf->code;
	const Instruction* p = code;

	for (;;)
	{
		Instruction instr = *p;

		switch (GET_OPCODE(instr))
		{
		case OP_GETLOCAL:
		{
			std::string argName;
			int argIndex = GETARG_U(instr);
			argName = "arg" + std::to_string(argIndex + 1);

			funcInfo.locals.insert(std::make_pair(argIndex, argName));
		}
		break;

		case OP_SETLOCAL:
		{
			std::string argName;
			int argIndex = GETARG_U(instr);
			argName = "arg" + std::to_string(argIndex + 1);

			funcInfo.locals.insert(std::make_pair(argIndex, argName));
		}
		break;
		}

		if (instr == OP_END)
		{
			break;
		}

		p++;
	}
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

	if (exitError)
	{
		// pause
		char f;
		std::cin >> f;
		std::exit(1);
	}
}