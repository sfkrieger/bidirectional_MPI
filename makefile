EXECS=electleader 
MPICC?=mpicc

all: ${EXECS}

electleader: elect_leader.c
	${MPICC} -o electleader elect_leader.c
	
clean:
	rm ${EXECS}
