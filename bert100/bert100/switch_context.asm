
/*************************************************************************************************
 # This is the CPU dependent part of the Thread Pool Operating system
 # The following code exchanges the register set and the stack 
 # For this it requires two arguments, one pointer to the new stack and one to the old stack
 #************************************************************************************************
*/
.globl _switch_context
_switch_context:
	pushm r1-r15
	pushc PSW 
	pushc FPSW 
	mov.l #label1,r5
	jsr r5
label1:
	pop r5
	add #(label2 - label1),r5
	push r5
	mov r0,[r2]
	mov [r1],r0
	rts
label2:
	popc FPSW
	popc PSW
	popm r1-r15
	rts
