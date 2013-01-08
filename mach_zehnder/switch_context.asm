#define IOA_SPL         0x5d
#define IOA_SPH         0x5e
#define IOA_SREG        0x5f


.globl _switch_context
_switch_context:
	push r0
	push r1
	push r2
	push r3
	push r4
	push r5
	push r6
	push r7
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push r16
	push r17
	push r18
	push r19
	push r20
	push r21
	push r22
	push r23
	push r24
	push r25
	push r26
	push r27
	push r28
	push r29
	push r30
	push r31
	lds   r31,IOA_SREG 
	push r31
	ldi r30,lo8(label1)
	ldi r31,hi8(label1)

	call label1
label1:
	pop r31
	pop r30 
	adiw r30,(label2 - label1) >> 1
	push r30
	push r31

	/* now exchange stack */
	mov r30,r22
	mov r31,r23
	lds r22,IOA_SPL
	lds r23,IOA_SPH 
	st Z+,r22
	st Z,r23
	
	mov r30,r24
	mov r31,r25
	ld r24,Z+ 
	ld r25,Z 
	cli
	sts IOA_SPL,r24
	sts IOA_SPH,r25
	sei
	ret

label2:
	pop r31
	sts IOA_SREG,r31	

	pop r31
	pop r30
	pop r29
	pop r28
	pop r27
	pop r26
	pop r25
	pop r24
	pop r23
	pop r22
	pop r21
	pop r20
	pop r19
	pop r18
	pop r17
	pop r16
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2
	pop r1
	pop r0
	ret
