	.file	"address.cpp"
	.text
	.section	.text._ZN5debug9exceptionD2Ev,"axG",@progbits,_ZN5debug9exceptionD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN5debug9exceptionD2Ev
	.type	_ZN5debug9exceptionD2Ev, @function
_ZN5debug9exceptionD2Ev:
.LFB14789:
	.cfi_startproc
	endbr64
	leaq	16+_ZTVN5debug9exceptionE(%rip), %rax
	movq	%rax, (%rdi)
	jmp	_ZNSt13runtime_errorD2Ev@PLT
	.cfi_endproc
.LFE14789:
	.size	_ZN5debug9exceptionD2Ev, .-_ZN5debug9exceptionD2Ev
	.weak	_ZN5debug9exceptionD1Ev
	.set	_ZN5debug9exceptionD1Ev,_ZN5debug9exceptionD2Ev
	.section	.text._ZN5debug9exceptionD0Ev,"axG",@progbits,_ZN5debug9exceptionD5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZN5debug9exceptionD0Ev
	.type	_ZN5debug9exceptionD0Ev, @function
_ZN5debug9exceptionD0Ev:
.LFB14791:
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
.LFE14791:
	.size	_ZN5debug9exceptionD0Ev, .-_ZN5debug9exceptionD0Ev
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev:
.LFB14971:
	.cfi_startproc
	endbr64
	movq	(%rdi), %rax
	leaq	16(%rdi), %rdx
	cmpq	%rdx, %rax
	je	.L5
	movq	16(%rdi), %rsi
	movq	%rax, %rdi
	addq	$1, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L5:
	ret
	.cfi_endproc
.LFE14971:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	.set	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED2Ev
	.section	.text._ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0,"axG",@progbits,_ZN3LLD11TemplateLRUISt6vectorIhSaIhEE9base_uintILj256EEE3PutERKS3_RKS5_,comdat
	.align 2
	.p2align 4
	.type	_ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0, @function
_ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0:
.LFB17709:
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
	je	.L7
.L43:
	movq	16(%rsp), %rax
	movq	24(%rax), %rax
	movq	%rax, (%rsp)
	testq	%rax, %rax
	je	.L9
.L40:
	movq	(%rsp), %rax
	movq	24(%rax), %rax
	movq	%rax, 8(%rsp)
	testq	%rax, %rax
	je	.L10
.L37:
	movq	8(%rsp), %rax
	movq	24(%rax), %r15
	testq	%r15, %r15
	je	.L11
.L34:
	movq	24(%r15), %rbp
	testq	%rbp, %rbp
	je	.L12
.L31:
	movq	24(%rbp), %rbx
	testq	%rbx, %rbx
	je	.L13
.L28:
	movq	24(%rbx), %r12
	testq	%r12, %r12
	je	.L14
.L25:
	movq	24(%r12), %r14
	testq	%r14, %r14
	je	.L15
.L22:
	movq	24(%r14), %rdx
	testq	%rdx, %rdx
	je	.L16
.L19:
	movq	24(%rdx), %rdi
	movq	%rdx, 24(%rsp)
	call	_ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0
	movq	24(%rsp), %rdx
	movq	32(%rdx), %rdi
	movq	48(%rdx), %rsi
	movq	16(%rdx), %r13
	subq	%rdi, %rsi
	testq	%rdi, %rdi
	je	.L17
	call	_ZdlPvm@PLT
	movq	24(%rsp), %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	jne	.L18
.L16:
	movq	32(%r14), %rdi
	movq	16(%r14), %r13
	testq	%rdi, %rdi
	je	.L20
	movq	48(%r14), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L20:
	movl	$64, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	je	.L15
	movq	%r13, %r14
	jmp	.L22
.L13:
	movq	32(%rbp), %rdi
	movq	16(%rbp), %rbx
	testq	%rdi, %rdi
	je	.L29
	movq	48(%rbp), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L29:
	movl	$64, %esi
	movq	%rbp, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L12
	movq	%rbx, %rbp
	jmp	.L31
.L17:
	movl	$64, %esi
	movq	%rdx, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	je	.L16
.L18:
	movq	%r13, %rdx
	jmp	.L19
	.p2align 4,,10
	.p2align 3
