#ifndef VIBEPARSER_LLVM_H
#define VIBEPARSER_LLVM_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>

// Define ExprResult structure
typedef struct {
    int value;
    LLVMValueRef llvm_value;
} ExprResult;
typedef struct {
    char *var_name;
    LLVMValueRef iterator;
    LLVMValueRef end_value;
    LLVMBasicBlockRef loop_block;
    LLVMBasicBlockRef body_block;
    LLVMBasicBlockRef incr_block;
    LLVMBasicBlockRef exit_block;
} ForContext;

#define MAX_FOR_DEPTH 10
extern ForContext for_stack[MAX_FOR_DEPTH];
extern int for_stack_top;
// LLVM global variables
extern LLVMModuleRef module;
extern LLVMBuilderRef builder;
extern LLVMExecutionEngineRef engine;
extern LLVMPassManagerRef pass_manager;
extern LLVMValueRef current_function;
extern LLVMBasicBlockRef current_block;

// Global merge block (defined in vibeparser_llvm.c)
extern LLVMBasicBlockRef merge_block;

// Function declarations
void init_llvm();
void cleanup_llvm();
LLVMValueRef gen_variable_decl(char *name, LLVMValueRef init_value, int is_string);
LLVMValueRef gen_expression(int value);
LLVMValueRef gen_binary_op(LLVMValueRef lhs, int op, LLVMValueRef rhs);
void gen_print(LLVMValueRef value, int is_string);
void gen_if(LLVMValueRef cond, LLVMBasicBlockRef *true_block, 
            LLVMBasicBlockRef *false_block, LLVMBasicBlockRef *merge_block);
void finalize_llvm(const char *filename);
LLVMValueRef get_symbol_value_llvm(char *name);

// Safe getter for the merge block
LLVMBasicBlockRef get_merge_block();

// New functions for proper if-else handling
void start_if_statement(LLVMValueRef condition);
void handle_else();
void end_if_statement();
void start_for_loop(char *var_name, LLVMValueRef init_val, LLVMValueRef end_val);
void end_for_loop(char *var_name);
LLVMValueRef get_for_iterator_value(char *var_name);
#endif // VIBEPARSER_LLVM_H