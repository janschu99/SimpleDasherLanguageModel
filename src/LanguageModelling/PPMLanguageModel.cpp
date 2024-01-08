#include "PPMLanguageModel.h"

#include "stdlib.h" //for abs
#include <string.h> //for memset

using namespace Dasher;

inline PPMLanguageModel::PPMNode::PPMNode(Symbol symbol) :
		symbol(symbol), vine(NULL), count(1), childrenArray(NULL), numOfChildSlots(0) {
	//empty
}

inline PPMLanguageModel::PPMNode::~PPMNode() {
	//single child = is direct pointer to node, not array...
	if (numOfChildSlots!=1) delete[] childrenArray;
}

inline PPMLanguageModel::ChildIterator PPMLanguageModel::PPMNode::children() const {
	//if numOfChildSlots = 0 / 1, 'childrenArray' is direct pointer, else pointer to array (of pointers)
	PPMNode *const *ppChild = (numOfChildSlots==0 || numOfChildSlots==1) ? &child : childrenArray;
	return ChildIterator(ppChild+abs(numOfChildSlots), ppChild-1);
}

inline const PPMLanguageModel::ChildIterator PPMLanguageModel::PPMNode::end() const {
	//if numOfChildSlots = 0 / 1, 'childrenArray' is direct pointer, else pointer to array (of pointers)
	PPMNode *const *ppChild = (numOfChildSlots==0 || numOfChildSlots==1) ? &child : childrenArray;
	return ChildIterator(ppChild, ppChild-1);
}

#define MAX_RUN 4

PPMLanguageModel::PPMNode* PPMLanguageModel::PPMNode::findSymbol(Symbol symbolToFind) const {
	//see if symbol is a child of node
	if (numOfChildSlots<0) //negative to mean "full alphabet", use direct indexing
		return childrenArray[symbolToFind];
	if (numOfChildSlots==1) {
		if (child->symbol==symbolToFind) return child;
		return 0;
	}
	if (numOfChildSlots<=MAX_RUN) {
		for (int i = 0; i<numOfChildSlots && childrenArray[i]; i++)
			if (childrenArray[i]->symbol==symbolToFind) return childrenArray[i];
		return 0;
	}
	//printf("finding symbol %d at node %d\n", symbol, node->id);
	for (int i = symbolToFind;; i++) { //search through elements which have overflowed into subsequent slots
		PPMNode *found = this->childrenArray[i%numOfChildSlots]; //wrap round
		if (!found) return 0; //null element
		if (found->symbol==symbolToFind) return found;
	}
	return 0;
}

void PPMLanguageModel::PPMNode::addChild(PPMNode *newChild, int numSymbols) {
	if (numOfChildSlots<0) {
		childrenArray[newChild->symbol] = newChild;
	} else {
		if (numOfChildSlots==0) {
			numOfChildSlots = 1;
			child = newChild;
			return;
		} else if (numOfChildSlots==1) {
			//no room, have to resize...
		} else if (numOfChildSlots<=MAX_RUN) {
			for (int i = 0; i<numOfChildSlots; i++)
				if (!childrenArray[i]) {
					childrenArray[i] = newChild;
					return;
				}
		} else {
			Symbol start = newChild->symbol;
			//find length of run (including to-be-inserted element)...
			while (childrenArray[start = (start+numOfChildSlots-1)%numOfChildSlots]);
			Symbol idx = newChild->symbol;
			while (childrenArray[idx %= numOfChildSlots]) ++idx;
			//found NULL
			Symbol stop = idx;
			while (childrenArray[stop = (stop+1)%numOfChildSlots]);
			//start and idx point to NULLs (with inserted element somewhere in between)
			int runLen = (numOfChildSlots+stop-(start+1))%numOfChildSlots;
			if (runLen<=MAX_RUN) {
				//ok, maintain size
				childrenArray[idx] = newChild;
				return;
			}
		}
		//resize!
		PPMNode **oldChildrenArray = childrenArray;
		int oldSlots = numOfChildSlots;
		int newNumElems;
		if (numOfChildSlots>=numSymbols/4) {
			numOfChildSlots = -numSymbols; // negative = "use direct indexing"
			newNumElems = numSymbols;
		} else {
			numOfChildSlots += numOfChildSlots+1;
			newNumElems = numOfChildSlots;
		}
		childrenArray = new PPMNode*[newNumElems]; //null terminator
		memset(childrenArray, 0, sizeof(PPMNode*)*newNumElems);
		if (oldSlots==1) addChild((PPMNode*) oldChildrenArray, numSymbols);
		else {
			while (oldSlots-->0)
				if (oldChildrenArray[oldSlots]) addChild(oldChildrenArray[oldSlots], numSymbols);
			delete[] oldChildrenArray;
		}
		addChild(newChild, numSymbols);
	}
}

PPMLanguageModel::PPMLanguageModel(int numOfSymbols, int maxOrder, bool updateExclusion, int alpha, int beta) :
		numOfSymbolsPlusOne(numOfSymbols+1), root(new PPMNode(-1)), contextAllocator(1024), numOfNodesAllocated(0), nodeAllocator(8192),
		maxOrder(maxOrder), updateExclusion(updateExclusion), alpha(alpha), beta(beta) {
	rootContext = contextAllocator.allocate();
	rootContext->head = root;
	rootContext->order = 0;
}