.L14:
	movq	32(%rbx), %rdi
	movq	16(%rbx), %r12
	testq	%rdi, %rdi
	je	.L26
	movq	48(%rbx), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L26:
	movl	$64, %esi
	movq	%rbx, %rdi
	call	_ZdlPvm@PLT
	testq	%r12, %r12
	je	.L13
	movq	%r12, %rbx
	jmp	.L28
.L12:
	movq	32(%r15), %rdi
	movq	16(%r15), %rbx
	testq	%rdi, %rdi
	je	.L32
	movq	48(%r15), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L32:
	movl	$64, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L11
	movq	%rbx, %r15
	jmp	.L34
	.p2align 4,,10
	.p2align 3
.L15:
	movq	32(%r12), %rdi
	movq	16(%r12), %r13
	testq	%rdi, %rdi
	je	.L23
	movq	48(%r12), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L23:
	movl	$64, %esi
	movq	%r12, %rdi
	call	_ZdlPvm@PLT
	testq	%r13, %r13
	je	.L14
	movq	%r13, %r12
	jmp	.L25
.L11:
	movq	8(%rsp), %r15
	movq	32(%r15), %rdi
	movq	16(%r15), %rbx
	testq	%rdi, %rdi
	je	.L35
	movq	48(%r15), %rax
	movq	%rax, %rsi
	movq	%rax, 8(%rsp)
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L10
.L36:
	movq	%rbx, 8(%rsp)
	jmp	.L37
.L35:
	movq	%r15, %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L36
.L10:
	movq	(%rsp), %r15
	movq	32(%r15), %rdi
	movq	16(%r15), %rbx
	testq	%rdi, %rdi
	je	.L38
	movq	48(%r15), %rax
	movq	%rax, %rsi
	movq	%rax, (%rsp)
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r15, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L9
.L39:
	movq	%rbx, (%rsp)
	jmp	.L40
.L38:
	movq	%r15, %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L39
.L9:
	movq	16(%rsp), %r14
	movq	32(%r14), %rdi
	movq	16(%r14), %rbx
	testq	%rdi, %rdi
	je	.L41
	movq	48(%r14), %rax
	movq	%rax, %rsi
	movq	%rax, (%rsp)
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
	movl	$64, %esi
	movq	%r14, %rdi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	je	.L7
.L42:
	movq	%rbx, 16(%rsp)
	jmp	.L43
.L41:
	movq	%r14, %rdi
	movl	$64, %esi
	call	_ZdlPvm@PLT
	testq	%rbx, %rbx
	jne	.L42
.L7:
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
.LFE17709:
	.size	_ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0, .-_ZNSt8_Rb_treeISt6vectorIhSaIhEESt4pairIKS2_PN3LLD12TemplateNodeIS2_9base_uintILj256EEEEESt10_Select1stISB_ESt4lessIS2_ESaISB_EE8_M_eraseEPSt13_Rb_tree_nodeISB_E.isra.0
	.text
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressC2Ev
	.type	_ZN3TAO8Register7AddressC2Ev, @function
_ZN3TAO8Register7AddressC2Ev:
.LFB14771:
	.cfi_startproc
	endbr64
	xorl	%esi, %esi
	jmp	_ZN9base_uintILj256EEC2Em@PLT
	.cfi_endproc
.LFE14771:
	.size	_ZN3TAO8Register7AddressC2Ev, .-_ZN3TAO8Register7AddressC2Ev
	.globl	_ZN3TAO8Register7AddressC1Ev
	.set	_ZN3TAO8Register7AddressC1Ev,_ZN3TAO8Register7AddressC2Ev
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE
	.type	_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE, @function
_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE:
.LFB14774:
	.cfi_startproc
	endbr64
	jmp	_ZN9base_uintILj256EEC2ERKS0_@PLT
	.cfi_endproc
.LFE14774:
	.size	_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE, .-_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE
	.globl	_ZN3TAO8Register7AddressC1ERK9base_uintILj256EE
	.set	_ZN3TAO8Register7AddressC1ERK9base_uintILj256EE,_ZN3TAO8Register7AddressC2ERK9base_uintILj256EE
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressC2EO9base_uintILj256EE
	.type	_ZN3TAO8Register7AddressC2EO9base_uintILj256EE, @function
