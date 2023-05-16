addi $t1, $0, 4
addi $t2, $0, 8
add $t3, $t1, $t2
sw $t3, 1000($t1)
addi $t4, $0, 12
lw $t5, 1000($t1)
add $t6, $t5, $t4
add $t7, $t6, $t3