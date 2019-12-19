#include "analyser.h"
#include "instruction/instruction.h"
#include <climits>
#include <sstream>

std::string command[20][1000];
int order;

std::string TwoCommand[3];
std::string  TwoFuncCommand[20][1000];
int Torder;

namespace cc0 {
	std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyser::Analyse() {

		auto err = analyseProgram();
		if (err.has_value())
			return std::make_pair(std::vector<Instruction>(), err);
		else
			return std::make_pair(_instructions, std::optional<CompilationError>());
	}

	// <程序> ::= <主过程>
	std::optional<CompilationError> Analyser::analyseProgram() {
		// 示例函数，示例如何调用子程序
		numberFunction = 1;
		// 全局变量声明
		Constindex = 0;order = 0;
		NowLevel = 0;nextSymbolLegendIndex = 0;
		auto err1 = analyse_variable();
		if(err1.has_value())
			return err1;
		//将start段一共有多少条语句写道二进制前端
		std::string s = TenToSixteen(order,4);
		TwoCommand[numberFunction] = s+ TwoCommand[numberFunction];


		numberFunction = 3;
		// 函数声明
		Constindex = 0;Functionindex = 0;
		auto err2 = analyseFunctionDeclaration();
		if(err2.has_value())
			return err2;


		//后续处理常量表
		numberFunction = 0;
		for(int i = 0;i<1000;i++){
			if(ConstTable[i].name != ""){
				command[numberFunction][i] = std::to_string(ConstTable[i].index) + " " +  
				ConstTable[i].type + " " + '"' + ConstTable[i].name +'"';
			}
		}
			//二进制处理
			//获取常量个数
			int ConstNumber = 0;
			for(int i = 0;i<1000;i++){
				if(ConstTable[i].name != "")
					ConstNumber++;
			}
			std::string s0 = TenToSixteen(ConstNumber,4);
			TwoCommand[numberFunction] = s0;

			for(int i = 0;i<1000;i++){
				if(ConstTable[i].name != "") {
					//type == string
					TwoCommand[numberFunction] = TwoCommand[numberFunction] + "00";
					//名字长度
					std::string name = ConstTable[i].name;
					int length = name.length();
					std::string s = TenToSixteen(length , 4);
					TwoCommand[numberFunction] = TwoCommand[numberFunction] + s;
					//名字的十六进制表示
					for(int i= 0;i<length;i++) {
						int value = name[i];
						std::string s = TenToSixteen(value,2);
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + s;
					}
				}
			}


		//后续处理函数表
		numberFunction = 2;
		for(int i = 0;i<1000;i++) {
			if(FunctionTable[i].name != ""){
				command[numberFunction][i] = std::to_string(FunctionTable[i].index) + " " + 
											std::to_string(ConstTable[i].index) + " " + 
											std::to_string(FunctionTable[i].size) + " " + 
											std::to_string(FunctionTable[i].level);
			}
		}
			//二进制处理
			int n = 0;
			for(int i = 0;i<1000;i++){
				if(FunctionTable[i].name != "")
					n++;
			}
			std::string s1 = TenToSixteen(n,4);
			TwoCommand[numberFunction] = s1;

		return {};
	}
	// 变量声明的主函数
	std::optional<CompilationError> Analyser::analyse_variable() {
		
		while(true){
			auto next = nextToken();
			if(!next.has_value())
				return {};
			//如果是常量声明语句：
			if(next.value().GetType() == TokenType::CONST){
				auto err = analyseConstantDeclaration();
				if (err.has_value())
					return err;
				continue;
			}

			//如果是INT
			else if(next.value().GetType() == TokenType::INT) {
				//判断是变量声明还是函数
				next = nextToken();
				if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

				next = nextToken();
				//如果是函数定义且当前当前层级level为0，就退出
				if(next.value().GetType() == TokenType::LEFT_BRACKET) {
					if(NowLevel == 0) {
						unreadToken();//该读(
						unreadToken();//该读标识符
						unreadToken();//该读INT
						return {};
						break;
					}
					if(NowLevel == 1)//函数里面不可再定义函数
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				}
				//如果是变量声明
				else if(next.value().GetType() == TokenType::EQUAL_SIGN 
						|| next.value().GetType() == TokenType::COMMA 
						|| next.value().GetType() == TokenType::SEMICOLON) 
				{
					unreadToken();//该读= || ; || ,
					unreadToken();//该读标识符
					auto err = analyseVariableDeclaration();
					if(err.has_value())
						return err;
					continue;
				}
				else
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			else{
				unreadToken();
				return {};
			}
		}

		return {};
	}

	// <常量声明> ::= {<常量声明语句>}
	// <常量声明语句> ::= 'const'<标识符>'='<常表达式>';'
	std::optional<CompilationError> Analyser::analyseConstantDeclaration() {
		
		// 预读一个 token
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::INT) {
			//这里还要修改报错信息，没有INT。
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		while (true) {

			// <常量声明语句>
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

			//当前层级是最外围即为0时
			if(NowLevel == 0){
				if (isDeclared_inLevel0(next.value().GetValueString()))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
			}
			//当前层级是1级时
			if(NowLevel == 1){
				if(isDeclared_inLevel1(next.value().GetValueString()))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
				else if(isDeclared_inLevel0(next.value().GetValueString()))
				{}
			}
			
			std::string sname = next.value().GetValueString();
			//addConstant(next.value());

			// '='
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);

			// <表达式>
			//int32_t val;
			auto err = analyseAdditiveExpression();
			if (err.has_value())
				return err;
			
			next = nextToken();
			//如果是,
			if(next.value().GetType() == TokenType::COMMA){
				//把这个变量加入符号表
				addConstant(sname);
				continue;
			}

			//如果是;
			else if(next.value().GetType() == TokenType::SEMICOLON){
				addConstant(sname);
				break;
			}

			else
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}



	//变量声明
	std::optional<CompilationError> Analyser::analyseVariableDeclaration() {
		
		while (true) {
			auto next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

			//当前层级是最外围即为0时
			if(NowLevel == 0){
				if (isDeclared_inLevel0(next.value().GetValueString()))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
			}
			//当前层级是1级时
			if(NowLevel == 1){
				if(isDeclared_inLevel1(next.value().GetValueString()))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
				else if(isDeclared_inLevel0(next.value().GetValueString()))
				{}
			}

			std::string sname = next.value().GetValueString();

			next = nextToken();
			//如果显式的赋值
			if(next.value().GetType() == TokenType::EQUAL_SIGN) {
				auto err = analyseAdditiveExpression();
				if (err.has_value())
					return err;
				
				next = nextToken();
				//如果是,
				if(next.value().GetType() == TokenType::COMMA){
				//把这个变量加入符号表
					addVariable(sname);
					continue;
				}
				//如果是;
				else if(next.value().GetType() == TokenType::SEMICOLON){
					addVariable(sname);
					return {};
					break;
				}
				else
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			//如果是隐式的赋值且后面是,
			else if(next.value().GetType() == TokenType::COMMA) {
				command[numberFunction][order] = std::to_string(order) + "\t" + "ipush " + "0"; order++;
					//二进制
					if(numberFunction == 1){
						std::string s = TenToSixteen(0,8);
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "02" + s;
					}
					else{
						std::string s = TenToSixteen(0,8);
						TwoFuncCommand[numberFunction][Torder] = "02" + s; Torder++;
					}
				addVariable(sname);
				continue;
			}
			//如果是隐式的赋值且后面是;
			else if(next.value().GetType() == TokenType::SEMICOLON) {
				command[numberFunction][order] = std::to_string(order) + "\t" + "ipush " + "0"; order++;
					//二进制
					if(numberFunction == 1){
						std::string s = TenToSixteen(0,8);
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "02" + s;
					}
					else{
						std::string s = TenToSixteen(0,8);
						TwoFuncCommand[numberFunction][Torder] = "02" + s; Torder++;
					}
				addVariable(sname);
				break;
			}

			else
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}



	// <表达式> ::= <乘法表达式>{<加法型运算符><乘法表达式>}
	std::optional<CompilationError> Analyser::analyseAdditiveExpression() {
		auto err = analyseMultiplicativeExpression();
		if (err.has_value())
			return err;

		// {<加法型运算符><乘法表达式>}
		while (true) {
			// 预读
			auto next = nextToken();
			if (!next.has_value())
				return {};
			auto type = next.value().GetType();
			if (type != TokenType::PLUS_SIGN && type != TokenType::MINUS_SIGN) {
				unreadToken();
				return {};
			}

			// <乘法表达式>
			err = analyseMultiplicativeExpression();
			if (err.has_value())
				return err;

			// 根据结果生成指令
			if (type == TokenType::PLUS_SIGN){
				
				command[numberFunction][order] = std::to_string(order) + "\t" + "iadd" ; order++;
					if(numberFunction == 1){
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "30";
					}
					else{
						TwoFuncCommand[numberFunction][Torder] = "30"; Torder++;
					}
			}	
			else if (type == TokenType::MINUS_SIGN){
				
				command[numberFunction][order] = std::to_string(order) + "\t" + "isub" ; order++;
					if(numberFunction == 1){
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "34";
					}
					else{
						TwoFuncCommand[numberFunction][Torder] = "34"; Torder++;
					}
			}
		}

		return {};
	}


	std::optional<CompilationError> Analyser::analyseMultiplicativeExpression() {
		auto err = analyseUnaryExpression();
		if (err.has_value())
			return err;
		
		while (true) {
			// 预读
			auto next = nextToken();
			if (!next.has_value())
				return {};
			auto type = next.value().GetType();
			if (type != TokenType::MULTIPLICATION_SIGN && type != TokenType::DIVISION_SIGN) {
				unreadToken();
				return {};
			}
			//<一元表达式>
			auto err = analyseUnaryExpression();
			if (err.has_value())
			return err;
			//根据结果生成指令
			if (type == TokenType::MULTIPLICATION_SIGN){
				command[numberFunction][order] = std::to_string(order) + "\t" + "imul" ; order++;
					if(numberFunction == 1){
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "38";
					}
					else{
						TwoFuncCommand[numberFunction][Torder] = "38"; Torder++;
					}
			}
			else if (type == TokenType::DIVISION_SIGN){
				command[numberFunction][order] = std::to_string(order) + "\t" + "idiv" ; order++;
					if(numberFunction == 1){
						TwoCommand[numberFunction] = TwoCommand[numberFunction] + "3c";
					}
					else{
						TwoFuncCommand[numberFunction][Torder] = "3c"; Torder++;
					}
			}
		}

		return {};
	}


	//一元表达式
	std::optional<CompilationError> Analyser::analyseUnaryExpression(){
		auto next = nextToken();
		auto prefix = 1;
		if (!next.has_value())//这个报错要改
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		
		if (next.value().GetType() == TokenType::PLUS_SIGN)
			prefix = 1;
		else if (next.value().GetType() == TokenType::MINUS_SIGN)
			prefix = -1;
		else
			unreadToken();
		//预读
		next = nextToken();
		if (!next.has_value())
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		switch (next.value().GetType()) {
			case IDENTIFIER:{
				//先看看是否是函数名
				//若是函数名
				if(isFunctionName(next.value().GetValueString()) && !isDeclared(next.value().GetValueString())) {
					std::string stype = getFunctionType(next.value().GetValueString());
					if(stype == "void")
					//这个报错要改，用void函数进行运算了
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);

					auto err = analyseFunctionCall(next.value().GetValueString());
					if(err.has_value())
						return err;
				}
				else {
					//若不是函数名
					//看看他是否声明过
					if(!isDeclared(next.value().GetValueString())){
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
					}
					int level_diff = getLevelDifference(next.value().GetValueString());
					int offset = getOffset(next.value().GetValueString());
					command[numberFunction][order] = std::to_string(order) + "\t" + "loada " + std::to_string(level_diff) + ", " + std::to_string(offset) ; order++;
						if(numberFunction == 1){
							std::string s1 = TenToSixteen(level_diff,4);
							std::string s2 = TenToSixteen(offset,8);
							TwoCommand[numberFunction] = TwoCommand[numberFunction] + "0a"+ s1+ s2;
						}
						else{
							std::string s1 = TenToSixteen(level_diff,4);
							std::string s2 = TenToSixteen(offset,8);
							TwoFuncCommand[numberFunction][Torder] = "0a"+ s1 + s2 ; Torder++;
						}
					command[numberFunction][order] = std::to_string(order) + "\t" + "iload"; order++;
						if(numberFunction == 1){
							TwoCommand[numberFunction] = TwoCommand[numberFunction] + "10";
						}
						else{
							TwoFuncCommand[numberFunction][Torder] = "10"; Torder++;
						}
				}
				break;
			}
			case DECIMAL_LITERAL:{
				std::string s = next.value().GetValueString();
				command[numberFunction][order] = std::to_string(order) + "\t" + "ipush " + s; order++;
				std::stringstream ss;
				int a;
				ss << s;
				ss >> a;
				std::string s1 =  TenToSixteen(a,8);
				if(numberFunction == 1){
					TwoCommand[numberFunction] = TwoCommand[numberFunction] + "02"+ s1 ;
				}
				else{
					TwoFuncCommand[numberFunction][Torder] = "02"+ s1 ; Torder++;
				}
				break;
			}
			case HEXADECIMAL_LITERAL:{
				std::string s = next.value().GetValueString();
				if(s.length() > 10)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIntegerOverflow);
				command[numberFunction][order] = std::to_string(order) + "\t" + "ipush " + s; order++;
				std::string s1 =  SixteenToSixteen(s,8);
				if(numberFunction == 1){
					TwoCommand[numberFunction] = TwoCommand[numberFunction] + "02"+ s1;
				}
				else{
					TwoFuncCommand[numberFunction][Torder] = "02"+ s1; Torder++;
				}
				break;
			}
			case LEFT_BRACKET:{
				auto err = analyseAdditiveExpression();
				if(err.has_value()){
					return err;
				}
				auto right = nextToken();
				if (!right.has_value() || right.value().GetType() != TokenType::RIGHT_BRACKET)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
				break;
			}
			default:
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		
		// 取负
		if (prefix == -1){
			command[numberFunction][order] = std::to_string(order) + "\t" + "ineg" ; order++;
			if(numberFunction == 1){
			 TwoCommand[numberFunction] = TwoCommand[numberFunction] + "40";
			}
			else{
				TwoFuncCommand[numberFunction][Torder] = "40";Torder++;
			}
		}
		return {};
	}


	//函数调用
	std::optional<CompilationError> Analyser::analyseFunctionCall(const std::string&functionName) {
		//查找函数内部有没有与函数名同名的变量名，如果有，报错不能进行函数调用
		for(int i = 0;i<1000;i++){
			if(SL[i].name == functionName)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}

		int parametersnumber = 0;
		auto next = nextToken();
		if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		//如果next是)
		next = nextToken();
		if(next.value().GetType() == TokenType::RIGHT_BRACKET)
			{}
		else{
			unreadToken();
			while(true){
				auto err = analyseAdditiveExpression();
				if (err.has_value())
					return err;
				next = nextToken();
				if(next.value().GetType() == TokenType::COMMA){
					parametersnumber++;
					continue;
				}
				else if(next.value().GetType() == TokenType::RIGHT_BRACKET){
					parametersnumber++;
					break;
				}
				else
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
		}
		if(parametersnumber != getParametersNumber(functionName))
		//报错要改，传入的参数个数错误。
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		else
		{
			int index = getFunctionIndex(functionName);
			command[numberFunction][order] = std::to_string(order) + "\t" + "call " + std::to_string(index); order++;

			//二进制
			std::string s = TenToSixteen(index , 4);
			TwoFuncCommand[numberFunction][Torder] = "80" + s; Torder++;
		}
		
		return {};
	}

	//函数定义
	std::optional<CompilationError> Analyser::analyseFunctionDeclaration() {
		while(true){
			order = 0;FunctionName = "";FunctionType = "";
			NowLevel = 1;nextSymbolLegendIndex = 0;
			FunctionSize = 0;numberOfParameters = 0;hasReturn = false;

			//二进制
			Torder = 4;

			auto next = nextToken();
			if(!next.has_value()) {
				//寻找main函数
				if(!isFunctionName("main"))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
				else 
					return {};
			}			
			if(next.value().GetType() == TokenType::VOID || next.value().GetType() == TokenType::INT) {
				FunctionType = next.value().GetValueString();
				next = nextToken();
				if(next.value().GetType() == TokenType::IDENTIFIER){
					FunctionName = next.value().GetValueString();
					
					//判断函数名是否在0级被声明过
					if(FunctionIsDeclared(FunctionName))//被声明过
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoEnd); 

					//将函数放进常量表
					addFunctionToConstTable(FunctionName);
					//将函数写入函数表（size和参数个数没放进去，因为还不知道）
					addFunctionToFunctionTable(FunctionName);

					next = nextToken();
					if(next.value().GetType() == TokenType::LEFT_BRACKET){
						//参数声明
						auto err1 = analyseParameterDeclaration();
						if(err1.has_value())
							return err1;
						
						//将参数个数和参数占用的solt数放进函数表
						FunctionTable[Functionindex-1].size = FunctionSize;
						FunctionTable[Functionindex-1].parameters_number = numberOfParameters;
						//此时，已经将函数放进了函数表

						//读入{
						next = nextToken();
						if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACE)//缺少左大括号
							return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoEnd);
						//进入复合语句
						auto err2 = analyseCompoundStatement();
						if(err2.has_value())
							return err2;
						//函数完毕了

						//此时看一下有没有return语句，如果没有填上return语句
						if(!hasReturn) {
							if(FunctionType == "void")
								{command[numberFunction][order] = std::to_string(order) + "\t" + "ret"; order++;
								 TwoFuncCommand[numberFunction][Torder] = "88"; Torder++;
								}
							else if(FunctionType == "int"){
								command[numberFunction][order] = std::to_string(order) + "\t" + "ipush 0"; order++;
								TwoFuncCommand[numberFunction][Torder] = "0200000000"; Torder++;
								command[numberFunction][order] = std::to_string(order) + "\t" + "iret"; order++;
								TwoFuncCommand[numberFunction][Torder] = "89"; Torder++;
							}
						}
						//二进制函数模块前面四个Torder的操作
						//函数index
						int name_index = getFunctionIndex(FunctionName);
						std::string s1 = TenToSixteen(name_index,4);
						TwoFuncCommand[numberFunction][0] = s1;
						//函数参数个数
						int params_length = getParametersNumber(FunctionName);
						std::string s2 = TenToSixteen(params_length,4);
						TwoFuncCommand[numberFunction][1] = s2;
						//level
						TwoFuncCommand[numberFunction][2] = "0001";
						//instructions_count
						int instructions_count = order;
						std::string s3 = TenToSixteen(instructions_count,4);
						TwoFuncCommand[numberFunction][3] = s3;

						//将符号表中2级level的标识符清空。
						EmptySymbolLegend();
						numberFunction++;
						continue;
						//////////////////////////////////////////////函数完毕
					}
					else//缺少左括号
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				}
				else{//报错要改，缺少函数名
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
				}
			}
			else {
				//报错要改，不是个类型
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLeadingZero);
			}
		}
	}

	//参数声明
	std::optional<CompilationError> Analyser::analyseParameterDeclaration() {
		bool if_isConstent = false;
		while(true){
			auto next = nextToken();
			if(next.value().GetType() == TokenType::INT) {
				next = nextToken();
				if(next.value().GetType() == TokenType::IDENTIFIER) {
					if(if_isConstent == false) {
						//将参数作为标识符放进符号表
						addVariable(next.value().GetValueString());//变量地址+1
						numberOfParameters++;//参数个数+1
						FunctionSize++;//函数大小+1
					}
					if(if_isConstent == true) {
						//将参数作为标识符放进符号表
						addConstant(next.value().GetValueString());
						numberOfParameters++;//参数个数+1
						FunctionSize++;//函数大小+1

						if_isConstent = false;
					}				
					next = nextToken();
					if(next.value().GetType() == TokenType::COMMA)
						continue;
					else if(next.value().GetType() == TokenType::RIGHT_BRACKET)
						return{};
					else//报错要改，没有逗号或者右括号
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
				}
				else
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
			}

			else if(next.value().GetType() == TokenType::CONST) {
				next = nextToken();
				if(next.value().GetType() == TokenType::INT) {
					unreadToken();
					if_isConstent = true;
					continue;
				}
				else {//缺少类型标识符INT
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
				}
			}

			else if(next.value().GetType() == TokenType::RIGHT_BRACKET) {
				return {};
			}
			else//报错要改，没有标识符或者右括号
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}

	}

	//复合语句
	std::optional<CompilationError> Analyser::analyseCompoundStatement() {
		//函数内部变量声明
		auto err1 = analyse_variable();
		if(err1.has_value())
			return err1;
		//语句序列
		auto err2 = analyseStatementSeq();
		if(err2.has_value())
			return err2;
		return {};
	}

	//语句序列
	std::optional<CompilationError> Analyser::analyseStatementSeq() {
		while(true) {
			auto next = nextToken();
			if(!next.has_value())//缺少右大括号
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			//如果是右大括号，说明语句序列结束，函数结束。
			if(next.value().GetType() == TokenType::RIGHT_BRACE) {
				//语句序列结束,返回。
				return {};
			}
			//如果是语句,就进入语句子程序
			else {
				unreadToken();
				auto err = analyseStatement();
				if(err.has_value())
					return err;
			}
		}
	}

	//语句
	std::optional<CompilationError> Analyser::analyseStatement() {
		auto next = nextToken();
		if(!next.has_value())//缺少右大括号
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		//如果是左大括号，说明是语句序列。
		if(next.value().GetType() == TokenType::LEFT_BRACE) {
			auto err = analyseStatementSeq();
			if(err.has_value())
				return err;
			
			return {};
		}
		//如果是分号，就是分号
		else if(next.value().GetType() == TokenType::SEMICOLON) {
			return {};
		}
		//如果是标识符，那么有两种情况，一种可能是赋值语句，另一种可能是函数调用
		else if(next.value().GetType() == TokenType::IDENTIFIER) {
			std::string sname = next.value().GetValueString();
			//接下来判断他是赋值语句还是标识符
			next = nextToken();
			//是赋值语句，转入赋值语句子程序；
			if(next.value().GetType() == TokenType::EQUAL_SIGN) {
				if(!isDeclared(sname))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
				if(isConstant(sname))
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
				//赋值语句
				int level_diff = getLevelDifference(sname);//获取层级差
				int offset = getOffset(sname);//获取地址偏移量
				command[numberFunction][order] = std::to_string(order) + "\t" + "loada " + std::to_string(level_diff) + ", " + std::to_string(offset) ; order++;
					//二进制
					std::string s1 = TenToSixteen(level_diff,4);
					std::string s2 = TenToSixteen(offset,8);
					TwoFuncCommand[numberFunction][Torder] = "0a"+ s1 + s2 ; Torder++;
				//表达式
				auto err =  analyseAdditiveExpression();
				if(err.has_value())
					return err;
				//将弹出一个数值，弹出一个地址值，然后将这个数值赋值到这个地址里面
				command[numberFunction][order] = std::to_string(order) + "\t" + "istore"; order++;
					//二进制
					TwoFuncCommand[numberFunction][Torder] = "20"; Torder++;
				//检查有没有;
				next = nextToken();
				if(next.value().GetType() != TokenType::SEMICOLON)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				return {};
			}
			//是函数调用，进入函数调用子程序
			else if(next.value().GetType() == TokenType::LEFT_BRACKET) {
				//看这个标识符的名称是否是函数名
				if(!isFunctionName(sname))//函数没有被定义过，这不是一个函数名
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoEnd);
				unreadToken();//该读左小括号
				//函数调用
				auto err = analyseFunctionCall(sname);
				if(err.has_value())
					return err;
				//检查有没有;
				next = nextToken();
				if(next.value().GetType() != TokenType::SEMICOLON)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				return {};
			}
			else 
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		//如果是if，则是条件语句
		else if(next.value().GetType() == TokenType::IF) {
			auto err = analyseConditionStatement();
			if(err.has_value())
				return err;
			return {};
		}
		//如果是while，则是循环语句
		else if(next.value().GetType() == TokenType::WHILE) {
			auto err = analyseLoopStatement();
			if(err.has_value())
				return  err;
			return {};
		}
		//如果是打印声明
		else if(next.value().GetType() == TokenType::PRINT) {
			auto next = nextToken();
			if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)//缺少左括号
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			auto err = analysePrintableList();
			if(err.has_value())
				return err;
			next = nextToken();
			if(next.value().GetType() != TokenType::SEMICOLON)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			return {};
		}
		//如果是扫描语句
		else if(next.value().GetType() == TokenType::SCAN) {
			auto next = nextToken();
			if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)//缺少左括号
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			next = nextToken();
			if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
			if(!isDeclared(next.value().GetValueString()))
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
			if(isConstant(next.value().GetValueString()))
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
			int level_diff = getLevelDifference(next.value().GetValueString());//获取层级差
			int offset = getOffset(next.value().GetValueString());//获取地址偏移量
			command[numberFunction][order] = std::to_string(order) + "\t" + "loada " + std::to_string(level_diff) + ", " + std::to_string(offset) ; order++;
				//二进制
				std::string s1 = TenToSixteen(level_diff,4);
				std::string s2 = TenToSixteen(offset,8);
				TwoFuncCommand[numberFunction][Torder] = "0a"+ s1 + s2 ; Torder++;
			
			//从标准输入解析一个可有符号的十进制整数，将其转换为int得到value，将value压入栈。
			command[numberFunction][order] = std::to_string(order) + "\t" + "iscan"; order++;
				TwoFuncCommand[numberFunction][Torder] = "b0"; Torder++;
			//将弹出一个数值，弹出一个地址值，然后将这个数值赋值到这个地址里面
			command[numberFunction][order] = std::to_string(order) + "\t" + "istore"; order++;
				TwoFuncCommand[numberFunction][Torder] = "20"; Torder++;
			//检查有没有右括号
			next = nextToken();
			if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)//缺少右括号
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			//检查有没有;
			next = nextToken();
			if(next.value().GetType() != TokenType::SEMICOLON)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			return {};
		}
		//如果是返回语句
		else if(next.value().GetType() == TokenType::RETURN) {
			hasReturn = true;

			//std::string Fname = FunctionName;
			std::string Ftype = FunctionType;
			next = nextToken();
			if(next.value().GetType() == TokenType::SEMICOLON) {
				if(Ftype == "void")
					{command[numberFunction][order] = std::to_string(order) + "\t" + "ret"; order++;
					 TwoFuncCommand[numberFunction][Torder] = "88"; Torder++;
					}
				else//不能在非void函数中使用无值的返回语句。
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			else {
				unreadToken();
				auto err = analyseAdditiveExpression();
				if(err.has_value())
					return err;
				if(Ftype == "int")
					{command[numberFunction][order] = std::to_string(order) + "\t" + "iret"; order++;
					 TwoFuncCommand[numberFunction][Torder] = "89"; Torder++;
					}
				else //不能在返回类型为void的函数中使用有值的返回语句
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoEnd);
				next = nextToken();
				if(next.value().GetType() != TokenType::SEMICOLON)
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			return {};
		}

		else//缺少右大括号
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	}

	//条件语句
	std::optional<CompilationError> Analyser::analyseConditionStatement() {
		//将位置初始化							//二进制
		int	beforeIf = -1,endIf = -1;			int	TbeforeIf = -1,TendIf = -1;
		auto next = nextToken();
		if(next.value().GetType() != TokenType::LEFT_BRACKET)//缺少左括号
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		//跳转到条件子程序
		auto err1 = analyseCondition(beforeIf,TbeforeIf);
		if(err1.has_value())
			return err1;
		//进入IF所对应的Statement
		auto err2 = analyseStatement();
		if(err2.has_value())
			return err2;

		//看看后面有没有else语句
		next = nextToken();
		if(!next.has_value() || next.value().GetType() != TokenType::ELSE){
			unreadToken();
			//此时的order位于If语句块的末尾，beforeIf的跳转，应该是跳转到这里。这里是在else语句块跳转之前。
			//所以将之前的跳转语句补充完整。
			if(beforeIf > 0){
				command[numberFunction][beforeIf] = command[numberFunction][beforeIf] + std::to_string(order);
				std::string s = TenToSixteen(Torder,4);
				TwoFuncCommand[numberFunction][TbeforeIf] = TwoFuncCommand[numberFunction][TbeforeIf] + s;
			}
			return{};
		}
		//如果有else语句
		//先执行跳转指令，因为如果执行了IF，在这里不能再执行else了,这里记录跳转指令所在的order。
		command[numberFunction][order] = std::to_string(order) + "\t" + "jmp "; endIf = order; order++;
			//二进制
			TwoFuncCommand[numberFunction][Torder] = "70"; TendIf = Torder; Torder++;

		//此时的order位于else语句块的开始，beforeIf的跳转，应该是跳转到这里。这里是在else语句块跳转之后。
		//所以将之前的跳转语句补充完整。
		if(beforeIf > 0){
			command[numberFunction][beforeIf] = command[numberFunction][beforeIf] + std::to_string(order);
			std::string s = TenToSixteen(order,4);
			TwoFuncCommand[numberFunction][TbeforeIf] = TwoFuncCommand[numberFunction][TbeforeIf] + s;
		}	

		//进入else所对应的Statement
		auto err3 = analyseStatement();
		if(err3.has_value())
			return err3;
		//此时order位于else语句块的末端，endIf的跳转应该是跳转到这里。
		command[numberFunction][endIf] = command[numberFunction][endIf] + std::to_string(order);
		std::string s = TenToSixteen(order,4);
		TwoFuncCommand[numberFunction][TendIf] = TwoFuncCommand[numberFunction][TendIf] + s;
		return {};
	}

	//条件
	std::optional<CompilationError> Analyser::analyseCondition(int&a,int&b) {
		std::string judge = "";
		auto err1 = analyseAdditiveExpression();
		if(err1.has_value())
			return err1;
		auto next = nextToken();
		if(next.value().GetType() == JUDGE_EQUAL_SIGN){
			judge = "==";
		}
		else if(next.value().GetType() == LESS_SIGN){
			judge = "<";
		}
		else if(next.value().GetType() == MORE_SIGN){
			judge = ">";
		}
		else if(next.value().GetType() == LESS_EQUAL_SIGN){
			judge = "<=";
		}
		else if(next.value().GetType() == MORE_EQUAL_SIGN){
			judge = ">=";
		}
		else if(next.value().GetType() == NO_EQUAL_SIGN){
			judge = "!=";
		}
		else if(next.value().GetType() == RIGHT_BRACKET){
			//如果是0，跳转至else前.
			command[numberFunction][order] = std::to_string(order) + "\t" + "je "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "71"; b = Torder; Torder++;
			return {};
		}
		else //缺少右括号
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		//后一个表达式
		auto err2 = analyseAdditiveExpression();
		if(err2.has_value())
			return err1;
		//跳转，并记录哪个指令跳转的。
		command[numberFunction][order] = std::to_string(order) + "\t" + "icmp"; order++;
		TwoFuncCommand[numberFunction][Torder] = "44"; Torder++;
		if(judge == "=="){
			//如果不是0（如果不相等），跳转至else之前。
			command[numberFunction][order] = std::to_string(order) + "\t" + "jne "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "72"; b = Torder; Torder++;
		}
		else if(judge == ">"){
			//如果不是正数（如果<=），跳转至else之前。
			command[numberFunction][order] = std::to_string(order) + "\t" + "jle "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "76"; b = Torder; Torder++;
		}
		else if(judge == "<"){
			//如果不是负数（如果>=），跳转至else之前。
			command[numberFunction][order] = std::to_string(order) + "\t" + "jge "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "74"; b = Torder; Torder++;
		}
		else if(judge == ">="){
			//如果是负数（如果<）,跳转至else前.
			command[numberFunction][order] = std::to_string(order) + "\t" + "jl "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "73"; b = Torder; Torder++;
		}
		else if(judge == "<="){
			//如果是正数（如果>）,跳转至else前.
			command[numberFunction][order] = std::to_string(order) + "\t" + "jg "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "75"; b = Torder; Torder++;
		}
		else if(judge == "!="){
			//如果是0，跳转至else前.
			command[numberFunction][order] = std::to_string(order) + "\t" + "je "; a = order; order++;
			TwoFuncCommand[numberFunction][Torder] = "71"; b = Torder; Torder++;
		}
		//检查右小括号
		next = nextToken();
		if(next.value().GetType() != TokenType::RIGHT_BRACKET)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		return {};
	}

	//循环语句
	std::optional<CompilationError> Analyser::analyseLoopStatement() {
		//将位置初始化								//二进制
		int	whileBegin = -1,whileEnd = -1;			int	TwhileBegin = -1,TwhileEnd = -1;
		auto next = nextToken();
		if(next.value().GetType() != TokenType::LEFT_BRACKET)//缺少左括号
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		
		//先记录一下一会跳转回来回到的位置,whileend时进行的跳转
		whileEnd = order;		TwhileEnd = order;

		//跳转到条件子程序
		auto err1 = analyseCondition(whileBegin,TwhileBegin);
		if(err1.has_value())
			return err1;
		//进入while所对应的Statement
		auto err2 = analyseStatement();
		if(err2.has_value())
			return err2;
		//跳回判断语句，进行下一回的判断
		command[numberFunction][order] = std::to_string(order) + "\t" + "jmp " + std::to_string(whileEnd); order++;
		std::string s = TenToSixteen(TwhileEnd,4);
		TwoFuncCommand[numberFunction][Torder] = "70"+s; Torder++;
		//如果条件满足，则跳转到这里，while语句结束.
		command[numberFunction][whileBegin] = command[numberFunction][whileBegin] + std::to_string(order);
		std::string s0 = TenToSixteen(order,4);
		TwoFuncCommand[numberFunction][TwhileBegin] = TwoFuncCommand[numberFunction][TwhileBegin] + s0 ;
		return {};
	}


	//可打印列表
	std::optional<CompilationError> Analyser::analysePrintableList() {
		auto next = nextToken();
		if(next.value().GetType() == TokenType::RIGHT_BRACKET)
			return {};
		else{
			unreadToken();
			auto err = analyseAdditiveExpression();
			if(err.has_value())
				return err;
		}
		while (true)
		{
			next = nextToken();
			if(next.value().GetType() == TokenType::COMMA)
			{
				command[numberFunction][order] = std::to_string(order) + "\t" + "iprint"; order++;
				command[numberFunction][order] = std::to_string(order) + "\t" + "bipush " + "32"; order++;
				command[numberFunction][order] = std::to_string(order) + "\t" + "cprint"; order++;

				TwoFuncCommand[numberFunction][Torder] = "a0"; Torder++;
				TwoFuncCommand[numberFunction][Torder] = "0120"; Torder++;
				TwoFuncCommand[numberFunction][Torder] = "a2"; Torder++;
				auto err1 = analyseAdditiveExpression();
				if(err1.has_value())
					return err1;
			}
			else if(next.value().GetType() == TokenType::RIGHT_BRACKET){
				command[numberFunction][order] = std::to_string(order) + "\t" + "iprint"; order++;
				command[numberFunction][order] = std::to_string(order) + "\t" + "printl"; order++;

				TwoFuncCommand[numberFunction][Torder] = "a0"; Torder++;
				TwoFuncCommand[numberFunction][Torder] = "af"; Torder++;
				return {};
			}
			else //缺少右括号。
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		
	}




	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}


	void Analyser::addVariable(const std::string& s) {
		for(int i=0;i<1000;i++){
			if(SL[i].name == ""){
				SL[i].name = s;
				SL[i].level = NowLevel;
				SL[i].index = nextSymbolLegendIndex;
				SL[i].change = true;
				nextSymbolLegendIndex++;
				break;
			}
		}
	}

	void Analyser::addConstant(const std::string& s) {
		for(int i=0;i<1000;i++){
			if(SL[i].name == ""){
				SL[i].name = s;
				SL[i].level = NowLevel;
				SL[i].index = nextSymbolLegendIndex;
				SL[i].change = false;
				nextSymbolLegendIndex++;
				break;
			}
		}
	}

	bool Analyser::isDeclared(const std::string& s) {
		return isDeclared_inLevel0(s) || isDeclared_inLevel1(s);
	}

	bool Analyser::isDeclared_inLevel0(const std::string& s) {
		for(int i=999;i>=0;i--){
			if(s == SL[i].name && SL[i].level == 0)
				return true;
		}
		return false;
	}

	bool Analyser::isDeclared_inLevel1(const std::string& s) {
		for(int i=999;i>=0;i--){
			if(s == SL[i].name && SL[i].level == 1)
				return true;
		}
		return false;
	}
	
	bool Analyser::isVariable(const std::string& s) {
		for(int i=999;i>=0;i--){
			if(s == SL[i].name && SL[i].change == true)
				return true;
		}
		return false;
	}

	bool Analyser::isConstant(const std::string&s) {
		for(int i=999;i>=0;i--){
			if(s == SL[i].name && SL[i].change == true)
				break;
			if(s == SL[i].name && SL[i].change == false){
				return true;
			}
		}
		return false;
	}

	int Analyser::getLevelDifference(const std::string&s) {
		int a,b;
		a = NowLevel;
		for(int i=999;i>=0;i--) {
			if(s == SL[i].name){
				b = SL[i].level;
				break;
			}
		}
		return a-b;
	}

	int Analyser::getOffset(const std::string&s) {
		int a;
		for(int i=999;i>=0;i--) {
			if(s == SL[i].name){
				a = SL[i].index;
				break;
			}
		}
		return a;
	}

	bool Analyser::isFunctionName(const std::string&s){
		for(int i=999;i>=0;i--) {
			if(s == FunctionTable[i].name) {
				return true;
			}
		}
		return false;
	}

	std::string Analyser::getFunctionType(const std::string&s) {
		for(int i=999;i>=0;i--) {
			if(s == FunctionTable[i].name) {
				return FunctionTable[i].type;
			}
		}
		return {};
	}

	int Analyser::getParametersNumber(const std::string&s) {
		for(int i=999;i>=0;i--) {
			if(s == FunctionTable[i].name) {
				return FunctionTable[i].parameters_number;
			}
		}
		return {};
	}

	int Analyser::getFunctionIndex(const std::string&s) {
		for(int i=999;i>=0;i--) {
			if(s == FunctionTable[i].name) {
				return FunctionTable[i].index;
			}
		}
		return {};
	}
	
	//将函数放进常量表
	void Analyser::addFunctionToConstTable(const std::string&s) {
		for(int i=0;i<1000;i++) {
			if(ConstTable[i].name == ""){
				ConstTable[i].name = s;
				ConstTable[i].index = Constindex;
				ConstTable[i].type = "S";
				Constindex++;
				break;
			}
		}
	}

	//将函数放进函数表里
	void Analyser::addFunctionToFunctionTable(const std::string&s) {
		for(int i = 0;i<1000;i++) {
			if(FunctionTable[i].name == ""){
				FunctionTable[i].name = s;
				FunctionTable[i].index = Functionindex;
				FunctionTable[i].type = FunctionType;
				//FunctionTable[i].size = FunctionSize;
				FunctionTable[i].level = NowLevel;
				//FunctionTable[i].parameters_number = numberOfParameters;
				Functionindex++;
				break;
			}
		}
	}

	//清空符号表中的二级标识符
	void Analyser::EmptySymbolLegend() {
		for(int i = 0;i<1000;i++){
			if(SL[i].level == 1){
				SL[i].change = true;
				SL[i].index = 0;
				SL[i].name = "";
				SL[i].level = 0;
			}
		}
	}

	//函数名是否被声明过(跟全局变量和以前的函数名相同都不行)
	bool Analyser::FunctionIsDeclared(const std::string&s) {
		for(int i = 0;i<1000;i++) {
			if(FunctionTable[i].name == s){
				return true;
			}
		}
		for(int i = 0;i<1000;i++) {
			if(SL[i].name == s && SL[i].level == 0){
				return true;
			}
		}
		return false;
	}



	//二进制函数
	//将数字转换为十六进制的字符串，参数并给出位数(十进制数字,要求转换的位数)
	std::string Analyser::TenToSixteen(int n,int w) {
		std::string s = "";std::string back = "";
		int m;int c = 0;
		while(n > 0){
			m = n % 16;
			n = n / 16;
			switch (m)
			{
			case 15:
				s = "f" + s;
				break;
			case 14:
				s = "e" + s;
				break;
			case 13:
				s = "d" + s;
				break;
			case 12:
				s = "c" + s;
				break;
			case 11:
				s = "b" + s;
				break;
			case 10:
				s = "a" + s;
				break;
			default:
				s =std::to_string(m) + s;
			}
			c++;
			//s =std::to_string(m) + s;
		}
		int cha = w -c;
		if(cha == 0){
			back = s;
			return back;
		} 
		if(cha != 0){
			while(cha != 0){
				s= "0" + s;
				cha--;
			}
			back = s;
			return back;
		}
		return back;
	}

	//十六进制格式化
	std::string Analyser::SixteenToSixteen(std::string s,int w) {
		int length = s.length();
		int a = length - 2;
		std::string back = "";//std::string r = "";
		for(int i=2;i<length;i++){
			back = back + s[i];
		}
		int cha = w-a;
		for(int i=1;i<=cha;i++){
			back = "0"+back;
		}
		
		return back;
	}

}