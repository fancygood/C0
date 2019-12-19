#include "argparse.hpp"
#include "fmt/core.h"

#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "fmts.hpp"

#include <iostream>
#include <fstream>


std::vector<cc0::Token> _tokenize(std::istream& input) {
	cc0::Tokenizer tkz(input);
	auto p = tkz.AllTokens();
	if (p.second.has_value()) {
		fmt::print(stderr, "Tokenization error: {}\n", p.second.value());
		exit(2);
	}
	return p.first;
}

int Number(char c) {
	int x;
	if(c<='9' && c>='0')
		x = c-'0';
	else if(c <='f' && c >='a')
		x = c - 'a' +10;
	return x;
}

void Tokenize(std::istream& input, std::ostream& output) {
	/*auto v = _tokenize(input);
	for (auto& it : v)
		output << fmt::format("{}\n", it);
	return;*/
	auto tks = _tokenize(input);
	cc0::Analyser analyser(tks);
	auto p = analyser.Analyse();
	if (p.second.has_value()) {
		fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
		exit(2);
	}
	/*
	output << "43 30 3a 29 ";
	output << "00 00 00 01 ";
	output << TwoCommand[0];
	output << TwoCommand[1];
	output << TwoCommand[2];
	for(int i = 3;i<20;i++) {
		for(int j = 0;j<1000;j++){
			if(TwoFuncCommand[i][j] != "")
				output << TwoFuncCommand[i][j];
		}
	}
	*/
	
	std::string out = "";
	out = "43303a29";
	out = out + "00000001";
	out = out + TwoCommand[0];
	out = out + TwoCommand[1];
	out = out + TwoCommand[2];
	for(int i = 3;i<20;i++) {
		for(int j = 0;j<1000;j++){
			if(TwoFuncCommand[i][j] != "")
				out = out + TwoFuncCommand[i][j];
		}
	}
//	output << out;
	int length = out.length();int i = 0;
	while(i<length) {
		if(out[i] == ' ')
			output << "沙雕";
		std::stringstream ss ;
		int a = 0;int b = 0;int value = 0;
		//ss << out[i];
		//ss >> a;
		//if(out[i] != ' '){
			a = Number(out[i]);
			i++;
		if(out[i] == ' ')
			output << "沙雕";
		//}
		//ss << out[i+1];
		//ss >> b;
		//if(out[i] != ' '){
			b = Number(out[i]);
			i++;
		//}
		value = a*16+b;
		char c = value;
		output << c;
		//output << "沙雕";
	
	}

	return;
	
}
void Analyse(std::istream& input, std::ostream& output){
	auto tks = _tokenize(input);
	cc0::Analyser analyser(tks);
	auto p = analyser.Analyse();
	if (p.second.has_value()) {
		fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
		exit(2);
	

	}
	/*
	auto v = p.first;
	for (auto& it : v)
		output << fmt::format("{}\n", it);*/
	for(int i = 0;i<20;i++) {
		if(i == 0)
			output << ".constants:\n";
		if(i == 1)
			output << ".start:\n";
		if(i == 2)
			output << ".functions:\n";
		if(i >= 3 && command[i][0] != ""){
			std::string s = ".F" + std::to_string(i-3) + ":";
			output << s + "\n";
		}			
		for(int j = 0;j<1000;j++){
			if(command[i][j] != ""){
				output << command[i][j] + "\n";
			}
		}
	}
	return;
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("cc0");
	program.add_argument("-c")
		.default_value(false)
		.implicit_value(true)
		.help("将输入的 c0 源代码翻译为二进制目标文件");
	program.add_argument("-s")
		.default_value(false)
		.implicit_value(true)
		.help("将输入的 c0 源代码翻译为文本汇编文件");
	program.add_argument("input")
		.help("speicify the file to be compiled.");
	program.add_argument("-o", "--output")
		.required()
		.default_value(std::string("-"))
		.help("输出到指定的文件 file");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		fmt::print(stderr, "{}\n\n", err.what());
		program.print_help();
		exit(2);
	}

	auto input_file = program.get<std::string>("input");
	auto output_file = program.get<std::string>("--output");
	std::istream* input;
	std::ostream* output;
	std::ifstream inf;
	std::ofstream outf;
	if (input_file != "-") {
		inf.open(input_file, std::ios::in);
		if (!inf) {
			fmt::print(stderr, "Fail to open {} for reading.\n", input_file);
			exit(2);
		}
		input = &inf;
	}
	else
		input = &std::cin;
	if (output_file != "-") {
		if(program["-c"] == true){
			outf.open(output_file, std::ios::out | std::ios::binary);
			if (!outf) {
				fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
				exit(2);
			}
			output = &outf;
		}
		if(program["-s"] == true){
			outf.open(output_file, std::ios::out | std::ios::trunc);
			if (!outf) {
				fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
				exit(2);
			}
			output = &outf;
		}
	}
	else{
		if(program["-c"] == true){
			outf.open("out", std::ios::out | std::ios::binary);
			if (!outf) {
				fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
				exit(2);
			}
			output = &outf;
		}
		if(program["-s"] == true){
			outf.open("out", std::ios::out | std::ios::trunc);
			if (!outf) {
				fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
				exit(2);
			}
			output = &outf;
		}
	}
		//output = &std::cout;
	if (program["-c"] == true && program["-s"] == true) {
		fmt::print(stderr, "You can only perform tokenization or syntactic analysis at one time.");
		exit(2);
	}
	if (program["-c"] == true) {
		Tokenize(*input, *output);
	}
	else if (program["-s"] == true) {
		Analyse(*input, *output);
	}
	else {
		fmt::print(stderr, "You must choose tokenization or syntactic analysis.");
		exit(2);
	}
	return 0;
}