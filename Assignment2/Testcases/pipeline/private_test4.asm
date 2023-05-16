addi $t1, $0, 0
addi $t2, $0, 3
addi $t3, $0, 0
addi $t4, $0, 3
addi $t5, $0, 0
outerloop:
addi $t3, $0, 0
innerloop:
add $t5, $t5, $t1
add $t5, $t5, $t3
addi $t3, $t3, 1
bne $t3, $t4, innerloop
addi $t1, $t1, 1
bne $t1, $t2, outerloop