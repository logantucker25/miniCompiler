.globl main
main:
	pushl %ebp
	movl %esp, %ebp
fn:
	subl	$12, %esp
	subl $20, %esp
	pushl %ebx
.LFB0:
	movl $10, -12(%ebp)
	movl $20, -16(%ebp)
	movl -12(%ebp), %ebx
	movl -16(%ebp), %ecx
	movl %ebx, %ebx
	addl %ecx, %ebx
	movl -20(%ebp), %eax
	movl -20(%ebp), %eax
	movl -12(%ebp), %edx
	movl 8(%ebp), %edx
	movl %edx, %eax
	cmpl %edx, %eax
	jl .L1
	jmp .L2
.L1:
	movl $30, -16(%ebp)
	jmp .L3
.L2:
	movl -20(%ebp), %ecx
	movl -16(%ebp), %eax
	movl -16(%ebp), %eax
	jmp .L3
.L3:
	movl -16(%ebp), %ecx
	movl -12(%ebp), %ebx
	movl %ecx, %ebx
	addl %ebx, %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