_ZN3TAO8Register7AddressC2EO9base_uintILj256EE:
.LFB14777:
	.cfi_startproc
	endbr64
	jmp	_ZN9base_uintILj256EEC2EOS0_@PLT
	.cfi_endproc
.LFE14777:
	.size	_ZN3TAO8Register7AddressC2EO9base_uintILj256EE, .-_ZN3TAO8Register7AddressC2EO9base_uintILj256EE
	.globl	_ZN3TAO8Register7AddressC1EO9base_uintILj256EE
	.set	_ZN3TAO8Register7AddressC1EO9base_uintILj256EE,_ZN3TAO8Register7AddressC2EO9base_uintILj256EE
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressaSERK9base_uintILj256EE
	.type	_ZN3TAO8Register7AddressaSERK9base_uintILj256EE, @function
_ZN3TAO8Register7AddressaSERK9base_uintILj256EE:
.LFB14779:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$32, %edx
	call	memmove@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14779:
	.size	_ZN3TAO8Register7AddressaSERK9base_uintILj256EE, .-_ZN3TAO8Register7AddressaSERK9base_uintILj256EE
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressaSEO9base_uintILj256EE
	.type	_ZN3TAO8Register7AddressaSEO9base_uintILj256EE, @function
_ZN3TAO8Register7AddressaSEO9base_uintILj256EE:
.LFB17707:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$32, %edx
	call	memmove@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE17707:
	.size	_ZN3TAO8Register7AddressaSEO9base_uintILj256EE, .-_ZN3TAO8Register7AddressaSEO9base_uintILj256EE
	.section	.text.unlikely,"ax",@progbits
	.align 2
.LCOLDB0:
	.text
.LHOTB0:
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7AddressC2Eh
	.type	_ZN3TAO8Register7AddressC2Eh, @function
_ZN3TAO8Register7AddressC2Eh:
.LFB14782:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA14782
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movl	%esi, %ebx
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
.LEHB0:
	call	_ZN3LLC10GetRand256Ev@PLT
.LEHE0:
	movzbl	%bl, %esi
	movq	%rbp, %rdi
.LEHB1:
	call	_ZN9base_uintILj256EE7SetTypeEh@PLT
.LEHE1:
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
.L108:
	.cfi_restore_state
	endbr64
	movq	%rax, %rbx
	jmp	.L107
	.globl	__gxx_personality_v0
	.section	.gcc_except_table,"a",@progbits
.LLSDA14782:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE14782-.LLSDACSB14782
.LLSDACSB14782:
	.uleb128 .LEHB0-.LFB14782
	.uleb128 .LEHE0-.LEHB0
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB1-.LFB14782
	.uleb128 .LEHE1-.LEHB1
	.uleb128 .L108-.LFB14782
	.uleb128 0
.LLSDACSE14782:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC14782
	.type	_ZN3TAO8Register7AddressC2Eh.cold, @function
_ZN3TAO8Register7AddressC2Eh.cold:
.LFSB14782:
.L107:
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -24
	.cfi_offset 6, -16
	movq	%rbp, %rdi
	call	_ZN9base_uintILj256EED2Ev@PLT
	movq	%rbx, %rdi
.LEHB2:
	call	_Unwind_Resume@PLT
.LEHE2:
	.cfi_endproc
.LFE14782:
	.section	.gcc_except_table
.LLSDAC14782:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC14782-.LLSDACSBC14782
.LLSDACSBC14782:
	.uleb128 .LEHB2-.LCOLDB0
	.uleb128 .LEHE2-.LEHB2
	.uleb128 0
	.uleb128 0
.LLSDACSEC14782:
	.section	.text.unlikely
	.text
	.size	_ZN3TAO8Register7AddressC2Eh, .-_ZN3TAO8Register7AddressC2Eh
	.section	.text.unlikely
	.size	_ZN3TAO8Register7AddressC2Eh.cold, .-_ZN3TAO8Register7AddressC2Eh.cold
.LCOLDE0:
	.text
.LHOTE0:
	.globl	_ZN3TAO8Register7AddressC1Eh
	.set	_ZN3TAO8Register7AddressC1Eh,_ZN3TAO8Register7AddressC2Eh
	.section	.text.unlikely
	.align 2
