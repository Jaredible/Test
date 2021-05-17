NOOP	; push all input values into stack except -1 then break
INPUTLOOP:	READ INPUTVALUE		; read in a value and assign it to INPUTVALUE which is stored in main memory
		LOAD INPUTVALUE		; load INPUTVALUE into ACC

		ADD 1
		BRZERO INPUTOUT

		LOAD INPUTVALUE
		PUSH			; increment stack
		STACKW 0		; then copy ACC into stack position zero

		ADD SUM			; add SUM, from main memory, to ACC
		STORE SUM		; then store that back into main memory as SUM

		LOAD SIZE
		ADD 1
		STORE SIZE

		BR INPUTLOOP		; you're waisting your time but go ahead

INPUTOUT:	NOOP

		COPY INDEX SIZE
		LOAD INDEX
		SUB 1
		STORE INDEX
DISPLAYLOOP:	NOOP
		LOAD SIZE
		BRZERO DISPLAYOUT

		STACKR 0
		POP
		
		STORE DISPLAYVALUE
		WRITE DISPLAYVALUE

		LOAD INDEX
		SUB 1
		BRNEG DISPLAYOUT
		STORE INDEX

		BR DISPLAYLOOP		; getting closer

DISPLAYOUT:	WRITE SUM		; finally I can catch some z's. time is 10:30PM NOV 7, 2018. DONE. *mic drop*

		STOP
		INPUTVALUE 0
		SUM 0
		SIZE 0
		INDEX 0
		DISPLAYVALUE 0

