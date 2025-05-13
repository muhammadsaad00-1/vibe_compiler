// Fixed vibeparser_llvm.c file with corrected for loop handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mood.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include "vibeparser_llvm.h"
#include "vibeparser.tab.h"

// LLVM global variables - made non-static so they can be accessed from the parser
LLVMModuleRef module;
LLVMBuilderRef builder;
LLVMExecutionEngineRef engine;
LLVMPassManagerRef pass_manager;
LLVMValueRef current_function;
LLVMBasicBlockRef current_block;

// Global merge block variable
LLVMBasicBlockRef merge_block = NULL;

// Stack to handle nested if statements
typedef struct {
    LLVMBasicBlockRef true_block;
    LLVMBasicBlockRef false_block;
    LLVMBasicBlockRef merge_block;
} IfContext;

#define MAX_IF_DEPTH 10
static IfContext if_stack[MAX_IF_DEPTH];
static int if_stack_top = -1;
ForContext for_stack[MAX_FOR_DEPTH];
int for_stack_top = -1;

// Push if context
void push_if_context(LLVMBasicBlockRef true_block, LLVMBasicBlockRef false_block, LLVMBasicBlockRef merge_block) {
    if (if_stack_top < MAX_IF_DEPTH - 1) {
        if_stack_top++;
        if_stack[if_stack_top].true_block = true_block;
        if_stack[if_stack_top].false_block = false_block;
        if_stack[if_stack_top].merge_block = merge_block;
    }
}

// Pop if context
IfContext pop_if_context() {
    IfContext context = {NULL, NULL, NULL};
    if (if_stack_top >= 0) {
        context = if_stack[if_stack_top];
        if_stack_top--;
    }
    return context;
}

// Get current if context
IfContext* get_current_if_context() {
    if (if_stack_top >= 0) {
        return &if_stack[if_stack_top];
    }
    return NULL;
}

// Internal symbol table for LLVM values
struct llvm_symbol {
    char *name;
    LLVMValueRef value;
    int is_string;
};

#define MAX_LLVM_SYMBOLS 100
static struct llvm_symbol llvm_symbol_table[MAX_LLVM_SYMBOLS];
static int llvm_sym_count = 0;

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
    if (builder) LLVMDisposeBuilder(builder);
    if (module) LLVMDisposeModule(module);
    
    // Clean up LLVM symbol table
    for (int i = 0; i < llvm_sym_count; i++) {
        if (llvm_symbol_table[i].name) {
            free(llvm_symbol_table[i].name);
        }
    }
    llvm_sym_count = 0;
    
    // Clean up for loop stack
    for (int i = 0; i <= for_stack_top; i++) {
        if (for_stack[i].var_name) {
            free(for_stack[i].var_name);
        }
    }
    for_stack_top = -1;
}

// Add a symbol to the LLVM symbol table
static void add_llvm_symbol(char *name, LLVMValueRef value, int is_string) {
    // Check if symbol already exists
    for (int i = 0; i < llvm_sym_count; i++) {
        if (strcmp(llvm_symbol_table[i].name, name) == 0) {
            llvm_symbol_table[i].value = value;
            llvm_symbol_table[i].is_string = is_string;
            return;
        }
    }
    
    // Add new symbol
    if (llvm_sym_count < MAX_LLVM_SYMBOLS) {
        llvm_symbol_table[llvm_sym_count].name = strdup(name);
        llvm_symbol_table[llvm_sym_count].value = value;
        llvm_symbol_table[llvm_sym_count].is_string = is_string;
        llvm_sym_count++;
    }
}

// Get symbol value from LLVM symbol table
LLVMValueRef get_symbol_value_llvm(char *name) {
    for (int i = 0; i < llvm_sym_count; i++) {
        if (strcmp(llvm_symbol_table[i].name, name) == 0) {
            return llvm_symbol_table[i].value;
        }
    }
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
    add_llvm_symbol(name, alloca, is_string);
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
            fprintf(stderr, "Unknown binary operator: %d\n", op);
            return NULL;
    }
}

