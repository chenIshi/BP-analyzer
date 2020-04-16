#define allocSize 1024
#define PIN_DEPRECATED_WARNINGS 0
#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <string>
#include "pin.H"

std::ofstream TraceFile;
using namespace std;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "output.log", "specify trace file name");

typedef struct {
    bool Taken;
    ADDRINT predTarget;
} BP_Info;

enum BranchStates {
	STRONGLY_NOT_TAKEN,
	WEAKLY_NOT_TAKEN,
	WEAKLY_TAKEN,
	STRONGLY_TAKEN,
};

class BranchPredictor {
private:
	BranchStates states [allocSize];
	ADDRINT currentIns [allocSize];

    void UpdateState(bool BrTaken, ADDRINT PC) {
		int key = getKey(PC);
        switch (states[key]) {
		case STRONGLY_NOT_TAKEN:
			states[key] = BrTaken ? WEAKLY_NOT_TAKEN : STRONGLY_NOT_TAKEN;
			break;
		case WEAKLY_NOT_TAKEN:
			states[key] = BrTaken ? WEAKLY_TAKEN : STRONGLY_NOT_TAKEN;
			break;
		case WEAKLY_TAKEN:
			states[key] = BrTaken ? STRONGLY_TAKEN : WEAKLY_NOT_TAKEN;
			break;
		case STRONGLY_TAKEN:
			states[key] = BrTaken ? STRONGLY_TAKEN : WEAKLY_TAKEN;
			break;
		}
    }


public:

	BranchPredictor() {
		// init state
		for (int i = 0; i<allocSize; i++) {
			states[i] = WEAKLY_TAKEN;
			currentIns[i] = 0;
		}
	}

	bool GetState(ADDRINT PC) {
		switch (states[getKey(PC)]) {
			case STRONGLY_NOT_TAKEN:
				return false;
			case WEAKLY_NOT_TAKEN:
				return false;
			case WEAKLY_TAKEN:
				return true;
			case STRONGLY_TAKEN:
				return true;
		}
		return true;
    }

    BP_Info GetPrediction(ADDRINT PC) {
        BP_Info tmp_info;
		int key = getKey(PC);
		if (currentIns[key] == 0) {
			tmp_info.Taken = true;
			tmp_info.predTarget = -1;
		} else {
			tmp_info.Taken = GetState(PC);
			tmp_info.predTarget = currentIns[key];
		}
        return tmp_info;
    }

    void Update(ADDRINT PC, bool BrTaken, ADDRINT targetPC) {
        UpdateState(BrTaken, PC);
		int key = getKey(PC);
		currentIns[key] = targetPC;
    }

	int getKey(ADDRINT PC) {
		return PC % allocSize;
	}
};

BranchPredictor myBPU;

long long int DirectionMissCount = 0;
long long int TargetMissCount = 0;
long long int BranchCount = 0;

VOID ProcessBranch(ADDRINT PC, ADDRINT targetPC, bool BrTaken) {
    
    BranchCount++;
    BP_Info pred = myBPU.GetPrediction(PC);
    if ( pred.Taken != BrTaken) {
        DirectionMissCount++;
    }
    if ( pred.predTarget != targetPC ) {
        TargetMissCount++;
    }
    myBPU.Update(PC, BrTaken, targetPC);
}
    
VOID Instruction(INS ins, VOID *v)
{
    if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) ProcessBranch, 
	IARG_ADDRINT, INS_Address(ins), 
	IARG_ADDRINT, INS_DirectBranchOrCallTargetAddress(ins),
	IARG_BRANCH_TAKEN, IARG_END);
}


VOID Fini(INT32 code, VOID *v)
{
    TraceFile << "################################################" << endl;
    TraceFile << "DirectionMissCount: " << DirectionMissCount << endl;
    TraceFile << "TargetMissCount: " << TargetMissCount << endl;
    TraceFile << "BranchCount: " << BranchCount << endl;
    TraceFile << "Branch Direction Miss rate: " << (DirectionMissCount / (float)BranchCount) * 100 << "%" << endl;
    TraceFile << "Branch Target Miss rate: " << (TargetMissCount / (float)BranchCount) * 100 << "%" << endl;
    TraceFile << "################################################" << endl;
}

int main(int argc, char * argv[])
{
    TraceFile.open(KnobOutputFile.Value().c_str());
    TraceFile.setf(ios::showbase);

    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    
    return 0;
}