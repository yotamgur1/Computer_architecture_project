// ===========================================================================================================================
//													Declarations
// ===========================================================================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>


// ===========================================================================================================================
//												  Definitions & Globals
// ===========================================================================================================================

#define INPUT_MAX_LEN 500
#define MEMIN_LINE_LENGTH 8
#define NUM_OF_REGISTERS 16
#define MEM_DEPTH 4096
#define LINE_LENGTH 32
#define PARAMETER_MAX 12
#define BUFFER 20
#define INSTRUCTION_Q_SIZE 16

typedef enum {
	LD = 0,
	ST = 1,
	ADD = 2,
	SUB = 3,
	MULT = 4,
	DIV = 5,
	HALT = 6
} Operations;


typedef enum {
	F0 = 0,
	F1 = 1,
	F2 = 2,
	F3 = 3,
	F4 = 4,
	F5 = 5,
	F6 = 6,
	F7 = 7,
	F8 = 8,
	F9 = 9,
	F10 = 10,
	F11 = 11,
	F12 = 12,
	F13 = 13,
	F14 = 14,
	F15 = 15
} ProcRegisters;

typedef enum {
	available = 0,
	busy = 1,
	first_time = 2
} Reg_Status;

typedef struct instruction {
	short int	OPCODE; 	 //	4 bits in use
	short int	DST;	 	 //	4 bits in use
	short int	SRC0;	 	 //	4 bits in use
	short int	SRC1;	 	 //	4 bits in use
	short int	IMM;		 //  12 bits in use; unlike the assembler, the simulator wants to uniqely identify negative numbers
	int	UNIT;				 //the unit execute the instruction
	int ADD_TO_Q;			 //hold the cycle when instruction was added to Q
	int CYCLE_ISSUED;		 //holds the cyckle when instruction was issued
	int	CYCLE_READ_OPERANDS; //holds the cyckle when instruction was in read_operands
	int CYCLE_MEM_READY;	 // for st/ld ops- holds when the memory was ready
	int	CYCLE_EXECUTE_END;   //holds the cyckle when instruction finished execution
	int	CYCLE_WRITE_RESULT;	 //holds the cyckle when instruction was in write result
	int qk[2];				 //hold PC for the instruction they are waiting for
	int qj[2];				 //hold PC for the instruction they are waiting for
	int rk;				     //hold available\busy for availability
	int rj;				     //hold available\busy for availability
	int rk_cycle_change;	 //hold the cycle when rk changed
	int rj_cycle_change;	 //hold the cycle when rj changed
	float fi;				 //hold R[instruction.DST]
	float fj;				 //hold R[instruction.SRC0]
	float fk;				 //hold R[instruction.SRC1]
	int PC;					 //number of instruction from MEM
	int unit_number;		 //holds the unit's number that "took" this instruction
	int waits_for_mem;	     // hold the pc of the st/ld that writes/reads to the mem addr or -1 if not waiting


} Instruction;


//=====================================================
//					  Declerations
//=====================================================
float R[NUM_OF_REGISTERS] = { 0 };
int r_busy[NUM_OF_REGISTERS][3] = { { -1, -1, -1},{ -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}, \
									{ -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1},\
									{ -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1},\
									{ -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}, \
}; //each line is a reg- first ccolumn is the unit, second is te unit number, third is pc
int MEM[MEM_DEPTH] = { 0 };
unsigned int cycle = 0;
unsigned int PC = 0;
int add_nr_units;
int sub_nr_units;
int mul_nr_units;
int div_nr_units;
int ld_nr_units;
int st_nr_units;
int add_delay;
int sub_delay;
int mul_delay;
int div_delay;
int ld_delay;
int st_delay;
int Q_index = 0;
int print_checker = 1;

char trace_unit[BUFFER];
int unit_for_trace;
double unit_number_for_trace = 0;

int halt_hit;
Instruction instruction_Q[INSTRUCTION_Q_SIZE];
Instruction array_for_trace_inst[MEM_DEPTH];
int how_many_insts = 0;
int pc_to_unit[MEM_DEPTH][2];	 // holds which pc was in which unit- each row is a pc- first column is the unit and second column is the unit number
int mem_busy[MEM_DEPTH] = { 0 }; // -1 for free PC for busy(PC represent the instruction that tryed to use this addr for the last time)
int num_of_inst_started = 0;
int num_of_inst_done = 0;





//=====================================================
//				Initialize Fucntions
//=====================================================
void initialize_mem_busy() { //initialive mem_busy array to -1
	for (int i = 0; i < MEM_DEPTH; i++) {
		mem_busy[i] = -1;
	}
}
void initialize_reg() { //initialive the register to thier starting values
	int i;
	float j = 0.0;
	for (i = 0; i < NUM_OF_REGISTERS; i++) {
		R[i] = j;
		j = j + 1;
	}
}

float convert_int_to_float(int x) { //function that converts int to float
	union {
		int x;
		float f;
	} data;

	data.x = x;
	return data.f;
}

