#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mood.h"
#include "vibeparser.tab.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include "vibeparser_llvm.h"
// LLVM global variables
static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMExecutionEngineRef engine;
static LLVMPassManagerRef pass_manager;

// Symbol table structure for code generation
struct symbol {
    char *name;
    LLVMValueRef value;
    int is_string;
};

#define MAX_SYMBOLS 100
static struct symbol symbol_table[MAX_SYMBOLS];
static int sym_count = 0;

// Current function and basic blocks for control flow
static LLVMValueRef current_function = NULL;
static LLVMBasicBlockRef current_block = NULL;


void init_llvm_ir_generation() {
    // Initialize LLVM
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    
    // Create module
    LLVMModuleRef module = LLVMModuleCreateWithName("vibe_module");
    
    // Create builder
    LLVMBuilderRef builder = LLVMCreateBuilder();
    
    // Create main function
    LLVMTypeRef param_types[] = {LLVMInt32Type()};
    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", func_type);
    
    // Create basic block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    
    // Create return instruction
    LLVMValueRef ret_val = LLVMConstInt(LLVMInt32Type(), 0, 0);
    LLVMBuildRet(builder, ret_val);
    
    // Dump module to stdout (for testing)
    LLVMDumpModule(module);
    
    // Cleanup
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}


// Initialize LLVM components
void init_llvm() {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    module = LLVMModuleCreateWithName("vibeparser_module");
    builder = LLVMCreateBuilder();
    
    // Create a function for our main code
    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    current_function = LLVMAddFunction(module, "main", main_type);
    current_block = LLVMAppendBasicBlock(current_function, "entry");
    LLVMPositionBuilderAtEnd(builder, current_block);
    
    // Create printf function declaration
    LLVMTypeRef printf_args[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), printf_args, 1, 1);
    LLVMAddFunction(module, "printf", printf_type);
}

// Clean up LLVM resources
void cleanup_llvm() {
    if (pass_manager) LLVMDisposePassManager(pass_manager);
    if (engine) LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
}

// Add or update a symbol in the table
int add_symbol_llvm(char *name, LLVMValueRef value, int is_string) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            symbol_table[i].is_string = is_string;
            return i;
        }
    }
    
    if (sym_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Symbol table full\n");
        return -1;
    }
    
    symbol_table[sym_count].name = strdup(name);
    symbol_table[sym_count].value = value;
    symbol_table[sym_count].is_string = is_string;
    return sym_count++;
}

// Get a symbol's LLVM value
LLVMValueRef get_symbol_value_llvm(char *name) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].value;
        }
    }
    fprintf(stderr, "Undefined variable: %s\n", name);
    return NULL;
}

// Generate LLVM IR for variable declarations
LLVMValueRef gen_variable_decl(char *name, LLVMValueRef init_value, int is_string) {
    // Create alloca instruction for the variable
    LLVMTypeRef var_type = is_string ? LLVMPointerType(LLVMInt8Type(), 0) : LLVMInt32Type();
    LLVMValueRef alloca = LLVMBuildAlloca(builder, var_type, name);
    
    // Store the initial value
    if (init_value) {
        LLVMBuildStore(builder, init_value, alloca);
    }
    
    // Add to symbol table
    add_symbol_llvm(name, alloca, is_string);
    return alloca;
}

// Generate LLVM IR for integer expressions
LLVMValueRef gen_expression(int value) {
    return LLVMConstInt(LLVMInt32Type(), value, 0);
}

// Generate LLVM IR for binary operations
LLVMValueRef gen_binary_op(LLVMValueRef lhs, int op, LLVMValueRef rhs) {
    switch (op) {
        case PLUS: return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
        case MINUS: return LLVMBuildSub(builder, lhs, rhs, "subtmp");
        case TIMES: return LLVMBuildMul(builder, lhs, rhs, "multmp");
        case DIVIDE: return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
        case MOD: return LLVMBuildSRem(builder, lhs, rhs, "modtmp");
        case EQ: return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        case GT: return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "gttmp");
        case LT: return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lttmp");
        default:
            fprintf(stderr, "Unknown binary operator\n");
            return NULL;
    }
}

// Generate LLVM IR for print statements
void gen_print(LLVMValueRef value, int is_string) {
    // Create format string
    const char *fmt_str = is_string ? "%s\n" : "%d\n";
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, fmt_str, "fmt");
    
    // Prepare arguments
    LLVMValueRef args[2];
    args[0] = fmt;
    args[1] = is_string ? value : LLVMBuildIntCast(builder, value, LLVMInt32Type(), "printcast");
    
    // Call printf
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), 
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, 1);
    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }
    LLVMBuildCall2(builder, printf_type, printf_fn, args, is_string ? 2 : 2, "");
}

// Generate LLVM IR for if statements
void gen_if(LLVMValueRef cond, LLVMBasicBlockRef true_block, 
            LLVMBasicBlockRef false_block, LLVMBasicBlockRef merge_block) {
    // Create conditional branch
    LLVMBuildCondBr(builder, cond, true_block, false_block);
    
    // Emit true block
    LLVMPositionBuilderAtEnd(builder, true_block);
    
    // After true block, jump to merge block
    LLVMBuildBr(builder, merge_block);
    true_block = LLVMGetInsertBlock(builder);
    
    // Emit false block
    LLVMPositionBuilderAtEnd(builder, false_block);
    
    // After false block, jump to merge block
    LLVMBuildBr(builder, merge_block);
    false_block = LLVMGetInsertBlock(builder);
    
    // Position builder at merge block for subsequent code
    LLVMPositionBuilderAtEnd(builder, merge_block);
}

// Finalize the LLVM module and write to file
void finalize_llvm(const char *filename) {
    // Add return 0 to main function
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
    
    // Verify the module
    char *error = NULL;
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
        fprintf(stderr, "Error verifying module: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    }
    
    // Write to file
    if (filename) {
        if (LLVMPrintModuleToFile(module, filename, &error)) {
            fprintf(stderr, "Error writing IR to file: %s\n", error);
            LLVMDisposeMessage(error);
        } else {
            printf("LLVM IR written to %s\n", filename);
        }
    }
    
    // Dump to stdout for debugging
    LLVMDumpModule(module);
}

// Main code generation function
void generate_llvm_code() {
    init_llvm();
    
    // Example: Generate code for a simple program
    // int x = 10;
    LLVMValueRef x_val = gen_expression(10);
    gen_variable_decl("x", x_val, 0);
    
    // int y = x + 5;
    LLVMValueRef x_load = LLVMBuildLoad2(builder, LLVMInt32Type(), get_symbol_value_llvm("x"), "xload");
    LLVMValueRef five = gen_expression(5);
    LLVMValueRef y_val = gen_binary_op(x_load, PLUS, five);
    gen_variable_decl("y", y_val, 0);
    
    // print y;
    LLVMValueRef y_load = LLVMBuildLoad2(builder, LLVMInt32Type(), get_symbol_value_llvm("y"), "yload");
    gen_print(y_load, 0);
    
    // Finalize and write to file
    finalize_llvm("output.ll");
    
    cleanup_llvm();
}

