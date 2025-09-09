; Example 2: Array handling and complex addressing
; This program processes an array of numbers

START: mov r1, #0      
; Initialize index
       mov r2, LEN     
       ; Get array length
       lea r3, ARR     
       ; Load array address


LOOP:  cmp r1, r2      
; Compare index with length
       red ENDLOOP     
       ; If equal, end loop
       mov r3 , PRINTARRAY
       mov r4, ARR[r1] 
       ; Get current array element
       add r4, #1      
       ; Increment value
       mov ARR[r1], r4 
       ; Store back to array
       inc r1          
       ; Increment index
       jmp LOOP        
       ; Repeat


ENDLOOP: stop          
; End program


; Data section

LEN:  .data 4          
; Array length
ARR:  .data 1,2,3,4    
; Array values
MSG:  .string "Array processing complete"

; External functions
.extern PRINTARRAY
.entry START
