  .set noat
	.text
	.align	2
	.globl	__start
	.ent	__start
	.type	__start, @function
__start:	

	sub $3, $1, $2
    lw $4, 0($3)
    lw $1, 4($1)
    and $4, $3, $5
    sw $4, 8($3)

	.end	__start
	.size	__start, .-__start