int convert_float_to_int(float f) { //function that converts float to int
	union {
		int x;
		float f;
	} data;

	data.f = f;
	return data.x;
}
void initialize_cfg(FILE* file_cfg) { //initialize the cfg values from cfg.txt
	char parameter[PARAMETER_MAX + 1];
	fscanf(file_cfg, "add_nr _units = %d", &add_nr_units);
	fscanf(file_cfg, "\nsub_nr_units = %d", &sub_nr_units);
	fscanf(file_cfg, "\nmul_nr_units = %d", &mul_nr_units);
	fscanf(file_cfg, "\ndiv_nr_units = %d", &div_nr_units);
	fscanf(file_cfg, "\nld_nr_units = %d", &ld_nr_units);
	fscanf(file_cfg, "\nst_nr_units = %d", &st_nr_units);
	fscanf(file_cfg, "\nadd_delay = %d", &add_delay);
	fscanf(file_cfg, "\nsub_delay = %d", &sub_delay);
	fscanf(file_cfg, "\nmul_delay = %d", &mul_delay);
	fscanf(file_cfg, "\ndiv_delay = %d", &div_delay);
	fscanf(file_cfg, "\nld_delay = %d", &ld_delay);
	fscanf(file_cfg, "\nst_delay = %d", &st_delay);
	fscanf(file_cfg, "\ntrace_unit = %s", &trace_unit);
}

int load_mem(FILE* file_memin) {   //load memin.txt into MEM[]
	int i = 0;
	char data[MEMIN_LINE_LENGTH + 1];
	while (fgets(data, MEMIN_LINE_LENGTH + 1, file_memin)) {
		if (strcmp(data, "\n") == 0)		//if its the end of line it sound'lt be counted as a line
			continue;
		MEM[i] = strtoll(data, NULL, 16);
		i++;
	}
}

Instruction* initialize_units(Instruction* add_units, Instruction* sub_units, Instruction* mul_units, \
	Instruction* div_units, Instruction* ld_units, Instruction* st_units) { //initialize all units to have -1 in their OPCODE field
	int i;
	for (i = 0; i < add_nr_units; i++) {
		add_units[i].OPCODE = -1;
	}
	for (i = 0; i < sub_nr_units; i++) {
		sub_units[i].OPCODE = -1;
	}
	for (i = 0; i < mul_nr_units; i++) {
		mul_units[i].OPCODE = -1;
	}
	for (i = 0; i < div_nr_units; i++) {
		div_units[i].OPCODE = -1;
	}
	for (i = 0; i < ld_nr_units; i++) {
		ld_units[i].OPCODE = -1;
	}
	for (i = 0; i < st_nr_units; i++) {
		st_units[i].OPCODE = -1;
	}
	return add_units, sub_units, mul_units, div_units, ld_units, st_units;
}


void instruction_decoder(long long int instruction_int, Instruction* instruction) { //decode the instrucrion
	instruction->OPCODE = ((instruction_int & 0xF000000) >> 24);
	instruction->DST = ((instruction_int & 0xF00000) >> 20);
	instruction->SRC0 = ((instruction_int & 0xF0000) >> 16);
	instruction->SRC1 = ((instruction_int & 0xF000) >> 12);
	instruction->IMM = ((instruction_int & 0xFFF));
	instruction->PC = PC;
}

int check_if_reg_available(Instruction* instruction) { //check if the needed registers are available by using the r_busy array
	int i;
	for (i = 0; i < NUM_OF_REGISTERS; i++)
		if ((r_busy[instruction->DST]) || (r_busy[instruction->SRC0]) || (r_busy[instruction->SRC1])) {
			return 1;
		}
	return 0;
}

void print_to_memout(FILE* file_memout) {  //called at the end of the run and print the MEM values into memout.txt
	int index_to_print;
	for (int i = 0; i < MEM_DEPTH; i++) { //prevent us from printing zeros at the end of MEMOUT.txt
		if (MEM[i] != 0) {
			index_to_print = i + 1;
		}
	}
	for (int i = 0; i < index_to_print; i++)
		fprintf(file_memout, "%.8x\n", (MEM[i] & 0xFFFFFFFF));

}

void print_to_regout(FILE* file_regout) { //called in the end of the run and prints the values of registers into regout.txt
	for (int i = 0; i < NUM_OF_REGISTERS; i++) {
		fprintf(file_regout, "%.6f\n", R[i]);
	}
}

