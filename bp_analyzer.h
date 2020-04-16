#ifndef BP_ANALYZER_H_
#define BP_ANALYZER_H_
//#include <map>
#include <pin.H>

enum BranchStates {
	STRONGLY_NOT_TAKEN,
	WEAKLY_NOT_TAKEN,
	WEAKLY_TAKEN,
	STRONGLY_TAKEN,
};

typedef struct _BP_Info {
	bool Taken;
	ADDRINT predTarget;
} BP_Info;

class BranchPredictor {
	public:
		BranchPredictor();
		~BranchPredictor();
		BP_Info GetPrediction(ADDRINT pc);
		void setIns(ADDRINT pc, ADDRINT targetpc);
		void Update(ADDRINT pc, bool brTaken, ADDR targetPc);

	private:
		ADDRINT currentIns [1024];
		int bpSuccessCount = 0;
		int bpFailedCount = 0;
		BranchStates states [1024];
	
		// translate ADDRINT to index of 
		// currentIns/accessCount/mispredictCount/BranchStates
		int getKey(ADDRINT pc);
		// translate state into brTaken
		bool as_bool(BranchStates state);
};

#endif // BP_ANALYZER_H_