LOOP: NOOP
	READ TEMP
	LOAD SUM
	ADD TEMP
	STORE SUM
	LOAD SUM
	SUB TOTAL
	BRNEG LOOP
	BRPOS LOOP

WRITE SUM

STOP
TOTAL 100
TEMP 0
SUM 0