// Generate LLVM IR for print statements
void gen_print(LLVMValueRef value, int is_string) {
    // Get printf function
    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), 
            (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, 1);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }
    
    // Create format string
    const char *fmt_str = is_string ? "%s\n" : "%d\n";
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, fmt_str, "fmt");
    
    // Prepare arguments
    LLVMValueRef args[2];
    args[0] = fmt;
    args[1] = value;
    
    // Call printf
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), 
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, 1);
    LLVMBuildCall2(builder, printf_type, printf_fn, args, 2, "");
}

// Generate LLVM IR for if statements
void gen_if(LLVMValueRef cond, LLVMBasicBlockRef *true_block, 
    LLVMBasicBlockRef *false_block, LLVMBasicBlockRef *merge_block) {
    // Create basic blocks
    *true_block = LLVMAppendBasicBlock(current_function, "if_true");
    *false_block = LLVMAppendBasicBlock(current_function, "if_false");
    *merge_block = LLVMAppendBasicBlock(current_function, "if_merge");
    
    // Push context to stack
    push_if_context(*true_block, *false_block, *merge_block);
    
    // Create conditional branch
    LLVMBuildCondBr(builder, cond, *true_block, *false_block);
    
    // Move to true block
    LLVMPositionBuilderAtEnd(builder, *true_block);
}

// Finalize the LLVM module and write to file
void finalize_llvm(const char *filename) {
    // Add return 0 to main function if not already added
    if (!LLVMGetLastInstruction(current_block) || 
        LLVMGetInstructionOpcode(LLVMGetLastInstruction(current_block)) != LLVMRet) {
        LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
    }
    
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
}

// New functions for handling if-else constructs

// Start if statement - called when we see IF
void start_if_statement(LLVMValueRef condition) {
    LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(current_function, "if_true");
    LLVMBasicBlockRef false_block = LLVMAppendBasicBlock(current_function, "if_false");
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(current_function, "if_merge");
    
    // Push context to stack
    push_if_context(true_block, false_block, merge_block);
    
    // Create conditional branch
    LLVMBuildCondBr(builder, condition, true_block, false_block);
    
    // Move to true block
    LLVMPositionBuilderAtEnd(builder, true_block);
    current_block = true_block;
}

// Handle ELSE - called when we see ELSE
void handle_else() {
    IfContext* context = get_current_if_context();
    if (context) {
        // Branch from true block to merge block
        LLVMBuildBr(builder, context->merge_block);
        
        // Move to false block
        LLVMPositionBuilderAtEnd(builder, context->false_block);
        current_block = context->false_block;
    }
}

// End if statement - called when we see ENDIF
void end_if_statement() {
    IfContext context = pop_if_context();
    if (context.merge_block) {
        // Branch to merge block from current block
        LLVMBuildBr(builder, context.merge_block);
        
        // If there's no else, we need to add a branch from false block to merge too
        LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
        if (current == context.true_block) {
            // No else clause, add branch from false block
            LLVMPositionBuilderAtEnd(builder, context.false_block);
            LLVMBuildBr(builder, context.merge_block);
        }
        
        // Move to merge block
        LLVMPositionBuilderAtEnd(builder, context.merge_block);
        current_block = context.merge_block;
    }
}

// Getter function for merge block
LLVMBasicBlockRef get_merge_block() {
    IfContext* context = get_current_if_context();
    if (context) {
        return context->merge_block;
    }
    return NULL;
}

