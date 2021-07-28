[global switch_to]

; 具体的线程切换操作，重点在于寄存器的保存与恢复
;switch_to有两个参数，一个是前一个进程的上下文指针，一个是待切换进程的上下文指针
switch_to:
        ;esp+4存放的是prev的上下文指针
        mov eax, [esp+4]
        ;将prev进程的寄存器保存在自己的上下文中
        mov [eax+0],  esp
        mov [eax+4],  ebp
        mov [eax+8],  ebx
        mov [eax+12], esi
        mov [eax+16], edi
        ;标志寄存器入栈
        pushf
        ;将标志寄存器的出栈到ecx中
        pop ecx
        ;标志寄存器的值进入上下文
        mov [eax+20], ecx


        ;esp+8是cur的上下文指针
        mov eax, [esp+8]
        ;用cur的上下文填充相关寄存器的值
        mov esp, [eax+0]
        mov ebp, [eax+4]
        mov ebx, [eax+8]
        mov esi, [eax+12]
        mov edi, [eax+16]
        ;将标志寄存器的值写入标志寄存器
        mov eax, [eax+20]
        push eax
        popf
 	
        ;返回，当前esp指针已经切换待执行的进程
        ret