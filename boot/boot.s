; ----------------------------------------------------------------
;
;   boot.s -- 内核从这里开始
;
; ----------------------------------------------------------------

; magic域，Multiboot 魔数，由规范决定的，multi_boot()将魔数0x2BADB002放入eax中，来告诉操作系统目前是由兼容Multiboot的bootloader在进行引导
MBOOT_HEADER_MAGIC  equ     0x1BADB002


; flag域，设置0位表示所有的引导模块将按页(4KB)边界对齐
MBOOT_PAGE_ALIGN    equ     1 << 0

; flag域，设置1位通过 Multiboot 信息结构的 mem_* 域包括可用内存的信息
; (告诉GRUB把内存空间的信息包含在Multiboot信息结构中)
MBOOT_MEM_INFO      equ     1 << 1    

; 定义我们使用的 Multiboot 的标记，flag域
MBOOT_HEADER_FLAGS  equ     MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO

; checksum域是一个32位的无符号值，当与其他的magic域(也就是magic和flags)
; 相加时，要求其结果必须是32位的无符号值 0 (即magic+flags+checksum = 0)
MBOOT_CHECKSUM      equ     -(MBOOT_HEADER_MAGIC+MBOOT_HEADER_FLAGS)

; 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头
; Multiboot 头的分布必须如下表所示：
; ----------------------------------------------------------
; 偏移量  类型  域名        备注
;
;   0     u32   magic       必需
;   4     u32   flags       必需 
;   8     u32   checksum    必需 
;
; 我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
;-----------------------------------------------------------

;-----------------------------------------------------------------------------

[BITS 32]       ; 所有代码以 32-bit 的方式编译
section .init.text   ; 临时代码段从这里开始

; 在代码段的起始位置设置符合 Multiboot 规范的标记

dd MBOOT_HEADER_MAGIC   ; GRUB 会通过这个魔数判断该映像是否支持
dd MBOOT_HEADER_FLAGS   ; GRUB 的一些加载时选项，其详细注释在定义处
dd MBOOT_CHECKSUM       ; 检测数值，其含义在定义处

[GLOBAL start]          ; 向外部声明内核代码入口，此处提供该声明给链接器
[GLOBAL mboot_ptr_tmp]  ; 向外部声明 struct multiboot * 变量
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数

start:
    cli                         ; 此时还没有设置好保护模式的中断处理，要关闭中断
    mov esp, STACK_TOP          ; 设置内核栈地址
    mov ebp, 0                  ; 帧指针修改为 0
    and esp, 0FFFFFFF0H         ; 栈地址按照16字节对齐
    mov [mboot_ptr_tmp], ebx    ; 将 ebx 中存储的指针存入全局变量，指向multiboot信息结构的物理地址，引导程序通过它将引导信息传递给操作系统
                                ; multiboot结构体则根据flag域每一位的设置启用结构体中对应的域。
    call kern_entry             ; 调用内核入口函数

;-----------------------------------------------------------------------------

;section .bss                ; 未初始化的数据段从这里开始，bss段在程序开始运行时，根据bss段的请况分配空间，并初始化为0

;stack:                      ; 在bss段申明stack变量，且预留32KB的未初始化的字节空间
;    resb 32768              ; 这里作为内核栈，resb申明未初始化的字节空间，后面跟的是字节数
;glb_mboot_ptr:              ; 在bss段申明全局的 multiboot 结构体指针变量
;   resb 4                   ; 预留四个字节的空间，用于存储指针变量

;STACK_TOP equ $-stack-1     ; 内核栈顶，$ 符指代是当前地址，表示bss段开始，向后移动（32K+1）个字节，正好移动到stack的顶部，即栈顶
                             ; bss段大概是这样分布的：stack-----32K-----stacktop（glb_mboot_ptr）---4---


section .init.data      ; 开启分页前临时的数据段
stack:    times 1024 db 0   ; 这里作为临时内核栈
STACK_TOP equ $-stack-1     ; 内核栈顶，$ 符指代是当前地址

mboot_ptr_tmp: dd 0     ; 全局的 multiboot 结构体指针


;-----------------------------------------------------------------------------