.LCOLDB1:
	.text
.LHOTB1:
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address7IsValidEv
	.type	_ZNK3TAO8Register7Address7IsValidEv, @function
_ZNK3TAO8Register7Address7IsValidEv:
.LFB14823:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA14823
	endbr64
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	movl	$255, %esi
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	movq	%rdi, %rbx
	subq	$32, %rsp
	.cfi_def_cfa_offset 64
	movq	%rsp, %rbp
	movq	%rbp, %rdi
.LEHB3:
	call	_ZN9base_uintILj256EEC1Em@PLT
.LEHE3:
	movq	%rbp, %rsi
	movq	%rbx, %rdi
.LEHB4:
	call	_ZNK9base_uintILj256EEleERKS0_@PLT
.LEHE4:
	movl	%eax, %r12d
	movq	%rbp, %rdi
	call	_ZN9base_uintILj256EED1Ev@PLT
	xorl	%eax, %eax
	testb	%r12b, %r12b
	je	.L118
	addq	$32, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L118:
	.cfi_restore_state
	movq	%rbx, %rdi
.LEHB5:
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
.LEHE5:
	cmpb	$-38, %al
	ja	.L113
	cmpb	$-48, %al
	seta	%al
	addq	$32, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L113:
	.cfi_restore_state
	cmpb	$-1, %al
	sete	%al
	addq	$32, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
.L116:
	.cfi_restore_state
	endbr64
	movq	%rax, %rbx
	jmp	.L114
	.section	.gcc_except_table
.LLSDA14823:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE14823-.LLSDACSB14823
.LLSDACSB14823:
	.uleb128 .LEHB3-.LFB14823
	.uleb128 .LEHE3-.LEHB3
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB4-.LFB14823
	.uleb128 .LEHE4-.LEHB4
	.uleb128 .L116-.LFB14823
	.uleb128 0
	.uleb128 .LEHB5-.LFB14823
	.uleb128 .LEHE5-.LEHB5
	.uleb128 0
	.uleb128 0
.LLSDACSE14823:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC14823
	.type	_ZNK3TAO8Register7Address7IsValidEv.cold, @function
_ZNK3TAO8Register7Address7IsValidEv.cold:
.LFSB14823:
.L114:
	.cfi_def_cfa_offset 64
	.cfi_offset 3, -32
	.cfi_offset 6, -24
	.cfi_offset 12, -16
	movq	%rbp, %rdi
	call	_ZN9base_uintILj256EED1Ev@PLT
	movq	%rbx, %rdi
.LEHB6:
	call	_Unwind_Resume@PLT
.LEHE6:
	.cfi_endproc
.LFE14823:
	.section	.gcc_except_table
.LLSDAC14823:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC14823-.LLSDACSBC14823
.LLSDACSBC14823:
	.uleb128 .LEHB6-.LCOLDB1
	.uleb128 .LEHE6-.LEHB6
	.uleb128 0
	.uleb128 0
.LLSDACSEC14823:
	.section	.text.unlikely
	.text
	.size	_ZNK3TAO8Register7Address7IsValidEv, .-_ZNK3TAO8Register7Address7IsValidEv
	.section	.text.unlikely
	.size	_ZNK3TAO8Register7Address7IsValidEv.cold, .-_ZNK3TAO8Register7Address7IsValidEv.cold
.LCOLDE1:
	.text
.LHOTE1:
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address10IsReadonlyEv
	.type	_ZNK3TAO8Register7Address10IsReadonlyEv, @function
_ZNK3TAO8Register7Address10IsReadonlyEv:
.LFB14824:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-47, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14824:
	.size	_ZNK3TAO8Register7Address10IsReadonlyEv, .-_ZNK3TAO8Register7Address10IsReadonlyEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address8IsAppendEv
	.type	_ZNK3TAO8Register7Address8IsAppendEv, @function
_ZNK3TAO8Register7Address8IsAppendEv:
.LFB14825:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-46, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14825:
	.size	_ZNK3TAO8Register7Address8IsAppendEv, .-_ZNK3TAO8Register7Address8IsAppendEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address5IsRawEv
	.type	_ZNK3TAO8Register7Address5IsRawEv, @function
