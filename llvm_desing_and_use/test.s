	.text
	.file	"test.bc"
	.globl	mult
	.p2align	4, 0x90
	.type	mult,@function
mult:                                   # @mult
	.cfi_startproc
# BB#0:
	movl	4(%esp), %eax
	imull	8(%esp), %eax
	retl
.Lfunc_end0:
	.size	mult, .Lfunc_end0-mult
	.cfi_endproc


	.section	".note.GNU-stack","",@progbits
