addi $1, $0, 8
addi $2, $0, 5
addi $3, $0, 3
add $4, $1, $2
sub $5, $4, $3
sw $4, 1024($1)
lw $6, 1024($1)
mul $7, $6, $5
add $t1, $4, $5
addi $t2, $0, 2
add $t3, $t1, $t2
add $t4, $7, $t3
add $t5, $t4, $6
addi $t6, $0, 3
sub $t7, $t5, $t6
sw $t7, 1032($1)
lw $s0, 1032($1)
add $s1, $s0, $t7
addi $s2, $0, 4
sub $s3, $s1, $s2
add $t1, $t7, $s3
add $t2, $t1, $7
addi $t3, $0, 1
add $t4, $t2, $t3