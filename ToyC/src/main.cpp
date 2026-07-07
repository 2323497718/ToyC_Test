#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <stdexcept>

#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "IRGenerator.h"
#include "Optimizer.h"
#include "CodeGenerator.h"

using namespace ToyC;

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [-opt] [input_file [output_file]]\n";
    std::cerr << "  -opt      Enable optimizations\n";
    std::cerr << "  input_file   Input file (default: stdin)\n";
    std::cerr << "  output_file  Output file (default: output.s)\n";
}

int main(int argc, char* argv[]) {
    bool enableOpt = false;
    std::string inputFile;
    std::string outputFile;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-opt") {
            enableOpt = true;
        } else if (inputFile.empty()) {
            inputFile = arg;
        } else if (outputFile.empty()) {
            outputFile = arg;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::string source;
    if (!inputFile.empty()) {
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
        file.close();
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();
    }
    
    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        
        Parser parser(tokens);
        std::unique_ptr<CompUnit> ast = parser.parse();
        
        if (parser.hasError()) {
            std::cerr << "Parse error: " << parser.getErrorMessage() << std::endl;
            return 1;
        }
        
        SemanticAnalyzer semanticAnalyzer;
        if (!semanticAnalyzer.analyze(ast.get())) {
            std::cerr << "Semantic error: " << semanticAnalyzer.getErrorMessage() << std::endl;
            return 1;
        }
        
        IRGenerator irGenerator;
        std::unique_ptr<IRProgram> ir = irGenerator.generate(ast.get());
        
        if (irGenerator.hasError()) {
            std::cerr << "IR generation error: " << irGenerator.getErrorMessage() << std::endl;
            return 1;
        }
        
        // Print IR for debugging
        ir->print(std::cerr);
        
        Optimizer optimizer(enableOpt);
        if (enableOpt) {
            optimizer.optimize(ir.get());
        }
        
        CodeGenerator codeGen(enableOpt);
        
        std::ofstream outFile;
        if (!outputFile.empty()) {
            outFile.open(outputFile);
        } else {
            outFile.open("output.s");
        }
        
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot open output file" << std::endl;
            return 1;
        }
        
        codeGen.generate(ir.get(), outFile);
        outFile.close();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
