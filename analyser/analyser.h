#pragma once

#include "error/error.h"
#include "instruction/instruction.h"
#include "tokenizer/token.h"

#include <vector>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstddef> // for std::size_t

//指令字符串数组
extern std::string command[20][1000];
extern int order;
//二进制指令字符串数组
extern std::string TwoCommand[3];
extern std::string TwoFuncCommand[20][1000];
extern int Torder;

namespace cc0 {

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
	public:
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _current_pos(0, 0) {}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;

		// 唯一接口
		std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyse();

		
	private:
		// 所有的递归子程序

		// <程序>
		std::optional<CompilationError> analyseProgram();
		// 变量
		std::optional<CompilationError> analyse_variable();
		// <常量声明>
		std::optional<CompilationError> analyseConstantDeclaration();
		// <变量声明>
		std::optional<CompilationError> analyseVariableDeclaration();
		// <加法表达式>
		std::optional<CompilationError> analyseAdditiveExpression();
		// <赋值语句>
		std::optional<CompilationError> analyseAssignmentStatement();
		// <输出语句>
		std::optional<CompilationError> analyseOutputStatement();
		// 乘法表达式
		std::optional<CompilationError> analyseMultiplicativeExpression();
		// <一元表达式>
		std::optional<CompilationError> analyseUnaryExpression();

		//<函数调用>
		std::optional<CompilationError> analyseFunctionCall(const std::string&);
		//<函数定义>
		std::optional<CompilationError> analyseFunctionDeclaration();
		//<参数声明>
		std::optional<CompilationError> analyseParameterDeclaration();
		//<复合语句>
		std::optional<CompilationError> analyseCompoundStatement();
		//<语句序列>
		std::optional<CompilationError> analyseStatementSeq();
		//<语句>
		std::optional<CompilationError> analyseStatement();
		//<条件语句>
		std::optional<CompilationError> analyseConditionStatement();
		//<条件>
		std::optional<CompilationError> analyseCondition(int&,int&);
		//<循环语句>
		std::optional<CompilationError> analyseLoopStatement();
		//<可打印列表>
		std::optional<CompilationError> analysePrintableList();
		//二进制过程的函数
		//将数字转换为十六进制的字符串，参数并给出位数(十进制数字，传回的字符串，要求转换的位数)
		std::string TenToSixteen(int ,int ); 
		//将十六进制转换为十六进制字符串，固定转换位数
		std::string SixteenToSixteen(std::string,int);


		// Token 缓冲区相关操作

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();

		// 下面是符号表相关操作

		// helper function
		// 添加变量、常量
		void addVariable(const std::string&);
		void addConstant(const std::string&);
		// 判断变量名是否被声明过
		bool isDeclared(const std::string&);
		// 是否是变量
		bool isVariable(const std::string&);
		// 是否是常量
		bool isConstant(const std::string&);
		//获取变量与当前层级的层级差
		int getLevelDifference(const std::string&);
		//获取变量的地址偏移量
		int getOffset(const std::string&);
		//是否是函数名
		bool isFunctionName(const std::string&);

		std::string getFunctionType(const std::string&);//根据函数名获取函数的类型
		int getParametersNumber(const std::string&);//根据函数名获取函数的参数个数
		int getFunctionIndex(const std::string&);

		//是否是level = 0的变量
		bool isDeclared_inLevel0(const std::string&);
		//是否是level = 1的变量
		bool isDeclared_inLevel1(const std::string&);
		//将函数放进常量表
		void addFunctionToConstTable(const std::string&);
		//将函数放进函数表里
		void addFunctionToFunctionTable(const std::string&);
		//清空函数表中二级标识符
		void EmptySymbolLegend();
		//判断函数名是否被声明过
		bool FunctionIsDeclared(const std::string&);



	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
		std::vector<Instruction> _instructions;
		std::pair<uint64_t, uint64_t> _current_pos;


		//符号表定义
		struct SymbolLegend{
			int level;//级
			std::string name;//变量名
			int index;//偏移量
			bool change;//能否改变
		};
		SymbolLegend SL[1000];
		int nextSymbolLegendIndex;//当前变量的偏移量

		int NowLevel;//当前程序所在的等级


		//常量表定义
		struct _ConstTable{
			int index;
			std::string type;
			std::string name;
		};
		_ConstTable ConstTable[1000];
		//常量表中函数名的index
		int Constindex;

		//函数表中函数名的index
		int Functionindex;
		//函数的大小
		int FunctionSize;
		//函数的参数个数
		int numberOfParameters;
		//函数的名字
		std::string FunctionName;
		//函数的类型
		std::string FunctionType;

		//函数表定义
		struct _FunctionTable{
			int index;
			std::string name;
			int size;
			int level;

			std::string type;//void || int
			int parameters_number; 
		};
		_FunctionTable FunctionTable[1000];

		//函数第几个
		int numberFunction;
		//有没有return语句
		bool hasReturn;
	};
}
