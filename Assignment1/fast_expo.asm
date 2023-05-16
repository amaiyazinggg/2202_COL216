.data
	prompt1: .asciiz "Enter the value of x: "
	prompt2: .asciiz "Enter the value of n: "
	message: .asciiz "Answer is: "
	
.text
.globl main
main:
	li $v0,4 # Set to display mode
	la $a0,prompt1 # Load address of the prompt1 into a0
	syscall # Print the prompt onto the console
	
	li $v0,5 # Taking input of x
	syscall 
	add $s0, $v0, $zero # Placing input value in s0
	
	li $v0,4 # Set to display mode
	la $a0,prompt2 # Load address of the prompt2 into a0
	syscall # Print the prompt onto the console
	
	li $v0,5 #Taking input of n
	syscall
	add $s1, $v0, $zero #Placing input value of n in s1
	
	addi $v1, $zero, 1 #initialize v1 to 1
	
	jal func # Caller is calling the callee function func
	
exit:
	li $v0,4 # Set to display mode
	la $a0,message # Load address of the message into a0
	syscall # Print the prompt onto the console
	
	li $v0, 1 # Set to print integer mode
	add $a0, $v1, $zero # v1 contains the value returned by the function
	syscall # Print v1
	
	li $v0,10 # Exit syscall
	syscall
	
func:
	addi $sp, $sp, -12 # Decrement stack pointer by 12
	sw $s0, 0($sp) # Store s0 in stack
	sw $s1, 4($sp) # Store s1 in stack
	sw $ra, 8($sp) # Stores ra in stack
	
	beq $s1, $zero, end # When s1 is 0, end
	
	andi $t0, $s1, 1 # store last bit in t0
	
	srl $s1, $s1, 1 # shift n right by 1 bit 
	
	beq $t0, $zero, other # if last bit is 0 go to other
	
	mul $v1,$s0,$v1 # if last bit is 1 multiply product by s0
	
other:
	mul $s0,$s0,$s0 #square s0
	jal func #recurse

end:
	lw $s0, 0($sp) # Loading caller values back to the register
	lw $s1, 4($sp) # Loading caller values back to the register
	lw $ra, 8($sp) # Loading caller values back to the register
	addi $sp, $sp, 12 # Incrementing back the stack pointer
	
	jr $ra # Jump to return address

