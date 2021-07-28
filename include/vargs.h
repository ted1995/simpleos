//可变形参表相关的宏定义


//c语言参数是从右往左入栈，即右边的参数存在栈高地址，是为了支持函数的变个数参数
//例如printf函数的第一个参数为格式，如果从右往左入栈，则第一个参数格式放在栈顶，可以通过偏移取得其他参数的起始地址，但是反过来则不行


/*
#define va_list char *
//根据第一个固定参数的地址和类型，计算出第一个可变参数的起始地址
#define va_start(p, first) (p = (va_list)&first + sizeof(first))
//(p += sizeof(next))让p指向下一个可变参数的起始地址
//− sizeof(next)保持表达式的值不变为p
//(*(next*)，将原地址强制转换为此变量的指针，再取此变量的值
#define va_arg(p, next) (*(next*)((p += sizeof(next) ) − sizeof(next)))
//可变参数取值完毕，置参数列表指针为空
#define va_end(p) (p = (va_list)NULL)
*/

#ifndef INCLUDE_VARGS_H_
#define INCLUDE_VARGS_H_

typedef __builtin_va_list va_list;

#define va_start(ap, last)         (__builtin_va_start(ap, last))
#define va_arg(ap, type)           (__builtin_va_arg(ap, type))
#define va_end(ap) 

#endif 	// INCLUDE_VARGS_H_