#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {
    struct AMDGPUMemoryOptimizer : public FunctionPass {
        static char ID;
        AMDGPUMemoryOptimizer() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            bool modified = false;

            for (BasicBlock &BB : F) {
                for (auto it = BB.begin(); it != BB.end(); ++it) {
                    if (LoadInst *load = dyn_cast<LoadInst>(&*it)) {
                        Value *pointer = load->getPointerOperand();
                        
                        // Check if the pointer is loaded multiple times - gg brother this doesnt work 
                        if (pointer->hasNUsesOrMore(2)) {
                            // Insert a cache for the loaded value
                            IRBuilder<> builder(load);
                            Value *cachedValue = builder.CreateLoad(pointer, "cachedValue");
                            
                            // Replaced subsequent uses with cached value?? - does this work check again ??
                            for (auto &U : pointer->uses()) {
                                if (Instruction *userInst = dyn_cast<Instruction>(U.getUser())) {
                                    if (userInst != load) {
                                        userInst->replaceUsesOfWith(pointer, cachedValue);
                                        modified = true;
                                    }
                                }
                            }

                            errs() << "Optimized redundant memory load for: " << *pointer << "\n";
                        }
                    }
                }
            }
            return modified;
        }
    };
}

char AMDGPUMemoryOptimizer::ID = 0;
static RegisterPass<AMDGPUMemoryOptimizer> X("amdgpu-mem-opt", "AMDGPU Memory Access Optimization", false, false);


// Brother how to run the file 

clang++ -shared -o AMDGPUMemoryOptimizer.so -fPIC OptimizeMemoryAccess.cpp
    $(llvm-config --cxxflags --ldflags --system-libs --libs core)

// Command to compile:
// clang -shared -o OptimizeAMDGPU.so -fPIC OptimizeAMDGPU.cpp $(llvm-config --cxxflags --ldflags --system-libs --libs core)

// Command to run:
// opt -load ./OptimizeAMDGPU.so -opt-amdgpu < input.ll > output.ll
