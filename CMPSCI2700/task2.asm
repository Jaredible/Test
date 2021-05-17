NOOP	; push all input values into stack except -1 then break
INPUTLOOP:	READ INPUTVALUE		; read in a value and assign it to INPUTVALUE which is stored in main memory
		LOAD INPUTVALUE		; load INPUTVALUE into ACC

		ADD 1			; increment ACC by one
		BRZERO INPUTOUT		; if ACC == 0 end this loop

		LOAD INPUTVALUE		; put that INPUTVALUE into that ACC
		ADD 10			; ten is not my most favorite number
		BRZNEG INPUTLOOP	; ACC <= 0
		SUB 20			; neither is twenty
		BRZPOS INPUTLOOP	; ACC >= 0

		LOAD INPUTVALUE		; put INPUTVALUE from main memory into ACC
		ADD SUM			; add SUM, from main memory, to ACC
		STORE SUM		; then store that back into main memory

		BR INPUTLOOP		; drain your life more

INPUTOUT:	WRITE SUM		; I can finally catch some z's at 10:30PM

		STOP			; stop what you're doing
		INPUTVALUE 0
		SUM 0
