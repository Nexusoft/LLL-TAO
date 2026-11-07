	.file	"write.cpp"
	.text
	.section	.text._ZN5debug9exceptionD2Ev,"axG",@progbits,_ZN5debug9exceptionD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN5debug9exceptionD2Ev
	.type	_ZN5debug9exceptionD2Ev, @function
_ZN5debug9exceptionD2Ev:
.LFB14461:
	.cfi_startproc
	endbr64
	leaq	16+_ZTVN5debug9exceptionE(%rip), %rax
	movq	%rax, (%rdi)
	jmp	_ZNSt13runtime_errorD2Ev@PLT
	.cfi_endproc
.LFE14461:
	.size	_ZN5debug9exceptionD2Ev, .-_ZN5debug9exceptionD2Ev
	.weak	_ZN5debug9exceptionD1Ev
	.set	_ZN5debug9exceptionD1Ev,_ZN5debug9exceptionD2Ev
	.section	.text._ZN5debug9exceptionD0Ev,"axG",@progbits,_ZN5debug9exceptionD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN5debug9exceptionD0Ev
	.type	_ZN5debug9exceptionD0Ev, @function
_ZN5debug9exceptionD0Ev:
.LFB14463:
	.cfi_startproc
	endbr64
	leaq	16+_ZTVN5debug9exceptionE(%rip), %rax
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rdi, %rbx
	movq	%rax, (%rdi)
	call	_ZNSt13runtime_errorD2Ev@PLT
	movq	%rbx, %rdi
	movl	$16, %esi
	popq	%rbx
	.cfi_def_cfa_offset 8
	jmp	_ZdlPvm@PLT
	.cfi_endproc
.LFE14463:
	.size	_ZN5debug9exceptionD0Ev, .-_ZN5debug9exceptionD0Ev
	.section	.text.unlikely,"ax",@progbits
	.align 2
	.type	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0, @function
_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0:
.LFB18886:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rdi, %rbx
	movq	(%rdi), %rdi
	testq	%rdi, %rdi
	je	.L5
	call	pthread_mutex_unlock@PLT
	movb	$0, 8(%rbx)
.L5:
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE18886:
	.size	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0, .-_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	.text
	.p2align 4
	.type	_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc.isra.0, @function
_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc.isra.0:
.LFB18888:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	testq	%rsi, %rsi
	je	.L14
	movq	%rsi, %rdi
	movq	%rsi, %rbx
	call	strlen@PLT
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	movq	%rbx, %rsi
	movq	%rbp, %rdi
	popq	%rbx
	.cfi_def_cfa_offset 16
	movq	%rax, %rdx
	popq	%rbp
	.cfi_def_cfa_offset 8
	jmp	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	.p2align 4,,10
	.p2align 3
.L14:
	.cfi_restore_state
	movq	(%rdi), %rax
	movq	-24(%rax), %rdi
	addq	%rbp, %rdi
	movl	32(%rdi), %esi
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	orl	$1, %esi
	jmp	_ZNSt9basic_iosIcSt11char_traitsIcEE5clearESt12_Ios_Iostate@PLT
	.cfi_endproc
.LFE18888:
	.size	_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc.isra.0, .-_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc.isra.0
	.align 2
	.p2align 4
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSEOS4_.isra.0, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSEOS4_.isra.0:
.LFB18890:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	leaq	16(%rbp), %rax
	movq	%rsi, %rbx
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	movq	(%rdi), %rdi
	movq	(%rsi), %rsi
	movq	8(%rbx), %rdx
	cmpq	%rax, %rdi
	leaq	16(%rbx), %rax
	je	.L31
	cmpq	%rsi, %rax
	je	.L17
	movq	%rsi, 0(%rbp)
	movq	16(%rbp), %rcx
	movq	%rdx, 8(%rbp)
	movq	16(%rbx), %rdx
	movq	%rdx, 16(%rbp)
	testq	%rdi, %rdi
	je	.L25
	movq	%rdi, (%rbx)
	movq	%rcx, 16(%rbx)
.L20:
	movq	$0, 8(%rbx)
	movb	$0, (%rdi)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L31:
	.cfi_restore_state
	cmpq	%rsi, %rax
	je	.L17
	movq	%rsi, 0(%rbp)
	movq	%rax, %rdi
	movq	%rdx, 8(%rbp)
	movq	16(%rbx), %rdx
	movq	%rdx, 16(%rbp)
.L23:
	movq	%rdi, (%rbx)
	jmp	.L20
	.p2align 4,,10
	.p2align 3
.L17:
	cmpq	%rbx, %rbp
	je	.L24
	testq	%rdx, %rdx
	je	.L21
	cmpq	$1, %rdx
	je	.L32
	call	memcpy@PLT
	movq	8(%rbx), %rdx
	movq	0(%rbp), %rdi
.L21:
	movq	%rdx, 8(%rbp)
	movb	$0, (%rdi,%rdx)
	movq	(%rbx), %rdi
	jmp	.L20
	.p2align 4,,10
	.p2align 3
.L25:
	movq	%rax, %rdi
	jmp	.L23
	.p2align 4,,10
	.p2align 3
.L32:
	movzbl	(%rsi), %eax
	movb	%al, (%rdi)
	movq	8(%rbx), %rdx
	movq	0(%rbp), %rdi
	jmp	.L21
	.p2align 4,,10
	.p2align 3
.L24:
	movq	%rsi, %rdi
	jmp	.L20
	.cfi_endproc
.LFE18890:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSEOS4_.isra.0, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSEOS4_.isra.0
	.align 2
	.p2align 4
	.type	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0, @function
_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0:
.LFB18892:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rcx, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movq	%rsi, %rbx
	movq	%rdx, %rsi
	movq	%rcx, %rdx
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	cmpq	%rcx, %rbx
	cmovbe	%rbx, %rdx
	testq	%rdx, %rdx
	je	.L34
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L33
.L34:
	subq	%rbp, %rbx
	movl	$2147483647, %eax
	cmpq	$2147483647, %rbx
	jg	.L33
	cmpq	$-2147483648, %rbx
	movl	$-2147483648, %eax
	cmovge	%ebx, %eax
.L33:
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE18892:
	.size	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0, .-_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	.section	.text._ZN10BaseStreamD2Ev,"axG",@progbits,_ZN10BaseStreamD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN10BaseStreamD2Ev
	.type	_ZN10BaseStreamD2Ev, @function
_ZN10BaseStreamD2Ev:
.LFB14452:
	.cfi_startproc
	endbr64
	leaq	16+_ZTV10BaseStream(%rip), %rax
	movq	%rax, (%rdi)
	movq	16(%rdi), %rax
	testq	%rax, %rax
	je	.L42
	movq	32(%rdi), %rsi
	movq	%rax, %rdi
	subq	%rax, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L42:
	ret
	.cfi_endproc
.LFE14452:
	.size	_ZN10BaseStreamD2Ev, .-_ZN10BaseStreamD2Ev
	.weak	_ZN10BaseStreamD1Ev
	.set	_ZN10BaseStreamD1Ev,_ZN10BaseStreamD2Ev
	.section	.text._ZN10BaseStreamD0Ev,"axG",@progbits,_ZN10BaseStreamD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN10BaseStreamD0Ev
	.type	_ZN10BaseStreamD0Ev, @function
_ZN10BaseStreamD0Ev:
.LFB14454:
	.cfi_startproc
	endbr64
	leaq	16+_ZTV10BaseStream(%rip), %rax
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rdi, %rbx
	movq	%rax, (%rdi)
	movq	16(%rdi), %rdi
	testq	%rdi, %rdi
	je	.L45
	movq	32(%rbx), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L45:
	movq	%rbx, %rdi
	movl	$40, %esi
	popq	%rbx
	.cfi_def_cfa_offset 8
	jmp	_ZdlPvm@PLT
	.cfi_endproc
.LFE14454:
	.size	_ZN10BaseStreamD0Ev, .-_ZN10BaseStreamD0Ev
	.section	.text._ZN3TAO9Operation6StreamD2Ev,"axG",@progbits,_ZN3TAO9Operation6StreamD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN3TAO9Operation6StreamD2Ev
	.type	_ZN3TAO9Operation6StreamD2Ev, @function
_ZN3TAO9Operation6StreamD2Ev:
.LFB14495:
	.cfi_startproc
	endbr64
	leaq	16+_ZTV10BaseStream(%rip), %rax
	movq	%rax, (%rdi)
	movq	16(%rdi), %rax
	testq	%rax, %rax
	je	.L50
	movq	32(%rdi), %rsi
	movq	%rax, %rdi
	subq	%rax, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L50:
	ret
	.cfi_endproc
.LFE14495:
	.size	_ZN3TAO9Operation6StreamD2Ev, .-_ZN3TAO9Operation6StreamD2Ev
	.weak	_ZN3TAO9Operation6StreamD1Ev
	.set	_ZN3TAO9Operation6StreamD1Ev,_ZN3TAO9Operation6StreamD2Ev
	.section	.text._ZN3TAO9Operation6StreamD0Ev,"axG",@progbits,_ZN3TAO9Operation6StreamD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN3TAO9Operation6StreamD0Ev
	.type	_ZN3TAO9Operation6StreamD0Ev, @function
_ZN3TAO9Operation6StreamD0Ev:
.LFB14497:
	.cfi_startproc
	endbr64
	leaq	16+_ZTV10BaseStream(%rip), %rax
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rdi, %rbx
	movq	%rax, (%rdi)
	movq	16(%rdi), %rdi
	testq	%rdi, %rdi
	je	.L53
	movq	32(%rbx), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L53:
	movq	%rbx, %rdi
	movl	$40, %esi
	popq	%rbx
	.cfi_def_cfa_offset 8
	jmp	_ZdlPvm@PLT
	.cfi_endproc
.LFE14497:
	.size	_ZN3TAO9Operation6StreamD0Ev, .-_ZN3TAO9Operation6StreamD0Ev
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC0:
	.string	"basic_string: construction from null is not valid"
	.section	.text.startup,"ax",@progbits
	.align 2
	.p2align 4
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0:
.LFB18895:
	.cfi_startproc
	pushq	%r13
	.cfi_def_cfa_offset 16
	.cfi_offset 13, -16
	leaq	16(%rdi), %r13
	pushq	%r12
	.cfi_def_cfa_offset 24
	.cfi_offset 12, -24
	pushq	%rbp
	.cfi_def_cfa_offset 32
	.cfi_offset 6, -32
	pushq	%rbx
	.cfi_def_cfa_offset 40
	.cfi_offset 3, -40
	subq	$8, %rsp
	.cfi_def_cfa_offset 48
	movq	%r13, (%rdi)
	testq	%rsi, %rsi
	je	.L68
	movq	%rdi, %rbp
	movq	%rsi, %rdi
	movq	%rsi, %r12
	call	strlen@PLT
	movq	%rax, %rbx
	cmpq	$15, %rax
	ja	.L69
	cmpq	$1, %rax
	je	.L70
	testq	%rax, %rax
	jne	.L61
.L63:
	movq	0(%rbp), %rax
	movq	%rbx, 8(%rbp)
	movb	$0, (%rax,%rbx)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbx
	.cfi_def_cfa_offset 32
	popq	%rbp
	.cfi_def_cfa_offset 24
	popq	%r12
	.cfi_def_cfa_offset 16
	popq	%r13
	.cfi_def_cfa_offset 8
	ret
.L70:
	.cfi_restore_state
	movzbl	(%r12), %eax
	movb	%al, 16(%rbp)
	jmp	.L63
.L69:
	leaq	1(%rax), %rdi
	call	_Znwm@PLT
	movq	%rbx, 16(%rbp)
	movq	%rax, 0(%rbp)
	movq	%rax, %r13
.L61:
	movq	%rbx, %rdx
	movq	%r12, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	jmp	.L63
.L68:
	leaq	.LC0(%rip), %rdi
	call	_ZSt19__throw_logic_errorPKc@PLT
	.cfi_endproc
.LFE18895:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev:
.LFB15318:
	.cfi_startproc
	endbr64
	movq	(%rdi), %rax
	leaq	16(%rdi), %rdx
	cmpq	%rdx, %rax
	je	.L71
	movq	16(%rdi), %rsi
	movq	%rax, %rdi
	addq	$1, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L71:
	ret
	.cfi_endproc
.LFE15318:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	.set	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"basic_string::replace"
	.section	.rodata.str1.8
	.align 8
.LC2:
	.string	"%s: __pos (which is %zu) > this->size() (which is %zu)"
	.section	.rodata.str1.1
.LC3:
	.string	"basic_string::_M_replace"
.LC4:
	.string	"basic_string::_M_create"
	.section	.text.unlikely
	.align 2
.LCOLDB5:
	.text
.LHOTB5:
	.align 2
	.p2align 4
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0:
.LFB18901:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rsi, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	8(%rdi), %rdi
	movq	%rdi, %r14
	subq	%rsi, %r14
	cmpq	%rdx, %r14
	cmovbe	%r14, %rdx
	cmpq	%rsi, %rdi
	jb	.L123
	movabsq	$9223372036854775807, %rax
	movq	%r8, %r13
	movq	%rdx, %rbp
	subq	%rdi, %rax
	addq	%rdx, %rax
	cmpq	%r8, %rax
	jb	.L124
	movq	(%rbx), %rax
	leaq	(%rdi,%r8), %r10
	leaq	16(%rbx), %r8
	subq	%rdx, %r10
	cmpq	%r8, %rax
	je	.L125
	movq	16(%rbx), %r15
	cmpq	%r10, %r15
	jb	.L126
.L77:
	addq	%rax, %r12
	subq	%rbp, %r14
	cmpq	%rax, %rcx
	jnb	.L127
.L81:
	testq	%r14, %r14
	je	.L83
	cmpq	%r13, %rbp
	je	.L83
	leaq	(%r12,%rbp), %rsi
	leaq	(%r12,%r13), %rdi
	cmpq	$1, %r14
	je	.L128
	movq	%r14, %rdx
	movq	%rcx, 8(%rsp)
	movq	%r10, (%rsp)
	call	memmove@PLT
	movq	8(%rsp), %rcx
	movq	(%rsp), %r10
.L83:
	testq	%r13, %r13
	je	.L122
.L85:
	cmpq	$1, %r13
	je	.L129
	movq	%r13, %rdx
	movq	%rcx, %rsi
	movq	%r12, %rdi
	movq	%r10, (%rsp)
	call	memcpy@PLT
	movq	(%rbx), %r14
	movq	(%rsp), %r10
	jmp	.L86
	.p2align 4,,10
	.p2align 3
.L129:
	movzbl	(%rcx), %eax
	movb	%al, (%r12)
.L122:
	movq	(%rbx), %r14
.L86:
	movq	%r10, 8(%rbx)
	movb	$0, (%r14,%r10)
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L127:
	.cfi_restore_state
	addq	%rdi, %rax
	cmpq	%rcx, %rax
	jnb	.L82
	jmp	.L81
	.p2align 4,,10
	.p2align 3
.L126:
	testq	%r10, %r10
	js	.L78
	subq	%rdx, %r14
	addq	%r15, %r15
	movq	%r14, 24(%rsp)
	cmpq	%r15, %r10
	jb	.L130
.L89:
	movq	%r10, %rdi
	movq	%r10, %r15
	addq	$1, %rdi
	js	.L90
.L91:
	movq	%rcx, 16(%rsp)
	movq	%r8, 8(%rsp)
	movq	%r10, (%rsp)
	call	_Znwm@PLT
	testq	%r12, %r12
	movq	(%rsp), %r10
	movq	8(%rsp), %r8
	movq	16(%rsp), %rcx
	movq	%rax, %r14
	je	.L92
	movq	(%rbx), %rsi
	cmpq	$1, %r12
	je	.L131
	movq	%r12, %rdx
	movq	%rax, %rdi
	movq	%rcx, 16(%rsp)
	movq	%r8, 8(%rsp)
	movq	%r10, (%rsp)
	call	memcpy@PLT
	movq	16(%rsp), %rcx
	movq	8(%rsp), %r8
	movq	(%rsp), %r10
.L92:
	testq	%rcx, %rcx
	je	.L94
	testq	%r13, %r13
	je	.L94
	leaq	(%r14,%r12), %rdi
	cmpq	$1, %r13
	je	.L132
	movq	%r13, %rdx
	movq	%rcx, %rsi
	movq	%r8, 8(%rsp)
	movq	%r10, (%rsp)
	call	memcpy@PLT
	movq	8(%rsp), %r8
	movq	(%rsp), %r10
.L94:
	movq	24(%rsp), %rax
	movq	(%rbx), %rcx
	testq	%rax, %rax
	jne	.L133
.L96:
	cmpq	%rcx, %r8
	je	.L98
	movq	16(%rbx), %rax
	movq	%rcx, %rdi
	movq	%r10, (%rsp)
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movq	(%rsp), %r10
.L98:
	movq	%r14, (%rbx)
	movq	%r15, 16(%rbx)
	jmp	.L86
	.p2align 4,,10
	.p2align 3
.L133:
	addq	%r12, %rbp
	addq	%r13, %r12
	leaq	(%rcx,%rbp), %rsi
	leaq	(%r14,%r12), %rdi
	cmpq	$1, %rax
	je	.L134
	movq	%rax, %rdx
	movq	%rcx, 16(%rsp)
	movq	%r8, 8(%rsp)
	movq	%r10, (%rsp)
	call	memcpy@PLT
	movq	16(%rsp), %rcx
	movq	8(%rsp), %r8
	movq	(%rsp), %r10
	jmp	.L96
	.p2align 4,,10
	.p2align 3
.L130:
	leaq	1(%r15), %rdi
	testq	%r15, %r15
	jns	.L91
.L90:
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L125:
	cmpq	$15, %r10
	jbe	.L77
	testq	%r10, %r10
	js	.L78
	subq	%rdx, %r14
	movq	%r14, 24(%rsp)
	cmpq	$29, %r10
	ja	.L89
	movl	$31, %edi
	movl	$30, %r15d
	jmp	.L91
	.p2align 4,,10
	.p2align 3
.L128:
	movzbl	(%rsi), %eax
	movb	%al, (%rdi)
	testq	%r13, %r13
	jne	.L85
	jmp	.L122
	.p2align 4,,10
	.p2align 3
.L131:
	movzbl	(%rsi), %eax
	movb	%al, (%r14)
	jmp	.L92
	.p2align 4,,10
	.p2align 3
.L132:
	movzbl	(%rcx), %eax
	movb	%al, (%rdi)
	jmp	.L94
	.p2align 4,,10
	.p2align 3
.L134:
	movzbl	(%rsi), %eax
	movb	%al, (%rdi)
	jmp	.L96
.L124:
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L123:
	movq	%rdi, %rcx
	movq	%rsi, %rdx
	leaq	.LC2(%rip), %rdi
	xorl	%eax, %eax
	leaq	.LC1(%rip), %rsi
	call	_ZSt24__throw_out_of_range_fmtPKcz@PLT
.L78:
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0.cold, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0.cold:
.LFSB18901:
.L82:
	.cfi_def_cfa_offset 96
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%rbp, %rdx
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%r10, (%rsp)
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
	movq	(%rbx), %r14
	movq	(%rsp), %r10
	jmp	.L86
	.cfi_endproc
.LFE18901:
	.text
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0
	.section	.text.unlikely
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0.cold, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0.cold
.LCOLDE5:
	.text
.LHOTE5:
	.align 2
	.p2align 4
	.type	_ZNKSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE4findERS7_.isra.0, @function
_ZNKSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE4findERS7_.isra.0:
.LFB18904:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	leaq	8(%rdi), %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$24, %rsp
	.cfi_def_cfa_offset 80
	movq	16(%rdi), %r14
	movq	%rbp, 8(%rsp)
	testq	%r14, %r14
	je	.L136
	movq	%rsi, %r12
	movq	%rdx, %r15
	movl	$2147483648, %r13d
	jmp	.L142
	.p2align 4,,10
	.p2align 3
.L140:
	movq	24(%r14), %r14
	testq	%r14, %r14
	je	.L156
.L142:
	movq	40(%r14), %rbx
	cmpq	%rbx, %r15
	movq	%rbx, %rdx
	cmovbe	%r15, %rdx
	testq	%rdx, %rdx
	je	.L137
	movq	32(%r14), %rdi
	movq	%r12, %rsi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L138
.L137:
	subq	%r15, %rbx
	cmpq	%r13, %rbx
	jge	.L139
	movabsq	$-2147483649, %rax
	cmpq	%rax, %rbx
	jle	.L140
	movl	%ebx, %eax
.L138:
	testl	%eax, %eax
	js	.L140
.L139:
	movq	%r14, %rbp
	movq	16(%r14), %r14
	testq	%r14, %r14
	jne	.L142
.L156:
	cmpq	%rbp, 8(%rsp)
	je	.L136
	movq	40(%rbp), %rbx
	cmpq	%rbx, %r15
	movq	%rbx, %rdx
	cmovbe	%r15, %rdx
	testq	%rdx, %rdx
	je	.L143
	movq	32(%rbp), %rsi
	movq	%r12, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L144
.L143:
	movq	%r15, %r8
	subq	%rbx, %r8
	cmpq	$2147483647, %r8
	jg	.L136
	cmpq	$-2147483648, %r8
	jl	.L146
	movl	%r8d, %eax
.L144:
	testl	%eax, %eax
	cmovs	8(%rsp), %rbp
.L136:
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbp, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L146:
	.cfi_restore_state
	movq	8(%rsp), %rbp
	jmp	.L136
	.cfi_endproc
.LFE18904:
	.size	_ZNKSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE4findERS7_.isra.0, .-_ZNKSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE4findERS7_.isra.0
	.align 2
	.p2align 4
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0:
.LFB18905:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	%rdi, 16(%rsp)
	testq	%rdi, %rdi
	je	.L157
.L202:
	movq	16(%rsp), %rax
	movq	24(%rax), %rax
	movq	%rax, 8(%rsp)
	testq	%rax, %rax
	je	.L159
.L198:
	movq	8(%rsp), %rax
	movq	24(%rax), %r14
	testq	%r14, %r14
	je	.L160
.L194:
	movq	24(%r14), %r15
	testq	%r15, %r15
	je	.L161
.L190:
	movq	24(%r15), %rbx
	testq	%rbx, %rbx
	je	.L162
.L186:
	movq	24(%rbx), %rbp
	testq	%rbp, %rbp
	je	.L163
.L182:
	movq	24(%rbp), %r12
	testq	%r12, %r12
	je	.L164
.L178:
	movq	24(%r12), %r13
	testq	%r13, %r13
	je	.L165
.L174:
	movq	24(%r13), %rdx
	testq	%rdx, %rdx
	jne	.L170
	jmp	.L166
	.p2align 4,,10
	.p2align 3
.L258:
	movq	48(%rdx), %rax
	movq	%rdx, 24(%rsp)
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movq	24(%rsp), %rdi
	movl	$104, %esi
	call	_ZdlPvm@PLT
	cmpq	$0, (%rsp)
	je	.L166
.L169:
	movq	(%rsp), %rdx
.L170:
	movq	24(%rdx), %rdi
	movq	%rdx, (%rsp)
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0
	movq	(%rsp), %rdx
	movq	16(%rdx), %rax
	movq	64(%rdx), %rdi
	leaq	80(%rdx), %rcx
	movq	%rax, (%rsp)
	cmpq	%rcx, %rdi
	je	.L167
	movq	80(%rdx), %rax
	movq	%rdx, 24(%rsp)
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movq	24(%rsp), %rdx
.L167:
	movq	32(%rdx), %rdi
	leaq	48(%rdx), %rcx
	cmpq	%rcx, %rdi
	jne	.L258
	movl	$104, %esi
	movq	%rdx, %rdi
	call	_ZdlPvm@PLT
	cmpq	$0, (%rsp)
	jne	.L169
	.p2align 4,,10
	.p2align 3
.L166:
	movq	16(%r13), %rax
	movq	64(%r13), %rdi
	leaq	80(%r13), %rdx
	movq	%rax, (%rsp)
	cmpq	%rdx, %rdi
	je	.L171
	movq	80(%r13), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L171:
	movq	32(%r13), %rdi
	leaq	48(%r13), %rdx
	cmpq	%rdx, %rdi
	je	.L172
	movq	48(%r13), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L172:
	movl	$104, %esi
	movq	%r13, %rdi
	call	_ZdlPvm@PLT
	cmpq	$0, (%rsp)
	je	.L165
	movq	(%rsp), %r13
	jmp	.L174