void print_to_traceinst(FILE* file_traceinst) { //called in the end of the run and prints to traceinst.txt using the array_for_trace_inst array
	for (int i = 0; i < how_many_insts; i++) {

		switch ((MEM[i] & 0x0f000000) >> 24) //checks the OPCODE if the next instruction to be printed
		{
		case ADD:
			fprintf(file_traceinst, " %.8x %d ADD%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		case SUB:
			fprintf(file_traceinst, " %.8x %d SUB%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		case MULT:
			fprintf(file_traceinst, " %.8x %d MULT%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		case DIV:
			fprintf(file_traceinst, " %.8x %d DIV%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		case LD:
			fprintf(file_traceinst, " %.8x %d LD%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		case ST:
			fprintf(file_traceinst, " %.8x %d ST%d %d %d %d %d\n", MEM[i], array_for_trace_inst[i].PC, array_for_trace_inst[i].unit_number, array_for_trace_inst[i].CYCLE_ISSUED, array_for_trace_inst[i].CYCLE_READ_OPERANDS, array_for_trace_inst[i].CYCLE_EXECUTE_END, array_for_trace_inst[i].CYCLE_WRITE_RESULT);
			break;
		}


	}

}

void find_which_trace_unit() { //find the unit to be trace according to whats specified in cfg.txt
	char str[100];
	switch (trace_unit[0]) {
	case 65: // "A"
		unit_for_trace = ADD;
		strcpy(str, trace_unit + 3);
		unit_number_for_trace = atoi(str);
		break;
	case 83: // "S"
		if (trace_unit[1] == 85) {
			unit_for_trace = SUB;
			strcpy(str, trace_unit + 3);
			unit_number_for_trace = atoi(str);
		}
		else {
			unit_for_trace = ST;
			strcpy(str, trace_unit + 2);
			unit_number_for_trace = atoi(str);
		}
		break;
	case 77: // "M"
		unit_for_trace = MULT;
		strcpy(str, trace_unit + 3);
		unit_number_for_trace = atoi(str);
		break;
	case 68: // "D"
		unit_for_trace = DIV;
		strcpy(str, trace_unit + 3);
		unit_number_for_trace = atoi(str);
		break;
	case 76: // "L"
		unit_for_trace = LD;
		strcpy(str, trace_unit + 2);
		unit_number_for_trace = atoi(str);
		break;
	}

}

char* conv_src_to_char(int src) { //convert the code we used to string with the register's name
	char str[4];
	switch (src) {
	case 0:  strcpy(str, "F0"); break;
	case 1:  strcpy(str, "F1"); break;
	case 2:  strcpy(str, "F2"); break;
	case 3:  strcpy(str, "F3"); break;
	case 4:  strcpy(str, "F4"); break;
	case 5:  strcpy(str, "F5"); break;
	case 6:  strcpy(str, "F6"); break;
	case 7:  strcpy(str, "F7"); break;
	case 8:  strcpy(str, "F8"); break;
	case 9:  strcpy(str, "F9"); break;
	case 10: strcpy(str, "F10"); break;
	case 11: strcpy(str, "F11"); break;
	case 12: strcpy(str, "F12"); break;
	case 13: strcpy(str, "F13"); break;
	case 14: strcpy(str, "F14"); break;
	case 15: strcpy(str, "F15"); break;
	}
	return str;
}

char* conv_unit_to_char(int src) { //converts the unit to string with the OPCODE name
	char str[5];
	switch (src) {
	case 0:  strcpy(str, "LD"); break;
	case 1:  strcpy(str, "ST"); break;
	case 2:  strcpy(str, "ADD"); break;
	case 3:  strcpy(str, "SUB"); break;
	case 4:  strcpy(str, "MULT"); break;
	case 5:  strcpy(str, "DIV"); break;
	case -1: strcpy(str, "-"); break;
	}
	return str;
}



void print_to_trace_unit(Instruction inst, FILE* trace_unit_file) { //check unit's status and print required fields to traceunit.txt
	char rj[4];
	char rk[4];
	char fi[4];
	char fj[4];
	char fk[4];
	char qj_unit[4];
	char qj_unit_num[10];
	char qk_unit[10];
	char qk_unit_num[10];
	char temp_str_j[10];
	char temp_str_k[10];
	if (inst.CYCLE_ISSUED < cycle) {
		if (inst.CYCLE_READ_OPERANDS == inst.CYCLE_ISSUED + 1) { // both done in the first try of read operands
			strcpy(rj, "Yes");
			strcpy(rk, "Yes");
		}
		else { //not read for both fj/fk
			if (cycle == inst.CYCLE_ISSUED + 1) { // we are in the first try of read operands
				if (inst.rj_cycle_change == cycle) strcpy(rj, "Yes");
				else strcpy(rj, "No");
				if (inst.rk_cycle_change == cycle) strcpy(rk, "Yes");
				else strcpy(rk, "No");
			}
			else { //not in the first try of read operands
				if (cycle > inst.rj_cycle_change && !((inst.OPCODE == ST) && (cycle > inst.CYCLE_READ_OPERANDS && (inst.CYCLE_EXECUTE_END == -1)))) {
					strcpy(rj, "Yes");
				}
				else {
					strcpy(rj, "No");
				}
				if ((cycle > inst.rk_cycle_change) || (inst.rk != 1) && (cycle == inst.CYCLE_READ_OPERANDS)) {
					strcpy(rk, "Yes");
				}
				else strcpy(rk, "No");
			}


		}

		if (inst.CYCLE_EXECUTE_END <= cycle) {
			strcpy(rj, "No");
			strcpy(rk, "No");
		}

		if (strcmp(rj, "No")) {
			strcpy(qj_unit, "-");
			strcpy(qj_unit_num, "");
		}
		else {
			strcpy(qj_unit, conv_unit_to_char(inst.qj[0]));
			if (!strcmp(qj_unit, "-")) strcpy(qj_unit_num, "");
			else {
				sprintf(temp_str_j, "%d", inst.qj[1]);
				strcpy(qj_unit_num, temp_str_j);
			}

		}
		if (strcmp(rk, "No")) {
			strcpy(qk_unit, "-");
			strcpy(qk_unit_num, "");
		}
		else {
			strcpy(qk_unit, conv_unit_to_char(inst.qk[0]));
			if (!strcmp(qk_unit, "-")) strcpy(qk_unit_num, "");
			else {
				sprintf(temp_str_k, "%d", inst.qk[1]);
				strcpy(qk_unit_num, temp_str_k);
			}

		}

		if (!strcmp(rj, "No") && inst.CYCLE_EXECUTE_END <= cycle) {
			strcpy(qj_unit, "-");
			strcpy(qj_unit_num, "");
		}

		if (!strcmp(rk, "No") && inst.CYCLE_EXECUTE_END <= cycle) {
			strcpy(qk_unit, "-");
			strcpy(qk_unit_num, "");
		}



		strcpy(fi, conv_src_to_char(inst.DST));
		strcpy(fj, conv_src_to_char(inst.SRC0));
		strcpy(fk, conv_src_to_char(inst.SRC1));

		if ((inst.OPCODE == ST) && (rk[0] == 78) && (rj[0] == 78)) { //ST in execution
			strcpy(qk_unit, "-");
			strcpy(qk_unit_num, "");
		}

		if ((inst.CYCLE_READ_OPERANDS < cycle) && (inst.CYCLE_EXECUTE_END == -1)) { //ST in execution
			strcpy(rj, "No");
			strcpy(rk, "No");
		}
		

		switch (inst.UNIT) {
		case ADD:
			fprintf(trace_unit_file, "%d ADD%d %s %s %s %s%s %s%s %s %s\n", cycle, inst.unit_number, fi, fj, fk, qj_unit, qj_unit_num, qk_unit, qk_unit_num, rj, rk);
			break;
		case SUB:
			fprintf(trace_unit_file, "%d SUB%d %s %s %s %s%s %s%s %s %s\n", cycle, inst.unit_number, fi, fj, fk, qj_unit, qj_unit_num, qk_unit, qk_unit_num, rj, rk);
			break;
		case MULT:
			fprintf(trace_unit_file, "%d MULT%d %s %s %s %s%s %s%s %s %s\n", cycle, inst.unit_number, fi, fj, fk, qj_unit, qj_unit_num, qk_unit, qk_unit_num, rj, rk);
			break;
		case DIV:
			fprintf(trace_unit_file, "%d DIV%d %s %s %s %s%s %s%s %s %s\n", cycle, inst.unit_number, fi, fj, fk, qj_unit, qj_unit_num, qk_unit, qk_unit_num, rj, rk);
			break;
		case LD:
			fprintf(trace_unit_file, "%d LD%d %s - - - - %s %s\n", cycle, inst.unit_number, fi, rj, rk);
			break;
		case ST:
			fprintf(trace_unit_file, "%d ST%d - - %s - %s%s %s %s\n", cycle, inst.unit_number, fk, qk_unit, qk_unit_num, rj, rk);
			break;
		}
	}
	return;

}

void check_which_unit_for_trace_unit(Instruction* units, int num_of_units, FILE* trace_unit_file) { //check which unit should be traced according to unit,
	//if so it sends the values to pint_to_trace_unit function
	for (int i = 0; i < num_of_units; i++) {
		if (units[i].UNIT == unit_for_trace && units[i].unit_number == unit_number_for_trace && (units[i].PC != -1 || units[i].CYCLE_WRITE_RESULT == cycle)) {
			print_to_trace_unit(units[i], trace_unit_file);
		}
	}
	return 0;

}

int add_to_fifo(Instruction instrucion) { //the function recieves an instruction and add it to FIFO, return 1 if it worked and zero if not
	if (Q_index < INSTRUCTION_Q_SIZE) { //checks that queue is not full
		instruction_Q[Q_index] = instrucion;
		instruction_Q[Q_index].ADD_TO_Q = cycle;
		Q_index++;
		return 1;
	}
	else {
		printf("fetch is stuck cycle=%0d, pc=%d\n", cycle, instrucion.PC);
		return 0;
	}
}

void remove_from_fifo() { //this function removes function from fifo and updates the Q_index
	Instruction* return_instruction = &instruction_Q[0];
	int i;
	for (i = 0; i < INSTRUCTION_Q_SIZE - 1; i++) {
		instruction_Q[i] = instruction_Q[i + 1];
	}
	instruction_Q[INSTRUCTION_Q_SIZE - 1].OPCODE = -1;
	Q_index--;
}


Instruction* issue_initialize(Instruction* units, int nr_units) { //initialize the units' fields to starting values of -1
	for (int i = 0; i < nr_units; i++) {
		if ((units[i].OPCODE == -1) && (cycle > 0) && units[i].CYCLE_EXECUTE_END != cycle - 1) { // the last condition is for the case an operation waits for unit but we dont want to overide a unit the needs to wirte it's result in this cycle
			units[i] = instruction_Q[0];
			remove_from_fifo();
			units[i].UNIT = units[i].OPCODE;
			units[i].CYCLE_ISSUED = cycle;
			units[i].CYCLE_READ_OPERANDS = -1;
			units[i].CYCLE_EXECUTE_END = -1;
			units[i].CYCLE_WRITE_RESULT = -1;
			units[i].rj = first_time;
			units[i].rk = first_time;
			units[i].rj_cycle_change = -1;
			units[i].rk_cycle_change = -1;
			units[i].unit_number = i;
			units[i].qj[0] = -1;
			units[i].qj[1] = -1;
			units[i].qk[0] = -1;
			units[i].qk[1] = -1;
			units[i].waits_for_mem = -1;
			pc_to_unit[units[i].PC][0] = units[i].UNIT;
			pc_to_unit[units[i].PC][1] = units[i].unit_number;
			i = nr_units;

			num_of_inst_started++;
		}
	}
}

void issue(Instruction* add_units, Instruction* sub_units, Instruction* mul_units, Instruction* div_units, \
	Instruction* ld_units, Instruction* st_units) { //this function receives the units and starts their issue stage according to the first instruction in the queue
	int op = instruction_Q[0].OPCODE;
	int local_pc = instruction_Q[0].PC;
	int i;
	switch (op)
	{
	case ADD:
		issue_initialize(add_units, add_nr_units);
		break;
	case SUB:
		issue_initialize(sub_units, sub_nr_units);
		break;
	case MULT:
		issue_initialize(mul_units, mul_nr_units);
		break;
	case DIV:
		issue_initialize(div_units, div_nr_units);
		break;
	case LD:
		issue_initialize(ld_units, ld_nr_units);
		break;
	case ST:
		issue_initialize(st_units, st_nr_units);
		break;
	case HALT:
		halt_hit = 1;
		break;

	}
	return;
}

void update_r_busy(Instruction inst) { //updates r_busy when registers are in use

	if ((r_busy[inst.DST][0] == -1) || (r_busy[inst.DST][2] < inst.PC)) {
		r_busy[inst.DST][0] = inst.UNIT;
		r_busy[inst.DST][1] = inst.unit_number;
		r_busy[inst.DST][2] = inst.PC;
	}

}

void read_operands_check_for_first_time(Instruction* unit, int nr_units) { //when function is in the read operands stage for the first time it uses this function
	for (int y = 0; y < nr_units; y++) {
		if ((unit[y].rj == first_time) && (unit[y].rk == first_time) && (unit[y].CYCLE_ISSUED < cycle)) {
			if (unit[y].OPCODE != LD && unit[y].OPCODE != ST) {
				// if we in the first time we should check if the src reg is available:
				//if yes we take the reg value and
				//if not we take the PC of the command we are waiting for to fj and update rj to busy
				if (r_busy[unit[y].SRC0][0] == -1 || unit[y].SRC0 == 0) {
					unit[y].fj = R[unit[y].SRC0];
					unit[y].rj = available;
					unit[y].rj_cycle_change = cycle;
				}
				else {
					unit[y].qj[0] = r_busy[unit[y].SRC0][0];
					unit[y].qj[1] = r_busy[unit[y].SRC0][1];
					unit[y].rj = busy;
				}

				// if we in the first time we should check if the src reg is available:
				//if yes we take the reg value and
				//if not we take the PC of the command we are waiting for
				if (r_busy[unit[y].SRC1][0] == -1 || unit[y].SRC1 == 0) {
					unit[y].fk = R[unit[y].SRC1];
					unit[y].rk = available;
					unit[y].rk_cycle_change = cycle;

				}
				else {
					unit[y].qk[0] = r_busy[unit[y].SRC1][0];
					unit[y].qk[1] = r_busy[unit[y].SRC1][1];
					unit[y].rk = busy;
				}
			}
			if (unit[y].OPCODE == ST) { // for ST we only care for src1
				unit[y].fj = R[unit[y].SRC0];
				unit[y].rj = available;
				unit[y].rj_cycle_change = cycle;
				if (r_busy[unit[y].SRC1][0] == -1 || unit[y].SRC1 == 0) {
					unit[y].fk = R[unit[y].SRC1];
					unit[y].rk = available;
					unit[y].rk_cycle_change = cycle;
				}
				else {
					unit[y].qk[0] = r_busy[unit[y].SRC1][0];
					unit[y].qk[1] = r_busy[unit[y].SRC1][1];
					unit[y].rk = busy;
				}

				if (mem_busy[unit[y].IMM] != -1) unit[y].waits_for_mem = mem_busy[unit[y].IMM];

			}
			if (unit[y].OPCODE == LD) {
				unit[y].fj = R[unit[y].SRC0];
				unit[y].rj = available;
				unit[y].rj_cycle_change = cycle;
				unit[y].fk = R[unit[y].SRC1];
				unit[y].rk = available;
				unit[y].rk_cycle_change = cycle;
				if (mem_busy[unit[y].IMM] == -1) unit[y].fi = convert_int_to_float(MEM[unit[y].IMM]);
				else unit[y].waits_for_mem = mem_busy[unit[y].IMM];

			}
			// if it is the first time and we updated both of the regs we update the read operands cycle
			if (unit[y].OPCODE != ST) update_r_busy(unit[y]);
			if (unit[y].OPCODE == ST) { // check if the addr is free
				if (mem_busy[unit[y].IMM] < unit[y].PC)
					mem_busy[unit[y].IMM] = unit[y].PC;
			}
			if ((unit[y].rj == available) && (unit[y].rk == available)) {
				unit[y].CYCLE_READ_OPERANDS = cycle;
			}
		}
	}
}

void read_operands(Instruction* add_units, Instruction* sub_units, Instruction* mul_units, \
	Instruction* div_units, Instruction* ld_units, Instruction* st_units) { //this function calls for the read operand function for each unit
	read_operands_check_for_first_time(add_units, add_nr_units);
	read_operands_check_for_first_time(sub_units, sub_nr_units);
	read_operands_check_for_first_time(mul_units, mul_nr_units);
	read_operands_check_for_first_time(div_units, div_nr_units);
	read_operands_check_for_first_time(ld_units, ld_nr_units);
	read_operands_check_for_first_time(st_units, st_nr_units);
}





void check_if_execute_is_done(Instruction* unit, int nr_units, int unit_delay, int op) { //this function checks for a unit if its execution stage is done
	int j = cycle - unit_delay + 1;
	for (int i = 0; i < nr_units; i++) {
		if ((unit[i].CYCLE_READ_OPERANDS == cycle - unit_delay + 1) && (j > 0) && op != LD) { // checks if execution is done
			switch (op) {
			case ADD:
				unit[i].fi = unit[i].fj + unit[i].fk;
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;
				break;
			case SUB:
				unit[i].fi = unit[i].fj - unit[i].fk;
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;
				break;
			case MULT:
				unit[i].fi = unit[i].fj * unit[i].fk;
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;
				break;
			case DIV:
				unit[i].fi = unit[i].fj / unit[i].fk;
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;
				break;
			case ST:
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;

				break;
			}
			unit[i].OPCODE = -1;
		}
		if ((unit[i].CYCLE_READ_OPERANDS <= cycle - unit_delay + 1) && (j > 0) && op == LD && unit[i].OPCODE != -1) { // checks if execution is done
			if (unit[i].waits_for_mem == -1) { // check if mem addr is ready
				unit[i].CYCLE_EXECUTE_END = cycle;
				unit[i].rj = -1;
				unit[i].rk = -1;
				unit[i].OPCODE = -1;
			}

		}
	}

}



Instruction* execute(Instruction* add_units, Instruction* sub_units, Instruction* mul_units, \
	Instruction* div_units, Instruction* ld_units, Instruction* st_units) { //this function calls the check_if_execute_is_done function for each unit
	check_if_execute_is_done(add_units, add_nr_units, add_delay, ADD);
	check_if_execute_is_done(sub_units, sub_nr_units, sub_delay, SUB);
	check_if_execute_is_done(mul_units, mul_nr_units, mul_delay, MULT);
	check_if_execute_is_done(div_units, div_nr_units, div_delay, DIV);
	check_if_execute_is_done(ld_units, ld_nr_units, ld_delay, LD);
	check_if_execute_is_done(st_units, st_nr_units, st_delay, ST);
	return;
}
void find_who_needs_the_result(Instruction* unit, int nr_units, Instruction inst, int math_or_memory) {// math_or_memory tells us if this is ld/st or the rest- 1->ld/st, 0->math
	if (inst.UNIT != ST) {
		if (!math_or_memory) {
			for (int i = 0; i < nr_units; i++) {
				if (!(unit[i].PC == inst.PC)) { // in case this is the same inst no need to do nothing
					// checks if fj is waiting
					if (unit[i].qj[0] == inst.UNIT && unit[i].qj[1] == inst.unit_number) {
						unit[i].fj = inst.fi;
						unit[i].rj = available;
						unit[i].rj_cycle_change = cycle;
					}
					// check if fk is waiting
					if (unit[i].qk[0] == inst.UNIT && unit[i].qk[1] == inst.unit_number) {
						unit[i].fk = inst.fi;
						unit[i].rk = available;
						unit[i].rk_cycle_change = cycle;
					}
					// checks if now everything is ready to read
					if ((unit[i].rj == available) && (unit[i].rk == available) && (unit[i].CYCLE_READ_OPERANDS == -1)) unit[i].CYCLE_READ_OPERANDS = cycle + 1;
				}
			}
		}
		else {
			//for ST
			for (int i = 0; i < nr_units; i++) {
				// check if fk is waiting
				if (!(unit[i].PC == inst.PC)) { // in case this is the same inst no need to do nothing
					if (unit[i].qk[0] == inst.UNIT && unit[i].qk[1] == inst.unit_number) {
						unit[i].fk = inst.fi;
						unit[i].rk = available;
						unit[i].rj = available;// st needs only source 1 so if rk is ready rj is not matter
						unit[i].CYCLE_READ_OPERANDS = cycle + 1;
					}
				}
			}
		}
	}
	else { // if finished ST
		for (int i = 0; i < nr_units; i++) {
			if (unit[i].OPCODE == LD) {// only relevant for LD
				if (unit[i].waits_for_mem == inst.PC) { // check is the LD waits for this ST
					unit[i].fi = inst.fk;
					unit[i].waits_for_mem = -1;
				}
			}
		}
	}

}

void update_trace_inst_array(Instruction unit) { //this function updates the global array_for_trace_inst array to be used in the printing in the end of the run
	array_for_trace_inst[unit.PC].CYCLE_ISSUED = unit.CYCLE_ISSUED;
	array_for_trace_inst[unit.PC].CYCLE_READ_OPERANDS = unit.CYCLE_READ_OPERANDS;
	array_for_trace_inst[unit.PC].CYCLE_EXECUTE_END = unit.CYCLE_EXECUTE_END;
	array_for_trace_inst[unit.PC].CYCLE_WRITE_RESULT = unit.CYCLE_WRITE_RESULT;
	array_for_trace_inst[unit.PC].PC = unit.PC;
	array_for_trace_inst[unit.PC].unit_number = unit.unit_number;
	array_for_trace_inst[unit.PC].OPCODE == unit.OPCODE;
	how_many_insts++;
	return;

}


void find_who_finished_exec(Instruction* unit, int nr_units, Instruction* add_units, Instruction* sub_units, Instruction* mul_units, \
	Instruction* div_units, Instruction* ld_units, Instruction* st_units, int st) { //this function checks what unit finished execution by calling find_who_needs_the_result for each OPCODE
	//if st==1 no need to update the R[]
	for (int i = 0; i < nr_units; i++) {
		if (unit[i].CYCLE_EXECUTE_END == cycle - 1 && unit[i].CYCLE_ISSUED < cycle) {
			find_who_needs_the_result(add_units, add_nr_units, unit[i], 0);
			find_who_needs_the_result(sub_units, sub_nr_units, unit[i], 0);
			find_who_needs_the_result(mul_units, mul_nr_units, unit[i], 0);
			find_who_needs_the_result(div_units, div_nr_units, unit[i], 0);
			find_who_needs_the_result(ld_units, ld_nr_units, unit[i], 1);
			find_who_needs_the_result(st_units, st_nr_units, unit[i], 1);


			if (!st) { // wirtes to reg
				if (r_busy[unit[i].DST][0] == unit[i].UNIT && r_busy[unit[i].DST][1] == unit[i].unit_number) { // update r_busy in case it was waiting for this PC
					r_busy[unit[i].DST][0] = -1;
					r_busy[unit[i].DST][1] = -1;
				}
				R[unit[i].DST] = unit[i].fi;
				unit[i].CYCLE_WRITE_RESULT = cycle;
				num_of_inst_done++;
				update_trace_inst_array(unit[i]);

				unit[i].PC = -1;

				unit[i].rj = busy; // for the traceunit print
				unit[i].rk = busy;// for the traceunit print


			}
			if (st) { // writes to mem
				if (mem_busy[unit[i].IMM] == unit[i].PC) {
					MEM[unit[i].IMM] = convert_float_to_int(unit[i].fk);
					mem_busy[unit[i].IMM] = -1;
				}
				unit[i].CYCLE_WRITE_RESULT = cycle;
				num_of_inst_done++;
				update_trace_inst_array(unit[i]);
				unit[i].PC = -1;
				unit[i].rj = busy; // for the traceunit print
				unit[i].rk = busy;// for the traceunit print
			}
		}
	}
}

void write_result(Instruction* add_units, Instruction* sub_units, Instruction* mul_units, \
	Instruction* div_units, Instruction* ld_units, Instruction* st_units) {
	find_who_finished_exec(add_units, add_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 0);
	find_who_finished_exec(sub_units, sub_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 0);
	find_who_finished_exec(mul_units, mul_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 0);
	find_who_finished_exec(div_units, div_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 0);
	find_who_finished_exec(ld_units, ld_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 0);
	find_who_finished_exec(st_units, st_nr_units, add_units, sub_units, mul_units, div_units, ld_units, st_units, 1);
	return add_units, sub_units, mul_units, div_units, ld_units, st_units;
}


void run_simulation(FILE* file_memout, FILE* file_regout, FILE* file_traceinst, FILE* file_traceunit, Instruction* add_units, Instruction* sub_units, \
	Instruction* mul_units, Instruction* div_units, Instruction* ld_units, Instruction* st_units) { //this function calls find_who_finished_exec function for each OPCODE
	int forward_pc = 0;
	while (PC < MEM_DEPTH) {

		if (Q_index < INSTRUCTION_Q_SIZE && halt_hit == 0) { //if  first condition take place -> there is space to fetch another instruction, if second condition so we havnt hit halt yet
			Instruction	current_instruction;
			long long int instruction_int;
			instruction_int = MEM[PC];
			instruction_decoder(instruction_int, &current_instruction);

			forward_pc = add_to_fifo(current_instruction);
		}
		if (halt_hit == 0)
			issue(add_units, sub_units, mul_units, div_units, ld_units, st_units);

		// here we check if this is the first time we try to read the operands. our assumption is that 
		//if it is not the first time it means that the desired regs were busy and in this case the command they
		// were waiting for should update them
		read_operands(add_units, sub_units, mul_units, div_units, ld_units, st_units);

		//exectute
		execute(add_units, sub_units, mul_units, div_units, ld_units, st_units);

		//write result
		write_result(add_units, sub_units, mul_units, div_units, ld_units, st_units);


		if (forward_pc == 1 || halt_hit == 1) PC++;
		forward_pc = 0;
		check_which_unit_for_trace_unit(add_units, add_nr_units, file_traceunit);
		check_which_unit_for_trace_unit(sub_units, sub_nr_units, file_traceunit);
		check_which_unit_for_trace_unit(mul_units, mul_nr_units, file_traceunit);
		check_which_unit_for_trace_unit(div_units, div_nr_units, file_traceunit);
		check_which_unit_for_trace_unit(ld_units, ld_nr_units, file_traceunit);
		check_which_unit_for_trace_unit(st_units, st_nr_units, file_traceunit);
		//PC++;
		cycle++;
		if (halt_hit == 1) {

			if (num_of_inst_started == num_of_inst_done) { //checks all orders started before halt hit finished before terminate the program
				PC = MEM_DEPTH;
			}




		}
	}
	print_to_memout(file_memout);
	print_to_regout(file_regout);
	print_to_traceinst(file_traceinst);
}

int main(int argc, char** argv) {
	
	if (argc != 7) {
		if (argc < 7)
			printf("Not enough arguments for valid execution of the Assembler!");
		else
			printf("Too many arguments for valid execution of the Assembler!");
		return 1;
	}

	FILE* file_cfg = fopen(argv[1], "r");
	FILE* file_memin = fopen(argv[2], "r");
	FILE* file_memout = fopen(argv[3], "w");
	FILE* file_regout = fopen(argv[4], "w");
	FILE* file_traceinst = fopen(argv[5], "w");
	FILE* file_traceunit = fopen(argv[6], "w");
	
	/*
	FILE* file_cfg = fopen("cfg.txt", "r");
	FILE* file_memout = fopen("memout.txt", "w");
	FILE* file_regout = fopen("regout.txt", "w");
	FILE* file_memin = fopen("memin.txt", "r");
	FILE* file_traceinst = fopen("traceinst.txt", "w");
	FILE* file_traceunit = fopen("traceunit.txt", "w");
	*/

	if (!(file_cfg && file_memin && file_memout && file_regout && file_traceinst && file_traceunit)) {
		printf("*Cant open one or more of the files");
		exit(1);
	}
	initialize_reg();					//initialize registers values
	initialize_cfg(file_cfg);			//initialize cfg parameters
	load_mem(file_memin);				//initialize memin into an array
	Instruction* add_units = (Instruction*)malloc(sizeof(Instruction) * add_nr_units); //creates an array for each operation based of size provided by cfg.txt
	Instruction* sub_units = (Instruction*)malloc(sizeof(Instruction) * sub_nr_units);
	Instruction* mul_units = (Instruction*)malloc(sizeof(Instruction) * mul_nr_units);
	Instruction* div_units = (Instruction*)malloc(sizeof(Instruction) * div_nr_units);
	Instruction* ld_units = (Instruction*)malloc(sizeof(Instruction) * ld_nr_units);
	Instruction* st_units = (Instruction*)malloc(sizeof(Instruction) * st_nr_units);

	add_units, sub_units, mul_units, div_units, ld_units, st_units = initialize_units(add_units, sub_units, mul_units, div_units, ld_units, st_units); //initialize units for starting values

	find_which_trace_unit(); //finds what unit is in traceunit.txt
	initialize_mem_busy(); //initialize MEM_busy to its initial values
	run_simulation(file_memout, file_regout, file_traceinst, file_traceunit, add_units, sub_units, mul_units, div_units, ld_units, st_units); //our main function- each run of the while loop is one cycle


	fclose(file_cfg);
	fclose(file_memin);
	fclose(file_memout);
	fclose(file_regout);
	fclose(file_traceinst);
	fclose(file_traceunit);
	free(add_units);
	free(sub_units);
	free(mul_units);
	free(div_units);
	free(ld_units);
	free(st_units);

	return 0;
}