_ZNK3TAO8Register7Address5IsRawEv:
.LFB14826:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-45, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14826:
	.size	_ZNK3TAO8Register7Address5IsRawEv, .-_ZNK3TAO8Register7Address5IsRawEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address8IsObjectEv
	.type	_ZNK3TAO8Register7Address8IsObjectEv, @function
_ZNK3TAO8Register7Address8IsObjectEv:
.LFB14827:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-44, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14827:
	.size	_ZNK3TAO8Register7Address8IsObjectEv, .-_ZNK3TAO8Register7Address8IsObjectEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address8IsCryptoEv
	.type	_ZNK3TAO8Register7Address8IsCryptoEv, @function
_ZNK3TAO8Register7Address8IsCryptoEv:
.LFB14828:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-43, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14828:
	.size	_ZNK3TAO8Register7Address8IsCryptoEv, .-_ZNK3TAO8Register7Address8IsCryptoEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address9IsAccountEv
	.type	_ZNK3TAO8Register7Address9IsAccountEv, @function
_ZNK3TAO8Register7Address9IsAccountEv:
.LFB14829:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-42, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14829:
	.size	_ZNK3TAO8Register7Address9IsAccountEv, .-_ZNK3TAO8Register7Address9IsAccountEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address7IsTokenEv
	.type	_ZNK3TAO8Register7Address7IsTokenEv, @function
_ZNK3TAO8Register7Address7IsTokenEv:
.LFB14830:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-41, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14830:
	.size	_ZNK3TAO8Register7Address7IsTokenEv, .-_ZNK3TAO8Register7Address7IsTokenEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address7IsTrustEv
	.type	_ZNK3TAO8Register7Address7IsTrustEv, @function
_ZNK3TAO8Register7Address7IsTrustEv:
.LFB14831:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-40, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14831:
	.size	_ZNK3TAO8Register7Address7IsTrustEv, .-_ZNK3TAO8Register7Address7IsTrustEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address6IsNameEv
	.type	_ZNK3TAO8Register7Address6IsNameEv, @function
_ZNK3TAO8Register7Address6IsNameEv:
.LFB14832:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-39, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14832:
	.size	_ZNK3TAO8Register7Address6IsNameEv, .-_ZNK3TAO8Register7Address6IsNameEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address11IsNamespaceEv
	.type	_ZNK3TAO8Register7Address11IsNamespaceEv, @function
_ZNK3TAO8Register7Address11IsNamespaceEv:
.LFB14833:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-38, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14833:
	.size	_ZNK3TAO8Register7Address11IsNamespaceEv, .-_ZNK3TAO8Register7Address11IsNamespaceEv
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address10IsWildcardEv
	.type	_ZNK3TAO8Register7Address10IsWildcardEv, @function
_ZNK3TAO8Register7Address10IsWildcardEv:
.LFB14834:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	cmpb	$-1, %al
	sete	%al
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14834:
	.size	_ZNK3TAO8Register7Address10IsWildcardEv, .-_ZNK3TAO8Register7Address10IsWildcardEv
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv, @function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv:
.LFB15228:
	.cfi_startproc
	endbr64
	movq	(%rdi), %rax
	leaq	16(%rdi), %rdx
	cmpq	%rdx, %rax
	je	.L141
	movq	16(%rdi), %rsi
	movq	%rax, %rdi
	addq	$1, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L141:
	ret
	.cfi_endproc
.LFE15228:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE10_M_disposeEv
	.section	.text._ZNSt6vectorIhSaIhEED2Ev,"axG",@progbits,_ZNSt6vectorIhSaIhEED5Ev,comdat
	.align 2
	.p2align 4
	.weak	_ZNSt6vectorIhSaIhEED2Ev
	.type	_ZNSt6vectorIhSaIhEED2Ev, @function
_ZNSt6vectorIhSaIhEED2Ev:
.LFB15525:
	.cfi_startproc
	endbr64
	movq	(%rdi), %rax
	testq	%rax, %rax
	je	.L143
	movq	16(%rdi), %rsi
	movq	%rax, %rdi
	subq	%rax, %rsi
	jmp	_ZdlPvm@PLT
	.p2align 4,,10
	.p2align 3