.L163:
	movq	64(%rbx), %rdi
	leaq	80(%rbx), %rax
	movq	16(%rbx), %rbp
	cmpq	%rax, %rdi
	je	.L183
	movq	80(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L183:
	movq	32(%rbx), %rdi
	leaq	48(%rbx), %rax
	cmpq	%rax, %rdi
	je	.L184
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L184:
	movl	$104, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	testq	%rbp, %rbp
	je	.L162
	movq	%rbp, %rbx
	jmp	.L186
	.p2align 4,,10
	.p2align 3
.L164:
	movq	64(%rbp), %rdi
	leaq	80(%rbp), %rax
	movq	16(%rbp), %r12
	cmpq	%rax, %rdi
	je	.L179
	movq	80(%rbp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L179:
	movq	32(%rbp), %rdi
	leaq	48(%rbp), %rax
	cmpq	%rax, %rdi
	je	.L180
	movq	48(%rbp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L180:
	movl	$104, %esi
	movq	%rbp, %rdi
	call	_ZdlPvm@PLT
	testq	%r12, %r12
	je	.L163
	movq	%r12, %rbp
	jmp	.L182
.L162:
	movq	64(%r15), %rdi
	leaq	80(%r15), %rax
	movq	16(%r15), %rbx
	cmpq	%rax, %rdi
	je	.L187
	movq	80(%r15), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L187:
	movq	32(%r15), %rdi
	leaq	48(%r15), %rax
	cmpq	%rax, %rdi
	je	.L188
	movq	48(%r15), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L188:
	movl	$104, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L161
	movq	%rbx, %r15
	jmp	.L190
	.p2align 4,,10
	.p2align 3
.L165:
	movq	64(%r12), %rdi
	leaq	80(%r12), %rax
	movq	16(%r12), %r13
	cmpq	%rax, %rdi
	je	.L175
	movq	80(%r12), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L175:
	movq	32(%r12), %rdi
	leaq	48(%r12), %rax
	cmpq	%rax, %rdi
	je	.L176
	movq	48(%r12), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L176:
	movl	$104, %esi
	movq	%r12, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	je	.L164
	movq	%r13, %r12
	jmp	.L178
.L161:
	movq	64(%r14), %rdi
	leaq	80(%r14), %rax
	movq	16(%r14), %rbx
	cmpq	%rax, %rdi
	je	.L191
	movq	80(%r14), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L191:
	movq	32(%r14), %rdi
	leaq	48(%r14), %rax
	cmpq	%rax, %rdi
	je	.L192
	movq	48(%r14), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L192:
	movl	$104, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L160
	movq	%rbx, %r14
	jmp	.L194
.L160:
	movq	8(%rsp), %rcx
	movq	64(%rcx), %rdi
	leaq	80(%rcx), %rax
	movq	16(%rcx), %rbx
	cmpq	%rax, %rdi
	je	.L195
	movq	80(%rcx), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
.L195:
	movq	8(%rsp), %r14
	movq	32(%r14), %rdi
	leaq	48(%r14), %rax
	cmpq	%rax, %rdi
	je	.L196
	movq	48(%r14), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
	movl	$104, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L159
.L197:
	movq	%rbx, 8(%rsp)
	jmp	.L198
.L196:
	movq	%r14, %rdi
	movl	$104, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L197
.L159:
	movq	16(%rsp), %rcx
	movq	64(%rcx), %rdi
	leaq	80(%rcx), %rax
	movq	16(%rcx), %rbx
	cmpq	%rax, %rdi
	je	.L199
	movq	80(%rcx), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
.L199:
	movq	16(%rsp), %r15
	movq	32(%r15), %rdi
	leaq	48(%r15), %rax
	cmpq	%rax, %rdi
	je	.L200
	movq	48(%r15), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
	movl	$104, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L157
.L201:
	movq	%rbx, 16(%rsp)
	jmp	.L202
.L200:
	movq	%r15, %rdi
	movl	$104, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L201
.L157:
	addq	$40, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE18905:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0
	.section	.text._ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev,"axG",@progbits,_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev
	.type	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev, @function
_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev:
.LFB18258:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	movq	16(%rdi), %rbx
	testq	%rbx, %rbx
	je	.L259
.L264:
	movq	24(%rbx), %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0
	movq	64(%rbx), %rdi
	leaq	80(%rbx), %rax
	movq	16(%rbx), %rbp
	cmpq	%rax, %rdi
	je	.L261
	movq	80(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L261:
	movq	32(%rbx), %rdi
	leaq	48(%rbx), %rax
	cmpq	%rax, %rdi
	je	.L262
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L262:
	movl	$104, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	testq	%rbp, %rbp
	je	.L259
	movq	%rbp, %rbx
	jmp	.L264
	.p2align 4,,10
	.p2align 3
.L259:
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE18258:
	.size	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev, .-_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev
	.weak	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED1Ev
	.set	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED1Ev,_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED2Ev
	.text
	.align 2
	.p2align 4
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0:
.LFB18909:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	%rdi, 24(%rsp)
	testq	%rdi, %rdi
	je	.L272
.L308:
	movq	24(%rsp), %rax
	movq	24(%rax), %rax
	movq	%rax, 16(%rsp)
	testq	%rax, %rax
	je	.L274
.L305:
	movq	16(%rsp), %rax
	movq	24(%rax), %r14
	testq	%r14, %r14
	je	.L275
.L302:
	movq	24(%r14), %r15
	testq	%r15, %r15
	je	.L276
.L299:
	movq	24(%r15), %rbx
	testq	%rbx, %rbx
	je	.L277
.L296:
	movq	24(%rbx), %rbp
	testq	%rbp, %rbp
	je	.L278
.L293:
	movq	24(%rbp), %r12
	testq	%r12, %r12
	je	.L279
.L290:
	movq	24(%r12), %r13
	testq	%r13, %r13
	je	.L280
.L287:
	movq	24(%r13), %rdx
	testq	%rdx, %rdx
	jne	.L284
	jmp	.L281
	.p2align 4,,10
	.p2align 3
.L283:
	movq	(%rsp), %rdx
.L284:
	movq	24(%rdx), %rdi
	movq	%rdx, (%rsp)
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0
	movq	(%rsp), %rdx
	movq	16(%rdx), %rax
	movq	32(%rdx), %rdi
	leaq	48(%rdx), %rcx
	movq	%rax, (%rsp)
	cmpq	%rcx, %rdi
	je	.L282
	movq	48(%rdx), %rax
	movq	%rdx, 8(%rsp)
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movq	8(%rsp), %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	cmpq	$0, (%rsp)
	jne	.L283
.L281:
	movq	16(%r13), %rdx
	movq	32(%r13), %rdi
	leaq	48(%r13), %rcx
	movq	%rdx, (%rsp)
	cmpq	%rcx, %rdi
	je	.L285
	movq	48(%r13), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L285:
	movl	$64, %esi
	movq	%r13, %rdi
	call	_ZdlPvm@PLT
	movq	(%rsp), %rdx
	testq	%rdx, %rdx
	je	.L280
	movq	%rdx, %r13
	jmp	.L287
.L278:
	movq	32(%rbx), %rdi
	leaq	48(%rbx), %rax
	movq	16(%rbx), %rbp
	cmpq	%rax, %rdi
	je	.L294
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L294:
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	testq	%rbp, %rbp
	je	.L277
	movq	%rbp, %rbx
	jmp	.L296
.L282:
	movl	$64, %esi
	movq	%rdx, %rdi
	call	_ZdlPvm@PLT
	cmpq	$0, (%rsp)
	jne	.L283
	jmp	.L281
	.p2align 4,,10
	.p2align 3
.L288:
	movl	$64, %esi
	movq	%r12, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	jne	.L289
	.p2align 4,,10
	.p2align 3
.L279:
	movq	32(%rbp), %rdi
	leaq	48(%rbp), %rax
	movq	16(%rbp), %r12
	cmpq	%rax, %rdi
	je	.L291
	movq	48(%rbp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L291:
	movl	$64, %esi
	movq	%rbp, %rdi
	call	_ZdlPvm@PLT
	testq	%r12, %r12
	je	.L278
	movq	%r12, %rbp
	jmp	.L293
.L277:
	movq	32(%r15), %rdi
	leaq	48(%r15), %rax
	movq	16(%r15), %rbx
	cmpq	%rax, %rdi
	je	.L297
	movq	48(%r15), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L297:
	movl	$64, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L276
	movq	%rbx, %r15
	jmp	.L299
	.p2align 4,,10
	.p2align 3
.L280:
	movq	32(%r12), %rdi
	leaq	48(%r12), %rax
	movq	16(%r12), %r13
	cmpq	%rax, %rdi
	je	.L288
	movq	48(%r12), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r12, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	je	.L279
.L289:
	movq	%r13, %r12
	jmp	.L290
.L276:
	movq	32(%r14), %rdi
	leaq	48(%r14), %rax
	movq	16(%r14), %rbx
	cmpq	%rax, %rdi
	je	.L300
	movq	48(%r14), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L300:
	movl	$64, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L275
	movq	%rbx, %r14
	jmp	.L302
.L275:
	movq	16(%rsp), %r14
	movq	32(%r14), %rdi
	leaq	48(%r14), %rax
	movq	16(%r14), %rbx
	cmpq	%rax, %rdi
	je	.L303
	movq	48(%r14), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L274
.L304:
	movq	%rbx, 16(%rsp)
	jmp	.L305
.L306:
	movq	%r15, %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L307
.L272:
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L303:
	.cfi_restore_state
	movq	%r14, %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L304
.L274:
	movq	24(%rsp), %r15
	movq	32(%r15), %rdi
	leaq	48(%r15), %rax
	movq	16(%r15), %rbx
	cmpq	%rax, %rdi
	je	.L306
	movq	48(%r15), %rax
	leaq	1(%rax), %rsi
	movq	%rax, (%rsp)
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L272
.L307:
	movq	%rbx, 24(%rsp)
	jmp	.L308
	.cfi_endproc
.LFE18909:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0
	.section	.text._ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev,"axG",@progbits,_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev
	.type	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev, @function
_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev:
.LFB15172:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	movq	16(%rdi), %rbx
	testq	%rbx, %rbx
	je	.L364
.L368:
	movq	24(%rbx), %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0
	movq	32(%rbx), %rdi
	leaq	48(%rbx), %rax
	movq	16(%rbx), %rbp
	cmpq	%rax, %rdi
	je	.L366
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L366:
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	testq	%rbp, %rbp
	je	.L364
	movq	%rbp, %rbx
	jmp	.L368
	.p2align 4,,10
	.p2align 3
.L364:
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE15172:
	.size	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev, .-_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev
	.weak	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED1Ev
	.set	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED1Ev,_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED2Ev
	.section	.text.unlikely
	.align 2
.LCOLDB6:
	.text
.LHOTB6:
	.align 2
	.p2align 4
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0:
.LFB18912:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA18912
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movl	$1, %r13d
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdi, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rcx, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$8, %rsp
	.cfi_def_cfa_offset 64
	testq	%rsi, %rsi
	je	.L403
.L377:
	movl	$64, %edi
.LEHB0:
	call	_Znwm@PLT
.LEHE0:
	movq	0(%rbp), %r15
	movq	8(%rbp), %rbp
	leaq	48(%rax), %rdi
	movq	%rax, %rbx
	movq	%rdi, 32(%rax)
	cmpq	$15, %rbp
	ja	.L404
	cmpq	$1, %rbp
	je	.L405
	testq	%rbp, %rbp
	jne	.L383
.L385:
	movq	%rbp, 40(%rbx)
	leaq	8(%r12), %rcx
	movq	%r14, %rdx
	movq	%rbx, %rsi
	movb	$0, (%rdi,%rbp)
	movzbl	%r13b, %edi
	call	_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_@PLT
	addq	$1, 40(%r12)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L405:
	.cfi_restore_state
	movzbl	(%r15), %eax
	movb	%al, 48(%rbx)
	jmp	.L385
	.p2align 4,,10
	.p2align 3
.L403:
	leaq	8(%rdi), %rax
	cmpq	%rax, %rdx
	je	.L377
	movq	8(%rcx), %rbx
	movq	40(%rdx), %r15
	cmpq	%r15, %rbx
	movq	%r15, %rdx
	cmovbe	%rbx, %rdx
	testq	%rdx, %rdx
	je	.L378
	movq	32(%r14), %rsi
	movq	(%rcx), %rdi
	call	memcmp@PLT
	movl	%eax, %r13d
	testl	%eax, %eax
	jne	.L379
.L378:
	subq	%r15, %rbx
	xorl	%r13d, %r13d
	cmpq	$2147483647, %rbx
	jg	.L377
	cmpq	$-2147483648, %rbx
	jl	.L392
	movl	%ebx, %r13d
.L379:
	shrl	$31, %r13d
	jmp	.L377
	.p2align 4,,10
	.p2align 3
.L404:
	testq	%rbp, %rbp
	js	.L406
	movq	%rbp, %rdi
	addq	$1, %rdi
	js	.L407
.LEHB1:
	call	_Znwm@PLT
	movq	%rax, 32(%rbx)
	movq	%rax, %rdi
	movq	%rbp, 48(%rbx)
.L383:
	movq	%rbp, %rdx
	movq	%r15, %rsi
	call	memcpy@PLT
	movq	32(%rbx), %rdi
	jmp	.L385
	.p2align 4,,10
	.p2align 3
.L407:
	call	_ZSt17__throw_bad_allocv@PLT
.L392:
	movl	$1, %r13d
	jmp	.L377
.L406:
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE1:
.L393:
	endbr64
	movq	%rax, %rdi
	jmp	.L387
	.globl	__gxx_personality_v0
	.section	.gcc_except_table,"a",@progbits
	.align 4
.LLSDA18912:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT18912-.LLSDATTD18912
.LLSDATTD18912:
	.byte	0x1
	.uleb128 .LLSDACSE18912-.LLSDACSB18912
.LLSDACSB18912:
	.uleb128 .LEHB0-.LFB18912
	.uleb128 .LEHE0-.LEHB0
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB1-.LFB18912
	.uleb128 .LEHE1-.LEHB1
	.uleb128 .L393-.LFB18912
	.uleb128 0x1
.LLSDACSE18912:
	.byte	0x1
	.byte	0
	.align 4
	.long	0

.LLSDATT18912:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC18912
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0.cold, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0.cold:
.LFSB18912:
.L387:
	.cfi_def_cfa_offset 64
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	call	__cxa_begin_catch@PLT
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
.LEHB2:
	call	__cxa_rethrow@PLT
.LEHE2:
.L394:
	endbr64
	movq	%rax, %rbx
	call	__cxa_end_catch@PLT
	movq	%rbx, %rdi
.LEHB3:
	call	_Unwind_Resume@PLT
.LEHE3:
	.cfi_endproc
.LFE18912:
	.section	.gcc_except_table
	.align 4
.LLSDAC18912:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATTC18912-.LLSDATTDC18912
.LLSDATTDC18912:
	.byte	0x1
	.uleb128 .LLSDACSEC18912-.LLSDACSBC18912
.LLSDACSBC18912:
	.uleb128 .LEHB2-.LCOLDB6
	.uleb128 .LEHE2-.LEHB2
	.uleb128 .L394-.LCOLDB6
	.uleb128 0
	.uleb128 .LEHB3-.LCOLDB6
	.uleb128 .LEHE3-.LEHB3
	.uleb128 0
	.uleb128 0
.LLSDACSEC18912:
	.byte	0x1
	.byte	0
	.align 4
	.long	0

.LLSDATTC18912:
	.section	.text.unlikely
	.text
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0
	.section	.text.unlikely
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0.cold, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0.cold
.LCOLDE6:
	.text
.LHOTE6:
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv:
.LFB15517:
	.cfi_startproc
	endbr64
	movq	(%rdi), %rax
	leaq	16(%rdi), %rdx
	cmpq	%rdx, %rax
	je	.L408
	movq	16(%rdi), %rsi
	movq	%rax, %rdi
	addq	$1, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L408:
	ret
	.cfi_endproc
.LFE15517:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_
	.type	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_, @function
_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_:
.LFB15099:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15099
	endbr64
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	movq	%rsi, %r14
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	movq	%rdx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	leaq	16(%rdi), %r12
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	%rdi, %rbx
	movq	%r12, (%rdi)
	movq	%rsi, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L432
	cmpq	$1, %rax
	je	.L433
	testq	%rax, %rax
	jne	.L412
.L414:
	movq	%rbp, 8(%rbx)
	movb	$0, (%r12,%rbp)
	movq	0(%r13), %r14
	leaq	48(%rbx), %r12
	movq	%r12, 32(%rbx)
	testq	%r14, %r14
	je	.L434
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L435
	cmpq	$1, %rax
	je	.L436
	testq	%rax, %rax
	jne	.L418
.L420:
	movq	%rbp, 40(%rbx)
	movb	$0, (%r12,%rbp)
	movzbl	8(%r13), %eax
	movb	%al, 64(%rbx)
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L433:
	.cfi_restore_state
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L414
	.p2align 4,,10
	.p2align 3
.L432:
	leaq	1(%rax), %rdi
.LEHB4:
	call	_Znwm@PLT
.LEHE4:
	movq	%rbp, 16(%rbx)
	movq	%rax, (%rbx)
	movq	%rax, %r12
.L412:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	(%rbx), %r12
	jmp	.L414
	.p2align 4,,10
	.p2align 3
.L435:
	leaq	1(%rax), %rdi
.LEHB5:
	call	_Znwm@PLT
	movq	%rax, 32(%rbx)
	movq	%rax, %r12
	movq	%rbp, 48(%rbx)
.L418:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	32(%rbx), %r12
	jmp	.L420
	.p2align 4,,10
	.p2align 3
.L436:
	movzbl	(%r14), %eax
	movb	%al, 48(%rbx)
	jmp	.L420
.L434:
	leaq	.LC0(%rip), %rdi
	call	_ZSt19__throw_logic_errorPKc@PLT
.LEHE5:
.L423:
	endbr64
	movq	%rax, %rbp
.L422:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbp, %rdi
.LEHB6:
	call	_Unwind_Resume@PLT
.LEHE6:
	.cfi_endproc
.LFE15099:
	.section	.gcc_except_table
.LLSDA15099:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15099-.LLSDACSB15099
.LLSDACSB15099:
	.uleb128 .LEHB4-.LFB15099
	.uleb128 .LEHE4-.LEHB4
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB5-.LFB15099
	.uleb128 .LEHE5-.LEHB5
	.uleb128 .L423-.LFB15099
	.uleb128 0
	.uleb128 .LEHB6-.LFB15099
	.uleb128 .LEHE6-.LEHB6
	.uleb128 0
	.uleb128 0
.LLSDACSE15099:
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_,comdat
	.size	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_, .-_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_
	.type	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_, @function
_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_:
.LFB15111:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15111
	endbr64
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	movq	%rsi, %r14
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	movq	%rdx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	leaq	16(%rdi), %r12
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	%rdi, %rbx
	movq	%r12, (%rdi)
	movq	%rsi, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L459
	cmpq	$1, %rax
	je	.L460
	testq	%rax, %rax
	jne	.L439
.L441:
	movq	%rbp, 8(%rbx)
	movb	$0, (%r12,%rbp)
	movq	0(%r13), %r14
	leaq	48(%rbx), %r12
	movq	%r12, 32(%rbx)
	testq	%r14, %r14
	je	.L461
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L462
	cmpq	$1, %rax
	je	.L463
	testq	%rax, %rax
	jne	.L445
.L447:
	movq	%rbp, 40(%rbx)
	movb	$0, (%r12,%rbp)
	movzbl	8(%r13), %eax
	movb	%al, 64(%rbx)
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L460:
	.cfi_restore_state
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L441
	.p2align 4,,10
	.p2align 3
.L459:
	leaq	1(%rax), %rdi
.LEHB7:
	call	_Znwm@PLT
.LEHE7:
	movq	%rbp, 16(%rbx)
	movq	%rax, (%rbx)
	movq	%rax, %r12
.L439:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	(%rbx), %r12
	jmp	.L441
	.p2align 4,,10
	.p2align 3
.L462:
	leaq	1(%rax), %rdi
.LEHB8:
	call	_Znwm@PLT
	movq	%rax, 32(%rbx)
	movq	%rax, %r12
	movq	%rbp, 48(%rbx)
.L445:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	32(%rbx), %r12
	jmp	.L447
	.p2align 4,,10
	.p2align 3
.L463:
	movzbl	(%r14), %eax
	movb	%al, 48(%rbx)
	jmp	.L447
.L461:
	leaq	.LC0(%rip), %rdi
	call	_ZSt19__throw_logic_errorPKc@PLT
.LEHE8:
.L450:
	endbr64
	movq	%rax, %rbp
.L449:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbp, %rdi
.LEHB9:
	call	_Unwind_Resume@PLT
.LEHE9:
	.cfi_endproc
.LFE15111:
	.section	.gcc_except_table
.LLSDA15111:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15111-.LLSDACSB15111
.LLSDACSB15111:
	.uleb128 .LEHB7-.LFB15111
	.uleb128 .LEHE7-.LEHB7
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB8-.LFB15111
	.uleb128 .LEHE8-.LEHB8
	.uleb128 .L450-.LFB15111
	.uleb128 0
	.uleb128 .LEHB9-.LFB15111
	.uleb128 .LEHE9-.LEHB9
	.uleb128 0
	.uleb128 0
.LLSDACSE15111:
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_,comdat
	.size	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_, .-_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_
	.type	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_, @function
_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_:
.LFB15124:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15124
	endbr64
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	movq	%rsi, %r14
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	movq	%rdx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	leaq	16(%rdi), %r12
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	%rdi, %rbx
	movq	%r12, (%rdi)
	movq	%rsi, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L486
	cmpq	$1, %rax
	je	.L487
	testq	%rax, %rax
	jne	.L466
.L468:
	movq	%rbp, 8(%rbx)
	movb	$0, (%r12,%rbp)
	movq	0(%r13), %r14
	leaq	48(%rbx), %r12
	movq	%r12, 32(%rbx)
	testq	%r14, %r14
	je	.L488
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L489
	cmpq	$1, %rax
	je	.L490
	testq	%rax, %rax
	jne	.L472
.L474:
	movq	%rbp, 40(%rbx)
	movb	$0, (%r12,%rbp)
	movzbl	8(%r13), %eax
	movb	%al, 64(%rbx)
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L487:
	.cfi_restore_state
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L468
	.p2align 4,,10
	.p2align 3
.L486:
	leaq	1(%rax), %rdi
.LEHB10:
	call	_Znwm@PLT
.LEHE10:
	movq	%rbp, 16(%rbx)
	movq	%rax, (%rbx)
	movq	%rax, %r12
.L466:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	(%rbx), %r12
	jmp	.L468
	.p2align 4,,10
	.p2align 3
.L489:
	leaq	1(%rax), %rdi
.LEHB11:
	call	_Znwm@PLT
	movq	%rax, 32(%rbx)
	movq	%rax, %r12
	movq	%rbp, 48(%rbx)
.L472:
	movq	%r12, %rdi
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	32(%rbx), %r12
	jmp	.L474
	.p2align 4,,10
	.p2align 3
.L490:
	movzbl	(%r14), %eax
	movb	%al, 48(%rbx)
	jmp	.L474
.L488:
	leaq	.LC0(%rip), %rdi
	call	_ZSt19__throw_logic_errorPKc@PLT
.LEHE11:
.L477:
	endbr64
	movq	%rax, %rbp
.L476:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbp, %rdi
.LEHB12:
	call	_Unwind_Resume@PLT
.LEHE12:
	.cfi_endproc
.LFE15124:
	.section	.gcc_except_table
.LLSDA15124:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15124-.LLSDACSB15124
.LLSDACSB15124:
	.uleb128 .LEHB10-.LFB15124
	.uleb128 .LEHE10-.LEHB10
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB11-.LFB15124
	.uleb128 .LEHE11-.LEHB11
	.uleb128 .L477-.LFB15124
	.uleb128 0
	.uleb128 .LEHB12-.LFB15124
	.uleb128 .LEHE12-.LEHB12
	.uleb128 0
	.uleb128 0
.LLSDACSE15124:
	.section	.text._ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_,"axG",@progbits,_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_,comdat
	.size	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_, .-_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16519:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16519
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB13:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE13:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB14:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE14:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L492
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L536
	cmpq	$15, %rax
	ja	.L496
	cmpq	%r13, %r14
	je	.L497
	testq	%rax, %rax
	je	.L514
	cmpq	$1, %rax
	je	.L537
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L514:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L516
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L516:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L532
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L496:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L538
	movl	$30, %r15d
	movl	$31, %edi
.L502:
.LEHB15:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L504
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L504:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L505
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L505:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L514
	.p2align 4,,10
	.p2align 3
.L537:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L514
	.p2align 4,,10
	.p2align 3
.L492:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L539
	testq	%r12, %r12
	je	.L514
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L512
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L514
	.p2align 4,,10
	.p2align 3
.L539:
	testq	%r12, %r12
	js	.L540
	cmpq	$29, %r12
	jbe	.L524
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L541
.L510:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L512:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L535:
	movq	(%rbx), %r13
	jmp	.L514
	.p2align 4,,10
	.p2align 3
.L538:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L502
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L532
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L524:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L510
.L541:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L532
	call	_ZSt17__throw_bad_allocv@PLT
.L518:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L519:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L520
.L532:
	call	__stack_chk_fail@PLT
.L536:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L532
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L540:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L532
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L527:
	endbr64
	movq	%rax, %r12
	jmp	.L518
.L526:
	endbr64
	movq	%rax, %r12
	jmp	.L519
.L497:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE15:
	jmp	.L535
.L520:
	movq	%r12, %rdi
.LEHB16:
	call	_Unwind_Resume@PLT
.LEHE16:
	.cfi_endproc
.LFE16519:
	.section	.gcc_except_table
.LLSDA16519:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16519-.LLSDACSB16519
.LLSDACSB16519:
	.uleb128 .LEHB13-.LFB16519
	.uleb128 .LEHE13-.LEHB13
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB14-.LFB16519
	.uleb128 .LEHE14-.LEHB14
	.uleb128 .L526-.LFB16519
	.uleb128 0
	.uleb128 .LEHB15-.LFB16519
	.uleb128 .LEHE15-.LEHB15
	.uleb128 .L527-.LFB16519
	.uleb128 0
	.uleb128 .LEHB16-.LFB16519
	.uleb128 .LEHE16-.LEHB16
	.uleb128 0
	.uleb128 0
.LLSDACSE16519:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_:
.LFB16522:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16522
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L585
.L542:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L584
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L585:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L586
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB17:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE17:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB18:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE18:
	movq	144(%rsp), %r8
	leaq	80(%rsp), %rbp
	movb	$0, 80(%rsp)
	movq	%rbp, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%r8, %r8
	je	.L546
	movq	128(%rsp), %rax
	movq	%r8, %rdx
	leaq	64(%rsp), %r12
	movq	136(%rsp), %rcx
	movq	%r12, %rdi
	cmpq	%r8, %rax
	cmovnb	%rax, %r8
	testq	%rax, %rax
	cmove	%rdx, %r8
	xorl	%edx, %edx
	xorl	%esi, %esi
	subq	%rcx, %r8
.LEHB19:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0
.LEHE19:
.L548:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L559
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L559:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r12, %rsi
	movq	%rax, 40(%rsp)
.LEHB20:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE20:
	movq	64(%rsp), %rdi
	cmpq	%rbp, %rdi
	je	.L565
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L565:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L542
	.p2align 4,,10
	.p2align 3
.L546:
	movq	184(%rsp), %r13
	cmpq	$15, %r13
	ja	.L587
	testq	%r13, %r13
	je	.L558
	movq	176(%rsp), %rsi
	cmpq	$1, %r13
	jne	.L574
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L558:
	movq	%rbp, %rax
.L557:
	movq	%r13, 72(%rsp)
	leaq	64(%rsp), %r12
	movb	$0, (%rax,%r13)
	jmp	.L548
	.p2align 4,,10
	.p2align 3
.L587:
	testq	%r13, %r13
	js	.L588
	cmpq	$29, %r13
	ja	.L589
	movl	$30, %r14d
	movl	$31, %edi
.L552:
	leaq	64(%rsp), %r12
.LEHB21:
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%rbp, %rdi
	je	.L554
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L554:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r14, 80(%rsp)
.L555:
	movq	%r13, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L557
.L589:
	movq	%r13, %rdi
	movq	%r13, %r14
	addq	$1, %rdi
	jns	.L552
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L584
	leaq	64(%rsp), %r12
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE21:
	.p2align 4,,10
	.p2align 3
.L574:
	movq	%rbp, %rbx
	jmp	.L555
.L566:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L564:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L567:
	subq	$1, %rbp
	jne	.L590
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L570
.L584:
	call	__stack_chk_fail@PLT
.L586:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L584
	movl	%eax, %edi
.LEHB22:
	call	_ZSt20__throw_system_errori@PLT
.LEHE22:
.L588:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L584
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r12
.LEHB23:
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE23:
.L575:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L567
.L579:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r13
	jmp	.L561
.L576:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L564
.L577:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L566
.L578:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r13
	jmp	.L562
.L561:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L562:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r13, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	jmp	.L564
.L590:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L584
.LEHB24:
	call	_Unwind_Resume@PLT
.LEHE24:
.L570:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16522:
	.section	.gcc_except_table
	.align 4
.LLSDA16522:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16522-.LLSDATTD16522
.LLSDATTD16522:
	.byte	0x1
	.uleb128 .LLSDACSE16522-.LLSDACSB16522
.LLSDACSB16522:
	.uleb128 .LEHB17-.LFB16522
	.uleb128 .LEHE17-.LEHB17
	.uleb128 .L576-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB18-.LFB16522
	.uleb128 .LEHE18-.LEHB18
	.uleb128 .L578-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB19-.LFB16522
	.uleb128 .LEHE19-.LEHB19
	.uleb128 .L579-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB20-.LFB16522
	.uleb128 .LEHE20-.LEHB20
	.uleb128 .L577-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB21-.LFB16522
	.uleb128 .LEHE21-.LEHB21
	.uleb128 .L579-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB22-.LFB16522
	.uleb128 .LEHE22-.LEHB22
	.uleb128 .L575-.LFB16522
	.uleb128 0x1
	.uleb128 .LEHB23-.LFB16522
	.uleb128 .LEHE23-.LEHB23
	.uleb128 .L579-.LFB16522
	.uleb128 0x3
	.uleb128 .LEHB24-.LFB16522
	.uleb128 .LEHE24-.LEHB24
	.uleb128 0
	.uleb128 0
.LLSDACSE16522:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16522:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_
	.section	.rodata.str1.8
	.align 8
.LC7:
	.string	"failed to write post-state to disk"
	.section	.rodata.str1.1
.LC8:
	.string	" : "
.LC9:
	.string	"\033[0m"
.LC10:
	.string	"Commit"
.LC11:
	.string	"\033[1m"
.LC12:
	.string	"ERROR: "
.LC13:
	.string	"\033[31;1m"
	.section	.text.unlikely
.LCOLDB14:
	.text
.LHOTB14:
	.p2align 4
	.globl	_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh
	.type	_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh, @function
_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh:
.LFB15157:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15157
	endbr64
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	movzbl	%dl, %ecx
	movq	%rdi, %rdx
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	subq	$48, %rsp
	.cfi_def_cfa_offset 80
	movq	_ZN3LLD8RegisterE(%rip), %rdi
	movq	%fs:40, %rax
	movq	%rax, 40(%rsp)
	xorl	%eax, %eax
.LEHB25:
	call	_ZN3LLD10RegisterDB10WriteStateERK9base_uintILj256EERKN3TAO8Register5StateEh@PLT
	movl	%eax, %ebx
	testb	%al, %al
	je	.L604
.L591:
	movq	40(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L605
	addq	$48, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	movl	%ebx, %eax
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L604:
	.cfi_restore_state
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L593
	call	_ZTHN5debug9fLogErrorE@PLT
.L593:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	je	.L591
	movq	%rsp, %r12
	leaq	.LC9(%rip), %rbp
	leaq	.LC7(%rip), %r9
	leaq	.LC8(%rip), %r8
	movq	%rbp, %rcx
	movq	%r12, %rdi
	leaq	.LC10(%rip), %rdx
	leaq	.LC11(%rip), %rsi
	call	_ZN5debug13safe_printstrIJRA5_KcRA7_S1_S3_RA4_S1_RA35_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE25:
.LEHB26:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE26:
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %rdi
	movq	%r12, %rsi
	addq	%fs:0, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEaSEOS4_.isra.0
	movq	(%rsp), %rdi
	leaq	16(%rsp), %rax
	cmpq	%rax, %rdi
	je	.L595
	movq	16(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L595:
	subq	$8, %rsp
	.cfi_def_cfa_offset 88
	leaq	.LC7(%rip), %rax
	movq	%rbp, %rcx
	xorl	%edi, %edi
	pushq	%rax
	.cfi_def_cfa_offset 96
	leaq	.LC8(%rip), %rax
	leaq	.LC10(%rip), %r9
	pushq	%rax
	.cfi_def_cfa_offset 104
	leaq	.LC11(%rip), %r8
	leaq	.LC12(%rip), %rdx
	pushq	%rbp
	.cfi_def_cfa_offset 112
	leaq	.LC13(%rip), %rsi
.LEHB27:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA7_S1_S5_RA4_S1_RA35_S1_EEEvjDpOT_
.LEHE27:
	addq	$32, %rsp
	.cfi_def_cfa_offset 80
	jmp	.L591
.L605:
	call	__stack_chk_fail@PLT
.L599:
	endbr64
	movq	%rax, %rbx
	jmp	.L596
	.section	.gcc_except_table
.LLSDA15157:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15157-.LLSDACSB15157
.LLSDACSB15157:
	.uleb128 .LEHB25-.LFB15157
	.uleb128 .LEHE25-.LEHB25
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB26-.LFB15157
	.uleb128 .LEHE26-.LEHB26
	.uleb128 .L599-.LFB15157
	.uleb128 0
	.uleb128 .LEHB27-.LFB15157
	.uleb128 .LEHE27-.LEHB27
	.uleb128 0
	.uleb128 0
.LLSDACSE15157:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC15157
	.type	_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh.cold, @function
_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh.cold:
.LFSB15157:
.L596:
	.cfi_def_cfa_offset 80
	.cfi_offset 3, -32
	.cfi_offset 6, -24
	.cfi_offset 12, -16
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	40(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L606
	movq	%rbx, %rdi
.LEHB28:
	call	_Unwind_Resume@PLT
.LEHE28:
.L606:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE15157:
	.section	.gcc_except_table
.LLSDAC15157:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC15157-.LLSDACSBC15157
.LLSDACSBC15157:
	.uleb128 .LEHB28-.LCOLDB14
	.uleb128 .LEHE28-.LEHB28
	.uleb128 0
	.uleb128 0
.LLSDACSEC15157:
	.section	.text.unlikely
	.text
	.size	_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh, .-_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh
	.section	.text.unlikely
	.size	_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh.cold, .-_ZN3TAO9Operation5Write6CommitERKNS_8Register5StateERK9base_uintILj256EEh.cold
.LCOLDE14:
	.text
.LHOTE14:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16392:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16392
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	464(%rsp), %rax
	movq	%r9, (%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%rax, 8(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB29:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE29:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB30:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %rax
	movq	%rbp, %rdi
	movq	(%rax), %rsi
	call	_ZNSo9_M_insertImEERSoT_@PLT
.LEHE30:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L608
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L652
	cmpq	$15, %rax
	ja	.L612
	cmpq	%r13, %r14
	je	.L613
	testq	%rax, %rax
	je	.L630
	cmpq	$1, %rax
	je	.L653
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L630:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L632
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L632:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L648
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L612:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L654
	movl	$30, %r15d
	movl	$31, %edi
.L618:
.LEHB31:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L620
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L620:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L621
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L621:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L630
	.p2align 4,,10
	.p2align 3
.L653:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L630
	.p2align 4,,10
	.p2align 3
.L608:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L655
	testq	%r12, %r12
	je	.L630
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L628
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L630
	.p2align 4,,10
	.p2align 3
.L655:
	testq	%r12, %r12
	js	.L656
	cmpq	$29, %r12
	jbe	.L640
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L657
.L626:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L628:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L651:
	movq	(%rbx), %r13
	jmp	.L630
	.p2align 4,,10
	.p2align 3
.L654:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L618
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L648
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L640:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L626
.L657:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L648
	call	_ZSt17__throw_bad_allocv@PLT
.L634:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L635:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L636
.L648:
	call	__stack_chk_fail@PLT
.L652:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L648
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L656:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L648
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L643:
	endbr64
	movq	%rax, %r12
	jmp	.L634
.L642:
	endbr64
	movq	%rax, %r12
	jmp	.L635
.L613:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE31:
	jmp	.L651
.L636:
	movq	%r12, %rdi
.LEHB32:
	call	_Unwind_Resume@PLT
.LEHE32:
	.cfi_endproc
.LFE16392:
	.section	.gcc_except_table
.LLSDA16392:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16392-.LLSDACSB16392
.LLSDACSB16392:
	.uleb128 .LEHB29-.LFB16392
	.uleb128 .LEHE29-.LEHB29
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB30-.LFB16392
	.uleb128 .LEHE30-.LEHB30
	.uleb128 .L642-.LFB16392
	.uleb128 0
	.uleb128 .LEHB31-.LFB16392
	.uleb128 .LEHE31-.LEHB31
	.uleb128 .L643-.LFB16392
	.uleb128 0
	.uleb128 .LEHB32-.LFB16392
	.uleb128 .LEHE32-.LEHB32
	.uleb128 0
	.uleb128 0
.LLSDACSE16392:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.rodata.str1.1
.LC15:
	.string	"reached end of stream "
.LC16:
	.string	"read"
	.section	.text.unlikely
	.align 2
.LCOLDB17:
	.text
.LHOTB17:
	.align 2
	.p2align 4
	.type	_ZNK10BaseStream4readEPcm.isra.0, @function
_ZNK10BaseStream4readEPcm.isra.0:
.LFB18920:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA18920
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdx, %r10
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	.cfi_offset 14, -24
	.cfi_offset 12, -32
	.cfi_offset 3, -40
	movq	%rdi, %rbx
	movq	%rsi, %rdi
	subq	$56, %rsp
	movq	8(%rbx), %rcx
	movq	16(%rbx), %rsi
	movq	%fs:40, %rax
	movq	%rax, -40(%rbp)
	xorl	%eax, %eax
	leaq	(%rcx,%rdx), %rax
	movq	24(%rbx), %rdx
	subq	%rsi, %rdx
	cmpq	%rax, %rdx
	jb	.L669
	addq	%rcx, %rsi
	movq	%r10, %r12
	cmpq	$1, %r10
	jle	.L663
	movq	%r10, %rdx
	call	memmove@PLT
	movq	8(%rbx), %rax
	addq	%r12, %rax
.L664:
	movq	%rax, 8(%rbx)
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L672
	leaq	-24(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
	.p2align 4,,10
	.p2align 3
.L663:
	.cfi_restore_state
	jne	.L664
	movzbl	(%rsi), %eax
	movb	%al, (%rdi)
	movq	8(%rbx), %rax
	addq	$1, %rax
	jmp	.L664
.L672:
	call	__stack_chk_fail@PLT
	.section	.gcc_except_table
.LLSDA18920:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE18920-.LLSDACSB18920
.LLSDACSB18920:
.LLSDACSE18920:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC18920
	.type	_ZNK10BaseStream4readEPcm.isra.0.cold, @function
_ZNK10BaseStream4readEPcm.isra.0.cold:
.LFSB18920:
.L669:
	.cfi_def_cfa 6, 16
	.cfi_offset 3, -40
	.cfi_offset 6, -16
	.cfi_offset 12, -32
	.cfi_offset 14, -24
	movl	$16, %edi
	addq	$8, %rbx
	leaq	-80(%rbp), %r12
	call	__cxa_allocate_exception@PLT
	leaq	.LC15(%rip), %r9
	movq	%r12, %rdi
	leaq	.LC8(%rip), %r8
	pushq	%rcx
	leaq	.LC16(%rip), %rdx
	leaq	.LC9(%rip), %rcx
	movq	%rax, %r14
	pushq	%rbx
	leaq	.LC11(%rip), %rsi
.LEHB33:
	.cfi_escape 0x2e,0x10
	call	_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE33:
	popq	%rax
	movq	%r12, %rsi
	popq	%rdx
	movq	%r14, %rdi
.LEHB34:
	.cfi_escape 0x2e,0
	call	_ZNSt13runtime_errorC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE34:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	leaq	16+_ZTVN5debug9exceptionE(%rip), %rdi
	movq	%rdi, (%r14)
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L673
	leaq	_ZN5debug9exceptionD1Ev(%rip), %rdx
	leaq	_ZTIN5debug9exceptionE(%rip), %rsi
	movq	%r14, %rdi
.LEHB35:
	call	__cxa_throw@PLT
.L667:
	endbr64
	movq	%rax, %rbx
.L662:
	movq	%r14, %rdi
	call	__cxa_free_exception@PLT
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L674
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE35:
.L673:
	call	__stack_chk_fail@PLT
.L668:
	endbr64
	movq	%r12, %rdi
	movq	%rax, %rbx
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L662
.L674:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE18920:
	.section	.gcc_except_table
.LLSDAC18920:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC18920-.LLSDACSBC18920
.LLSDACSBC18920:
	.uleb128 .LEHB33-.LCOLDB17
	.uleb128 .LEHE33-.LEHB33
	.uleb128 .L667-.LCOLDB17
	.uleb128 0
	.uleb128 .LEHB34-.LCOLDB17
	.uleb128 .LEHE34-.LEHB34
	.uleb128 .L668-.LCOLDB17
	.uleb128 0
	.uleb128 .LEHB35-.LCOLDB17
	.uleb128 .LEHE35-.LEHB35
	.uleb128 0
	.uleb128 0
.LLSDACSEC18920:
	.section	.text.unlikely
	.text
	.size	_ZNK10BaseStream4readEPcm.isra.0, .-_ZNK10BaseStream4readEPcm.isra.0
	.section	.text.unlikely
	.size	_ZNK10BaseStream4readEPcm.isra.0.cold, .-_ZNK10BaseStream4readEPcm.isra.0.cold
.LCOLDE17:
	.text
.LHOTE17:
	.section	.rodata.str1.8
	.align 8
.LC18:
	.string	"ReadCompactSize() : size too large"
	.section	.rodata.str1.1
.LC19:
	.string	"basic_string::_M_replace_aux"
	.section	.text.unlikely
.LCOLDB20:
	.text
.LHOTB20:
	.p2align 4
	.type	_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0, @function
_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0:
.LFB18922:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA18922
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movl	$1, %edx
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rdi, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rsi, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	leaq	39(%rsp), %rsi
.LEHB36:
	call	_ZNK10BaseStream4readEPcm.isra.0
	movzbl	39(%rsp), %ebx
	cmpb	$-4, %bl
	jbe	.L677
	leaq	40(%rsp), %rsi
	cmpb	$-3, %bl
	je	.L705
	cmpb	$-2, %bl
	je	.L706
	movl	$8, %edx
	movq	%r13, %rdi
	call	_ZNK10BaseStream4readEPcm.isra.0
	movq	40(%rsp), %rbx
.L680:
	cmpq	$33554432, %rbx
	ja	.L702
.L677:
	movq	8(%r12), %rbp
	cmpq	%rbx, %rbp
	jb	.L707
	cmpq	%rbp, %rbx
	jb	.L708
.L698:
	testq	%rbx, %rbx
	jne	.L697
	addq	$56, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L708:
	.cfi_restore_state
	movq	(%r12), %rax
	movq	%rbx, 8(%r12)
	movb	$0, (%rax,%rbx)
	jmp	.L698
	.p2align 4,,10
	.p2align 3
.L707:
	movabsq	$9223372036854775807, %rax
	movq	%rbx, %rcx
	subq	%rbp, %rcx
	subq	%rbp, %rax
	cmpq	%rcx, %rax
	jb	.L709
	movq	(%r12), %r14
	leaq	16(%r12), %r9
	cmpq	%r9, %r14
	je	.L710
	movq	16(%r12), %r15
	cmpq	%rbx, %r15
	jb	.L711
.L686:
	leaq	(%r14,%rbp), %rdi
	cmpq	$1, %rcx
	je	.L712
	movq	%rcx, %rdx
	xorl	%esi, %esi
	call	memset@PLT
.L696:
	movq	(%r12), %rax
	movq	%rbx, 8(%r12)
	movb	$0, (%rax,%rbx)
.L697:
	movq	(%r12), %rsi
	addq	$56, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rdx
	movq	%r13, %rdi
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	_ZNK10BaseStream4readEPcm.isra.0
	.p2align 4,,10
	.p2align 3
.L705:
	.cfi_restore_state
	movl	$2, %edx
	movq	%r13, %rdi
	call	_ZNK10BaseStream4readEPcm.isra.0
	movzwl	40(%rsp), %ebx
	jmp	.L677
	.p2align 4,,10
	.p2align 3
.L711:
	addq	%r15, %r15
.L687:
	leaq	1(%r15), %rdi
	cmpq	%r15, %rbx
	jnb	.L713
.L689:
	movq	%r9, 16(%rsp)
	movq	%rcx, 8(%rsp)
	call	_Znwm@PLT
	testq	%rbp, %rbp
	movq	(%r12), %r8
	movq	8(%rsp), %rcx
	movq	16(%rsp), %r9
	movq	%rax, %r14
	jne	.L690
.L691:
	cmpq	%r8, %r9
	je	.L694
	movq	16(%r12), %rax
	movq	%r8, %rdi
	movq	%rcx, 8(%rsp)
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	movq	8(%rsp), %rcx
.L694:
	movq	%r14, (%r12)
	movq	%r15, 16(%r12)
	jmp	.L686
	.p2align 4,,10
	.p2align 3
.L706:
	movl	$4, %edx
	movq	%r13, %rdi
	call	_ZNK10BaseStream4readEPcm.isra.0
	movl	40(%rsp), %ebx
	jmp	.L680
	.p2align 4,,10
	.p2align 3
.L710:
	movl	$30, %r15d
	cmpq	$15, %rbx
	jbe	.L686
	jmp	.L687
	.p2align 4,,10
	.p2align 3
.L712:
	movb	$0, (%rdi)
	jmp	.L696
	.p2align 4,,10
	.p2align 3
.L690:
	cmpq	$1, %rbp
	je	.L714
	movq	%r8, %rsi
	movq	%rbp, %rdx
	movq	%rax, %rdi
	movq	%r9, 24(%rsp)
	movq	%rcx, 16(%rsp)
	movq	%r8, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %r9
	movq	16(%rsp), %rcx
	movq	8(%rsp), %r8
	jmp	.L691
	.p2align 4,,10
	.p2align 3
.L713:
	leaq	1(%rbx), %rdi
	movq	%rbx, %r15
	jmp	.L689
	.p2align 4,,10
	.p2align 3
.L714:
	movzbl	(%r8), %eax
	movb	%al, (%r14)
	jmp	.L691
.L709:
	leaq	.LC19(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE36:
	.section	.gcc_except_table
.LLSDA18922:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE18922-.LLSDACSB18922
.LLSDACSB18922:
	.uleb128 .LEHB36-.LFB18922
	.uleb128 .LEHE36-.LEHB36
	.uleb128 0
	.uleb128 0
.LLSDACSE18922:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC18922
	.type	_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0.cold, @function
_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0.cold:
.LFSB18922:
.L702:
	.cfi_def_cfa_offset 112
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	movl	$16, %edi
	call	__cxa_allocate_exception@PLT
	leaq	.LC18(%rip), %rsi
	movq	%rax, %rdi
	movq	%rax, %rbx
.LEHB37:
	call	_ZNSt13runtime_errorC1EPKc@PLT
.LEHE37:
	movq	_ZNSt13runtime_errorD1Ev@GOTPCREL(%rip), %rdx
	leaq	_ZTISt13runtime_error(%rip), %rsi
	movq	%rbx, %rdi
.LEHB38:
	call	__cxa_throw@PLT
.L701:
	endbr64
	movq	%rax, %rbp
	movq	%rbx, %rdi
	call	__cxa_free_exception@PLT
	movq	%rbp, %rdi
	call	_Unwind_Resume@PLT
.LEHE38:
	.cfi_endproc
.LFE18922:
	.section	.gcc_except_table
.LLSDAC18922:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC18922-.LLSDACSBC18922
.LLSDACSBC18922:
	.uleb128 .LEHB37-.LCOLDB20
	.uleb128 .LEHE37-.LEHB37
	.uleb128 .L701-.LCOLDB20
	.uleb128 0
	.uleb128 .LEHB38-.LCOLDB20
	.uleb128 .LEHE38-.LEHB38
	.uleb128 0
	.uleb128 0
.LLSDACSEC18922:
	.section	.text.unlikely
	.text
	.size	_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0, .-_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0
	.section	.text.unlikely
	.size	_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0.cold, .-_Z11UnserializeIKN3TAO9Operation6StreamEcEvRT_RNSt7__cxx1112basic_stringIT0_St11char_traitsIS8_ESaIS8_EEEjj.isra.0.cold
.LCOLDE20:
	.text
.LHOTE20:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16568:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16568
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB39:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE39:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB40:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE40:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L716
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L760
	cmpq	$15, %rax
	ja	.L720
	cmpq	%r13, %r14
	je	.L721
	testq	%rax, %rax
	je	.L738
	cmpq	$1, %rax
	je	.L761
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L738:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L740
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L740:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L756
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L720:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L762
	movl	$30, %r15d
	movl	$31, %edi
.L726:
.LEHB41:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L728
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L728:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L729
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L729:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L738
	.p2align 4,,10
	.p2align 3
.L761:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L738
	.p2align 4,,10
	.p2align 3
.L716:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L763
	testq	%r12, %r12
	je	.L738
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L736
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L738
	.p2align 4,,10
	.p2align 3
.L763:
	testq	%r12, %r12
	js	.L764
	cmpq	$29, %r12
	jbe	.L748
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L765
.L734:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L736:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L759:
	movq	(%rbx), %r13
	jmp	.L738
	.p2align 4,,10
	.p2align 3
.L762:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L726
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L756
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L748:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L734
.L765:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L756
	call	_ZSt17__throw_bad_allocv@PLT
.L742:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L743:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L744
.L756:
	call	__stack_chk_fail@PLT
.L760:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L756
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L764:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L756
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L751:
	endbr64
	movq	%rax, %r12
	jmp	.L742
.L750:
	endbr64
	movq	%rax, %r12
	jmp	.L743
.L721:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE41:
	jmp	.L759
.L744:
	movq	%r12, %rdi
.LEHB42:
	call	_Unwind_Resume@PLT
.LEHE42:
	.cfi_endproc
.LFE16568:
	.section	.gcc_except_table
.LLSDA16568:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16568-.LLSDACSB16568
.LLSDACSB16568:
	.uleb128 .LEHB39-.LFB16568
	.uleb128 .LEHE39-.LEHB39
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB40-.LFB16568
	.uleb128 .LEHE40-.LEHB40
	.uleb128 .L750-.LFB16568
	.uleb128 0
	.uleb128 .LEHB41-.LFB16568
	.uleb128 .LEHE41-.LEHB41
	.uleb128 .L751-.LFB16568
	.uleb128 0
	.uleb128 .LEHB42-.LFB16568
	.uleb128 .LEHE42-.LEHB42
	.uleb128 0
	.uleb128 0
.LLSDACSE16568:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_:
.LFB16569:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16569
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L826
.L766:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L826:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L827
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB43:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE43:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB44:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE44:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L770
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L828
	cmpq	$15, %rbp
	ja	.L774
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L775
	testq	%rbp, %rbp
	jne	.L776
.L779:
	movq	%r12, %rbx
.L777:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L784:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L795
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L795:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB45:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE45:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L801
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L801:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L766
	.p2align 4,,10
	.p2align 3
.L776:
	cmpq	$1, %rbp
	je	.L829
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L779
	.p2align 4,,10
	.p2align 3
.L770:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L830
	testq	%rbp, %rbp
	je	.L794
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L812
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L794:
	movq	%r12, %rax
.L793:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L784
	.p2align 4,,10
	.p2align 3
.L774:
	cmpq	$29, %rbp
	jbe	.L808
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L831
.L780:
	leaq	64(%rsp), %r14
.LEHB46:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L782
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L782:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L783
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L783:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L777
	.p2align 4,,10
	.p2align 3
.L808:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L780
	.p2align 4,,10
	.p2align 3
.L829:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L779
	.p2align 4,,10
	.p2align 3
.L830:
	testq	%rbp, %rbp
	js	.L832
	cmpq	$29, %rbp
	ja	.L833
	movl	$30, %r13d
	movl	$31, %edi
.L788:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L790
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L790:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L791:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L793
.L833:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L788
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L831:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE46:
	.p2align 4,,10
	.p2align 3
.L812:
	movq	%r12, %rbx
	jmp	.L791
.L797:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L798:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L800:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L803:
	subq	$1, %rbp
	jne	.L834
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L806
.L825:
	call	__stack_chk_fail@PLT
.L827:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L825
	movl	%eax, %edi
.LEHB47:
	call	_ZSt20__throw_system_errori@PLT
.LEHE47:
.L828:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB48:
	call	_ZSt20__throw_length_errorPKc@PLT
.L832:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L816:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L798
.L775:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE48:
	movq	64(%rsp), %rbx
	jmp	.L777
.L813:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L803
.L815:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L802
.L817:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L797
.L814:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L800
.L802:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L800
.L834:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L825
.LEHB49:
	call	_Unwind_Resume@PLT
.LEHE49:
.L806:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16569:
	.section	.gcc_except_table
	.align 4
.LLSDA16569:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16569-.LLSDATTD16569
.LLSDATTD16569:
	.byte	0x1
	.uleb128 .LLSDACSE16569-.LLSDACSB16569
.LLSDACSB16569:
	.uleb128 .LEHB43-.LFB16569
	.uleb128 .LEHE43-.LEHB43
	.uleb128 .L814-.LFB16569
	.uleb128 0x3
	.uleb128 .LEHB44-.LFB16569
	.uleb128 .LEHE44-.LEHB44
	.uleb128 .L816-.LFB16569
	.uleb128 0x3
	.uleb128 .LEHB45-.LFB16569
	.uleb128 .LEHE45-.LEHB45
	.uleb128 .L815-.LFB16569
	.uleb128 0x3
	.uleb128 .LEHB46-.LFB16569
	.uleb128 .LEHE46-.LEHB46
	.uleb128 .L817-.LFB16569
	.uleb128 0x3
	.uleb128 .LEHB47-.LFB16569
	.uleb128 .LEHE47-.LEHB47
	.uleb128 .L813-.LFB16569
	.uleb128 0x1
	.uleb128 .LEHB48-.LFB16569
	.uleb128 .LEHE48-.LEHB48
	.uleb128 .L817-.LFB16569
	.uleb128 0x3
	.uleb128 .LEHB49-.LFB16569
	.uleb128 .LEHE49-.LEHB49
	.uleb128 0
	.uleb128 0
.LLSDACSE16569:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16569:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_:
.LFB16006:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16006
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L836
.LEHB50:
	call	_ZTHN5debug9fLogErrorE@PLT
.L836:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L860
.L837:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L858
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L860:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE50:
.LEHB51:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE51:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L861
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L862
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L845
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L844:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L846
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L846:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB52:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA30_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L837
	.p2align 4,,10
	.p2align 3
.L862:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L845:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L844
	.p2align 4,,10
	.p2align 3
.L861:
	testq	%rdx, %rdx
	je	.L840
	cmpq	$1, %rdx
	je	.L863
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L840:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L844
.L863:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L840
.L847:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L848
.L858:
	call	__stack_chk_fail@PLT
.L850:
	endbr64
	movq	%rax, %rbx
	jmp	.L847
.L848:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE52:
	.cfi_endproc
.LFE16006:
	.section	.gcc_except_table
.LLSDA16006:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16006-.LLSDACSB16006
.LLSDACSB16006:
	.uleb128 .LEHB50-.LFB16006
	.uleb128 .LEHE50-.LEHB50
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB51-.LFB16006
	.uleb128 .LEHE51-.LEHB51
	.uleb128 .L850-.LFB16006
	.uleb128 0
	.uleb128 .LEHB52-.LFB16006
	.uleb128 .LEHE52-.LEHB52
	.uleb128 0
	.uleb128 0
.LLSDACSE16006:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA30_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16523:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16523
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB53:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE53:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB54:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE54:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L865
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L909
	cmpq	$15, %rax
	ja	.L869
	cmpq	%r13, %r14
	je	.L870
	testq	%rax, %rax
	je	.L887
	cmpq	$1, %rax
	je	.L910
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L887:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L889
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L889:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L905
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L869:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L911
	movl	$30, %r15d
	movl	$31, %edi
.L875:
.LEHB55:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L877
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L877:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L878
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L878:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L887
	.p2align 4,,10
	.p2align 3
.L910:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L887
	.p2align 4,,10
	.p2align 3
.L865:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L912
	testq	%r12, %r12
	je	.L887
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L885
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L887
	.p2align 4,,10
	.p2align 3
.L912:
	testq	%r12, %r12
	js	.L913
	cmpq	$29, %r12
	jbe	.L897
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L914
.L883:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L885:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L908:
	movq	(%rbx), %r13
	jmp	.L887
	.p2align 4,,10
	.p2align 3
.L911:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L875
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L905
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L897:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L883
.L914:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L905
	call	_ZSt17__throw_bad_allocv@PLT
.L891:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L892:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L893
.L905:
	call	__stack_chk_fail@PLT
.L909:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L905
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L913:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L905
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L900:
	endbr64
	movq	%rax, %r12
	jmp	.L891
.L899:
	endbr64
	movq	%rax, %r12
	jmp	.L892
.L870:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE55:
	jmp	.L908
.L893:
	movq	%r12, %rdi
.LEHB56:
	call	_Unwind_Resume@PLT
.LEHE56:
	.cfi_endproc
.LFE16523:
	.section	.gcc_except_table
.LLSDA16523:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16523-.LLSDACSB16523
.LLSDACSB16523:
	.uleb128 .LEHB53-.LFB16523
	.uleb128 .LEHE53-.LEHB53
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB54-.LFB16523
	.uleb128 .LEHE54-.LEHB54
	.uleb128 .L899-.LFB16523
	.uleb128 0
	.uleb128 .LEHB55-.LFB16523
	.uleb128 .LEHE55-.LEHB55
	.uleb128 .L900-.LFB16523
	.uleb128 0
	.uleb128 .LEHB56-.LFB16523
	.uleb128 .LEHE56-.LEHB56
	.uleb128 0
	.uleb128 0
.LLSDACSE16523:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_:
.LFB16524:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16524
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L975
.L915:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L975:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L976
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB57:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE57:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB58:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE58:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L919
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L977
	cmpq	$15, %rbp
	ja	.L923
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L924
	testq	%rbp, %rbp
	jne	.L925
.L928:
	movq	%r12, %rbx
.L926:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L933:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L944
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L944:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB59:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE59:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L950
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L950:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L915
	.p2align 4,,10
	.p2align 3
.L925:
	cmpq	$1, %rbp
	je	.L978
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L928
	.p2align 4,,10
	.p2align 3
.L919:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L979
	testq	%rbp, %rbp
	je	.L943
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L961
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L943:
	movq	%r12, %rax
.L942:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L933
	.p2align 4,,10
	.p2align 3
.L923:
	cmpq	$29, %rbp
	jbe	.L957
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L980
.L929:
	leaq	64(%rsp), %r14
.LEHB60:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L931
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L931:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L932
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L932:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L926
	.p2align 4,,10
	.p2align 3
.L957:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L929
	.p2align 4,,10
	.p2align 3
.L978:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L928
	.p2align 4,,10
	.p2align 3
.L979:
	testq	%rbp, %rbp
	js	.L981
	cmpq	$29, %rbp
	ja	.L982
	movl	$30, %r13d
	movl	$31, %edi
.L937:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L939
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L939:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L940:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L942
.L982:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L937
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L980:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE60:
	.p2align 4,,10
	.p2align 3
.L961:
	movq	%r12, %rbx
	jmp	.L940
.L946:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L947:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L949:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L952:
	subq	$1, %rbp
	jne	.L983
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L955
.L974:
	call	__stack_chk_fail@PLT
.L976:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L974
	movl	%eax, %edi
.LEHB61:
	call	_ZSt20__throw_system_errori@PLT
.LEHE61:
.L977:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB62:
	call	_ZSt20__throw_length_errorPKc@PLT
.L981:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L965:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L947
.L924:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE62:
	movq	64(%rsp), %rbx
	jmp	.L926
.L962:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L952
.L964:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L951
.L966:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L946
.L963:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L949
.L951:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L949
.L983:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L974
.LEHB63:
	call	_Unwind_Resume@PLT
.LEHE63:
.L955:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16524:
	.section	.gcc_except_table
	.align 4
.LLSDA16524:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16524-.LLSDATTD16524
.LLSDATTD16524:
	.byte	0x1
	.uleb128 .LLSDACSE16524-.LLSDACSB16524
.LLSDACSB16524:
	.uleb128 .LEHB57-.LFB16524
	.uleb128 .LEHE57-.LEHB57
	.uleb128 .L963-.LFB16524
	.uleb128 0x3
	.uleb128 .LEHB58-.LFB16524
	.uleb128 .LEHE58-.LEHB58
	.uleb128 .L965-.LFB16524
	.uleb128 0x3
	.uleb128 .LEHB59-.LFB16524
	.uleb128 .LEHE59-.LEHB59
	.uleb128 .L964-.LFB16524
	.uleb128 0x3
	.uleb128 .LEHB60-.LFB16524
	.uleb128 .LEHE60-.LEHB60
	.uleb128 .L966-.LFB16524
	.uleb128 0x3
	.uleb128 .LEHB61-.LFB16524
	.uleb128 .LEHE61-.LEHB61
	.uleb128 .L962-.LFB16524
	.uleb128 0x1
	.uleb128 .LEHB62-.LFB16524
	.uleb128 .LEHE62-.LEHB62
	.uleb128 .L966-.LFB16524
	.uleb128 0x3
	.uleb128 .LEHB63-.LFB16524
	.uleb128 .LEHE63-.LEHB63
	.uleb128 0
	.uleb128 0
.LLSDACSE16524:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16524:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_:
.LFB15981:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15981
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L985
.LEHB64:
	call	_ZTHN5debug9fLogErrorE@PLT
.L985:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1009
.L986:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1007
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1009:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE64:
.LEHB65:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE65:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1010
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1011
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L994
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L993:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L995
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L995:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB66:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA32_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L986
	.p2align 4,,10
	.p2align 3
.L1011:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L994:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L993
	.p2align 4,,10
	.p2align 3
.L1010:
	testq	%rdx, %rdx
	je	.L989
	cmpq	$1, %rdx
	je	.L1012
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L989:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L993
.L1012:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L989
.L996:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L997
.L1007:
	call	__stack_chk_fail@PLT
.L999:
	endbr64
	movq	%rax, %rbx
	jmp	.L996
.L997:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE66:
	.cfi_endproc
.LFE15981:
	.section	.gcc_except_table
.LLSDA15981:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15981-.LLSDACSB15981
.LLSDACSB15981:
	.uleb128 .LEHB64-.LFB15981
	.uleb128 .LEHE64-.LEHB64
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB65-.LFB15981
	.uleb128 .LEHE65-.LEHB65
	.uleb128 .L999-.LFB15981
	.uleb128 0
	.uleb128 .LEHB66-.LFB15981
	.uleb128 .LEHE66-.LEHB66
	.uleb128 0
	.uleb128 0
.LLSDACSE15981:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA32_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16576:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16576
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB67:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE67:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB68:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE68:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1014
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1058
	cmpq	$15, %rax
	ja	.L1018
	cmpq	%r13, %r14
	je	.L1019
	testq	%rax, %rax
	je	.L1036
	cmpq	$1, %rax
	je	.L1059
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1036:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1038
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1038:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1054
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1018:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1060
	movl	$30, %r15d
	movl	$31, %edi
.L1024:
.LEHB69:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1026
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1026:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1027
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1027:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1036
	.p2align 4,,10
	.p2align 3
.L1059:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1036
	.p2align 4,,10
	.p2align 3
.L1014:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1061
	testq	%r12, %r12
	je	.L1036
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1034
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1036
	.p2align 4,,10
	.p2align 3
.L1061:
	testq	%r12, %r12
	js	.L1062
	cmpq	$29, %r12
	jbe	.L1046
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1063
.L1032:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1034:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1057:
	movq	(%rbx), %r13
	jmp	.L1036
	.p2align 4,,10
	.p2align 3
.L1060:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1024
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1054
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1046:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1032
.L1063:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1054
	call	_ZSt17__throw_bad_allocv@PLT
.L1040:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1041:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1042
.L1054:
	call	__stack_chk_fail@PLT
.L1058:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1054
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1062:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1054
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1049:
	endbr64
	movq	%rax, %r12
	jmp	.L1040
.L1048:
	endbr64
	movq	%rax, %r12
	jmp	.L1041
.L1019:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE69:
	jmp	.L1057
.L1042:
	movq	%r12, %rdi
.LEHB70:
	call	_Unwind_Resume@PLT
.LEHE70:
	.cfi_endproc
.LFE16576:
	.section	.gcc_except_table
.LLSDA16576:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16576-.LLSDACSB16576
.LLSDACSB16576:
	.uleb128 .LEHB67-.LFB16576
	.uleb128 .LEHE67-.LEHB67
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB68-.LFB16576
	.uleb128 .LEHE68-.LEHB68
	.uleb128 .L1048-.LFB16576
	.uleb128 0
	.uleb128 .LEHB69-.LFB16576
	.uleb128 .LEHE69-.LEHB69
	.uleb128 .L1049-.LFB16576
	.uleb128 0
	.uleb128 .LEHB70-.LFB16576
	.uleb128 .LEHE70-.LEHB70
	.uleb128 0
	.uleb128 0
.LLSDACSE16576:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_:
.LFB16577:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16577
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1124
.L1064:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1124:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1125
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB71:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE71:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB72:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE72:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L1068
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L1126
	cmpq	$15, %rbp
	ja	.L1072
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L1073
	testq	%rbp, %rbp
	jne	.L1074
.L1077:
	movq	%r12, %rbx
.L1075:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L1082:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1093
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1093:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB73:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE73:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1099
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1099:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1064
	.p2align 4,,10
	.p2align 3
.L1074:
	cmpq	$1, %rbp
	je	.L1127
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L1077
	.p2align 4,,10
	.p2align 3
.L1068:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L1128
	testq	%rbp, %rbp
	je	.L1092
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L1110
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1092:
	movq	%r12, %rax
.L1091:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L1082
	.p2align 4,,10
	.p2align 3
.L1072:
	cmpq	$29, %rbp
	jbe	.L1106
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L1129
.L1078:
	leaq	64(%rsp), %r14
.LEHB74:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L1080
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1080:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1081
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1081:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L1075
	.p2align 4,,10
	.p2align 3
.L1106:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L1078
	.p2align 4,,10
	.p2align 3
.L1127:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L1077
	.p2align 4,,10
	.p2align 3
.L1128:
	testq	%rbp, %rbp
	js	.L1130
	cmpq	$29, %rbp
	ja	.L1131
	movl	$30, %r13d
	movl	$31, %edi
.L1086:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L1088
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1088:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L1089:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1091
.L1131:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L1086
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1129:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE74:
	.p2align 4,,10
	.p2align 3
.L1110:
	movq	%r12, %rbx
	jmp	.L1089
.L1095:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1096:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L1098:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1101:
	subq	$1, %rbp
	jne	.L1132
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1104
.L1123:
	call	__stack_chk_fail@PLT
.L1125:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1123
	movl	%eax, %edi
.LEHB75:
	call	_ZSt20__throw_system_errori@PLT
.LEHE75:
.L1126:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB76:
	call	_ZSt20__throw_length_errorPKc@PLT
.L1130:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L1114:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1096
.L1073:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE76:
	movq	64(%rsp), %rbx
	jmp	.L1075
.L1111:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1101
.L1113:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1100
.L1115:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1095
.L1112:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1098
.L1100:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L1098
.L1132:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1123
.LEHB77:
	call	_Unwind_Resume@PLT
.LEHE77:
.L1104:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16577:
	.section	.gcc_except_table
	.align 4
.LLSDA16577:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16577-.LLSDATTD16577
.LLSDATTD16577:
	.byte	0x1
	.uleb128 .LLSDACSE16577-.LLSDACSB16577
.LLSDACSB16577:
	.uleb128 .LEHB71-.LFB16577
	.uleb128 .LEHE71-.LEHB71
	.uleb128 .L1112-.LFB16577
	.uleb128 0x3
	.uleb128 .LEHB72-.LFB16577
	.uleb128 .LEHE72-.LEHB72
	.uleb128 .L1114-.LFB16577
	.uleb128 0x3
	.uleb128 .LEHB73-.LFB16577
	.uleb128 .LEHE73-.LEHB73
	.uleb128 .L1113-.LFB16577
	.uleb128 0x3
	.uleb128 .LEHB74-.LFB16577
	.uleb128 .LEHE74-.LEHB74
	.uleb128 .L1115-.LFB16577
	.uleb128 0x3
	.uleb128 .LEHB75-.LFB16577
	.uleb128 .LEHE75-.LEHB75
	.uleb128 .L1111-.LFB16577
	.uleb128 0x1
	.uleb128 .LEHB76-.LFB16577
	.uleb128 .LEHE76-.LEHB76
	.uleb128 .L1115-.LFB16577
	.uleb128 0x3
	.uleb128 .LEHB77-.LFB16577
	.uleb128 .LEHE77-.LEHB77
	.uleb128 0
	.uleb128 0
.LLSDACSE16577:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16577:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_:
.LFB16019:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16019
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1134
.LEHB78:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1134:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1158
.L1135:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1156
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1158:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE78:
.LEHB79:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE79:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1159
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1160
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1143
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1142:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1144
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1144:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB80:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA33_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1135
	.p2align 4,,10
	.p2align 3
.L1160:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1143:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1142
	.p2align 4,,10
	.p2align 3
.L1159:
	testq	%rdx, %rdx
	je	.L1138
	cmpq	$1, %rdx
	je	.L1161
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1138:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1142
.L1161:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1138
.L1145:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1146
.L1156:
	call	__stack_chk_fail@PLT
.L1148:
	endbr64
	movq	%rax, %rbx
	jmp	.L1145
.L1146:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE80:
	.cfi_endproc
.LFE16019:
	.section	.gcc_except_table
.LLSDA16019:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16019-.LLSDACSB16019
.LLSDACSB16019:
	.uleb128 .LEHB78-.LFB16019
	.uleb128 .LEHE78-.LEHB78
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB79-.LFB16019
	.uleb128 .LEHE79-.LEHB79
	.uleb128 .L1148-.LFB16019
	.uleb128 0
	.uleb128 .LEHB80-.LFB16019
	.uleb128 .LEHE80-.LEHB80
	.uleb128 0
	.uleb128 0
.LLSDACSE16019:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA33_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16578:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16578
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB81:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE81:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB82:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE82:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1163
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1207
	cmpq	$15, %rax
	ja	.L1167
	cmpq	%r13, %r14
	je	.L1168
	testq	%rax, %rax
	je	.L1185
	cmpq	$1, %rax
	je	.L1208
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1185:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1187
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1187:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1203
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1167:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1209
	movl	$30, %r15d
	movl	$31, %edi
.L1173:
.LEHB83:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1175
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1175:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1176
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1176:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1185
	.p2align 4,,10
	.p2align 3
.L1208:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1185
	.p2align 4,,10
	.p2align 3
.L1163:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1210
	testq	%r12, %r12
	je	.L1185
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1183
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1185
	.p2align 4,,10
	.p2align 3
.L1210:
	testq	%r12, %r12
	js	.L1211
	cmpq	$29, %r12
	jbe	.L1195
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1212
.L1181:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1183:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1206:
	movq	(%rbx), %r13
	jmp	.L1185
	.p2align 4,,10
	.p2align 3
.L1209:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1173
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1203
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1195:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1181
.L1212:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1203
	call	_ZSt17__throw_bad_allocv@PLT
.L1189:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1190:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1191
.L1203:
	call	__stack_chk_fail@PLT
.L1207:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1203
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1211:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1203
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1198:
	endbr64
	movq	%rax, %r12
	jmp	.L1189
.L1197:
	endbr64
	movq	%rax, %r12
	jmp	.L1190
.L1168:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE83:
	jmp	.L1206
.L1191:
	movq	%r12, %rdi
.LEHB84:
	call	_Unwind_Resume@PLT
.LEHE84:
	.cfi_endproc
.LFE16578:
	.section	.gcc_except_table
.LLSDA16578:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16578-.LLSDACSB16578
.LLSDACSB16578:
	.uleb128 .LEHB81-.LFB16578
	.uleb128 .LEHE81-.LEHB81
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB82-.LFB16578
	.uleb128 .LEHE82-.LEHB82
	.uleb128 .L1197-.LFB16578
	.uleb128 0
	.uleb128 .LEHB83-.LFB16578
	.uleb128 .LEHE83-.LEHB83
	.uleb128 .L1198-.LFB16578
	.uleb128 0
	.uleb128 .LEHB84-.LFB16578
	.uleb128 .LEHE84-.LEHB84
	.uleb128 0
	.uleb128 0
.LLSDACSE16578:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16581:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16581
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB85:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE85:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB86:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE86:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1214
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1258
	cmpq	$15, %rax
	ja	.L1218
	cmpq	%r13, %r14
	je	.L1219
	testq	%rax, %rax
	je	.L1236
	cmpq	$1, %rax
	je	.L1259
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1236:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1238
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1238:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1254
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1218:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1260
	movl	$30, %r15d
	movl	$31, %edi
.L1224:
.LEHB87:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1226
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1226:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1227
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1227:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1236
	.p2align 4,,10
	.p2align 3
.L1259:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1236
	.p2align 4,,10
	.p2align 3
.L1214:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1261
	testq	%r12, %r12
	je	.L1236
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1234
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1236
	.p2align 4,,10
	.p2align 3
.L1261:
	testq	%r12, %r12
	js	.L1262
	cmpq	$29, %r12
	jbe	.L1246
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1263
.L1232:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1234:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1257:
	movq	(%rbx), %r13
	jmp	.L1236
	.p2align 4,,10
	.p2align 3
.L1260:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1224
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1254
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1246:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1232
.L1263:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1254
	call	_ZSt17__throw_bad_allocv@PLT
.L1240:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1241:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1242
.L1254:
	call	__stack_chk_fail@PLT
.L1258:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1254
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1262:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1254
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1249:
	endbr64
	movq	%rax, %r12
	jmp	.L1240
.L1248:
	endbr64
	movq	%rax, %r12
	jmp	.L1241
.L1219:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE87:
	jmp	.L1257
.L1242:
	movq	%r12, %rdi
.LEHB88:
	call	_Unwind_Resume@PLT
.LEHE88:
	.cfi_endproc
.LFE16581:
	.section	.gcc_except_table
.LLSDA16581:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16581-.LLSDACSB16581
.LLSDACSB16581:
	.uleb128 .LEHB85-.LFB16581
	.uleb128 .LEHE85-.LEHB85
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB86-.LFB16581
	.uleb128 .LEHE86-.LEHB86
	.uleb128 .L1248-.LFB16581
	.uleb128 0
	.uleb128 .LEHB87-.LFB16581
	.uleb128 .LEHE87-.LEHB87
	.uleb128 .L1249-.LFB16581
	.uleb128 0
	.uleb128 .LEHB88-.LFB16581
	.uleb128 .LEHE88-.LEHB88
	.uleb128 0
	.uleb128 0
.LLSDACSE16581:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_:
.LFB16582:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16582
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1324
.L1264:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1324:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1325
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB89:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE89:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB90:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE90:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L1268
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L1326
	cmpq	$15, %rbp
	ja	.L1272
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L1273
	testq	%rbp, %rbp
	jne	.L1274
.L1277:
	movq	%r12, %rbx
.L1275:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L1282:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1293
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1293:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB91:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE91:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1299
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1299:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1264
	.p2align 4,,10
	.p2align 3
.L1274:
	cmpq	$1, %rbp
	je	.L1327
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L1277
	.p2align 4,,10
	.p2align 3
.L1268:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L1328
	testq	%rbp, %rbp
	je	.L1292
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L1310
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1292:
	movq	%r12, %rax
.L1291:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L1282
	.p2align 4,,10
	.p2align 3
.L1272:
	cmpq	$29, %rbp
	jbe	.L1306
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L1329
.L1278:
	leaq	64(%rsp), %r14
.LEHB92:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L1280
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1280:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1281
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1281:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L1275
	.p2align 4,,10
	.p2align 3
.L1306:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L1278
	.p2align 4,,10
	.p2align 3
.L1327:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L1277
	.p2align 4,,10
	.p2align 3
.L1328:
	testq	%rbp, %rbp
	js	.L1330
	cmpq	$29, %rbp
	ja	.L1331
	movl	$30, %r13d
	movl	$31, %edi
.L1286:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L1288
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1288:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L1289:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1291
.L1331:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L1286
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1329:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE92:
	.p2align 4,,10
	.p2align 3
.L1310:
	movq	%r12, %rbx
	jmp	.L1289
.L1295:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1296:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L1298:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1301:
	subq	$1, %rbp
	jne	.L1332
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1304
.L1323:
	call	__stack_chk_fail@PLT
.L1325:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1323
	movl	%eax, %edi
.LEHB93:
	call	_ZSt20__throw_system_errori@PLT
.LEHE93:
.L1326:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB94:
	call	_ZSt20__throw_length_errorPKc@PLT
.L1330:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L1314:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1296
.L1273:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE94:
	movq	64(%rsp), %rbx
	jmp	.L1275
.L1311:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1301
.L1313:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1300
.L1315:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1295
.L1312:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1298
.L1300:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L1298
.L1332:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1323
.LEHB95:
	call	_Unwind_Resume@PLT
.LEHE95:
.L1304:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16582:
	.section	.gcc_except_table
	.align 4
.LLSDA16582:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16582-.LLSDATTD16582
.LLSDATTD16582:
	.byte	0x1
	.uleb128 .LLSDACSE16582-.LLSDACSB16582
.LLSDACSB16582:
	.uleb128 .LEHB89-.LFB16582
	.uleb128 .LEHE89-.LEHB89
	.uleb128 .L1312-.LFB16582
	.uleb128 0x3
	.uleb128 .LEHB90-.LFB16582
	.uleb128 .LEHE90-.LEHB90
	.uleb128 .L1314-.LFB16582
	.uleb128 0x3
	.uleb128 .LEHB91-.LFB16582
	.uleb128 .LEHE91-.LEHB91
	.uleb128 .L1313-.LFB16582
	.uleb128 0x3
	.uleb128 .LEHB92-.LFB16582
	.uleb128 .LEHE92-.LEHB92
	.uleb128 .L1315-.LFB16582
	.uleb128 0x3
	.uleb128 .LEHB93-.LFB16582
	.uleb128 .LEHE93-.LEHB93
	.uleb128 .L1311-.LFB16582
	.uleb128 0x1
	.uleb128 .LEHB94-.LFB16582
	.uleb128 .LEHE94-.LEHB94
	.uleb128 .L1315-.LFB16582
	.uleb128 0x3
	.uleb128 .LEHB95-.LFB16582
	.uleb128 .LEHE95-.LEHB95
	.uleb128 0
	.uleb128 0
.LLSDACSE16582:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16582:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_:
.LFB16022:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16022
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1334
.LEHB96:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1334:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1358
.L1335:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1356
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1358:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE96:
.LEHB97:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE97:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1359
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1360
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1343
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1342:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1344
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1344:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB98:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA22_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1335
	.p2align 4,,10
	.p2align 3
.L1360:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1343:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1342
	.p2align 4,,10
	.p2align 3
.L1359:
	testq	%rdx, %rdx
	je	.L1338
	cmpq	$1, %rdx
	je	.L1361
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1338:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1342
.L1361:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1338
.L1345:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1346
.L1356:
	call	__stack_chk_fail@PLT
.L1348:
	endbr64
	movq	%rax, %rbx
	jmp	.L1345
.L1346:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE98:
	.cfi_endproc
.LFE16022:
	.section	.gcc_except_table
.LLSDA16022:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16022-.LLSDACSB16022
.LLSDACSB16022:
	.uleb128 .LEHB96-.LFB16022
	.uleb128 .LEHE96-.LEHB96
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB97-.LFB16022
	.uleb128 .LEHE97-.LEHB97
	.uleb128 .L1348-.LFB16022
	.uleb128 0
	.uleb128 .LEHB98-.LFB16022
	.uleb128 .LEHE98-.LEHB98
	.uleb128 0
	.uleb128 0
.LLSDACSE16022:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA22_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB16540:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16540
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB99:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE99:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB100:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE100:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1363
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1407
	cmpq	$15, %rax
	ja	.L1367
	cmpq	%r13, %r14
	je	.L1368
	testq	%rax, %rax
	je	.L1385
	cmpq	$1, %rax
	je	.L1408
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1385:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1387
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1387:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1403
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1367:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1409
	movl	$30, %r15d
	movl	$31, %edi
.L1373:
.LEHB101:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1375
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1375:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1376
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1376:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1385
	.p2align 4,,10
	.p2align 3
.L1408:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1385
	.p2align 4,,10
	.p2align 3
.L1363:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1410
	testq	%r12, %r12
	je	.L1385
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1383
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1385
	.p2align 4,,10
	.p2align 3
.L1410:
	testq	%r12, %r12
	js	.L1411
	cmpq	$29, %r12
	jbe	.L1395
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1412
.L1381:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1383:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1406:
	movq	(%rbx), %r13
	jmp	.L1385
	.p2align 4,,10
	.p2align 3
.L1409:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1373
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1403
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1395:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1381
.L1412:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1403
	call	_ZSt17__throw_bad_allocv@PLT
.L1389:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1390:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1391
.L1403:
	call	__stack_chk_fail@PLT
.L1407:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1403
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1411:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1403
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1398:
	endbr64
	movq	%rax, %r12
	jmp	.L1389
.L1397:
	endbr64
	movq	%rax, %r12
	jmp	.L1390
.L1368:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE101:
	jmp	.L1406
.L1391:
	movq	%r12, %rdi
.LEHB102:
	call	_Unwind_Resume@PLT
.LEHE102:
	.cfi_endproc
.LFE16540:
	.section	.gcc_except_table
.LLSDA16540:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16540-.LLSDACSB16540
.LLSDACSB16540:
	.uleb128 .LEHB99-.LFB16540
	.uleb128 .LEHE99-.LEHB99
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB100-.LFB16540
	.uleb128 .LEHE100-.LEHB100
	.uleb128 .L1397-.LFB16540
	.uleb128 0
	.uleb128 .LEHB101-.LFB16540
	.uleb128 .LEHE101-.LEHB101
	.uleb128 .L1398-.LFB16540
	.uleb128 0
	.uleb128 .LEHB102-.LFB16540
	.uleb128 .LEHE102-.LEHB102
	.uleb128 0
	.uleb128 0
.LLSDACSE16540:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_:
.LFB16541:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16541
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1473
.L1413:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1473:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1474
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB103:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE103:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB104:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE104:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L1417
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L1475
	cmpq	$15, %rbp
	ja	.L1421
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L1422
	testq	%rbp, %rbp
	jne	.L1423
.L1426:
	movq	%r12, %rbx
.L1424:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L1431:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1442
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1442:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB105:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE105:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1448
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1448:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1413
	.p2align 4,,10
	.p2align 3
.L1423:
	cmpq	$1, %rbp
	je	.L1476
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L1426
	.p2align 4,,10
	.p2align 3
.L1417:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L1477
	testq	%rbp, %rbp
	je	.L1441
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L1459
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1441:
	movq	%r12, %rax
.L1440:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L1431
	.p2align 4,,10
	.p2align 3
.L1421:
	cmpq	$29, %rbp
	jbe	.L1455
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L1478
.L1427:
	leaq	64(%rsp), %r14
.LEHB106:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L1429
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1429:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1430
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1430:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L1424
	.p2align 4,,10
	.p2align 3
.L1455:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L1427
	.p2align 4,,10
	.p2align 3
.L1476:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L1426
	.p2align 4,,10
	.p2align 3
.L1477:
	testq	%rbp, %rbp
	js	.L1479
	cmpq	$29, %rbp
	ja	.L1480
	movl	$30, %r13d
	movl	$31, %edi
.L1435:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L1437
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1437:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L1438:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1440
.L1480:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L1435
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1478:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE106:
	.p2align 4,,10
	.p2align 3
.L1459:
	movq	%r12, %rbx
	jmp	.L1438
.L1444:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1445:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L1447:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1450:
	subq	$1, %rbp
	jne	.L1481
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1453
.L1472:
	call	__stack_chk_fail@PLT
.L1474:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1472
	movl	%eax, %edi
.LEHB107:
	call	_ZSt20__throw_system_errori@PLT
.LEHE107:
.L1475:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB108:
	call	_ZSt20__throw_length_errorPKc@PLT
.L1479:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L1463:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1445
.L1422:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE108:
	movq	64(%rsp), %rbx
	jmp	.L1424
.L1460:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1450
.L1462:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1449
.L1464:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1444
.L1461:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1447
.L1449:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L1447
.L1481:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1472
.LEHB109:
	call	_Unwind_Resume@PLT
.LEHE109:
.L1453:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE16541:
	.section	.gcc_except_table
	.align 4
.LLSDA16541:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16541-.LLSDATTD16541
.LLSDATTD16541:
	.byte	0x1
	.uleb128 .LLSDACSE16541-.LLSDACSB16541
.LLSDACSB16541:
	.uleb128 .LEHB103-.LFB16541
	.uleb128 .LEHE103-.LEHB103
	.uleb128 .L1461-.LFB16541
	.uleb128 0x3
	.uleb128 .LEHB104-.LFB16541
	.uleb128 .LEHE104-.LEHB104
	.uleb128 .L1463-.LFB16541
	.uleb128 0x3
	.uleb128 .LEHB105-.LFB16541
	.uleb128 .LEHE105-.LEHB105
	.uleb128 .L1462-.LFB16541
	.uleb128 0x3
	.uleb128 .LEHB106-.LFB16541
	.uleb128 .LEHE106-.LEHB106
	.uleb128 .L1464-.LFB16541
	.uleb128 0x3
	.uleb128 .LEHB107-.LFB16541
	.uleb128 .LEHE107-.LEHB107
	.uleb128 .L1460-.LFB16541
	.uleb128 0x1
	.uleb128 .LEHB108-.LFB16541
	.uleb128 .LEHE108-.LEHB108
	.uleb128 .L1464-.LFB16541
	.uleb128 0x3
	.uleb128 .LEHB109-.LFB16541
	.uleb128 .LEHE109-.LEHB109
	.uleb128 0
	.uleb128 0
.LLSDACSE16541:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16541:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_:
.LFB15994:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15994
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1483
.LEHB110:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1483:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1507
.L1484:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1505
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1507:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE110:
.LEHB111:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE111:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1508
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1509
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1492
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1491:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1493
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1493:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB112:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA31_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1484
	.p2align 4,,10
	.p2align 3
.L1509:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1492:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1491
	.p2align 4,,10
	.p2align 3
.L1508:
	testq	%rdx, %rdx
	je	.L1487
	cmpq	$1, %rdx
	je	.L1510
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1487:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1491
.L1510:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1487
.L1494:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1495
.L1505:
	call	__stack_chk_fail@PLT
.L1497:
	endbr64
	movq	%rax, %rbx
	jmp	.L1494
.L1495:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE112:
	.cfi_endproc
.LFE15994:
	.section	.gcc_except_table
.LLSDA15994:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE15994-.LLSDACSB15994
.LLSDACSB15994:
	.uleb128 .LEHB110-.LFB15994
	.uleb128 .LEHE110-.LEHB110
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB111-.LFB15994
	.uleb128 .LEHE111-.LEHB111
	.uleb128 .L1497-.LFB15994
	.uleb128 0
	.uleb128 .LEHB112-.LFB15994
	.uleb128 .LEHE112-.LEHB112
	.uleb128 0
	.uleb128 0
.LLSDACSE15994:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA31_S1_EEEbDpOT_
	.section	.text._Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_,"axG",@progbits,_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_,comdat
	.p2align 4
	.weak	_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_
	.type	_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_, @function
_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_:
.LFB17052:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17052
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r14
	pushq	%r12
	pushq	%rbx
	.cfi_offset 14, -24
	.cfi_offset 12, -32
	.cfi_offset 3, -40
	movq	%rdi, %rbx
	subq	$72, %rsp
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rsi
	movq	%fs:40, %rax
	movq	%rax, -40(%rbp)
	xorl	%eax, %eax
	movq	24(%rdi), %rax
	leaq	1(%rcx), %rdx
	subq	%rsi, %rax
	cmpq	%rdx, %rax
	jb	.L1534
	movzbl	(%rsi,%rcx), %eax
	movq	%rdx, 8(%rdi)
	cmpb	$-4, %al
	jbe	.L1511
	leaq	-88(%rbp), %rsi
	cmpb	$-3, %al
	je	.L1535
	cmpb	$-2, %al
	je	.L1536
	movl	$8, %edx
.LEHB113:
	call	_ZNK10BaseStream4readEPcm.isra.0
	movq	-88(%rbp), %rax
.L1522:
	cmpq	$33554432, %rax
	ja	.L1537
.L1511:
	movq	-40(%rbp), %rdx
	subq	%fs:40, %rdx
	jne	.L1533
	leaq	-24(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r14
	popq	%rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
	.p2align 4,,10
	.p2align 3
.L1535:
	.cfi_restore_state
	movl	$2, %edx
	call	_ZNK10BaseStream4readEPcm.isra.0
	movzwl	-88(%rbp), %eax
	jmp	.L1511
	.p2align 4,,10
	.p2align 3
.L1536:
	movl	$4, %edx
	call	_ZNK10BaseStream4readEPcm.isra.0
.LEHE113:
	movl	-88(%rbp), %eax
	jmp	.L1522
.L1534:
	movl	$16, %edi
	addq	$8, %rbx
	leaq	-80(%rbp), %r12
	call	__cxa_allocate_exception@PLT
	leaq	.LC15(%rip), %r9
	movq	%r12, %rdi
	leaq	.LC8(%rip), %r8
	pushq	%rcx
	leaq	.LC16(%rip), %rdx
	leaq	.LC9(%rip), %rcx
	movq	%rax, %r14
	pushq	%rbx
	leaq	.LC11(%rip), %rsi
.LEHB114:
	.cfi_escape 0x2e,0x10
	call	_ZN5debug13safe_printstrIJRA5_KcS3_S3_RA4_S1_RA23_S1_RmEEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE114:
	popq	%rax
	movq	%r12, %rsi
	popq	%rdx
	movq	%r14, %rdi
.LEHB115:
	.cfi_escape 0x2e,0
	call	_ZNSt13runtime_errorC2ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE115:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	leaq	16+_ZTVN5debug9exceptionE(%rip), %rcx
	movq	%rcx, (%r14)
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	je	.L1513
.L1533:
	call	__stack_chk_fail@PLT
.L1537:
	movl	$16, %edi
	call	__cxa_allocate_exception@PLT
	leaq	.LC18(%rip), %rsi
	movq	%rax, %rdi
	movq	%rax, %rbx
.LEHB116:
	call	_ZNSt13runtime_errorC1EPKc@PLT
.LEHE116:
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L1533
	movq	_ZNSt13runtime_errorD1Ev@GOTPCREL(%rip), %rdx
	leaq	_ZTISt13runtime_error(%rip), %rsi
	movq	%rbx, %rdi
.LEHB117:
	call	__cxa_throw@PLT
.L1513:
	leaq	_ZN5debug9exceptionD1Ev(%rip), %rdx
	leaq	_ZTIN5debug9exceptionE(%rip), %rsi
	movq	%r14, %rdi
	call	__cxa_throw@PLT
.L1527:
	endbr64
	movq	%rax, %r14
	jmp	.L1524
.L1529:
	endbr64
	movq	%rax, %rbx
	jmp	.L1514
.L1528:
	endbr64
	movq	%rax, %rbx
	jmp	.L1515
.L1524:
	movq	%rbx, %rdi
	call	__cxa_free_exception@PLT
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L1533
	movq	%r14, %rdi
	call	_Unwind_Resume@PLT
.L1514:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1515:
	movq	%r14, %rdi
	call	__cxa_free_exception@PLT
	movq	-40(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L1533
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE117:
	.cfi_endproc
.LFE17052:
	.section	.gcc_except_table
.LLSDA17052:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE17052-.LLSDACSB17052
.LLSDACSB17052:
	.uleb128 .LEHB113-.LFB17052
	.uleb128 .LEHE113-.LEHB113
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB114-.LFB17052
	.uleb128 .LEHE114-.LEHB114
	.uleb128 .L1528-.LFB17052
	.uleb128 0
	.uleb128 .LEHB115-.LFB17052
	.uleb128 .LEHE115-.LEHB115
	.uleb128 .L1529-.LFB17052
	.uleb128 0
	.uleb128 .LEHB116-.LFB17052
	.uleb128 .LEHE116-.LEHB116
	.uleb128 .L1527-.LFB17052
	.uleb128 0
	.uleb128 .LEHB117-.LFB17052
	.uleb128 .LEHE117-.LEHB117
	.uleb128 0
	.uleb128 0
.LLSDACSE17052:
	.section	.text._Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_,"axG",@progbits,_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_,comdat
	.size	_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_, .-_Z15ReadCompactSizeIKN3TAO9Operation6StreamEEmRT_
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_:
.LFB17061:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	16(%rdi), %r15
	movq	%rdi, 24(%rsp)
	movq	%rsi, 16(%rsp)
	testq	%r15, %r15
	je	.L1565
	movq	(%rsi), %rdx
	movq	8(%rsi), %r14
	movl	$2147483648, %ebp
	movq	%rdx, 8(%rsp)
	jmp	.L1546
	.p2align 4,,10
	.p2align 3
.L1544:
	movq	16(%r15), %rax
	movl	$1, %esi
	testq	%rax, %rax
	je	.L1566
.L1551:
	movq	%rax, %r15
.L1546:
	movq	40(%r15), %r12
	movq	32(%r15), %r13
	cmpq	%r12, %r14
	movq	%r12, %rbx
	cmovbe	%r14, %rbx
	testq	%rbx, %rbx
	je	.L1541
	movq	8(%rsp), %rdi
	movq	%rbx, %rdx
	movq	%r13, %rsi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L1542
.L1541:
	movq	%r14, %rax
	subq	%r12, %rax
	cmpq	%rbp, %rax
	jge	.L1543
	movabsq	$-2147483649, %rcx
	cmpq	%rcx, %rax
	jle	.L1544
.L1542:
	testl	%eax, %eax
	js	.L1544
.L1543:
	movq	24(%r15), %rax
	xorl	%esi, %esi
	testq	%rax, %rax
	jne	.L1551
.L1566:
	movq	%r15, %rbp
	testb	%sil, %sil
	jne	.L1540
.L1547:
	testq	%rbx, %rbx
	je	.L1549
	movq	8(%rsp), %rsi
	movq	%rbx, %rdx
	movq	%r13, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	je	.L1549
.L1550:
	xorl	%edx, %edx
	testl	%eax, %eax
	cmovns	%rdx, %rbp
	cmovs	%rdx, %r15
.L1548:
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%r15, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	movq	%rbp, %rdx
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1549:
	.cfi_restore_state
	movq	%r12, %rcx
	subq	%r14, %rcx
	cmpq	$2147483647, %rcx
	jg	.L1554
	cmpq	$-2147483648, %rcx
	jl	.L1555
	movl	%ecx, %eax
	jmp	.L1550
	.p2align 4,,10
	.p2align 3
.L1565:
	leaq	8(%rdi), %r15
.L1540:
	movq	24(%rsp), %rax
	cmpq	%r15, 24(%rax)
	je	.L1553
	movq	%r15, %rdi
	movq	%r15, %rbp
	call	_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base@PLT
	movq	16(%rsp), %rdx
	movq	40(%rax), %r12
	movq	32(%rax), %r13
	movq	%rax, %r15
	movq	8(%rdx), %r14
	movq	(%rdx), %rcx
	movq	%r12, %rbx
	cmpq	%r12, %r14
	movq	%rcx, 8(%rsp)
	cmovbe	%r14, %rbx
	jmp	.L1547
	.p2align 4,,10
	.p2align 3
.L1553:
	movq	%r15, %rbp
	xorl	%r15d, %r15d
	jmp	.L1548
	.p2align 4,,10
	.p2align 3
.L1554:
	xorl	%ebp, %ebp
	jmp	.L1548
.L1555:
	xorl	%r15d, %r15d
	jmp	.L1548
	.cfi_endproc
.LFE17061:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB17093:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17093
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB118:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE118:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB119:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE119:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1568
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1612
	cmpq	$15, %rax
	ja	.L1572
	cmpq	%r13, %r14
	je	.L1573
	testq	%rax, %rax
	je	.L1590
	cmpq	$1, %rax
	je	.L1613
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1590:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1592
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1592:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1608
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1572:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1614
	movl	$30, %r15d
	movl	$31, %edi
.L1578:
.LEHB120:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1580
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1580:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1581
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1581:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1590
	.p2align 4,,10
	.p2align 3
.L1613:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1590
	.p2align 4,,10
	.p2align 3
.L1568:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1615
	testq	%r12, %r12
	je	.L1590
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1588
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1590
	.p2align 4,,10
	.p2align 3
.L1615:
	testq	%r12, %r12
	js	.L1616
	cmpq	$29, %r12
	jbe	.L1600
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1617
.L1586:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1588:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1611:
	movq	(%rbx), %r13
	jmp	.L1590
	.p2align 4,,10
	.p2align 3
.L1614:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1578
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1608
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1600:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1586
.L1617:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1608
	call	_ZSt17__throw_bad_allocv@PLT
.L1594:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1595:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1596
.L1608:
	call	__stack_chk_fail@PLT
.L1612:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1608
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1616:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1608
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1603:
	endbr64
	movq	%rax, %r12
	jmp	.L1594
.L1602:
	endbr64
	movq	%rax, %r12
	jmp	.L1595
.L1573:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE120:
	jmp	.L1611
.L1596:
	movq	%r12, %rdi
.LEHB121:
	call	_Unwind_Resume@PLT
.LEHE121:
	.cfi_endproc
.LFE17093:
	.section	.gcc_except_table
.LLSDA17093:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE17093-.LLSDACSB17093
.LLSDACSB17093:
	.uleb128 .LEHB118-.LFB17093
	.uleb128 .LEHE118-.LEHB118
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB119-.LFB17093
	.uleb128 .LEHE119-.LEHB119
	.uleb128 .L1602-.LFB17093
	.uleb128 0
	.uleb128 .LEHB120-.LFB17093
	.uleb128 .LEHE120-.LEHB120
	.uleb128 .L1603-.LFB17093
	.uleb128 0
	.uleb128 .LEHB121-.LFB17093
	.uleb128 .LEHE121-.LEHB121
	.uleb128 0
	.uleb128 0
.LLSDACSE17093:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_:
.LFB17094:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17094
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1678
.L1618:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1678:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1679
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB122:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE122:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB123:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE123:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L1622
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L1680
	cmpq	$15, %rbp
	ja	.L1626
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L1627
	testq	%rbp, %rbp
	jne	.L1628
.L1631:
	movq	%r12, %rbx
.L1629:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L1636:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1647
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1647:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB124:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE124:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1653
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1653:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1618
	.p2align 4,,10
	.p2align 3
.L1628:
	cmpq	$1, %rbp
	je	.L1681
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L1631
	.p2align 4,,10
	.p2align 3
.L1622:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L1682
	testq	%rbp, %rbp
	je	.L1646
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L1664
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1646:
	movq	%r12, %rax
.L1645:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L1636
	.p2align 4,,10
	.p2align 3
.L1626:
	cmpq	$29, %rbp
	jbe	.L1660
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L1683
.L1632:
	leaq	64(%rsp), %r14
.LEHB125:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L1634
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1634:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1635
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1635:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L1629
	.p2align 4,,10
	.p2align 3
.L1660:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L1632
	.p2align 4,,10
	.p2align 3
.L1681:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L1631
	.p2align 4,,10
	.p2align 3
.L1682:
	testq	%rbp, %rbp
	js	.L1684
	cmpq	$29, %rbp
	ja	.L1685
	movl	$30, %r13d
	movl	$31, %edi
.L1640:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L1642
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1642:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L1643:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1645
.L1685:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L1640
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1683:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE125:
	.p2align 4,,10
	.p2align 3
.L1664:
	movq	%r12, %rbx
	jmp	.L1643
.L1649:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1650:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L1652:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1655:
	subq	$1, %rbp
	jne	.L1686
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1658
.L1677:
	call	__stack_chk_fail@PLT
.L1679:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1677
	movl	%eax, %edi
.LEHB126:
	call	_ZSt20__throw_system_errori@PLT
.LEHE126:
.L1680:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB127:
	call	_ZSt20__throw_length_errorPKc@PLT
.L1684:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L1668:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1650
.L1627:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE127:
	movq	64(%rsp), %rbx
	jmp	.L1629
.L1665:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1655
.L1667:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1654
.L1669:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1649
.L1666:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1652
.L1654:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L1652
.L1686:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1677
.LEHB128:
	call	_Unwind_Resume@PLT
.LEHE128:
.L1658:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE17094:
	.section	.gcc_except_table
	.align 4
.LLSDA17094:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT17094-.LLSDATTD17094
.LLSDATTD17094:
	.byte	0x1
	.uleb128 .LLSDACSE17094-.LLSDACSB17094
.LLSDACSB17094:
	.uleb128 .LEHB122-.LFB17094
	.uleb128 .LEHE122-.LEHB122
	.uleb128 .L1666-.LFB17094
	.uleb128 0x3
	.uleb128 .LEHB123-.LFB17094
	.uleb128 .LEHE123-.LEHB123
	.uleb128 .L1668-.LFB17094
	.uleb128 0x3
	.uleb128 .LEHB124-.LFB17094
	.uleb128 .LEHE124-.LEHB124
	.uleb128 .L1667-.LFB17094
	.uleb128 0x3
	.uleb128 .LEHB125-.LFB17094
	.uleb128 .LEHE125-.LEHB125
	.uleb128 .L1669-.LFB17094
	.uleb128 0x3
	.uleb128 .LEHB126-.LFB17094
	.uleb128 .LEHE126-.LEHB126
	.uleb128 .L1665-.LFB17094
	.uleb128 0x1
	.uleb128 .LEHB127-.LFB17094
	.uleb128 .LEHE127-.LEHB127
	.uleb128 .L1669-.LFB17094
	.uleb128 0x3
	.uleb128 .LEHB128-.LFB17094
	.uleb128 .LEHE128-.LEHB128
	.uleb128 0
	.uleb128 0
.LLSDACSE17094:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT17094:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_:
.LFB16563:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16563
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1688
.LEHB129:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1688:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1712
.L1689:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1710
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1712:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE129:
.LEHB130:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE130:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1713
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1714
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1697
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1696:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1698
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1698:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB131:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA42_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1689
	.p2align 4,,10
	.p2align 3
.L1714:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1697:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1696
	.p2align 4,,10
	.p2align 3
.L1713:
	testq	%rdx, %rdx
	je	.L1692
	cmpq	$1, %rdx
	je	.L1715
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1692:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1696
.L1715:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1692
.L1699:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1700
.L1710:
	call	__stack_chk_fail@PLT
.L1702:
	endbr64
	movq	%rax, %rbx
	jmp	.L1699
.L1700:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE131:
	.cfi_endproc
.LFE16563:
	.section	.gcc_except_table
.LLSDA16563:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16563-.LLSDACSB16563
.LLSDACSB16563:
	.uleb128 .LEHB129-.LFB16563
	.uleb128 .LEHE129-.LEHB129
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB130-.LFB16563
	.uleb128 .LEHE130-.LEHB130
	.uleb128 .L1702-.LFB16563
	.uleb128 0
	.uleb128 .LEHB131-.LFB16563
	.uleb128 .LEHE131-.LEHB131
	.uleb128 0
	.uleb128 0
.LLSDACSE16563:
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA42_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB17096:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17096
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB132:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE132:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB133:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE133:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1717
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1761
	cmpq	$15, %rax
	ja	.L1721
	cmpq	%r13, %r14
	je	.L1722
	testq	%rax, %rax
	je	.L1739
	cmpq	$1, %rax
	je	.L1762
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1739:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1741
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1741:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1757
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1721:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1763
	movl	$30, %r15d
	movl	$31, %edi
.L1727:
.LEHB134:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1729
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1729:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1730
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1730:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1739
	.p2align 4,,10
	.p2align 3
.L1762:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1739
	.p2align 4,,10
	.p2align 3
.L1717:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1764
	testq	%r12, %r12
	je	.L1739
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1737
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1739
	.p2align 4,,10
	.p2align 3
.L1764:
	testq	%r12, %r12
	js	.L1765
	cmpq	$29, %r12
	jbe	.L1749
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1766
.L1735:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1737:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1760:
	movq	(%rbx), %r13
	jmp	.L1739
	.p2align 4,,10
	.p2align 3
.L1763:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1727
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1757
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1749:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1735
.L1766:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1757
	call	_ZSt17__throw_bad_allocv@PLT
.L1743:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1744:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1745
.L1757:
	call	__stack_chk_fail@PLT
.L1761:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1757
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1765:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1757
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1752:
	endbr64
	movq	%rax, %r12
	jmp	.L1743
.L1751:
	endbr64
	movq	%rax, %r12
	jmp	.L1744
.L1722:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE134:
	jmp	.L1760
.L1745:
	movq	%r12, %rdi
.LEHB135:
	call	_Unwind_Resume@PLT
.LEHE135:
	.cfi_endproc
.LFE17096:
	.section	.gcc_except_table
.LLSDA17096:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE17096-.LLSDACSB17096
.LLSDACSB17096:
	.uleb128 .LEHB132-.LFB17096
	.uleb128 .LEHE132-.LEHB132
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB133-.LFB17096
	.uleb128 .LEHE133-.LEHB133
	.uleb128 .L1751-.LFB17096
	.uleb128 0
	.uleb128 .LEHB134-.LFB17096
	.uleb128 .LEHE134-.LEHB134
	.uleb128 .L1752-.LFB17096
	.uleb128 0
	.uleb128 .LEHB135-.LFB17096
	.uleb128 .LEHE135-.LEHB135
	.uleb128 0
	.uleb128 0
.LLSDACSE17096:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_:
.LFB17097:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17097
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1810
.L1767:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1809
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1810:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1811
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB136:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE136:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB137:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE137:
	movq	144(%rsp), %r8
	leaq	80(%rsp), %rbp
	movb	$0, 80(%rsp)
	movq	%rbp, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%r8, %r8
	je	.L1771
	movq	128(%rsp), %rax
	movq	%r8, %rdx
	leaq	64(%rsp), %r12
	movq	136(%rsp), %rcx
	movq	%r12, %rdi
	cmpq	%r8, %rax
	cmovnb	%rax, %r8
	testq	%rax, %rax
	cmove	%rdx, %r8
	xorl	%edx, %edx
	xorl	%esi, %esi
	subq	%rcx, %r8
.LEHB138:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0
.LEHE138:
.L1773:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1784
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1784:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r12, %rsi
	movq	%rax, 40(%rsp)
.LEHB139:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE139:
	movq	64(%rsp), %rdi
	cmpq	%rbp, %rdi
	je	.L1790
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1790:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1767
	.p2align 4,,10
	.p2align 3
.L1771:
	movq	184(%rsp), %r13
	cmpq	$15, %r13
	ja	.L1812
	testq	%r13, %r13
	je	.L1783
	movq	176(%rsp), %rsi
	cmpq	$1, %r13
	jne	.L1799
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1783:
	movq	%rbp, %rax
.L1782:
	movq	%r13, 72(%rsp)
	leaq	64(%rsp), %r12
	movb	$0, (%rax,%r13)
	jmp	.L1773
	.p2align 4,,10
	.p2align 3
.L1812:
	testq	%r13, %r13
	js	.L1813
	cmpq	$29, %r13
	ja	.L1814
	movl	$30, %r14d
	movl	$31, %edi
.L1777:
	leaq	64(%rsp), %r12
.LEHB140:
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%rbp, %rdi
	je	.L1779
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1779:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r14, 80(%rsp)
.L1780:
	movq	%r13, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1782
.L1814:
	movq	%r13, %rdi
	movq	%r13, %r14
	addq	$1, %rdi
	jns	.L1777
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1809
	leaq	64(%rsp), %r12
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE140:
	.p2align 4,,10
	.p2align 3
.L1799:
	movq	%rbp, %rbx
	jmp	.L1780
.L1791:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1789:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1792:
	subq	$1, %rbp
	jne	.L1815
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1795
.L1809:
	call	__stack_chk_fail@PLT
.L1811:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1809
	movl	%eax, %edi
.LEHB141:
	call	_ZSt20__throw_system_errori@PLT
.LEHE141:
.L1813:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1809
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r12
.LEHB142:
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE142:
.L1800:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1792
.L1804:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r13
	jmp	.L1786
.L1801:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1789
.L1802:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1791
.L1803:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r13
	jmp	.L1787
.L1786:
	movq	%r12, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1787:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r13, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	jmp	.L1789
.L1815:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1809
.LEHB143:
	call	_Unwind_Resume@PLT
.LEHE143:
.L1795:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE17097:
	.section	.gcc_except_table
	.align 4
.LLSDA17097:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT17097-.LLSDATTD17097
.LLSDATTD17097:
	.byte	0x1
	.uleb128 .LLSDACSE17097-.LLSDACSB17097
.LLSDACSB17097:
	.uleb128 .LEHB136-.LFB17097
	.uleb128 .LEHE136-.LEHB136
	.uleb128 .L1801-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB137-.LFB17097
	.uleb128 .LEHE137-.LEHB137
	.uleb128 .L1803-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB138-.LFB17097
	.uleb128 .LEHE138-.LEHB138
	.uleb128 .L1804-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB139-.LFB17097
	.uleb128 .LEHE139-.LEHB139
	.uleb128 .L1802-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB140-.LFB17097
	.uleb128 .LEHE140-.LEHB140
	.uleb128 .L1804-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB141-.LFB17097
	.uleb128 .LEHE141-.LEHB141
	.uleb128 .L1800-.LFB17097
	.uleb128 0x1
	.uleb128 .LEHB142-.LFB17097
	.uleb128 .LEHE142-.LEHB142
	.uleb128 .L1804-.LFB17097
	.uleb128 0x3
	.uleb128 .LEHB143-.LFB17097
	.uleb128 .LEHE143-.LEHB143
	.uleb128 0
	.uleb128 0
.LLSDACSE17097:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT17097:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_:
.LFB16565:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16565
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1817
.LEHB144:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1817:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1841
.L1818:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1839
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1841:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE144:
.LEHB145:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE145:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1842
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1843
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1826
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1825:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1827
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1827:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB146:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA17_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1818
	.p2align 4,,10
	.p2align 3
.L1843:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1826:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1825
	.p2align 4,,10
	.p2align 3
.L1842:
	testq	%rdx, %rdx
	je	.L1821
	cmpq	$1, %rdx
	je	.L1844
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1821:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1825
.L1844:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1821
.L1828:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1829
.L1839:
	call	__stack_chk_fail@PLT
.L1831:
	endbr64
	movq	%rax, %rbx
	jmp	.L1828
.L1829:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE146:
	.cfi_endproc
.LFE16565:
	.section	.gcc_except_table
.LLSDA16565:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16565-.LLSDACSB16565
.LLSDACSB16565:
	.uleb128 .LEHB144-.LFB16565
	.uleb128 .LEHE144-.LEHB144
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB145-.LFB16565
	.uleb128 .LEHE145-.LEHB145
	.uleb128 .L1831-.LFB16565
	.uleb128 0
	.uleb128 .LEHB146-.LFB16565
	.uleb128 .LEHE146-.LEHB146
	.uleb128 0
	.uleb128 0
.LLSDACSE16565:
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA17_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB17098:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17098
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB147:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE147:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB148:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE148:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1846
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L1890
	cmpq	$15, %rax
	ja	.L1850
	cmpq	%r13, %r14
	je	.L1851
	testq	%rax, %rax
	je	.L1868
	cmpq	$1, %rax
	je	.L1891
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L1868:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L1870
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1870:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1886
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1850:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L1892
	movl	$30, %r15d
	movl	$31, %edi
.L1856:
.LEHB149:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L1858
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1858:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L1859
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1859:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L1868
	.p2align 4,,10
	.p2align 3
.L1891:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L1868
	.p2align 4,,10
	.p2align 3
.L1846:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L1893
	testq	%r12, %r12
	je	.L1868
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L1866
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L1868
	.p2align 4,,10
	.p2align 3
.L1893:
	testq	%r12, %r12
	js	.L1894
	cmpq	$29, %r12
	jbe	.L1878
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L1895
.L1864:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L1866:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L1889:
	movq	(%rbx), %r13
	jmp	.L1868
	.p2align 4,,10
	.p2align 3
.L1892:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L1856
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1886
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1878:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L1864
.L1895:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1886
	call	_ZSt17__throw_bad_allocv@PLT
.L1872:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1873:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1874
.L1886:
	call	__stack_chk_fail@PLT
.L1890:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1886
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1894:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1886
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L1881:
	endbr64
	movq	%rax, %r12
	jmp	.L1872
.L1880:
	endbr64
	movq	%rax, %r12
	jmp	.L1873
.L1851:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE149:
	jmp	.L1889
.L1874:
	movq	%r12, %rdi
.LEHB150:
	call	_Unwind_Resume@PLT
.LEHE150:
	.cfi_endproc
.LFE17098:
	.section	.gcc_except_table
.LLSDA17098:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE17098-.LLSDACSB17098
.LLSDACSB17098:
	.uleb128 .LEHB147-.LFB17098
	.uleb128 .LEHE147-.LEHB147
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB148-.LFB17098
	.uleb128 .LEHE148-.LEHB148
	.uleb128 .L1880-.LFB17098
	.uleb128 0
	.uleb128 .LEHB149-.LFB17098
	.uleb128 .LEHE149-.LEHB149
	.uleb128 .L1881-.LFB17098
	.uleb128 0
	.uleb128 .LEHB150-.LFB17098
	.uleb128 .LEHE150-.LEHB150
	.uleb128 0
	.uleb128 0
.LLSDACSE17098:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_:
.LFB17099:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17099
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L1956
.L1896:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1956:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L1957
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB151:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE151:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB152:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE152:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L1900
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L1958
	cmpq	$15, %rbp
	ja	.L1904
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L1905
	testq	%rbp, %rbp
	jne	.L1906
.L1909:
	movq	%r12, %rbx
.L1907:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L1914:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L1925
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1925:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB153:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE153:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1931
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1931:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L1896
	.p2align 4,,10
	.p2align 3
.L1906:
	cmpq	$1, %rbp
	je	.L1959
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L1909
	.p2align 4,,10
	.p2align 3
.L1900:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L1960
	testq	%rbp, %rbp
	je	.L1924
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L1942
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L1924:
	movq	%r12, %rax
.L1923:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L1914
	.p2align 4,,10
	.p2align 3
.L1904:
	cmpq	$29, %rbp
	jbe	.L1938
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L1961
.L1910:
	leaq	64(%rsp), %r14
.LEHB154:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L1912
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L1912:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L1913
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1913:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L1907
	.p2align 4,,10
	.p2align 3
.L1938:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L1910
	.p2align 4,,10
	.p2align 3
.L1959:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L1909
	.p2align 4,,10
	.p2align 3
.L1960:
	testq	%rbp, %rbp
	js	.L1962
	cmpq	$29, %rbp
	ja	.L1963
	movl	$30, %r13d
	movl	$31, %edi
.L1918:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L1920
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1920:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L1921:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L1923
.L1963:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L1918
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L1961:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE154:
	.p2align 4,,10
	.p2align 3
.L1942:
	movq	%r12, %rbx
	jmp	.L1921
.L1927:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L1928:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L1930:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L1933:
	subq	$1, %rbp
	jne	.L1964
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1936
.L1955:
	call	__stack_chk_fail@PLT
.L1957:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L1955
	movl	%eax, %edi
.LEHB155:
	call	_ZSt20__throw_system_errori@PLT
.LEHE155:
.L1958:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB156:
	call	_ZSt20__throw_length_errorPKc@PLT
.L1962:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L1946:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1928
.L1905:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE156:
	movq	64(%rsp), %rbx
	jmp	.L1907
.L1943:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L1933
.L1945:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1932
.L1947:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L1927
.L1944:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L1930
.L1932:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L1930
.L1964:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1955
.LEHB157:
	call	_Unwind_Resume@PLT
.LEHE157:
.L1936:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE17099:
	.section	.gcc_except_table
	.align 4
.LLSDA17099:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT17099-.LLSDATTD17099
.LLSDATTD17099:
	.byte	0x1
	.uleb128 .LLSDACSE17099-.LLSDACSB17099
.LLSDACSB17099:
	.uleb128 .LEHB151-.LFB17099
	.uleb128 .LEHE151-.LEHB151
	.uleb128 .L1944-.LFB17099
	.uleb128 0x3
	.uleb128 .LEHB152-.LFB17099
	.uleb128 .LEHE152-.LEHB152
	.uleb128 .L1946-.LFB17099
	.uleb128 0x3
	.uleb128 .LEHB153-.LFB17099
	.uleb128 .LEHE153-.LEHB153
	.uleb128 .L1945-.LFB17099
	.uleb128 0x3
	.uleb128 .LEHB154-.LFB17099
	.uleb128 .LEHE154-.LEHB154
	.uleb128 .L1947-.LFB17099
	.uleb128 0x3
	.uleb128 .LEHB155-.LFB17099
	.uleb128 .LEHE155-.LEHB155
	.uleb128 .L1943-.LFB17099
	.uleb128 0x1
	.uleb128 .LEHB156-.LFB17099
	.uleb128 .LEHE156-.LEHB156
	.uleb128 .L1947-.LFB17099
	.uleb128 0x3
	.uleb128 .LEHB157-.LFB17099
	.uleb128 .LEHE157-.LEHB157
	.uleb128 0
	.uleb128 0
.LLSDACSE17099:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT17099:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_:
.LFB16566:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16566
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L1966
.LEHB158:
	call	_ZTHN5debug9fLogErrorE@PLT
.L1966:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L1990
.L1967:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L1988
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1990:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE158:
.LEHB159:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE159:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L1991
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L1992
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L1975
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L1974:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L1976
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L1976:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB160:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA14_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L1967
	.p2align 4,,10
	.p2align 3
.L1992:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L1975:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L1974
	.p2align 4,,10
	.p2align 3
.L1991:
	testq	%rdx, %rdx
	je	.L1970
	cmpq	$1, %rdx
	je	.L1993
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L1970:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L1974
.L1993:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L1970
.L1977:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L1978
.L1988:
	call	__stack_chk_fail@PLT
.L1980:
	endbr64
	movq	%rax, %rbx
	jmp	.L1977
.L1978:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE160:
	.cfi_endproc
.LFE16566:
	.section	.gcc_except_table
.LLSDA16566:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16566-.LLSDACSB16566
.LLSDACSB16566:
	.uleb128 .LEHB158-.LFB16566
	.uleb128 .LEHE158-.LEHB158
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB159-.LFB16566
	.uleb128 .LEHE159-.LEHB159
	.uleb128 .L1980-.LFB16566
	.uleb128 0
	.uleb128 .LEHB160-.LFB16566
	.uleb128 .LEHE160-.LEHB160
	.uleb128 0
	.uleb128 0
.LLSDACSE16566:
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA14_S1_EEEbDpOT_
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.type	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, @function
_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_:
.LFB17100:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17100
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rsi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rdx, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%r8, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$408, %rsp
	.cfi_def_cfa_offset 464
	movq	%r9, 8(%rsp)
	leaq	16(%rsp), %rbp
	movq	%rbp, %rdi
	movq	%fs:40, %rax
	movq	%rax, 392(%rsp)
	xorl	%eax, %eax
.LEHB161:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE161:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
.LEHB162:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbp, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE162:
	movq	64(%rsp), %rax
	leaq	16(%rbx), %r13
	movq	$0, 8(%rbx)
	movq	%r13, (%rbx)
	movb	$0, 16(%rbx)
	testq	%rax, %rax
	je	.L1995
	movq	48(%rsp), %rdx
	movq	%rax, %rcx
	movq	56(%rsp), %r14
	cmpq	%rax, %rdx
	cmovnb	%rdx, %rax
	testq	%rdx, %rdx
	cmove	%rcx, %rax
	subq	%r14, %rax
	movq	%rax, %r12
	js	.L2039
	cmpq	$15, %rax
	ja	.L1999
	cmpq	%r13, %r14
	je	.L2000
	testq	%rax, %rax
	je	.L2017
	cmpq	$1, %rax
	je	.L2040
	movq	%rax, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	memcpy@PLT
	.p2align 4,,10
	.p2align 3
.L2017:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%r12, 8(%rbx)
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movb	$0, 0(%r13,%r12)
	movq	%rcx, %xmm0
	movq	96(%rsp), %rdi
	movq	%rax, 128(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rax, %xmm1
	leaq	112(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 16(%rsp)
	cmpq	%rax, %rdi
	je	.L2019
	movq	112(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2019:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	80(%rsp), %rdi
	movq	%rax, 24(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	128(%rsp), %rdi
	movq	%rax, 16(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 16(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 128(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2035
	addq	$408, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L1999:
	.cfi_restore_state
	cmpq	$29, %rax
	ja	.L2041
	movl	$30, %r15d
	movl	$31, %edi
.L2005:
.LEHB163:
	call	_Znwm@PLT
	movq	%rax, %rbp
	testq	%r14, %r14
	je	.L2007
	movq	%r12, %rdx
	movq	%r14, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L2007:
	movq	(%rbx), %rdi
	cmpq	%rdi, %r13
	je	.L2008
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2008:
	movq	%rbp, (%rbx)
	movq	%rbp, %r13
	movq	%r15, 16(%rbx)
	jmp	.L2017
	.p2align 4,,10
	.p2align 3
.L2040:
	movzbl	(%r14), %eax
	movb	%al, 16(%rbx)
	jmp	.L2017
	.p2align 4,,10
	.p2align 3
.L1995:
	movq	104(%rsp), %r12
	cmpq	$15, %r12
	ja	.L2042
	testq	%r12, %r12
	je	.L2017
	movq	96(%rsp), %rsi
	cmpq	$1, %r12
	jne	.L2015
	movzbl	(%rsi), %eax
	movb	%al, 16(%rbx)
	jmp	.L2017
	.p2align 4,,10
	.p2align 3
.L2042:
	testq	%r12, %r12
	js	.L2043
	cmpq	$29, %r12
	jbe	.L2027
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L2044
.L2013:
	call	_Znwm@PLT
	movq	%rax, %r13
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%r13, (%rbx)
	movq	96(%rsp), %rsi
	movq	%r14, 16(%rbx)
.L2015:
	movq	%r12, %rdx
	movq	%r13, %rdi
	call	memcpy@PLT
.L2038:
	movq	(%rbx), %r13
	jmp	.L2017
	.p2align 4,,10
	.p2align 3
.L2041:
	movq	%rax, %rdi
	movq	%rax, %r15
	addq	$1, %rdi
	jns	.L2005
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2035
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L2027:
	movl	$30, %r14d
	movl	$31, %edi
	jmp	.L2013
.L2044:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2035
	call	_ZSt17__throw_bad_allocv@PLT
.L2021:
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L2022:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	je	.L2023
.L2035:
	call	__stack_chk_fail@PLT
.L2039:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2035
	leaq	.LC3(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L2043:
	movq	392(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2035
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.L2030:
	endbr64
	movq	%rax, %r12
	jmp	.L2021
.L2029:
	endbr64
	movq	%rax, %r12
	jmp	.L2022
.L2000:
	xorl	%r9d, %r9d
	movq	%rax, %r8
	movq	%r13, %rcx
	xorl	%edx, %edx
	movq	%r13, %rsi
	movq	%rbx, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE163:
	jmp	.L2038
.L2023:
	movq	%r12, %rdi
.LEHB164:
	call	_Unwind_Resume@PLT
.LEHE164:
	.cfi_endproc
.LFE17100:
	.section	.gcc_except_table
.LLSDA17100:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE17100-.LLSDACSB17100
.LLSDACSB17100:
	.uleb128 .LEHB161-.LFB17100
	.uleb128 .LEHE161-.LEHB161
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB162-.LFB17100
	.uleb128 .LEHE162-.LEHB162
	.uleb128 .L2029-.LFB17100
	.uleb128 0
	.uleb128 .LEHB163-.LFB17100
	.uleb128 .LEHE163-.LEHB163
	.uleb128 .L2030-.LFB17100
	.uleb128 0
	.uleb128 .LEHB164-.LFB17100
	.uleb128 .LEHE164-.LEHB164
	.uleb128 0
	.uleb128 0
.LLSDACSE17100:
	.section	.text._ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,"axG",@progbits,_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_,comdat
	.size	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_, .-_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_:
.LFB17101:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA17101
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L2105
.L2045:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2105:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L2106
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB165:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE165:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB166:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE166:
	movq	144(%rsp), %rbp
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%rbp, %rbp
	je	.L2049
	movq	128(%rsp), %rax
	movq	%rbp, %rdx
	movq	136(%rsp), %r13
	cmpq	%rbp, %rax
	cmovnb	%rax, %rbp
	testq	%rax, %rax
	cmove	%rdx, %rbp
	subq	%r13, %rbp
	js	.L2107
	cmpq	$15, %rbp
	ja	.L2053
	leaq	64(%rsp), %r14
	cmpq	%r12, %r13
	je	.L2054
	testq	%rbp, %rbp
	jne	.L2055
.L2058:
	movq	%r12, %rbx
.L2056:
	movq	%rbp, 72(%rsp)
	movb	$0, (%rbx,%rbp)
.L2063:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L2074
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2074:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%r14, %rsi
	movq	%rax, 40(%rsp)
.LEHB167:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE167:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L2080
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2080:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L2045
	.p2align 4,,10
	.p2align 3
.L2055:
	cmpq	$1, %rbp
	je	.L2108
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%r12, %rdi
	call	memcpy@PLT
	jmp	.L2058
	.p2align 4,,10
	.p2align 3
.L2049:
	movq	184(%rsp), %rbp
	cmpq	$15, %rbp
	ja	.L2109
	testq	%rbp, %rbp
	je	.L2073
	movq	176(%rsp), %rsi
	cmpq	$1, %rbp
	jne	.L2091
	movzbl	(%rsi), %eax
	movb	%al, 80(%rsp)
.L2073:
	movq	%r12, %rax
.L2072:
	movq	%rbp, 72(%rsp)
	leaq	64(%rsp), %r14
	movb	$0, (%rax,%rbp)
	jmp	.L2063
	.p2align 4,,10
	.p2align 3
.L2053:
	cmpq	$29, %rbp
	jbe	.L2087
	movq	%rbp, %rdi
	movq	%rbp, %r15
	addq	$1, %rdi
	js	.L2110
.L2059:
	leaq	64(%rsp), %r14
.LEHB168:
	call	_Znwm@PLT
	movq	%rax, %rbx
	testq	%r13, %r13
	je	.L2061
	movq	%rbp, %rdx
	movq	%r13, %rsi
	movq	%rax, %rdi
	call	memcpy@PLT
.L2061:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L2062
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2062:
	movq	%rbx, 64(%rsp)
	leaq	64(%rsp), %r14
	movq	%r15, 80(%rsp)
	jmp	.L2056
	.p2align 4,,10
	.p2align 3
.L2087:
	movl	$30, %r15d
	movl	$31, %edi
	jmp	.L2059
	.p2align 4,,10
	.p2align 3
.L2108:
	movzbl	0(%r13), %eax
	movb	%al, 80(%rsp)
	jmp	.L2058
	.p2align 4,,10
	.p2align 3
.L2109:
	testq	%rbp, %rbp
	js	.L2111
	cmpq	$29, %rbp
	ja	.L2112
	movl	$30, %r13d
	movl	$31, %edi
.L2067:
	leaq	64(%rsp), %r14
	call	_Znwm@PLT
	movq	64(%rsp), %rdi
	movq	%rax, %rbx
	cmpq	%r12, %rdi
	je	.L2069
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2069:
	movq	%rbx, 64(%rsp)
	movq	176(%rsp), %rsi
	movq	%r13, 80(%rsp)
.L2070:
	movq	%rbp, %rdx
	movq	%rbx, %rdi
	call	memcpy@PLT
	movq	64(%rsp), %rax
	jmp	.L2072
.L2112:
	movq	%rbp, %rdi
	movq	%rbp, %r13
	addq	$1, %rdi
	jns	.L2067
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L2110:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
	leaq	64(%rsp), %r14
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE168:
	.p2align 4,,10
	.p2align 3
.L2091:
	movq	%r12, %rbx
	jmp	.L2070
.L2076:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L2077:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	movq	%r12, %rbp
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
.L2079:
	leaq	48(%rsp), %rdi
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L2082:
	subq	$1, %rbp
	jne	.L2113
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L2085
.L2104:
	call	__stack_chk_fail@PLT
.L2106:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L2104
	movl	%eax, %edi
.LEHB169:
	call	_ZSt20__throw_system_errori@PLT
.LEHE169:
.L2107:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
	leaq	.LC3(%rip), %rdi
	leaq	64(%rsp), %r14
.LEHB170:
	call	_ZSt20__throw_length_errorPKc@PLT
.L2111:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
	leaq	.LC4(%rip), %rdi
	leaq	64(%rsp), %r14
	call	_ZSt20__throw_length_errorPKc@PLT
.L2095:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L2077
.L2054:
	xorl	%r9d, %r9d
	movq	%rbp, %r8
	movq	%r12, %rcx
	xorl	%edx, %edx
	movq	%r12, %rsi
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE15_M_replace_coldEPcmPKcmm@PLT
.LEHE170:
	movq	64(%rsp), %rbx
	jmp	.L2056
.L2092:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L2082
.L2094:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L2081
.L2096:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L2076
.L2093:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %rbp
	jmp	.L2079
.L2081:
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L2079
.L2113:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2104
.LEHB171:
	call	_Unwind_Resume@PLT
.LEHE171:
.L2085:
	addq	$488, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
	.cfi_endproc
.LFE17101:
	.section	.gcc_except_table
	.align 4
.LLSDA17101:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT17101-.LLSDATTD17101
.LLSDATTD17101:
	.byte	0x1
	.uleb128 .LLSDACSE17101-.LLSDACSB17101
.LLSDACSB17101:
	.uleb128 .LEHB165-.LFB17101
	.uleb128 .LEHE165-.LEHB165
	.uleb128 .L2093-.LFB17101
	.uleb128 0x3
	.uleb128 .LEHB166-.LFB17101
	.uleb128 .LEHE166-.LEHB166
	.uleb128 .L2095-.LFB17101
	.uleb128 0x3
	.uleb128 .LEHB167-.LFB17101
	.uleb128 .LEHE167-.LEHB167
	.uleb128 .L2094-.LFB17101
	.uleb128 0x3
	.uleb128 .LEHB168-.LFB17101
	.uleb128 .LEHE168-.LEHB168
	.uleb128 .L2096-.LFB17101
	.uleb128 0x3
	.uleb128 .LEHB169-.LFB17101
	.uleb128 .LEHE169-.LEHB169
	.uleb128 .L2092-.LFB17101
	.uleb128 0x1
	.uleb128 .LEHB170-.LFB17101
	.uleb128 .LEHE170-.LEHB170
	.uleb128 .L2096-.LFB17101
	.uleb128 0x3
	.uleb128 .LEHB171-.LFB17101
	.uleb128 .LEHE171-.LEHB171
	.uleb128 0
	.uleb128 0
.LLSDACSE17101:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT17101:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_:
.LFB16567:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16567
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L2115
.LEHB172:
	call	_ZTHN5debug9fLogErrorE@PLT
.L2115:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L2139
.L2116:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2137
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2139:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE172:
.LEHB173:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE173:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L2140
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L2141
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L2124
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L2123:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L2125
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2125:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB174:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_RA6_S1_S5_RA4_S1_RA25_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L2116
	.p2align 4,,10
	.p2align 3
.L2141:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L2124:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L2123
	.p2align 4,,10
	.p2align 3
.L2140:
	testq	%rdx, %rdx
	je	.L2119
	cmpq	$1, %rdx
	je	.L2142
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L2119:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L2123
.L2142:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L2119
.L2126:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L2127
.L2137:
	call	__stack_chk_fail@PLT
.L2129:
	endbr64
	movq	%rax, %rbx
	jmp	.L2126
.L2127:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE174:
	.cfi_endproc
.LFE16567:
	.section	.gcc_except_table
.LLSDA16567:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16567-.LLSDACSB16567
.LLSDACSB16567:
	.uleb128 .LEHB172-.LFB16567
	.uleb128 .LEHE172-.LEHB172
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB173-.LFB16567
	.uleb128 .LEHE173-.LEHB173
	.uleb128 .L2129-.LFB16567
	.uleb128 0
	.uleb128 .LEHB174-.LFB16567
	.uleb128 .LEHE174-.LEHB174
	.uleb128 0
	.uleb128 0
.LLSDACSE16567:
	.section	.text._ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA6_S1_S3_RA4_S1_RA25_S1_EEEbDpOT_
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_:
.LFB17472:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	leaq	8(%rdi), %rax
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$24, %rsp
	.cfi_def_cfa_offset 80
	cmpq	%rax, %rsi
	je	.L2170
	movq	8(%rdx), %r13
	movq	40(%rsi), %r14
	movq	%rsi, %rbx
	movq	(%rdx), %r15
	movq	32(%rsi), %rcx
	cmpq	%r14, %r13
	movq	%r14, %rdx
	cmovbe	%r13, %rdx
	testq	%rdx, %rdx
	je	.L2147
	movq	%rcx, %rsi
	movq	%r15, %rdi
	movq	%rdx, 8(%rsp)
	movq	%rcx, (%rsp)
	call	memcmp@PLT
	movq	(%rsp), %rcx
	movq	8(%rsp), %rdx
	testl	%eax, %eax
	jne	.L2168
	movq	%r13, %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2152
	cmpq	$-2147483648, %rax
	jl	.L2150
	testl	%eax, %eax
.L2168:
	js	.L2150
.L2152:
	movq	%r15, %rsi
	movq	%rcx, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L2155
.L2151:
	movq	%r14, %rax
	subq	%r13, %rax
	cmpq	$2147483647, %rax
	jg	.L2156
	cmpq	$-2147483648, %rax
	jl	.L2157
.L2155:
	testl	%eax, %eax
	js	.L2157
.L2156:
	movq	%rbx, %rax
	xorl	%edx, %edx
.L2162:
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2147:
	.cfi_restore_state
	movq	%r13, %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2151
	cmpq	$-2147483648, %rax
	jl	.L2150
	testl	%eax, %eax
	jns	.L2151
.L2150:
	movq	%rbx, %rax
	movq	%rbx, %rdx
	cmpq	%rbx, 24(%rbp)
	je	.L2162
	movq	%rbx, %rdi
	call	_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base@PLT
	movq	%r13, %rcx
	movq	%r15, %rdx
	movq	40(%rax), %rsi
	movq	32(%rax), %rdi
	movq	%rax, %r14
	call	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	testl	%eax, %eax
	js	.L2171
.L2145:
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%r12, %rsi
	movq	%rbp, %rdi
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_
	.p2align 4,,10
	.p2align 3
.L2170:
	.cfi_restore_state
	cmpq	$0, 40(%rdi)
	je	.L2145
	movq	32(%rdi), %rbx
	movq	8(%rdx), %rcx
	movq	(%rdx), %rdx
	movq	40(%rbx), %rsi
	movq	32(%rbx), %rdi
	call	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	testl	%eax, %eax
	jns	.L2145
.L2169:
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rdx
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2157:
	.cfi_restore_state
	cmpq	%rbx, 32(%rbp)
	je	.L2169
	movq	%rbx, %rdi
	call	_ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base@PLT
	movq	%r13, %rsi
	movq	%r15, %rdi
	movq	40(%rax), %rcx
	movq	32(%rax), %rdx
	movq	%rax, %r14
	call	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	testl	%eax, %eax
	jns	.L2145
	xorl	%eax, %eax
	cmpq	$0, 24(%rbx)
	cmovne	%r14, %rbx
	cmovne	%r14, %rax
	movq	%rbx, %rdx
	jmp	.L2162
	.p2align 4,,10
	.p2align 3
.L2171:
	xorl	%eax, %eax
	cmpq	$0, 24(%r14)
	cmovne	%rbx, %rax
	cmove	%r14, %rbx
	movq	%rbx, %rdx
	jmp	.L2162
	.cfi_endproc
.LFE17472:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_:
.LFB17476:
	.cfi_startproc
	endbr64
	cmpq	%rsi, %rdi
	je	.L2190
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	leaq	16(%rdi), %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$8, %rsp
	.cfi_def_cfa_offset 64
	movq	(%rdi), %r13
	movq	8(%rsi), %r12
	cmpq	%r13, %r15
	je	.L2184
	movq	16(%rdi), %r14
.L2174:
	cmpq	%r12, %r14
	jb	.L2193
	testq	%r12, %r12
	jne	.L2181
.L2182:
	movq	%r12, 8(%rbx)
	movb	$0, 0(%r13,%r12)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2193:
	.cfi_restore_state
	testq	%r12, %r12
	js	.L2194
	addq	%r14, %r14
	cmpq	%r14, %r12
	jb	.L2195
	movq	%r12, %rdi
	movq	%r12, %r14
	addq	$1, %rdi
	js	.L2178
.L2179:
	call	_Znwm@PLT
	movq	(%rbx), %rdi
	movq	%rax, %r13
	cmpq	%rdi, %r15
	je	.L2180
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2180:
	movq	%r13, (%rbx)
	movq	%r14, 16(%rbx)
.L2181:
	movq	0(%rbp), %rsi
	cmpq	$1, %r12
	je	.L2196
	movq	%r13, %rdi
	movq	%r12, %rdx
	call	memcpy@PLT
	movq	(%rbx), %r13
	jmp	.L2182
	.p2align 4,,10
	.p2align 3
.L2190:
	.cfi_def_cfa_offset 8
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	.cfi_restore 13
	.cfi_restore 14
	.cfi_restore 15
	ret
	.p2align 4,,10
	.p2align 3
.L2195:
	.cfi_def_cfa_offset 64
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	leaq	1(%r14), %rdi
	testq	%r14, %r14
	jns	.L2179
.L2178:
	call	_ZSt17__throw_bad_allocv@PLT
	.p2align 4,,10
	.p2align 3
.L2184:
	movl	$15, %r14d
	jmp	.L2174
	.p2align 4,,10
	.p2align 3
.L2196:
	movzbl	(%rsi), %eax
	movb	%al, 0(%r13)
	movq	(%rbx), %r13
	jmp	.L2182
.L2194:
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
	.cfi_endproc
.LFE17476:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_
	.type	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_, @function
_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_:
.LFB16579:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16579
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$488, %rsp
	.cfi_def_cfa_offset 544
	movq	544(%rsp), %rax
	movq	%rax, 8(%rsp)
	movq	552(%rsp), %rax
	movq	%rax, 16(%rsp)
	movq	560(%rsp), %rax
	movq	%rax, 24(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 472(%rsp)
	xorl	%eax, %eax
	movl	_ZN6config8nVerboseE(%rip), %eax
	cmpl	%edi, %eax
	jnb	.L2227
.L2197:
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2226
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2227:
	.cfi_restore_state
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rax
	movq	%rsi, %r15
	movq	%rdx, %r14
	movq	%rcx, %r13
	movq	%rax, %rdi
	movq	%rax, 48(%rsp)
	movq	%r8, %r12
	movq	%r9, %rbp
	call	pthread_mutex_lock@PLT
	testl	%eax, %eax
	jne	.L2228
	leaq	96(%rsp), %rbx
	movb	$1, 56(%rsp)
	movq	%rbx, %rdi
.LEHB175:
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEC1Ev@PLT
.LEHE175:
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
.LEHB176:
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r13, %rdi
	call	strlen@PLT
	movq	%r13, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%r12, %rdi
	call	strlen@PLT
	movq	%r12, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	%rbp, %rdi
	call	strlen@PLT
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	8(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	16(%rsp), %r15
	movq	%r15, %rdi
	call	strlen@PLT
	movq	%r15, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
	movq	24(%rsp), %r14
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%r14, %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	call	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l@PLT
.LEHE176:
	leaq	80(%rsp), %r12
	movb	$0, 80(%rsp)
	leaq	64(%rsp), %rbp
	movq	144(%rsp), %r8
	movq	%r12, 64(%rsp)
	movq	$0, 72(%rsp)
	testq	%r8, %r8
	je	.L2201
	movq	128(%rsp), %rax
	movq	%r8, %rdx
	movq	136(%rsp), %rcx
	movq	%rbp, %rdi
	cmpq	%r8, %rax
	cmovnb	%rax, %r8
	testq	%rax, %rax
	cmove	%rdx, %r8
	xorl	%edx, %edx
	xorl	%esi, %esi
	subq	%rcx, %r8
.LEHB177:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm.isra.0
.LEHE177:
.L2205:
	leaq	64+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	leaq	24+_ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rcx
	movq	176(%rsp), %rdi
	movq	%rax, 208(%rsp)
	leaq	16+_ZTVNSt7__cxx1115basic_stringbufIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	%rcx, %xmm0
	movq	%rax, %xmm1
	leaq	192(%rsp), %rax
	punpcklqdq	%xmm1, %xmm0
	movaps	%xmm0, 96(%rsp)
	cmpq	%rax, %rdi
	je	.L2203
	movq	192(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2203:
	leaq	16+_ZTVSt15basic_streambufIcSt11char_traitsIcEE(%rip), %rax
	leaq	160(%rsp), %rdi
	movq	%rax, 104(%rsp)
	call	_ZNSt6localeD1Ev@PLT
	movq	8+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rax
	movq	16+_ZTTNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE(%rip), %rdx
	leaq	208(%rsp), %rdi
	movq	%rax, 96(%rsp)
	movq	-24(%rax), %rax
	movq	%rdx, 96(%rsp,%rax)
	leaq	16+_ZTVSt9basic_iosIcSt11char_traitsIcEE(%rip), %rax
	movq	%rax, 208(%rsp)
	call	_ZNSt8ios_baseD2Ev@PLT
	xorl	%edi, %edi
	call	time@PLT
	leaq	40(%rsp), %rdi
	movq	%rbp, %rsi
	movq	%rax, 40(%rsp)
.LEHB178:
	call	_ZN5debug4_logERKlRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.LEHE178:
	movq	64(%rsp), %rdi
	cmpq	%r12, %rdi
	je	.L2210
	movq	80(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2210:
	leaq	_ZN5debug11DEBUG_MUTEXE(%rip), %rdi
	call	pthread_mutex_unlock@PLT
	jmp	.L2197
	.p2align 4,,10
	.p2align 3
.L2201:
	leaq	176(%rsp), %rsi
	movq	%rbp, %rdi
.LEHB179:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_assignERKS4_
.LEHE179:
	jmp	.L2205
.L2211:
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L2209:
	leaq	48(%rsp), %rdi
	movq	%r12, %rbp
	call	_ZNSt11unique_lockISt5mutexE6unlockEv.constprop.0
	movq	%rbx, %rdi
.L2212:
	subq	$1, %rbp
	je	.L2213
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	je	.L2214
.L2226:
	call	__stack_chk_fail@PLT
.L2228:
	movq	472(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L2226
	movl	%eax, %edi
.LEHB180:
	call	_ZSt20__throw_system_errori@PLT
.LEHE180:
.L2221:
	endbr64
	movq	%rax, %r12
	movq	%rdx, %r13
	jmp	.L2206
.L2219:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %r12
	jmp	.L2211
.L2220:
	endbr64
	movq	%rax, %rbp
	movq	%rdx, %r12
	jmp	.L2207
.L2218:
	endbr64
	movq	%rax, %rbx
	movq	%rdx, %r12
	jmp	.L2209
.L2217:
	endbr64
	movq	%rax, %rdi
	movq	%rdx, %rbp
	jmp	.L2212
.L2206:
	movq	%rbp, %rdi
	movq	%r12, %rbp
	movq	%r13, %r12
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
.L2207:
	movq	%rbx, %rdi
	movq	%rbp, %rbx
	call	_ZNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEED1Ev@PLT
	jmp	.L2209
.L2213:
	call	__cxa_begin_catch@PLT
	movq	472(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2226
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_end_catch@PLT
.L2214:
	.cfi_restore_state
.LEHB181:
	call	_Unwind_Resume@PLT
.LEHE181:
	.cfi_endproc
.LFE16579:
	.section	.gcc_except_table
	.align 4
.LLSDA16579:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT16579-.LLSDATTD16579
.LLSDATTD16579:
	.byte	0x1
	.uleb128 .LLSDACSE16579-.LLSDACSB16579
.LLSDACSB16579:
	.uleb128 .LEHB175-.LFB16579
	.uleb128 .LEHE175-.LEHB175
	.uleb128 .L2218-.LFB16579
	.uleb128 0x3
	.uleb128 .LEHB176-.LFB16579
	.uleb128 .LEHE176-.LEHB176
	.uleb128 .L2220-.LFB16579
	.uleb128 0x3
	.uleb128 .LEHB177-.LFB16579
	.uleb128 .LEHE177-.LEHB177
	.uleb128 .L2221-.LFB16579
	.uleb128 0x3
	.uleb128 .LEHB178-.LFB16579
	.uleb128 .LEHE178-.LEHB178
	.uleb128 .L2219-.LFB16579
	.uleb128 0x3
	.uleb128 .LEHB179-.LFB16579
	.uleb128 .LEHE179-.LEHB179
	.uleb128 .L2221-.LFB16579
	.uleb128 0x3
	.uleb128 .LEHB180-.LFB16579
	.uleb128 .LEHE180-.LEHB180
	.uleb128 .L2217-.LFB16579
	.uleb128 0x1
	.uleb128 .LEHB181-.LFB16579
	.uleb128 .LEHE181-.LEHB181
	.uleb128 0
	.uleb128 0
.LLSDACSE16579:
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x7d
	.align 4
	.long	DW.ref._ZTISt9exception-.
.LLSDATT16579:
	.section	.text._ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_,"axG",@progbits,_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_,comdat
	.size	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_, .-_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_,comdat
	.p2align 4
	.weak	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_
	.type	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_, @function
_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_:
.LFB16020:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA16020
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%r8, %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rcx, %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	cmpq	$0, _ZTHN5debug9fLogErrorE@GOTPCREL(%rip)
	je	.L2230
.LEHB182:
	call	_ZTHN5debug9fLogErrorE@PLT
.L2230:
	movq	_ZN5debug9fLogErrorE@gottpoff(%rip), %rax
	cmpb	$0, %fs:(%rax)
	jne	.L2254
.L2231:
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L2252
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2254:
	.cfi_restore_state
	leaq	16(%rsp), %r15
	movq	%r14, %r9
	movq	%r13, %r8
	movq	%r12, %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	movq	%r15, %rdi
	call	_ZN5debug13safe_printstrIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEDpOT_
.LEHE182:
.LEHB183:
	call	_ZTHN5debug12strLastErrorB5cxx11E@PLT
.LEHE183:
	movq	16(%rsp), %rsi
	leaq	32(%rsp), %rcx
	movq	24(%rsp), %rdx
	movq	_ZN5debug12strLastErrorB5cxx11E@gottpoff(%rip), %r15
	movq	%fs:(%r15), %rdi
	cmpq	%rcx, %rsi
	je	.L2255
	movq	%fs:0, %rax
	movq	%rdx, %xmm0
	movhps	32(%rsp), %xmm0
	addq	%r15, %rax
	addq	$16, %rax
	cmpq	%rax, %rdi
	je	.L2256
	movq	%fs:16(%r15), %rax
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
	testq	%rdi, %rdi
	je	.L2239
	movq	%rdi, 16(%rsp)
	movq	%rax, 32(%rsp)
.L2238:
	movq	$0, 24(%rsp)
	movb	$0, (%rdi)
	movq	16(%rsp), %rdi
	cmpq	%rcx, %rdi
	je	.L2240
	movq	32(%rsp), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2240:
	subq	$8, %rsp
	.cfi_def_cfa_offset 136
	movq	%rbp, %r9
	movq	%rbx, %r8
	xorl	%edi, %edi
	pushq	%r14
	.cfi_def_cfa_offset 144
	leaq	.LC9(%rip), %rcx
	leaq	.LC12(%rip), %rdx
	pushq	%r13
	.cfi_def_cfa_offset 152
	leaq	.LC13(%rip), %rsi
	pushq	%r12
	.cfi_def_cfa_offset 160
.LEHB184:
	call	_ZN5debug3logIJRA8_KcS3_RA5_S1_S5_S3_S5_RA4_S1_RA23_S1_EEEvjDpOT_
	addq	$32, %rsp
	.cfi_def_cfa_offset 128
	jmp	.L2231
	.p2align 4,,10
	.p2align 3
.L2256:
	movq	%rsi, %fs:(%r15)
	movups	%xmm0, %fs:8(%r15)
.L2239:
	movq	%rcx, 16(%rsp)
	leaq	32(%rsp), %rcx
	movq	%rcx, %rdi
	jmp	.L2238
	.p2align 4,,10
	.p2align 3
.L2255:
	testq	%rdx, %rdx
	je	.L2234
	cmpq	$1, %rdx
	je	.L2257
	movq	%rcx, %rsi
	movq	%rcx, 8(%rsp)
	call	memcpy@PLT
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	movq	8(%rsp), %rcx
.L2234:
	movq	%rdx, %fs:8(%r15)
	movb	$0, (%rdi,%rdx)
	movq	16(%rsp), %rdi
	jmp	.L2238
.L2257:
	movzbl	32(%rsp), %eax
	movb	%al, (%rdi)
	movq	24(%rsp), %rdx
	movq	%fs:(%r15), %rdi
	jmp	.L2234
.L2241:
	movq	%r15, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	je	.L2242
.L2252:
	call	__stack_chk_fail@PLT
.L2244:
	endbr64
	movq	%rax, %rbx
	jmp	.L2241
.L2242:
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE184:
	.cfi_endproc
.LFE16020:
	.section	.gcc_except_table
.LLSDA16020:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE16020-.LLSDACSB16020
.LLSDACSB16020:
	.uleb128 .LEHB182-.LFB16020
	.uleb128 .LEHE182-.LEHB182
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB183-.LFB16020
	.uleb128 .LEHE183-.LEHB183
	.uleb128 .L2244-.LFB16020
	.uleb128 0
	.uleb128 .LEHB184-.LFB16020
	.uleb128 .LEHE184-.LEHB184
	.uleb128 0
	.uleb128 0
.LLSDACSE16020:
	.section	.text._ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_,"axG",@progbits,_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_,comdat
	.size	_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_, .-_ZN5debug5errorIJRA5_KcRA8_S1_S3_RA4_S1_RA23_S1_EEEbDpOT_
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_:
.LFB17802:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	16(%rdi), %r15
	movq	%rdi, 24(%rsp)
	movq	%rsi, 16(%rsp)
	testq	%r15, %r15
	je	.L2285
	movq	(%rsi), %rdx
	movq	8(%rsi), %r14
	movl	$2147483648, %ebp
	movq	%rdx, 8(%rsp)
	jmp	.L2266
	.p2align 4,,10
	.p2align 3
.L2264:
	movq	16(%r15), %rax
	movl	$1, %esi
	testq	%rax, %rax
	je	.L2286
.L2271:
	movq	%rax, %r15
.L2266:
	movq	40(%r15), %r12
	movq	32(%r15), %r13
	cmpq	%r12, %r14
	movq	%r12, %rbx
	cmovbe	%r14, %rbx
	testq	%rbx, %rbx
	je	.L2261
	movq	8(%rsp), %rdi
	movq	%rbx, %rdx
	movq	%r13, %rsi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L2262
.L2261:
	movq	%r14, %rax
	subq	%r12, %rax
	cmpq	%rbp, %rax
	jge	.L2263
	movabsq	$-2147483649, %rcx
	cmpq	%rcx, %rax
	jle	.L2264
.L2262:
	testl	%eax, %eax
	js	.L2264
.L2263:
	movq	24(%r15), %rax
	xorl	%esi, %esi
	testq	%rax, %rax
	jne	.L2271
.L2286:
	movq	%r15, %rbp
	testb	%sil, %sil
	jne	.L2260
.L2267:
	testq	%rbx, %rbx
	je	.L2269
	movq	8(%rsp), %rsi
	movq	%rbx, %rdx
	movq	%r13, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	je	.L2269
.L2270:
	xorl	%edx, %edx
	testl	%eax, %eax
	cmovns	%rdx, %rbp
	cmovs	%rdx, %r15
.L2268:
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%r15, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	movq	%rbp, %rdx
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2269:
	.cfi_restore_state
	movq	%r12, %rcx
	subq	%r14, %rcx
	cmpq	$2147483647, %rcx
	jg	.L2274
	cmpq	$-2147483648, %rcx
	jl	.L2275
	movl	%ecx, %eax
	jmp	.L2270
	.p2align 4,,10
	.p2align 3
.L2285:
	leaq	8(%rdi), %r15
.L2260:
	movq	24(%rsp), %rax
	cmpq	%r15, 24(%rax)
	je	.L2273
	movq	%r15, %rdi
	movq	%r15, %rbp
	call	_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base@PLT
	movq	16(%rsp), %rdx
	movq	40(%rax), %r12
	movq	32(%rax), %r13
	movq	%rax, %r15
	movq	8(%rdx), %r14
	movq	(%rdx), %rcx
	movq	%r12, %rbx
	cmpq	%r12, %r14
	movq	%rcx, 8(%rsp)
	cmovbe	%r14, %rbx
	jmp	.L2267
	.p2align 4,,10
	.p2align 3
.L2273:
	movq	%r15, %rbp
	xorl	%r15d, %r15d
	jmp	.L2268
	.p2align 4,,10
	.p2align 3
.L2274:
	xorl	%ebp, %ebp
	jmp	.L2268
.L2275:
	xorl	%r15d, %r15d
	jmp	.L2268
	.cfi_endproc
.LFE17802:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_
	.section	.text._ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_,"axG",@progbits,_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC5ESt16initializer_listISB_ERKS9_RKSC_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_
	.type	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_, @function
_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_:
.LFB15944:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA15944
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	leaq	(%rdx,%rdx,8), %rax
	leaq	8(%rdi), %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	leaq	(%rsi,%rax,8), %rax
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movl	$0, 8(%rdi)
	movq	$0, 16(%rdi)
	movq	%r15, 24(%rdi)
	movq	%r15, 32(%rdi)
	movq	$0, 40(%rdi)
	movq	%rax, 16(%rsp)
	cmpq	%rsi, %rax
	je	.L2287
	movq	%rdi, %r13
	movq	%rsi, %rbx
	xorl	%ebp, %ebp
	.p2align 4,,10
	.p2align 3
.L2288:
	testq	%rbp, %rbp
	je	.L2289
	movq	32(%r13), %r12
	movq	8(%rbx), %r14
	movq	(%rbx), %rsi
	movq	40(%r12), %rcx
	movq	%r14, %rdx
	movq	32(%r12), %rdi
	cmpq	%r14, %rcx
	cmovbe	%rcx, %rdx
	testq	%rdx, %rdx
	je	.L2345
	movq	%rcx, 8(%rsp)
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L2294
	movq	8(%rsp), %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2289
	cmpq	$-2147483648, %rax
	jl	.L2320
.L2294:
	testl	%eax, %eax
	js	.L2320
	.p2align 4,,10
	.p2align 3
.L2289:
	movq	%rbx, %rsi
	movq	%r13, %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_
	movq	%rdx, %r12
	testq	%rdx, %rdx
	je	.L2296
.L2295:
	testq	%rax, %rax
	setne	%dl
	cmpq	%r12, %r15
	sete	%al
	orb	%al, %dl
	movb	%dl, 8(%rsp)
	je	.L2346
.L2292:
	movl	$104, %edi
.LEHB185:
	call	_Znwm@PLT
.LEHE185:
	leaq	48(%rax), %rdi
	movq	%rax, %rbp
	movq	8(%rbx), %r14
	movq	%rdi, 32(%rax)
	movq	(%rbx), %rax
	movq	%rax, 24(%rsp)
	cmpq	$15, %r14
	ja	.L2347
	cmpq	$1, %r14
	je	.L2348
	testq	%r14, %r14
	jne	.L2301
.L2303:
	movq	%r14, 40(%rbp)
	movb	$0, (%rdi,%r14)
	movq	32(%rbx), %rax
	leaq	80(%rbp), %rdi
	movq	40(%rbx), %r14
	movq	%rdi, 64(%rbp)
	movq	%rax, 24(%rsp)
	cmpq	$15, %r14
	ja	.L2349
	cmpq	$1, %r14
	je	.L2350
	testq	%r14, %r14
	jne	.L2308
.L2310:
	movq	%r14, 72(%rbp)
	movq	%rbp, %rsi
	movq	%r15, %rcx
	movq	%r12, %rdx
	movb	$0, (%rdi,%r14)
	movzbl	64(%rbx), %eax
	movzbl	8(%rsp), %edi
	movb	%al, 96(%rbp)
	call	_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_@PLT
	movq	40(%r13), %rax
	leaq	1(%rax), %rbp
	movq	%rbp, 40(%r13)
.L2296:
	movq	16(%rsp), %rax
	addq	$72, %rbx
	cmpq	%rax, %rbx
	jne	.L2288
.L2287:
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2347:
	.cfi_restore_state
	testq	%r14, %r14
	js	.L2351
	movq	%r14, %rdi
	addq	$1, %rdi
	js	.L2352
.LEHB186:
	call	_Znwm@PLT
.LEHE186:
	movq	%rax, 32(%rbp)
	movq	%rax, %rdi
	movq	%r14, 48(%rbp)
.L2301:
	movq	24(%rsp), %rsi
	movq	%r14, %rdx
	call	memcpy@PLT
	movq	32(%rbp), %rdi
	jmp	.L2303
	.p2align 4,,10
	.p2align 3
.L2348:
	movzbl	(%rax), %eax
	movb	%al, 48(%rbp)
	jmp	.L2303
	.p2align 4,,10
	.p2align 3
.L2349:
	testq	%r14, %r14
	js	.L2353
	movq	%r14, %rdi
	addq	$1, %rdi
	js	.L2354
.LEHB187:
	call	_Znwm@PLT
.LEHE187:
	movq	%rax, 64(%rbp)
	movq	%rax, %rdi
	movq	%r14, 80(%rbp)
.L2308:
	movq	24(%rsp), %rsi
	movq	%r14, %rdx
	call	memcpy@PLT
	movq	64(%rbp), %rdi
	jmp	.L2310
	.p2align 4,,10
	.p2align 3
.L2350:
	movzbl	(%rax), %eax
	movb	%al, 80(%rbp)
	jmp	.L2310
	.p2align 4,,10
	.p2align 3
.L2346:
	movq	8(%rbx), %r14
	movq	40(%r12), %rcx
	movq	(%rbx), %rdi
	movq	32(%r12), %rsi
	cmpq	%rcx, %r14
	movq	%rcx, %rdx
	cmovbe	%r14, %rdx
	testq	%rdx, %rdx
	je	.L2293
	movq	%rcx, 8(%rsp)
	call	memcmp@PLT
	movq	8(%rsp), %rcx
	testl	%eax, %eax
	jne	.L2297
.L2293:
	subq	%rcx, %r14
	movb	$0, 8(%rsp)
	cmpq	$2147483647, %r14
	jg	.L2292
	cmpq	$-2147483648, %r14
	jl	.L2322
	movl	%r14d, %eax
.L2297:
	shrl	$31, %eax
	movl	%eax, 8(%rsp)
	jmp	.L2292
	.p2align 4,,10
	.p2align 3
.L2345:
	movq	%rcx, %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2289
	cmpq	$-2147483648, %rax
	jge	.L2294
	movb	$1, 8(%rsp)
	cmpq	%r12, %r15
	je	.L2292
	jmp	.L2293
	.p2align 4,,10
	.p2align 3
.L2320:
	xorl	%eax, %eax
	jmp	.L2295
	.p2align 4,,10
	.p2align 3
.L2352:
.LEHB188:
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE188:
	.p2align 4,,10
	.p2align 3
.L2354:
.LEHB189:
	call	_ZSt17__throw_bad_allocv@PLT
.L2322:
	movb	$1, 8(%rsp)
	jmp	.L2292
.L2353:
	leaq	.LC4(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE189:
.L2351:
	leaq	.LC4(%rip), %rdi
.LEHB190:
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE190:
.L2323:
	endbr64
	movq	%rax, %rbx
	jmp	.L2315
.L2326:
	endbr64
	movq	%rax, %rbx
	jmp	.L2312
.L2324:
	endbr64
	movq	%rax, %rdi
	jmp	.L2313
.L2314:
	call	__cxa_end_catch@PLT
.L2315:
	movq	16(%r13), %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_IS5_bEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE8_M_eraseEPSt13_Rb_tree_nodeIS9_E.isra.0
	movq	%rbx, %rdi
.LEHB191:
	call	_Unwind_Resume@PLT
.LEHE191:
.L2312:
	leaq	32(%rbp), %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbx, %rdi
.L2313:
	call	__cxa_begin_catch@PLT
	movl	$104, %esi
	movq	%rbp, %rdi
	call	_ZdlPvm@PLT
.LEHB192:
	call	__cxa_rethrow@PLT
.LEHE192:
.L2325:
	endbr64
	movq	%rax, %rbx
	jmp	.L2314
	.cfi_endproc
.LFE15944:
	.section	.gcc_except_table
	.align 4
.LLSDA15944:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT15944-.LLSDATTD15944
.LLSDATTD15944:
	.byte	0x1
	.uleb128 .LLSDACSE15944-.LLSDACSB15944
.LLSDACSB15944:
	.uleb128 .LEHB185-.LFB15944
	.uleb128 .LEHE185-.LEHB185
	.uleb128 .L2323-.LFB15944
	.uleb128 0
	.uleb128 .LEHB186-.LFB15944
	.uleb128 .LEHE186-.LEHB186
	.uleb128 .L2324-.LFB15944
	.uleb128 0x1
	.uleb128 .LEHB187-.LFB15944
	.uleb128 .LEHE187-.LEHB187
	.uleb128 .L2326-.LFB15944
	.uleb128 0x1
	.uleb128 .LEHB188-.LFB15944
	.uleb128 .LEHE188-.LEHB188
	.uleb128 .L2324-.LFB15944
	.uleb128 0x1
	.uleb128 .LEHB189-.LFB15944
	.uleb128 .LEHE189-.LEHB189
	.uleb128 .L2326-.LFB15944
	.uleb128 0x1
	.uleb128 .LEHB190-.LFB15944
	.uleb128 .LEHE190-.LEHB190
	.uleb128 .L2324-.LFB15944
	.uleb128 0x1
	.uleb128 .LEHB191-.LFB15944
	.uleb128 .LEHE191-.LEHB191
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB192-.LFB15944
	.uleb128 .LEHE192-.LEHB192
	.uleb128 .L2325-.LFB15944
	.uleb128 0
.LLSDACSE15944:
	.byte	0x1
	.byte	0
	.align 4
	.long	0

.LLSDATT15944:
	.section	.text._ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_,"axG",@progbits,_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC5ESt16initializer_listISB_ERKS9_RKSC_,comdat
	.size	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_, .-_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_
	.weak	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC1ESt16initializer_listISB_ERKS9_RKSC_
	.set	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC1ESt16initializer_listISB_ERKS9_RKSC_,_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC2ESt16initializer_listISB_ERKS9_RKSC_
	.section	.rodata.str1.1
.LC21:
	.string	"NXS"
.LC22:
	.string	"nxs"
.LC23:
	.string	"NEXUS"
.LC24:
	.string	"nexus"
.LC25:
	.string	"uint64"
.LC26:
	.string	"balance"
.LC27:
	.string	"uint256"
.LC28:
	.string	"token"
.LC29:
	.string	"disabled"
.LC30:
	.string	"require"
.LC31:
	.string	"supply"
.LC32:
	.string	"trust"
.LC33:
	.string	"stake"
.LC34:
	.string	"system"
.LC35:
	.string	"nexus.io"
.LC36:
	.string	"nexus.com"
.LC37:
	.string	"nexusearth.com"
	.section	.text.unlikely
.LCOLDB39:
	.section	.text.startup
.LHOTB39:
	.p2align 4
	.type	_Z41__static_initialization_and_destruction_0v, @function
_Z41__static_initialization_and_destruction_0v:
.LFB18256:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA18256
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	leaq	.LC21(%rip), %rax
	leaq	__dso_handle(%rip), %rdx
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	movq	%rax, %xmm1
	leaq	_ZN3LLDL13DATABASE_NAMEE(%rip), %rsi
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	leaq	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev(%rip), %rbx
	movq	%rbx, %rdi
	subq	$808, %rsp
	.cfi_def_cfa_offset 864
	movhps	.LC38(%rip), %xmm1
	leaq	144(%rsp), %r12
	movaps	%xmm1, (%rsp)
	call	__cxa_atexit@PLT
	movq	%rbx, %rdi
	leaq	__dso_handle(%rip), %rdx
	leaq	_ZN3TAO8Register9NAMESPACEL6GLOBALE(%rip), %rsi
	call	__cxa_atexit@PLT
	movq	%rbx, %rdi
	leaq	__dso_handle(%rip), %rdx
	leaq	_ZN3LLPL15strProtocolNameE(%rip), %rsi
	call	__cxa_atexit@PLT
	leaq	.LC25(%rip), %rbx
	leaq	32(%rsp), %rdx
	movq	%r12, %rdi
	leaq	.LC26(%rip), %rsi
	movq	%rbx, 32(%rsp)
	movq	$1, 40(%rsp)
.LEHB193:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_
.LEHE193:
	leaq	.LC27(%rip), %rax
	leaq	48(%rsp), %rdx
	movq	$0, 56(%rsp)
	leaq	216(%rsp), %rdi
	leaq	.LC28(%rip), %rsi
	movq	%rax, 48(%rsp)
.LEHB194:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_
.LEHE194:
	leaq	.LC29(%rip), %rbp
	leaq	64(%rsp), %rdx
	movq	$0, 72(%rsp)
	leaq	288(%rsp), %rdi
	leaq	.LC30(%rip), %rsi
	movq	%rbp, 64(%rsp)
.LEHB195:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA8_KcS_IPSA_bEEEOT_OT0_
.LEHE195:
	leaq	80(%rsp), %rdx
	leaq	.LC31(%rip), %rsi
	movq	$0, 88(%rsp)
	leaq	360(%rsp), %rdi
	movq	%rbx, 80(%rsp)
.LEHB196:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_
.LEHE196:
	leaq	448(%rsp), %rax
	leaq	96(%rsp), %rdx
	movq	$8, 440(%rsp)
	movq	%rax, 432(%rsp)
	leaq	504(%rsp), %rdi
	movabsq	$8317129734393914724, %rax
	leaq	.LC32(%rip), %rsi
	movq	%rax, 448(%rsp)
	leaq	480(%rsp), %rax
	movb	$0, 456(%rsp)
	movq	%rax, 464(%rsp)
	movl	$1953393013, 480(%rsp)
	movb	$56, 484(%rsp)
	movq	$5, 472(%rsp)
	movb	$0, 485(%rsp)
	movb	$0, 496(%rsp)
	movq	%rbx, 96(%rsp)
	movq	$1, 104(%rsp)
.LEHB197:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_
.LEHE197:
	leaq	112(%rsp), %rdx
	leaq	576(%rsp), %rdi
	movq	%rbx, 112(%rsp)
	movq	$1, 120(%rsp)
	leaq	.LC33(%rip), %rsi
.LEHB198:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA6_KcS_IPSA_bEEEOT_OT0_
.LEHE198:
	leaq	128(%rsp), %rdx
	leaq	648(%rsp), %rdi
	movq	$0, 136(%rsp)
	leaq	.LC34(%rip), %rsi
	movq	%rbp, 128(%rsp)
.LEHB199:
	call	_ZNSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES_IS5_bEEC1IRA7_KcS_IPSA_bEEEOT_OT0_
.LEHE199:
	movl	$26478, %esi
	leaq	30(%rsp), %rcx
	leaq	31(%rsp), %r8
	movl	$9, %edx
	leaq	736(%rsp), %rax
	leaq	_ZN3TAO8RegisterL8RESERVEDE(%rip), %rbp
	movw	%si, 772(%rsp)
	movq	%r12, %rsi
	movq	%rax, 720(%rsp)
	movq	%rbp, %rdi
	leaq	768(%rsp), %rax
	movl	$1701667182, 736(%rsp)
	movq	$4, 728(%rsp)
	movb	$0, 740(%rsp)
	movq	%rax, 752(%rsp)
	movl	$1769108595, 768(%rsp)
	movq	$6, 760(%rsp)
	movb	$0, 774(%rsp)
	movb	$0, 784(%rsp)
.LEHB200:
	call	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEEC1ESt16initializer_listISB_ERKS9_RKSC_
.LEHE200:
	leaq	720(%rsp), %rbx
	jmp	.L2378
	.p2align 4,,10
	.p2align 3
.L2474:
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	leaq	-72(%rbx), %rax
	cmpq	%rbx, %r12
	je	.L2376
.L2377:
	movq	%rax, %rbx
.L2378:
	movq	32(%rbx), %rdi
	leaq	48(%rbx), %rax
	cmpq	%rax, %rdi
	je	.L2374
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2374:
	movq	(%rbx), %rdi
	leaq	16(%rbx), %rax
	cmpq	%rax, %rdi
	jne	.L2474
	leaq	-72(%rbx), %rax
	cmpq	%rbx, %r12
	jne	.L2377
	.p2align 4,,10
	.p2align 3
.L2376:
	leaq	__dso_handle(%rip), %rdx
	movq	%rbp, %rsi
	leaq	_ZNSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIS5_bESt4lessIS5_ESaIS6_IKS5_S7_EEED1Ev(%rip), %rdi
	movq	%r12, %r13
	call	__cxa_atexit@PLT
	leaq	.LC24(%rip), %rax
	movdqa	(%rsp), %xmm2
	leaq	.LC23(%rip), %rcx
	movq	%rax, %xmm3
	movq	%rcx, %xmm0
	leaq	8+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rax
	movl	$0, 8+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
	movq	$0, 16+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
	punpcklqdq	%xmm3, %xmm0
	movq	%rax, 24+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
	movq	%rax, 32+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
	movq	$0, 40+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
	movaps	%xmm2, 144(%rsp)
	movaps	%xmm0, 160(%rsp)
.L2393:
	movl	$64, %edi
.LEHB201:
	call	_Znwm@PLT
.LEHE201:
	movq	0(%r13), %r14
	leaq	48(%rax), %r15
	movq	%rax, %rbx
	movq	%r15, 32(%rax)
	testq	%r14, %r14
	je	.L2475
	movq	%r14, %rdi
	call	strlen@PLT
	movq	%rax, %rbp
	cmpq	$15, %rax
	ja	.L2476
	cmpq	$1, %rax
	je	.L2477
	testq	%rax, %rax
	jne	.L2478
.L2384:
	movq	%r15, %rax
.L2383:
	movq	%rbp, 40(%rbx)
	leaq	32(%rbx), %rsi
	leaq	_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rdi
	movb	$0, (%rax,%rbp)
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE24_M_get_insert_unique_posERKS5_
	movq	%rdx, %rbp
	testq	%rdx, %rdx
	je	.L2479
	leaq	8+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rcx
	cmpq	%rdx, %rcx
	je	.L2427
	testq	%rax, %rax
	je	.L2480
.L2427:
	movl	$1, %edi
.L2390:
	leaq	8+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rcx
	movq	%rbp, %rdx
	movq	%rbx, %rsi
	call	_ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_@PLT
	addq	$1, 40+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip)
.L2391:
	addq	$8, %r13
	leaq	176(%rsp), %rbx
	cmpq	%rbx, %r13
	jne	.L2393
	leaq	_ZNSt3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_EED1Ev(%rip), %r15
	leaq	__dso_handle(%rip), %rdx
	leaq	_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rsi
	movq	%r15, %rdi
	call	__cxa_atexit@PLT
	movq	8+_ZN3TAO8Register9NAMESPACEL6GLOBALE(%rip), %rbp
	leaq	160(%rsp), %rax
	movq	_ZN3TAO8Register9NAMESPACEL6GLOBALE(%rip), %r13
	movq	%rax, 144(%rsp)
	cmpq	$15, %rbp
	ja	.L2481
	cmpq	$1, %rbp
	jne	.L2404
	movzbl	0(%r13), %eax
	movb	%al, 160(%rsp)
.L2405:
	movq	144(%rsp), %rax
	movq	%rbp, 152(%rsp)
	leaq	.LC22(%rip), %rsi
	movq	%rbx, %rdi
	movb	$0, (%rax,%rbp)
.LEHB202:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0
.LEHE202:
	leaq	208(%rsp), %rdi
	leaq	.LC35(%rip), %rsi
.LEHB203:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0
.LEHE203:
	leaq	240(%rsp), %rdi
	leaq	.LC36(%rip), %rsi
.LEHB204:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0
.LEHE204:
	leaq	272(%rsp), %rdi
	leaq	.LC37(%rip), %rsi
.LEHB205:
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_.isra.0
.LEHE205:
	leaq	8+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip), %r13
	xorl	%eax, %eax
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	movl	%eax, 8+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip)
	movq	%r12, %rbx
	leaq	-8(%r13), %rbp
	leaq	304(%rsp), %r14
	movq	%rdx, 16+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip)
	movq	%r13, 24+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip)
	movq	%r13, 32+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip)
	movq	%rcx, 40+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip)
	.p2align 4,,10
	.p2align 3
.L2415:
	movq	%rbx, %rdx
	movq	%r13, %rsi
	movq	%rbp, %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS5_ERKS5_
	testq	%rdx, %rdx
	je	.L2417
	movq	%rbx, %rcx
	movq	%rax, %rsi
	movq	%rbp, %rdi
.LEHB206:
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE10_M_insert_IRKS5_NSB_11_Alloc_nodeEEESt17_Rb_tree_iteratorIS5_EPSt18_Rb_tree_node_baseSJ_OT_RT0_.isra.0
.LEHE206:
.L2417:
	addq	$32, %rbx
	cmpq	%r14, %rbx
	jne	.L2415
	.p2align 4,,10
	.p2align 3
.L2416:
	subq	$32, %rbx
	movq	(%rbx), %rdi
	leaq	16(%rbx), %rax
	cmpq	%rax, %rdi
	je	.L2420
	movq	16(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
	cmpq	%r12, %rbx
	jne	.L2416
.L2421:
	addq	$808, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbp, %rsi
	movq	%r15, %rdi
	popq	%rbx
	.cfi_def_cfa_offset 48
	leaq	__dso_handle(%rip), %rdx
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	__cxa_atexit@PLT
	.p2align 4,,10
	.p2align 3
.L2479:
	.cfi_restore_state
	movq	32(%rbx), %rdi
	cmpq	%rdi, %r15
	je	.L2392
	movq	48(%rbx), %rax
	leaq	1(%rax), %rsi
	call	_ZdlPvm@PLT
.L2392:
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	jmp	.L2391
	.p2align 4,,10
	.p2align 3
.L2476:
	leaq	1(%rax), %rdi
.LEHB207:
	call	_Znwm@PLT
.LEHE207:
	movq	%rax, 32(%rbx)
	movq	%rax, %rdi
	movq	%rbp, 48(%rbx)
.L2381:
	movq	%rbp, %rdx
	movq	%r14, %rsi
	call	memcpy@PLT
	movq	32(%rbx), %rax
	jmp	.L2383
	.p2align 4,,10
	.p2align 3
.L2477:
	movzbl	(%r14), %eax
	movb	%al, 48(%rbx)
	jmp	.L2384
.L2480:
	movq	40(%rdx), %rcx
	movq	32(%rbx), %rdi
	movq	32(%rdx), %rdx
	movq	40(%rbx), %rsi
	call	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	shrl	$31, %eax
	movl	%eax, %edi
	jmp	.L2390
.L2404:
	testq	%rbp, %rbp
	je	.L2405
	movq	%rax, %rdi
	jmp	.L2403
.L2420:
	cmpq	%r12, %rbx
	jne	.L2416
	jmp	.L2421
.L2481:
	testq	%rbp, %rbp
	js	.L2482
	movq	%rbp, %rdi
	addq	$1, %rdi
	js	.L2483
.LEHB208:
	call	_Znwm@PLT
.LEHE208:
	movq	%rax, 144(%rsp)
	movq	%rax, %rdi
	movq	%rbp, 160(%rsp)
.L2403:
	movq	%rbp, %rdx
	movq	%r13, %rsi
	call	memcpy@PLT
	jmp	.L2405
.L2478:
	movq	%r15, %rdi
	jmp	.L2381
.L2483:
.LEHB209:
	call	_ZSt17__throw_bad_allocv@PLT
.LEHE209:
.L2475:
	leaq	.LC0(%rip), %rdi
.LEHB210:
	call	_ZSt19__throw_logic_errorPKc@PLT
.LEHE210:
.L2482:
	leaq	.LC4(%rip), %rdi
.LEHB211:
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE211:
.L2432:
	endbr64
	movq	%rax, %rbx
	jmp	.L2418
.L2446:
	endbr64
	movq	%rax, %rbx
	jmp	.L2473
.L2444:
	endbr64
	movq	%rax, %rbx
	jmp	.L2413
.L2441:
	endbr64
	movq	%rax, %rbx
	jmp	.L2409
.L2436:
	endbr64
	movq	%rax, %rbx
	jmp	.L2360
.L2439:
	endbr64
	movq	%rax, %rbx
	jmp	.L2368
.L2445:
	endbr64
	movq	%rax, %rbx
	jmp	.L2473
.L2428:
	endbr64
	movq	%rax, %rbx
	jmp	.L2372
.L2437:
	endbr64
	movq	%rax, %rbx
	jmp	.L2370
.L2443:
	endbr64
	movq	%rax, %rbx
	jmp	.L2407
.L2438:
	endbr64
	movq	%rax, %rbx
	jmp	.L2364
.L2435:
	endbr64
	movq	%rax, %rbx
	jmp	.L2362
.L2440:
	endbr64
	movq	%rax, %rbx
	jmp	.L2366
.L2429:
	endbr64
	movq	%rax, %rbx
	jmp	.L2389
.L2430:
	endbr64
	movq	%rax, %rdi
	jmp	.L2387
.L2433:
	endbr64
	movq	%rax, %rbx
	jmp	.L2472
.L2442:
	endbr64
	movq	%rax, %rbx
	jmp	.L2411
.L2434:
	endbr64
	movq	%rax, %rbx
	jmp	.L2473
	.section	.gcc_except_table
	.align 4
.LLSDA18256:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATT18256-.LLSDATTD18256
.LLSDATTD18256:
	.byte	0x1
	.uleb128 .LLSDACSE18256-.LLSDACSB18256
.LLSDACSB18256:
	.uleb128 .LEHB193-.LFB18256
	.uleb128 .LEHE193-.LEHB193
	.uleb128 .L2433-.LFB18256
	.uleb128 0
	.uleb128 .LEHB194-.LFB18256
	.uleb128 .LEHE194-.LEHB194
	.uleb128 .L2436-.LFB18256
	.uleb128 0
	.uleb128 .LEHB195-.LFB18256
	.uleb128 .LEHE195-.LEHB195
	.uleb128 .L2435-.LFB18256
	.uleb128 0
	.uleb128 .LEHB196-.LFB18256
	.uleb128 .LEHE196-.LEHB196
	.uleb128 .L2438-.LFB18256
	.uleb128 0
	.uleb128 .LEHB197-.LFB18256
	.uleb128 .LEHE197-.LEHB197
	.uleb128 .L2440-.LFB18256
	.uleb128 0
	.uleb128 .LEHB198-.LFB18256
	.uleb128 .LEHE198-.LEHB198
	.uleb128 .L2439-.LFB18256
	.uleb128 0
	.uleb128 .LEHB199-.LFB18256
	.uleb128 .LEHE199-.LEHB199
	.uleb128 .L2437-.LFB18256
	.uleb128 0
	.uleb128 .LEHB200-.LFB18256
	.uleb128 .LEHE200-.LEHB200
	.uleb128 .L2428-.LFB18256
	.uleb128 0
	.uleb128 .LEHB201-.LFB18256
	.uleb128 .LEHE201-.LEHB201
	.uleb128 .L2429-.LFB18256
	.uleb128 0
	.uleb128 .LEHB202-.LFB18256
	.uleb128 .LEHE202-.LEHB202
	.uleb128 .L2443-.LFB18256
	.uleb128 0
	.uleb128 .LEHB203-.LFB18256
	.uleb128 .LEHE203-.LEHB203
	.uleb128 .L2441-.LFB18256
	.uleb128 0
	.uleb128 .LEHB204-.LFB18256
	.uleb128 .LEHE204-.LEHB204
	.uleb128 .L2442-.LFB18256
	.uleb128 0
	.uleb128 .LEHB205-.LFB18256
	.uleb128 .LEHE205-.LEHB205
	.uleb128 .L2444-.LFB18256
	.uleb128 0
	.uleb128 .LEHB206-.LFB18256
	.uleb128 .LEHE206-.LEHB206
	.uleb128 .L2432-.LFB18256
	.uleb128 0
	.uleb128 .LEHB207-.LFB18256
	.uleb128 .LEHE207-.LEHB207
	.uleb128 .L2430-.LFB18256
	.uleb128 0x1
	.uleb128 .LEHB208-.LFB18256
	.uleb128 .LEHE208-.LEHB208
	.uleb128 .L2445-.LFB18256
	.uleb128 0
	.uleb128 .LEHB209-.LFB18256
	.uleb128 .LEHE209-.LEHB209
	.uleb128 .L2434-.LFB18256
	.uleb128 0
	.uleb128 .LEHB210-.LFB18256
	.uleb128 .LEHE210-.LEHB210
	.uleb128 .L2430-.LFB18256
	.uleb128 0x1
	.uleb128 .LEHB211-.LFB18256
	.uleb128 .LEHE211-.LEHB211
	.uleb128 .L2446-.LFB18256
	.uleb128 0
.LLSDACSE18256:
	.byte	0x1
	.byte	0
	.align 4
	.long	0

.LLSDATT18256:
	.section	.text.startup
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC18256
	.type	_Z41__static_initialization_and_destruction_0v.cold, @function
_Z41__static_initialization_and_destruction_0v.cold:
.LFSB18256:
.L2418:
	.cfi_def_cfa_offset 864
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	movq	16+_ZN3TAO8Register9NAMESPACEL8RESERVEDE(%rip), %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0
.L2419:
	subq	$32, %r14
	movq	%r14, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	cmpq	%r12, %r14
	jne	.L2419
.L2473:
	movl	$4, %eax
.L2398:
	movl	$4, %ebp
	subq	%rax, %rbp
	salq	$5, %rbp
	addq	%r12, %rbp
.L2426:
	cmpq	%r12, %rbp
	jne	.L2484
	movq	%rbx, %rdi
.LEHB212:
	call	_Unwind_Resume@PLT
.L2372:
	leaq	792(%rsp), %rbp
.L2373:
	subq	$72, %rbp
	leaq	32(%rbp), %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	cmpq	%r12, %rbp
	jne	.L2373
.L2472:
	movl	$8, %eax
.L2358:
	movl	$8, %ebp
	subq	%rax, %rbp
	imulq	$72, %rbp, %rbp
	addq	%r12, %rbp
.L2424:
	cmpq	%r12, %rbp
	jne	.L2485
	movq	%rbx, %rdi
	call	_Unwind_Resume@PLT
.LEHE212:
.L2370:
	movl	$1, %eax
	jmp	.L2358
.L2413:
	xorl	%eax, %eax
	jmp	.L2398
.L2407:
	movl	$3, %eax
	jmp	.L2398
.L2485:
	subq	$72, %rbp
	leaq	32(%rbp), %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L2424
.L2364:
	movl	$5, %eax
	jmp	.L2358
.L2362:
	movl	$6, %eax
	jmp	.L2358
.L2360:
	movl	$7, %eax
	jmp	.L2358
.L2366:
	movl	$3, %eax
	jmp	.L2358
.L2431:
	endbr64
	movq	%rax, %rbx
	call	__cxa_end_catch@PLT
.L2389:
	movq	16+_ZN3TAO8Register4NAMEL8RESERVEDE(%rip), %rdi
	call	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES5_St9_IdentityIS5_ESt4lessIS5_ESaIS5_EE8_M_eraseEPSt13_Rb_tree_nodeIS5_E.isra.0
	movq	%rbx, %rdi
.LEHB213:
	call	_Unwind_Resume@PLT
.LEHE213:
.L2387:
	call	__cxa_begin_catch@PLT
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
.LEHB214:
	call	__cxa_rethrow@PLT
.LEHE214:
.L2409:
	movl	$2, %eax
	jmp	.L2398
.L2411:
	movl	$1, %eax
	jmp	.L2398
.L2368:
	movl	$2, %eax
	jmp	.L2358
.L2484:
	subq	$32, %rbp
	movq	%rbp, %rdi
	call	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	jmp	.L2426
	.cfi_endproc
.LFE18256:
	.section	.gcc_except_table
	.align 4
.LLSDAC18256:
	.byte	0xff
	.byte	0x9b
	.uleb128 .LLSDATTC18256-.LLSDATTDC18256
.LLSDATTDC18256:
	.byte	0x1
	.uleb128 .LLSDACSEC18256-.LLSDACSBC18256
.LLSDACSBC18256:
	.uleb128 .LEHB212-.LCOLDB39
	.uleb128 .LEHE212-.LEHB212
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB213-.LCOLDB39
	.uleb128 .LEHE213-.LEHB213
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB214-.LCOLDB39
	.uleb128 .LEHE214-.LEHB214
	.uleb128 .L2431-.LCOLDB39
	.uleb128 0
.LLSDACSEC18256:
	.byte	0x1
	.byte	0
	.align 4
	.long	0

.LLSDATTC18256:
	.section	.text.unlikely
	.section	.text.startup
	.size	_Z41__static_initialization_and_destruction_0v, .-_Z41__static_initialization_and_destruction_0v
	.section	.text.unlikely
	.size	_Z41__static_initialization_and_destruction_0v.cold, .-_Z41__static_initialization_and_destruction_0v.cold
.LCOLDE39:
	.section	.text.startup
.LHOTE39:
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_:
.LFB17853:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	movq	16(%rdi), %r15
	movq	%rdi, 24(%rsp)
	movq	%rsi, 16(%rsp)
	testq	%r15, %r15
	je	.L2513
	movq	(%rsi), %rdx
	movq	8(%rsi), %r14
	movl	$2147483648, %ebp
	movq	%rdx, 8(%rsp)
	jmp	.L2494
	.p2align 4,,10
	.p2align 3
.L2492:
	movq	16(%r15), %rax
	movl	$1, %esi
	testq	%rax, %rax
	je	.L2514
.L2499:
	movq	%rax, %r15
.L2494:
	movq	40(%r15), %r12
	movq	32(%r15), %r13
	cmpq	%r12, %r14
	movq	%r12, %rbx
	cmovbe	%r14, %rbx
	testq	%rbx, %rbx
	je	.L2489
	movq	8(%rsp), %rdi
	movq	%rbx, %rdx
	movq	%r13, %rsi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L2490
.L2489:
	movq	%r14, %rax
	subq	%r12, %rax
	cmpq	%rbp, %rax
	jge	.L2491
	movabsq	$-2147483649, %rcx
	cmpq	%rcx, %rax
	jle	.L2492
.L2490:
	testl	%eax, %eax
	js	.L2492
.L2491:
	movq	24(%r15), %rax
	xorl	%esi, %esi
	testq	%rax, %rax
	jne	.L2499
.L2514:
	movq	%r15, %rbp
	testb	%sil, %sil
	jne	.L2488
.L2495:
	testq	%rbx, %rbx
	je	.L2497
	movq	8(%rsp), %rsi
	movq	%rbx, %rdx
	movq	%r13, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	je	.L2497
.L2498:
	xorl	%edx, %edx
	testl	%eax, %eax
	cmovns	%rdx, %rbp
	cmovs	%rdx, %r15
.L2496:
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%r15, %rax
	popq	%rbx
	.cfi_def_cfa_offset 48
	movq	%rbp, %rdx
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2497:
	.cfi_restore_state
	movq	%r12, %rcx
	subq	%r14, %rcx
	cmpq	$2147483647, %rcx
	jg	.L2502
	cmpq	$-2147483648, %rcx
	jl	.L2503
	movl	%ecx, %eax
	jmp	.L2498
	.p2align 4,,10
	.p2align 3
.L2513:
	leaq	8(%rdi), %r15
.L2488:
	movq	24(%rsp), %rax
	cmpq	%r15, 24(%rax)
	je	.L2501
	movq	%r15, %rdi
	movq	%r15, %rbp
	call	_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base@PLT
	movq	16(%rsp), %rdx
	movq	40(%rax), %r12
	movq	32(%rax), %r13
	movq	%rax, %r15
	movq	8(%rdx), %r14
	movq	(%rdx), %rcx
	movq	%r12, %rbx
	cmpq	%r12, %r14
	movq	%rcx, 8(%rsp)
	cmovbe	%r14, %rbx
	jmp	.L2495
	.p2align 4,,10
	.p2align 3
.L2501:
	movq	%r15, %rbp
	xorl	%r15d, %r15d
	jmp	.L2496
	.p2align 4,,10
	.p2align 3
.L2502:
	xorl	%ebp, %ebp
	jmp	.L2496
.L2503:
	xorl	%r15d, %r15d
	jmp	.L2496
	.cfi_endproc
.LFE17853:
	.size	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_, .-_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE24_M_get_insert_unique_posERS7_
	.section	.text._ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS9_ERS7_,"axG",@progbits,_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS9_ERS7_,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS9_ERS7_
	.type	_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS9_ERS7_, @function
_ZNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_S6_ItbEESt10_Select1stIS9_ESt4lessIS5_ESaIS9_EE29_M_get_insert_hint_unique_posESt23_Rb_tree_const_iteratorIS9_ERS7_:
.LFB17514:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	leaq	8(%rdi), %rax
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdx, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$24, %rsp
	.cfi_def_cfa_offset 80
	cmpq	%rax, %rsi
	je	.L2542
	movq	8(%rdx), %r13
	movq	40(%rsi), %r14
	movq	%rsi, %rbx
	movq	(%rdx), %r15
	movq	32(%rsi), %rcx
	cmpq	%r14, %r13
	movq	%r14, %rdx
	cmovbe	%r13, %rdx
	testq	%rdx, %rdx
	je	.L2519
	movq	%rcx, %rsi
	movq	%r15, %rdi
	movq	%rdx, 8(%rsp)
	movq	%rcx, (%rsp)
	call	memcmp@PLT
	movq	(%rsp), %rcx
	movq	8(%rsp), %rdx
	testl	%eax, %eax
	jne	.L2540
	movq	%r13, %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2524
	cmpq	$-2147483648, %rax
	jl	.L2522
	testl	%eax, %eax
.L2540:
	js	.L2522
.L2524:
	movq	%r15, %rsi
	movq	%rcx, %rdi
	call	memcmp@PLT
	testl	%eax, %eax
	jne	.L2527
.L2523:
	movq	%r14, %rax
	subq	%r13, %rax
	cmpq	$2147483647, %rax
	jg	.L2528
	cmpq	$-2147483648, %rax
	jl	.L2529
.L2527:
	testl	%eax, %eax
	js	.L2529
.L2528:
	movq	%rbx, %rax
	xorl	%edx, %edx
.L2534:
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L2519:
	.cfi_restore_state
	movq	%r13, %rax
	subq	%r14, %rax
	cmpq	$2147483647, %rax
	jg	.L2523
	cmpq	$-2147483648, %rax
	jl	.L2522
	testl	%eax, %eax
	jns	.L2523
.L2522:
	movq	%rbx, %rax
	movq	%rbx, %rdx
	cmpq	%rbx, 24(%rbp)
	je	.L2534
	movq	%rbx, %rdi
	call	_ZSt18_Rb_tree_decrementPSt18_Rb_tree_node_base@PLT
	movq	%r13, %rcx
	movq	%r15, %rdx
	movq	40(%rax), %rsi
	movq	32(%rax), %rdi
	movq	%rax, %r14
	call	_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_.isra.0
	testl	%eax, %eax
	js	.L2543
.L2517:
	addq	$24, %rsp
	.cfi