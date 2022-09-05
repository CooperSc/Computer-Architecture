  .set noat
	.text
	.align	2
	.globl	__start
	.ent	__start
	.type	__start, @function
__start:

	 addi $1, $1, 100

__loop:

	addi $2, $2, 1
	bne $1, $2, __loop

	addi $5, $5, 5
	 

	.end	__start
	.size	__start, .-__start