.L143:
	ret
	.cfi_endproc
.LFE15525:
	.size	_ZNSt6vectorIhSaIhEED2Ev, .-_ZNSt6vectorIhSaIhEED2Ev
	.weak	_ZNSt6vectorIhSaIhEED1Ev
	.set	_ZNSt6vectorIhSaIhEED1Ev,_ZNSt6vectorIhSaIhEED2Ev
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC2:
	.string	"vector::_M_realloc_insert"
	.section	.text.unlikely
	.align 2
.LCOLDB3:
	.text
.LHOTB3:
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev
	.type	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev, @function
_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev:
.LFB14838:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA14838
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
	movq	%rdi, %rbp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rsi, %rbx
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	leaq	16(%rsp), %r13
	movq	%r13, %rdi
.LEHB7:
	call	_ZNK9base_uintILj256EE8GetBytesEv@PLT
.LEHE7:
	movq	%rbx, %rdi
.LEHB8:
	call	_ZNK9base_uintILj256EE7GetTypeEv@PLT
	movl	%eax, %r14d
	movq	16(%rsp), %r12
	movq	24(%rsp), %rax
	cmpq	32(%rsp), %rax
	je	.L146
	cmpq	%rax, %r12
	je	.L169
	movzbl	-1(%rax), %edx
	movb	%dl, (%rax)
	movq	24(%rsp), %rdi
	leaq	-1(%rdi), %rdx
	leaq	1(%rdi), %rax
	subq	%r12, %rdx
	movq	%rax, 24(%rsp)
	cmpq	$1, %rdx
	jle	.L149
	subq	%rdx, %rdi
	movq	%r12, %rsi
	call	memmove@PLT
.L150:
	movb	%r14b, (%r12)
.L148:
	movq	%r13, %rsi
	movq	%rbp, %rdi
	call	_ZN8encoding17EncodeBase58CheckB5cxx11ERKSt6vectorIhSaIhEE@PLT
	movq	16(%rsp), %rdi
	testq	%rdi, %rdi
	je	.L145
	movq	32(%rsp), %rsi
	subq	%rdi, %rsi
	call	_ZdlPvm@PLT
.L145:
	addq	$56, %rsp
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
	.p2align 4,,10
	.p2align 3
.L169:
	.cfi_restore_state
	movb	%r14b, (%r12)
	addq	$1, 24(%rsp)
	jmp	.L148
	.p2align 4,,10
	.p2align 3
.L146:
	subq	%r12, %rax
	movq	%rax, %rbx
	movabsq	$9223372036854775807, %rax
	cmpq	%rax, %rbx
	je	.L170
	testq	%rbx, %rbx
	je	.L152
	movabsq	$9223372036854775807, %r15
	leaq	(%rbx,%rbx), %rdx
	cmpq	%r15, %rdx
	cmovbe	%rdx, %r15
	cmpq	%rbx, %rdx
	cmovb	%rax, %r15
	movq	%r15, %rdi
	call	_Znwm@PLT
	movb	%r14b, (%rax)
	movq	%rax, %rcx
	leaq	1(%rax), %rdi
	leaq	(%rax,%r15), %rax
	movq	%rax, (%rsp)
	movq	32(%rsp), %r14
	testq	%rbx, %rbx
	jle	.L154
	movq	%rbx, %rdx
	movq	%r12, %rsi
	movq	%rcx, 8(%rsp)
	leaq	(%rdi,%rbx), %r15
	call	memmove@PLT
	movq	8(%rsp), %rcx
	subq	%r12, %r14
.L155:
	movq	%r14, %rsi
	movq	%r12, %rdi
	movq	%rcx, 8(%rsp)
	call	_ZdlPvm@PLT
	movq	8(%rsp), %rcx
.L156:
	movq	(%rsp), %rax
	movq	%rcx, 16(%rsp)
	movq	%r15, 24(%rsp)
	movq	%rax, 32(%rsp)
	jmp	.L148
	.p2align 4,,10
	.p2align 3
.L152:
	movl	$1, %edi
	call	_Znwm@PLT
	leaq	1(%rax), %rdi
	movb	%r14b, (%rax)
	movq	%rax, %rcx
	movq	%rdi, (%rsp)