//Get the probability distribution at the context
void PPMLanguageModel::getProbs(Context context, std::vector<unsigned int> &probs, int norm, int uniform) const {
	const PPMContext *ppmContext = (const PPMContext*) (context);
	//DASHER_ASSERT(isValidContext(context)); //method removed, simply checked whether setOfContexts contains context
	probs.resize(numOfSymbolsPlusOne);
	std::vector<bool> exclusions(numOfSymbolsPlusOne);
	unsigned int toSpend = norm;
	unsigned int uniformLeft = uniform;
	//TODO: Sort out zero symbol case
	probs[0] = 0;
	exclusions[0] = false;
	for (int i = 1; i<numOfSymbolsPlusOne; i++) {
		probs[i] = uniformLeft/(numOfSymbolsPlusOne-i);
		uniformLeft -= probs[i];
		toSpend -= probs[i];
		exclusions[i] = false;
	}
	//DASHER_ASSERT(uniformLeft == 0);
	//bool doExclusion = GetLongParameter(LP_LM_ALPHA);
	bool doExclusion = 0; //FIXME
	for (PPMNode *temp = ppmContext->head; temp; temp = temp->vine) {
		int total = 0;
		for (ChildIterator symbolIterator = temp->children(); symbolIterator!=temp->end(); symbolIterator.next()) {
			Symbol sym = (*symbolIterator)->symbol;
			if (!(exclusions[sym] && doExclusion)) total += (*symbolIterator)->count;
		}
		if (total) {
			unsigned int sizeOfSlice = toSpend;
			for (ChildIterator symbolIterator = temp->children(); symbolIterator!=temp->end(); symbolIterator.next()) {
				if (!(exclusions[(*symbolIterator)->symbol] && doExclusion)) {
					exclusions[(*symbolIterator)->symbol] = 1;
					unsigned int p = static_cast<int64>(sizeOfSlice)*(100*(*symbolIterator)->count-beta)/(100*total+alpha);
					probs[(*symbolIterator)->symbol] += p;
					toSpend -= p;
				}
				//printf("symbol %u counts %d p %u toSpend %u \n", symbol, s->count, p, toSpend);
			}
		}
	}
	unsigned int sizeOfSlice = toSpend;
	int symbolsLeft = 0;
	for (int i = 1; i<numOfSymbolsPlusOne; i++)
		if (!(exclusions[i] && doExclusion)) symbolsLeft++;
	for (int i = 1; i<numOfSymbolsPlusOne; i++) {
		if (!(exclusions[i] && doExclusion)) {
			unsigned int p = sizeOfSlice/symbolsLeft;
			probs[i] += p;
			toSpend -= p;
		}
	}
	int iLeft = numOfSymbolsPlusOne-1;
	for (int i = 1; i<numOfSymbolsPlusOne; i++) {
		unsigned int p = toSpend/iLeft;
		probs[i] += p;
		--iLeft;
		toSpend -= p;
	}
	//DASHER_ASSERT(toSpend == 0);
}

//Update context with symbol 'symbol'
void PPMLanguageModel::enterSymbol(Context c, Symbol symbol) {
	if (symbol==0) return;
	//DASHER_ASSERT(symbol >= 0 && symbol < GetSize());
	PPMContext &context = *(PPMContext*) (c);
	while (context.head) {
		if (context.order<maxOrder) { //Only try to extend the context if it's not going to make it too long
			if (PPMNode *find = context.head->findSymbol(symbol)) {
				context.order++;
				context.head = find;
				//printf("found context %x order %d\n", head, order);
				return;
			}
		}
		//If we can't extend the current context, follow vine pointer to shorten it and try again
		context.order--;
		context.head = context.head->vine;
	}
	if (context.head==NULL) {
		context.head = root;
		context.order = 0;
	}
}

//Add symbol to the context
//Creates new nodes, updates counts, and leaves 'context' at the new context.
void PPMLanguageModel::learnSymbol(Context c, Symbol symbol) {
	if (symbol==0) return;
	//DASHER_ASSERT(symbol >= 0 && symbol < GetSize());
	PPMContext &context = *(PPMContext*) (c);
	PPMNode *n = addSymbolToNode(context.head, symbol);
	//DASHER_ASSERT(n == context.head->findSymbol(symbol));
	context.head = n;
	context.order++;
	while (context.order>maxOrder) {
		context.head = context.head->vine;
		context.order--;
	}
}

PPMLanguageModel::PPMNode* PPMLanguageModel::addSymbolToNode(PPMNode *node, Symbol symbol) {
	PPMNode *returnVal = node->findSymbol(symbol);
	if (returnVal!=NULL) {
		returnVal->count++;
		if (!updateExclusion) {
			//Update vine contexts too. Guaranteed to exist if child does!
			for (PPMNode *v = returnVal->vine; v; v = v->vine) {
				//DASHER_ASSERT(v == root || v->symbol == symbol);
				v->count++;
			}
		}
	} else {
		//symbol does not exist at this level
		returnVal = makeNode(symbol); //count initialized to 1 but no vine pointer
		node->addChild(returnVal, numOfSymbolsPlusOne);
		returnVal->vine = (node==root) ? root : addSymbolToNode(node->vine, symbol);
	}
	return returnVal;
}

PPMLanguageModel::PPMNode* PPMLanguageModel::makeNode(Symbol symbol) {
	PPMNode *res = nodeAllocator.allocate();
	res->symbol = symbol;
	++numOfNodesAllocated;
	return res;
}
