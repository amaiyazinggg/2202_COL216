.data
	prompt1: .asciiz "Enter n: "
	message: .asciiz "Enter list elements: "
	prompt2: .asciiz "Enter x: "
	output: .asciiz "Yes at index "
	dne: .asciiz "Not found"
	
.text
.globl main
main:
	li $v0,4 # Set to display mode
	la $a0,prompt1 # Load address of the prompt1 into a0
	syscall # Print the prompt onto the console
	
	li $v0,5 #Taking input of n
	syscall 
	add $s0, $v0, $zero #Placing input value of n in s0
	
	li $v0,9 # Set to Dynamic Memory allocation mode
	li $a0,120 # Specifying maximum bytes as 30 x 4
	syscall # Allocates specified bytes in the memory and returns address of allocated memory in v0
	
	add $s1, $v0, $zero # Store address of the memory where list will be stored
	
	add $t0, $zero, $zero # Set t0 as 0
	
arrloop:
	beq $s0, $t0, continue # Break when t0 = n
	
	li $v0,4 # Set to display mode
	la $a0,message # Load address of the prompt1 into a0
	syscall # Print the prompt onto the console

	li $v0,5 # Taking input of array elements
	syscall 
	add $t1, $v0, $zero # Placing input value in t1
	
	sll $t2, $t0, 2 # Multiply t0 by 4 and store in t2
	add $t2, $s1, $t2 # Add t2 to base address of array
	sw $t1, 0($t2) # Placing input value in memory
	
	addi $t0, $t0, 1 # Increment t0 by 1
	
	j arrloop # jump to arrloop

continue:	
	li $v0,4 # Set to display mode
	la $a0, prompt2 # Load address of the prompt2 into a0
	syscall # Print the prompt onto the console
	
	li $v0,5 # Taking input of x
	syscall 
	
	add $s2, $v0, $zero # Placing input value of x in s2
	
	add $t0, $zero, $zero # initiate mid as 0
	add $t1, $zero, $zero # initiate start as 0
	addi $t2, $s0, -1 # initiate stop as n - 1
		
search_loop:
	add $t0, $t1, $t2 # mid = start + stop
	srl $t0, $t0, 1 # mid = mid//2
	sll $t5, $t0, 2 # t5 stores mid x 4
	add $t5, $t5, $s1 # t5 now stores 4*mid + base address of the array
	lw $t4, 0($t5) # Load the value at the t5 address into t4
	beq $t1, $t2, check # If start == stop, jump to check
	slt $t3, $t4, $s2 # if arr[mid] < x then t3 set to 1
	beq $t3, $zero, set # if t3 is zero, jump to set
	addi $t1, $t0, 1 # 
	j search_loop
	
set:
	add $t2, $t0, $zero # set stop = mid
	j search_loop # jump to search_loop
	
check:
	beq $s2, $t4, found_exit # If x = arr[mid], jump to found_exit
	
notfound_exit:
	li $v0,4 # Set to display mode
	la $a0,dne # Load address of the prompt1 into a0
	syscall # Print the prompt onto the console
	
	li $v0,10 # exit syscall
	syscall
	
found_exit:
	li $v0,4 # Set to display mode
	la $a0,output # Load address of the prompt1 into a0
	syscall # Print the prompt onto the console
	
	addi $v0, $zero, 1 # Set v0 to print integer mode
	add $a0, $t0, $zero # Set a0 to mid
	syscall # Print a0
	
	li $v0,10 # exit syscall
	syscall


	
	
	