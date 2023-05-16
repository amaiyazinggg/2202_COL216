.data
	prompt: .asciiz "Enter input string: "
	message: .asciiz "Output is: "
	
.text
.globl main
main:
	li $v0,9 # Set to Dynamic Memory allocation mode
	li $a0,100 # Specifying maximum bytes as 100
	syscall # Allocates specified bytes in the memory and returns address of allocated memory in v0
	
	add $s0, $v0, $zero
	
	li $v0,4 # Set to display mode
	la $a0,prompt # Load address of the prompt into a0
	syscall # Print the prompt onto the console
	
	li $v0,8 # Set to read string mode
	add $a0, $s0, $zero # Put the address of the dynamic memory into a0
	li $a1,100 # Set maximum character to be read as 100
	syscall
	
	li $v0,4 # Set to display mode
	la $a0,message # Load address of the message into a0
	syscall # Print the message onto the console
	
	li $v0,4  # Set to display mode
	add $a0, $s0, $zero # Load address of the string into a0
	syscall # Print the string onto the console
	
	li $v0,10 # Exit syscall
	syscall