.L154:
	leaq	(%rdi,%rbx), %r15
	testq	%r12, %r12
	je	.L156
	movq	32(%rsp), %r14
	subq	%r12, %r14
	jmp	.L155
	.p2align 4,,10
	.p2align 3
.L149:
	jne	.L150
	movzbl	(%r12), %eax
	movb	%al, -1(%rdi)
	jmp	.L150
.L170:
	leaq	.LC2(%rip), %rdi
	call	_ZSt20__throw_length_errorPKc@PLT
.LEHE8:
.L160:
	endbr64
	movq	%rax, %rbx
	jmp	.L158
	.section	.gcc_except_table
.LLSDA14838:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE14838-.LLSDACSB14838
.LLSDACSB14838:
	.uleb128 .LEHB7-.LFB14838
	.uleb128 .LEHE7-.LEHB7
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB8-.LFB14838
	.uleb128 .LEHE8-.LEHB8
	.uleb128 .L160-.LFB14838
	.uleb128 0
.LLSDACSE14838:
	.text
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDAC14838
	.type	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev.cold, @function
_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev.cold:
.LFSB14838:
.L158:
	.cfi_def_cfa_offset 112
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	movq	%r13, %rdi
	call	_ZNSt6vectorIhSaIhEED1Ev
	movq	%rbx, %rdi
.LEHB9:
	call	_Unwind_Resume@PLT
.LEHE9:
	.cfi_endproc
.LFE14838:
	.section	.gcc_except_table
.LLSDAC14838:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSEC14838-.LLSDACSBC14838
.LLSDACSBC14838:
	.uleb128 .LEHB9-.LCOLDB3
	.uleb128 .LEHE9-.LEHB9
	.uleb128 0
	.uleb128 0
.LLSDACSEC14838:
	.section	.text.unlikely
	.text
	.size	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev, .-_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev
	.section	.text.unlikely
	.size	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev.cold, .-_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev.cold
.LCOLDE3:
	.text
.LHOTE3:
	.align 2
	.p2align 4
	.globl	_ZNK3TAO8Register7Address8ToStringB5cxx11Ev
	.type	_ZNK3TAO8Register7Address8ToStringB5cxx11Ev, @function
_ZNK3TAO8Register7Address8ToStringB5cxx11Ev:
.LFB14839:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsi, %rbp
	xorl	%esi, %esi
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movq	%rdi, %rbx
	movq	%rbp, %rdi
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	call	_ZNK9base_uintILj256EEeqEm@PLT
	testb	%al, %al
	je	.L172
	leaq	16(%rbx), %rax
	movq	$1, 8(%rbx)
	movq	%rax, (%rbx)
	movl	$48, %eax
	movw	%ax, 16(%rbx)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L172:
	.cfi_restore_state
	movq	%rbp, %rsi
	movq	%rbx, %rdi
	call	_ZNK3TAO8Register7Address8ToBase58B5cxx11Ev
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	movq	%rbx, %rax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE14839:
	.size	_ZNK3TAO8Register7Address8ToStringB5cxx11Ev, .-_ZNK3TAO8Register7Address8ToStringB5cxx11Ev
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC5:
	.string	"cannot create std::vector larger than max_size()"
	.section	.text.unlikely
	.align 2
.LCOLDB6:
	.text
.LHOTB6:
	.align 2
	.p2align 4
	.globl	_ZN3TAO8Register7Address9SetBase58ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
	.type	_ZN3TAO8Register7Address9SetBase58ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE, @function
_ZN3TAO8Register7Address9SetBase58ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE:
.LFB14835:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA14835
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pxor	%xmm0, %xmm0
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	movq	%rdi, %r12
	movq	%rsi, %rdi
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%rsp, %rbp
	movaps	%xmm0, (%rsp)
	movq	$0, 16(%rsp)
	movq	%rbp, %rsi
.LEHB10:
	call	_ZN8encoding17DecodeBase58CheckERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERSt6vectorIhSaIhEE@PLT
.LEHE10:
	movq	(%rsp), %r14
	testb	%al, %al
	je	.L176
	movq	8(%rsp), %rbx
	pxor	%xmm0, %xmm0
	leaq	1(%r14), %r13
	movq	$0, 48(%rsp)
	mov