// Push for context
void push_for_context(char *var_name, LLVMValueRef iterator, LLVMValueRef end_value,
    LLVMBasicBlockRef loop_block, LLVMBasicBlockRef incr_block, 
    LLVMBasicBlockRef exit_block) {
    if (for_stack_top < MAX_FOR_DEPTH - 1) {
        for_stack_top++;
        for_stack[for_stack_top].var_name = strdup(var_name);
        for_stack[for_stack_top].iterator = iterator;
        for_stack[for_stack_top].end_value = end_value;
        for_stack[for_stack_top].loop_block = loop_block;
        for_stack[for_stack_top].incr_block = incr_block;
        for_stack[for_stack_top].exit_block = exit_block;
    }
}

// Pop for context
ForContext pop_for_context() {
    ForContext context = {NULL, NULL, NULL, NULL, NULL, NULL};
    if (for_stack_top >= 0) {
        context = for_stack[for_stack_top];
        for_stack_top--;
    }
    return context;
}

// Get current for context
ForContext* get_current_for_context() {
    if (for_stack_top >= 0) {
        return &for_stack[for_stack_top];
    }
    return NULL;
}
// Complete corrected for loop implementation for vibeparser_llvm.c
// Fixed version of start_for_loop and end_for_loop functions

void start_for_loop(char *var_name, LLVMValueRef init_val, LLVMValueRef end_val) {
    // Create all required blocks for the loop
    LLVMBasicBlockRef loop_header = LLVMAppendBasicBlock(current_function, "for_header");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(current_function, "for_body");
    LLVMBasicBlockRef incr_block = LLVMAppendBasicBlock(current_function, "for_incr");
    LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(current_function, "for_exit");

    // Create the loop variable and initialize it
    LLVMValueRef iterator = gen_variable_decl(var_name, init_val, 0);
    
    // Jump to the loop header
    LLVMBuildBr(builder, loop_header);
    
    // Position builder at the loop header
    LLVMPositionBuilderAtEnd(builder, loop_header);
    
    // Load the current value of the iterator
    LLVMValueRef iter_val = LLVMBuildLoad2(builder, LLVMInt32Type(), iterator, "iter_val");
    
    // Compare with the end value and branch accordingly
    LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntSLT, iter_val, end_val, "for_cond");
    LLVMBuildCondBr(builder, cond, body_block, exit_block);
    
    // Move to the loop body
    LLVMPositionBuilderAtEnd(builder, body_block);
    
    // Store the loop context for later use when ending the loop
    push_for_context(var_name, iterator, end_val, loop_header, incr_block, exit_block);
}

void end_for_loop(char *var_name) {
    ForContext* context = get_current_for_context();
    if (!context) return;
    
    // Branch from the end of the loop body to the increment block
    LLVMBuildBr(builder, context->incr_block);
    
    // Position builder at the increment block
    LLVMPositionBuilderAtEnd(builder, context->incr_block);
    
    // Load the current iterator value
    LLVMValueRef iter_val = LLVMBuildLoad2(builder, LLVMInt32Type(), context->iterator, "iter_val");
    
    // Increment the iterator
    LLVMValueRef next_val = LLVMBuildAdd(builder, iter_val, LLVMConstInt(LLVMInt32Type(), 1, 0), "iter_inc");
    
    // Store the new value back
    LLVMBuildStore(builder, next_val, context->iterator);
    
    // Jump back to the loop header for the next iteration check
    LLVMBuildBr(builder, context->loop_block);
    
    // Position builder at the exit block for code after the loop
    LLVMPositionBuilderAtEnd(builder, context->exit_block);
    
    // Clean up the context
    pop_for_context();
}

// Get for iterator value
LLVMValueRef get_for_iterator_value(char *var_name) {
    ForContext* context = get_current_for_context();
    if (context && context->var_name && strcmp(context->var_name, var_name) == 0) {
        return LLVMBuildLoad2(builder, LLVMInt32Type(), context->iterator, "for_iter_load");
    }
    // Fallback to regular symbol lookup
    LLVMValueRef val = get_symbol_value_llvm(var_name);
    if (val) {
        return LLVMBuildLoad2(builder, LLVMInt32Type(), val, "var_load");
    }
    return NULL;
}