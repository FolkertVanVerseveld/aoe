/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef RE_CPU_H
#define RE_CPU_H

#include <stdint.h>

#define REG_EDI  8
#define REG_ESI  9
#define REG_EBP 10
#define REG_EBX 11
#define REG_EDX 12
#define REG_EAX 13
#define REG_ECX 14
#define REG_ESP 15
#define REG_EIP 16

#define REG_R8   0
#define REG_R9   1
#define REG_R10  2
#define REG_R11  3
#define REG_R12  4
#define REG_R13  5
#define REG_R14  6
#define REG_R15  7
#define REG_RDI  8
#define REG_RSI  9
#define REG_RBP 10
#define REG_RBX 11
#define REG_RDX 12
#define REG_RAX 13
#define REG_RCX 14
#define REG_RSP 15
#define REG_RIP 16

/* ucontext-like copy taken from GNU glibc <sys/ucontext.h> */
struct cpu {
	int64_t r8, r9, r10, r11, r12, r13, r14, r15;
	int64_t rdi, rsi, rbp, rbx, rdx, rax, rcx, rsp, rip;
	union {
		int32_t e[17];
		int64_t r[17];
	} regs;
	int64_t efl;
	int64_t csgsfs;
	int64_t err, trapno, oldmask;
	int64_t cr2;
};

#endif
