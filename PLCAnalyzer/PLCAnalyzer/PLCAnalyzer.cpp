//The pass file for final project of EECS583 in Fall 2018
//@auther: Shenghao Lin, Xumiao Zhang

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Dominators.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constant.h"

#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"

#include <queue>
#include <vector>
#include <set>
#include <map>
#include <string>

using namespace std;

using namespace llvm;

#define DEBUG_TYPE "plcanalyzer"

namespace {

    struct PLCAnalyzer : public FunctionPass {
        
        static char ID; 

        PLCAnalyzer() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {

			DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
			AliasAnalysis *AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
			MemorySSA *MSSA = &getAnalysis<MemorySSAWrapperPass>().getMSSA();

			std::vector<Instruction *> store_list;	

			for (Function::iterator b = F.begin(); b != F.end(); b++) {
				BasicBlock *bb = &(*b);
				
				MemoryPhi *phi = MSSA -> getMemoryAccess(bb);
				//if (phi != NULL) {
				//
				//	phi -> print(errs());
				//	errs().write('\n');
				//}

				for (BasicBlock::iterator i_iter = bb -> begin(); i_iter != bb -> end(); ++ i_iter) {
					Instruction *I = &(*i_iter);
					if (I -> getOpcode() == Instruction::Store) {
						//MemoryUseOrDef *ud = MSSA -> getMemoryAccess(I);
						//if (ud != NULL) {
						//
						//	ud -> print(errs());
						//	errs().write('\n');
						//}
						store_list.push_back(I);						
					}
				}
			}

			for (int i = 0; i < store_list.size(); i ++) {
				set<Value *> queried;
				queue<Value *> to_query;
				to_query.push(store_list[i]);
				
				MemoryDef *MA = dyn_cast<MemoryDef>(MSSA -> getMemoryAccess(store_list[i]));
				if (MA && MA ->getID())
				errs().write('\n') << MA -> getID() << " " << store_list[i] -> getOperand(1) -> getName() << ": \n";

				while (to_query.size()) {
					Value *dd = to_query.front();

					to_query.pop();
					if (queried.find(dd) != queried.end()) continue;
					queried.insert(dd);
					
					if (MemoryPhi *phi = dyn_cast<MemoryPhi>(dd)) {
						for (const auto &op : phi -> operands()) {
							to_query.push(op);
						}

						errs() << "phi\n";
					}
					else if (Instruction* d = dyn_cast<Instruction>(dd))
					if (d -> getOpcode () == Instruction::Store) {
						Value *v = d -> getOperand(0);
						if (Constant* CI = dyn_cast<Constant>(v)) {
							if (GlobalValue *gv = dyn_cast<GlobalValue>(CI)) 
								errs() << "related global value:" << gv -> getName() << "\n";
							continue;
						}
						else {
							to_query.push(dyn_cast<Instruction>(v));
						}
						errs() << "store\n";
					}
					

					else if (d -> getOpcode() == Instruction::Load) {
						if (GlobalValue *gv = dyn_cast<GlobalValue>(d -> getOperand(0))){
							errs() <<"related global value: " << gv ->getName() << "\n";
							continue;
						}
						MemoryUse *MU = dyn_cast<MemoryUse>(MSSA -> getMemoryAccess(d));

						MemoryAccess *UO = MU -> getDefiningAccess();
						if (MemoryDef *MD = dyn_cast<MemoryDef>(UO)) {
							to_query.push(MD -> getMemoryInst());
						}
						else {
							to_query.push(UO);
						}
						errs() << "load\n";
					}

					else {
						for (int j = 0; j < d -> getNumOperands(); ++ j) {
							Value *v = d -> getOperand(j);
							if (Constant *c = dyn_cast<Constant>(v)) {
								if (GlobalValue *gv = dyn_cast<GlobalValue>(c)) 
									errs() << "related global value:" << gv -> getName() << "\n";
								continue;
							}
							else if (Instruction *inst = dyn_cast<Instruction>(v)) {
								to_query.push(inst);
							}
						}
						errs() << "else\n";
					}

					
				}
			}

			return false;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const {
 
			AU.addRequired<DominatorTreeWrapperPass>();
			AU.addRequired<BranchProbabilityInfoWrapperPass>();
            AU.addRequired<BlockFrequencyInfoWrapperPass>();
			AU.addRequired<AAResultsWrapperPass>();
			AU.addRequired<MemorySSAWrapperPass>();
        }
    };

}

char PLCAnalyzer::ID = 0;
static RegisterPass<PLCAnalyzer> X("plcanalyzer", "PLCAnalyzer");