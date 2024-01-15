#ifndef PPM_LANGUAGE_MODEL_INCLUDED
#define PPM_LANGUAGE_MODEL_INCLUDED

#include <set>
#include "../Common/DasherTypes.h"
#include "../Common/PooledAllocator.h"

namespace Dasher {
	
	//"Standard" PPM language model: getProbs uses counts in PPM child nodes.
	//Implements the PPM tree, including fast hashing of child nodes by symbol number; and entering and
	//learning symbols in a context, i.e. navigating and updating the tree, with optional update exclusion.
	class PPMLanguageModel {
		public:
			typedef size_t Context; //Index of registered context
			PPMLanguageModel(int numOfSymbols, int maxOrder);
			Context createEmptyContext();
			void releaseContext(Context context);
			void enterSymbol(Context context, Symbol symbol);
			void learnSymbol(Context context, Symbol symbol);
			void getProbs(Context context, std::vector<unsigned int>& probs, int alpha, int beta, int uniform) const;
		private:
			class PPMNode;
			class ChildIterator;
			class PPMContext;
			const int numOfSymbols; //The number of symbols over which we are making predictions
			const int maxOrder;
			PPMContext* rootContext;
			PPMNode* root;
			PooledAllocator<PPMContext> contextAllocator;
			std::set<const PPMContext*> setOfContexts;
			SimplePooledAllocator<PPMNode> nodeAllocator;
			//disallow default copy-constructor and assignment operator
			PPMLanguageModel(const PPMLanguageModel&);
			PPMLanguageModel& operator=(const PPMLanguageModel&);
			PPMNode* makeNode(Symbol symbol); //makes a standard PPMNode, but using a pooled
			                                  //allocator (nodeAllocator) - faster!
			PPMNode* addSymbolToNode(PPMNode* node, Symbol symbol);
			class PPMNode {
				public:
					Symbol symbol;
					PPMNode* vine;
					unsigned short int count;
					PPMNode(Symbol symbol = 0); //default value for symbol doesn't seem to matter, previously
					                            //there was a separate no-argument constructor which simply 
					                            //didn't initialize symbol, which created a warning
					~PPMNode();
					ChildIterator children() const;
					const ChildIterator end() const;
					void addChild(PPMNode* newChild, int numSymbols);
					PPMNode* findSymbol(Symbol symbol) const;
				private:
					//Elements in below array, including nulls, as follows:
					// (a) negative -> absolute value is number of elems in 'childrenArray', but use direct indexing
					// (b) 1 -> use 'child' as direct pointer to PPMNode (no array)
					// (c) 2-MAX_RUN -> 'childrenArray' is unordered array of that many elems
					// (d) >MAX_RUN -> 'childrenArray' is an inline hash (overflow to next elem) with that many slots
					int numOfChildSlots;
					union {
						PPMNode** childrenArray;
						PPMNode* child;
					};
			};
			class ChildIterator {
				public:
					ChildIterator(PPMNode *const *ppChild, PPMNode *const *ppStop) :
							child(ppChild), stop(ppStop) {
						next();
					}
					void next() {
						if (child==stop) return;
						while ((--child)!=stop)
							if (*child) break;
					}
					bool operator!=(const ChildIterator &other) const {
						return child!=other.child || stop!=other.stop;
					}
					PPMNode* operator*() const {
						return (child==stop) ? NULL : *child;
					}
				private:
					PPMNode *const *child;
					PPMNode *const *stop;
			};
			class PPMContext {
				public:
					PPMNode* head;
					int order;
					PPMContext() : head(NULL), order(0) {
						//empty
					}
			};
	};
}

#endif
