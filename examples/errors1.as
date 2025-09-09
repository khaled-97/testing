; Example of common assembly errors

1INVALID: mov r1, r2    
; Error: Label cannot start with number
LOOP$ mov r3, #4       
; Error: Invalid label character '$'
      movv r1, r2      
      ; Error: Unknown operation 'movv'
      add              
      ; Error: Missing operands
      mov r8, LENGTH   
      ; Error: Invalid register number (r8)
      cmp r1, r2, r3   
      ; Error: Too many operands

; Data declaration errors
DATA1: .data 1.5       
; Error: Data must be integer
DATA2: .data           
; Error: Missing data values
.string "Unterminated  
; Error: Unterminated string


; Invalid entry/extern

.entry 123             
; Error: Entry must be valid label
.extern                
; Error: Missing external label
