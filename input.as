; file ps.as
.entry LIST
.extern W
MAIN: add r3, LIST
LOOP: prn #48
lea W, r6
inc r6
mcro soho
cmp r6, #0 
jmp END
dec r3
mcroend
mov r3, K
sub r1, r4
bne END
cmp K, #-6
bne &END
dec W
.entry MAIN
soho
jmp &LOOP
add L3, L3
END: stop
STR: .string "abcd"
LIST: .data 6, -9
.data -100
K: .data 31
.extern L3