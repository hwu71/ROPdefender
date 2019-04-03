#include <stdio.h>
#include "pin.H"

/* ===================================================================== */
/* Basic stack operations                                                */
/* ===================================================================== */
class Stack {
public:
	ADDRINT address;
	Stack *next;

    Stack(){};
	Stack(ADDRINT addr): address(addr){};

	void push(ADDRINT address) {
		Stack *s = new Stack(address);
		s->next = next;
		next = s;
	}

	void pop() {
		if(!next) {
			//cout << "stack empty!" << endl;
		} else {
			Stack *s = next;
			next = next->next;
			delete s;
		}
	}

	ADDRINT top() {
		return next->address;
	}
};

FILE * ROPdefender;
int indent = 0;
Stack s;

VOID onCall(ADDRINT address)
{
    s.push(address);
    for(int i = 0; i < indent; i++){
        fprintf(ROPdefender, "  ");
    }
    indent++;
    fprintf(ROPdefender, "call\t%lx\n", address);
}

VOID onRet(ADDRINT  address)
{
    for(int i = 0; i < indent; i++){
        fprintf(ROPdefender, "  ");
    }
    indent--;
    fprintf(ROPdefender, "ret\t%lx", address);
    if(s.top() != address){
        fprintf(ROPdefender,"\treturn address doesn't match!\n");
        fprintf(ROPdefender, "#eof\n");
        fclose(ROPdefender);
        PIN_ERROR("return address doesn't match!\n");
    } else {
        fprintf(ROPdefender, "\n");
    }
    s.pop();
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    ADDRINT nextAddress;
    if(INS_IsCall(ins)){
        nextAddress = INS_NextAddress(ins);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)onCall, IARG_ADDRINT, nextAddress, IARG_END);
    }
    if(INS_IsRet(ins)){
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)onRet, IARG_BRANCH_TARGET_ADDR, IARG_END);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    fprintf(ROPdefender, "#eof\n");
    fclose(ROPdefender);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool  help protect return address.\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    ROPdefender = fopen("iROPdefender.out", "w");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}