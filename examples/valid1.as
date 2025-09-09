; Example 1: A program demonstrating various assembly operations and data types
; This program adds numbers and stores results

MAIN: mov r3, LENGTH    
; Move length to r3
LOOP: mov r6, STR[r3]   
; Get current character
      cmp r6, #0        
      ; Check if end of string
      jmp END           
      ; If equal, jump to end
      dec r3           
      ; Decrease counter
      jmp LOOP         
      ; Repeat
END:  stop             
; End program

; Data declarations
LENGTH: .data 6
STR: .string "Hello!"   
; String with 6 chars

; External declarations
.extern PRINTF
; Entry declarations
.entry MAIN
