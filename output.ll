; ModuleID = 'vibeparser_module'
source_filename = "vibeparser_module"

@str_print = private unnamed_addr constant [20 x i8] c"Oh wow, the sum is:\00", align 1
@fmt = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.1 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str_print.2 = private unnamed_addr constant [32 x i8] c"The mind-blowing difference is:\00", align 1
@fmt.3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.4 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str_print.5 = private unnamed_addr constant [31 x i8] c"The groundbreaking product is:\00", align 1
@fmt.6 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.7 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str_print.8 = private unnamed_addr constant [34 x i8] c"The earth-shattering division is:\00", align 1
@fmt.9 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.10 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str_print.11 = private unnamed_addr constant [32 x i8] c"The life-changing remainder is:\00", align 1
@fmt.12 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.13 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str_lit = private unnamed_addr constant [9 x i8] c"Einstein\00", align 1
@str_print.14 = private unnamed_addr constant [17 x i8] c"Oh hello there, \00", align 1
@fmt.15 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@str_print.16 = private unnamed_addr constant [9 x i8] c"Einstein\00", align 1
@fmt.17 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@str_print.18 = private unnamed_addr constant [54 x i8] c"This definitely won't print - y is NOT greater than x\00", align 1
@fmt.19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@fmt.20 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 4, ptr %x, align 4
  %y = alloca i32, align 4
  store i32 5, ptr %y, align 4
  %result = alloca i32, align 4
  store i32 0, ptr %result, align 4
  %varload = load i32, ptr %x, align 4
  %varload1 = load i32, ptr %y, align 4
  %addtmp = add i32 %varload, %varload1
  %result2 = alloca i32, align 4
  store i32 9, ptr %result2, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @fmt, ptr @str_print)
  %1 = call i32 (ptr, ...) @printf(ptr @fmt.1, i32 9)
  %varload3 = load i32, ptr %x, align 4
  %varload4 = load i32, ptr %y, align 4
  %subtmp = sub i32 %varload3, %varload4
  %result5 = alloca i32, align 4
  store i32 -1, ptr %result5, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @fmt.3, ptr @str_print.2)
  %3 = call i32 (ptr, ...) @printf(ptr @fmt.4, i32 -1)
  %varload6 = load i32, ptr %x, align 4
  %varload7 = load i32, ptr %y, align 4
  %multmp = mul i32 %varload6, %varload7
  %result8 = alloca i32, align 4
  store i32 20, ptr %result8, align 4
  %4 = call i32 (ptr, ...) @printf(ptr @fmt.6, ptr @str_print.5)
  %5 = call i32 (ptr, ...) @printf(ptr @fmt.7, i32 20)
  %varload9 = load i32, ptr %x, align 4
  %varload10 = load i32, ptr %y, align 4
  %divtmp = sdiv i32 %varload9, %varload10
  %result11 = alloca i32, align 4
  store i32 0, ptr %result11, align 4
  %6 = call i32 (ptr, ...) @printf(ptr @fmt.9, ptr @str_print.8)
  %7 = call i32 (ptr, ...) @printf(ptr @fmt.10, i32 0)
  %varload12 = load i32, ptr %x, align 4
  %varload13 = load i32, ptr %y, align 4
  %modtmp = srem i32 %varload12, %varload13
  %result14 = alloca i32, align 4
  store i32 4, ptr %result14, align 4
  %8 = call i32 (ptr, ...) @printf(ptr @fmt.12, ptr @str_print.11)
  %9 = call i32 (ptr, ...) @printf(ptr @fmt.13, i32 4)
  %name = alloca i32, align 4
  store i32 0, ptr %name, align 4
  %name15 = alloca ptr, align 8
  store ptr @str_lit, ptr %name15, align 8
  %10 = call i32 (ptr, ...) @printf(ptr @fmt.15, ptr @str_print.14)
  %11 = call i32 (ptr, ...) @printf(ptr @fmt.17, ptr @str_print.16)
  %varload16 = load i32, ptr %x, align 4
  %varload17 = load i32, ptr %y, align 4
  %gttmp = icmp sgt i32 %varload16, %varload17
  %varload18 = load i32, ptr %y, align 4
  %varload19 = load i32, ptr %x, align 4
  %gttmp20 = icmp sgt i32 %varload18, %varload19
  br i1 %gttmp20, label %if_true, label %if_false

if_true:                                          ; preds = %entry
  %12 = call i32 (ptr, ...) @printf(ptr @fmt.19, ptr @str_print.18)
  br label %if_merge

if_false:                                         ; preds = %entry
  br label %if_merge

if_merge:                                         ; preds = %if_false, %if_true
  %i = alloca i32, align 4
  store i32 1, ptr %i, align 4
  %i21 = alloca i32, align 4
  store i32 1, ptr %i21, align 4
  %i22 = alloca i32, align 4
  store i32 2, ptr %i22, align 4
  %i23 = alloca i32, align 4
  store i32 3, ptr %i23, align 4
  %i24 = alloca i32, align 4
  store i32 4, ptr %i24, align 4
  %i25 = alloca i32, align 4
  store i32 5, ptr %i25, align 4
  %13 = call i32 (ptr, ...) @printf(ptr @fmt.20, i32 5)
  ret i32 0
}

declare i32 @printf(ptr, ...)
