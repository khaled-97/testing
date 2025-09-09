; Example of syntax and semantic assembly errors

LABEL: .entry LABEL    
; Error: Label cannot be both entry and defined
LABEL: mov r1, r2      
; Error: Label defined twice


       mov #1, #2      
       ; Error: Invalid addressing - immediate to immediate
       add STR, r1     
       ; Error: Cannot add string to register
       lea #1, r1      
       ; Error: Invalid source operand for lea
       stop r1         
       ; Error: Stop doesn't take operands

; Invalid label usage
       jmp            
       ; Error: Missing label operand
Next   mov r1, r2     
; Error: Label missing colon
       jmp NON_EXIST  
       ; Error: Jump to undefined label

; Directive errors
.data "123"           
; Error: String in .data directive
.string 123           
; Error: Number in .string directive
.data 5, , 6         
; Error: Empty value in data list
.entry                
; Error: Entry without label
.extern LABEL        
; Error: Label cannot be both external and defined
