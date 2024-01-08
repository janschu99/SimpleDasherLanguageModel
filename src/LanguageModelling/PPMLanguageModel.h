#ifndef PPM_LANGUAGE_MODEL_INCLUDED
#define PPM_LANGUAGE_MODEL_INCLUDED

#include "../Common/DasherTypes.h"
#include <set>
#include "../Common/PooledAllocator.h"

namespace Dasher {
	
	//"Standard" PPM language model: getProbs uses counts in PPM child nodes.
	//Implements the PPM tree, including fast hashing of child nodes by symbol number; and entering and
	//learning symbols in a context, i.e. navigating and updating the tree, with optional update exclusion.
	class PPMLanguageModel {
		public:
			typedef size_t Context; //Index of registered context
			PPMLanguageModel(int numOfSymbols, int maxOrder, bool updateExclusion, int alpha, int beta);
			Context createEmptyContext();
			void releaseContext(Context context);
			Context cloneContext(Context context);
			void enterSymbol(Context context, Symbol symbol);
			void learnSymbol(Context context, Symbol symbol);
			void getProbs(Context context, std::vector<unsigned int> &probs, int norm, int uniform) const;
		private:
			class ChildIterator;
			class PPMNode {
				public:
					Symbol symbol;
					PPMNode *vine;
					unsigned short int count;
					//default value for symbol doesn't seem to matter, previously there was a separate no-argument
					//constructor which simply didn't initialize symbol, which created a warning
					PPMNode(Symbol symbol = 0);
					~PPMNode();
					ChildIterator children() const;
					const ChildIterator end() const;
					void addChild(PPMNode *newChild, int numSymbols);
					PPMNode* findSymbol(Symbol symbol) const;
				private:
					union {
						PPMNode **childrenArray;
						PPMNode *child;
					};
					//Elements in above array, including nulls, as follows:
					// (a) negative -> absolute value is number of elems in 'childrenArray', but use direct indexing
					// (b) 1 -> use 'child' as direct pointer to PPMNode (no array)
					// (c) 2-MAX_RUN -> 'childrenArray' is unordered array of that many elems
					// (d) >MAX_RUN -> 'childrenArray' is an inline hash (overflow to next elem) with that many slots
					int numOfChildSlots;
			};
			class ChildIterator {
				private:
					PPMNode *const *child;
					PPMNode *const *stop;
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
			};
			class PPMContext {
				public:
					PPMContext() :
							head(NULL), order(0) {
						//empty
					};
					PPMNode *head;
					int order;
			};
			//disallow default copy-constructor and assignment operator
			PPMLanguageModel(const PPMLanguageModel&);
			PPMLanguageModel& operator=(const PPMLanguageModel&);
			//Makes a standard PPMNode, but using a pooled allocator (nodeAllocator) - faster!
			PPMNode* makeNode(Symbol symbol);
			PPMNode* addSymbolToNode(PPMNode *pNode, Symbol symbol);
			//The number of symbols over which we are making predictions, plus one
			//(to leave space for an initial 0).
			const int numOfSymbolsPlusOne;
			PPMContext *rootContext;
			PPMNode *root;
			PooledAllocator<PPMContext> contextAllocator;
			std::set<const PPMContext*> setOfContexts;
			int numOfNodesAllocated;
			SimplePooledAllocator<PPMNode> nodeAllocator;
			//Cache parameters that don't make sense to adjust during the life of a language model...
			const int maxOrder;
			const bool updateExclusion;
			const int alpha;
			const int beta;
	};
	
	inline PPMLanguageModel::Context PPMLanguageModel::createEmptyContext() {
		PPMContext *allocatedContext = contextAllocator.allocate();
		*allocatedContext = *rootContext;
		setOfContexts.insert(allocatedContext);
		return (Context) allocatedContext;
	}
	
	inline void PPMLanguageModel::releaseContext(Context release) {
		setOfContexts.erase(setOfContexts.find((PPMContext*) release));
		contextAllocator.free((PPMContext*) release);
	}
	
	inline PPMLanguageModel::Context PPMLanguageModel::cloneContext(Context context) {
		PPMContext *allocatedContext = contextAllocator.allocate();
		PPMContext *copyOfContext = (PPMContext*) context;
		*allocatedContext = *copyOfContext;
		setOfContexts.insert(allocatedContext);
		return (Context) allocatedContext;
	}
}

#endif
