#include <iostream>
#include <fstream>
#include "bp_analyzer.h"

using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;
BranchPredictor myBPU;

// analysis coroutine
void ProcessBranch(ADDRINT PC, ADDRINT targetPC, bool BrTaken) {
	BP_Info pred = myBPU.GetPrediction(PC);
	if (pred.Taken != BrTaken) {
		// Direction Mispredicted
	}
	if (pred.predTarget != targetPC) {
		// Target Mispredicted
		BranchPredictor::setIns(PC, targetPC);
	}
	myBPU.Update(PC, BrTaken, targetPC);
}

void Instruction(INS ins, void *v) {
	if (INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) ProcessBranch, 
		IARG_ADDRINT, INS_Address(ins),
		IARG_ADDRINT, INS_DirectBranchOrCallTargetAddress(ins),
		IARG_BRANCH_TAKEN, IARG_END);
	}
}

int main() {
	PIN_Init();
	INS_AddInstrumentationFunction(Instruction, 0);
	PIN_StartProgram();
}

// BranchPredictor impl
BranchPredictor::BranchPredictor() {
	// init state
	for (int i = 0; i<1024; i++) {
		states[i] = WEAKLY_TAKEN;
	}
}

BranchPredictor::~BranchPredictor() {
	// dump message
	double successRate = ((double) bpSuccessCount / (double) (bpFailedCount + bpSuccessCount)) * 100;
	fprintf(stdout, "Success bp rate: %ld\n", successRate);
}

BP_Info BranchPredictor::GetPrediction(ADDRINT pc) {
	int key = getKey(pc);
	BP_Info ret;
	ret.Taken = as_bool(states[key]);
	ret.predTarget = currentIns[key];
	return ret;
}

void BranchPredictor::Update(ADDRINT pc, bool brTaken, ADDRINT targetPc) {
	int key = getKey(pc);
	// update analysis log
	// guessed the right branch
	if (as_bool(states[key]) == brTaken) {
		bpSuccessCount ++;
	} else {
		bpFailedCount ++;
	}
	// apply state change
	switch (states[key]) {
		case STRONGLY_NOT_TAKEN:
			states[key] = brTaken ? WEAKLY_NOT_TAKEN : STRONGLY_NOT_TAKEN;
			break;
		case WEAKLY_NOT_TAKEN:
			states[key] = brTaken ? WEAKLY_TAKEN : STRONGLY_NOT_TAKEN;
			break;
		case WEAKLY_TAKEN:
			states[key] = brTaken ? STRONGLY_TAKEN : WEAKLY_NOT_TAKEN;
			break;
		case STRONGLY_TAKEN:
			states[key] = brTaken ? STRONGLY_TAKEN : WEAKLY_TAKEN;
			break;
	}
}

void BranchPredictor::setIns(ADDRINT pc, ADDRINT targetPc) {
	int key = getKey(pc);
	currentIns[key] = targetPc;
}