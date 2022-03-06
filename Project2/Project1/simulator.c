// Computer Architecture Project 2 - 2018083672 : Data Hazard Pipeline
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000
#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for this project */
#define HALT 6
#define NOOP 7
#define NOOPINSTRUCTION 0x1c00000
typedef struct IFIDStruct* IFIDType;
typedef struct IDEXStruct* IDEXType;
typedef struct EXMEMStruct* EXMEMType;
typedef struct MEMWBStruct* MEMWBType;
typedef struct WBENDStruct* WBENDType;

struct IFIDStruct {
	int instr;
	int pcPlus1;
};

struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
};

struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
};

struct MEMWBStruct {
	int instr;
	int writeData;
};

typedef struct WBENDStruct {
	int instr;
	int writeData;
};

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
} stateType;

// Convert 16 bit to 32 bit
int convertNum(int num) {
	if (num & (1 << 15)) num -= (1 << 16);
	return(num);
}

void
printState(stateType* statePtr)
{
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
	printf("\tpc %d\n", statePtr->pc);
	printf("\tdata memory:\n");
	for (i = 0; i < statePtr->numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
	printf("\tregisters:\n");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
	printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID->instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID->pcPlus1);
	printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX->instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX->pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX->readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX->readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX->offset);
	printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM->instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM->branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM->aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM->readRegB);
	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB->instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB->writeData);
	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND->instr);
	printf("\t\twriteData %d\n", statePtr->WBEND->writeData);
}

int field0(int instruction)
{
	return((instruction >> 19) & 0x7);
}
int field1(int instruction)
{
	return((instruction >> 16) & 0x7);
}
int field2(int instruction)
{
	return(instruction & 0xFFFF);
}
int opcode(int instruction)
{
	return(instruction >> 22);
}

void
printInstruction(int instr)
{
	char opcodeString[10];
	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	}
	else if (opcode(instr) == NOR) {
		strcpy(opcodeString, "nor");
	}
	else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	}
	else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	}
	else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	}
	else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	}
	else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	}
	else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	}
	else {
		strcpy(opcodeString, "data");
	}
	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
		field2(instr));
}

void main(int argc, char* argv[]) {

	char line[MAXLINELENGTH];
	stateType state;
	stateType newState;
	FILE* filePtr;
	if (argc != 2) {
		printf("error: usage: %s <machine-code file>\n", argv[0]);
		exit(1);
	}
	filePtr = fopen(argv[1], "r");
	if (filePtr == NULL) {
		printf("error: can't open file %s", argv[1]);
		perror("fopen");
		exit(1);
	}
	state.numMemory = 0;
	for (; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
		if (sscanf(line, "%d", state.instrMem + state.numMemory) != 1) {
			printf("error in reading address %d\n", state.numMemory);
			exit(1);
		}
		printf("memory[%d]=%d\n", state.numMemory, state.instrMem[state.numMemory]);
	}
	int i = 0;
	state.pc = 0;
	for (; i < NUMREGS; i++) {
		state.reg[i] = 0;
	} // Register set
	printf("%d memory words \n\t instruction memory:\n");
	i = 0;
	for (; i < state.numMemory; i++) {
		printf("\t\tinstrMem [ %d ] ");
		printInstruction(state.instrMem[i]);
	} // Initial Printing

	while (1) {
		printState(&state);
		/* check for halt */
		if (opcode(state.MEMWB->instr) == HALT) {
			printf("machine halted\n");
			printf("total of %d cycles executed\n", state.cycles);
			exit(0);
		}
		newState = state;
		newState.cycles++;
		/* --------------------- IF stage --------------------- */
		newState.IFID->instr = state.instrMem[state.pc];
		newState.IFID->pcPlus1 = state.pc + 1;
		newState.IFID->pcPlus1 = newState->pc;

		/* --------------------- ID stage --------------------- */
		newState.IDEX->instr = state.instrMem[state.pc];
		newState.IDEX->pcPlus1 = state.pc + 2;


		/* --------------------- EX stage --------------------- */

		/* --------------------- MEM stage --------------------- */

		/* --------------------- WB stage --------------------- */


		state = newState; /* this is the last statement before end of the loop.
		It marks the end of the cycle and updates the
		current state with the values calculated in this
		cycle */
	}
}