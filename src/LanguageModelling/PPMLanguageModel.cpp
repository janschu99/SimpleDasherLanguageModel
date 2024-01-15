#include "PPMLanguageModel.h"

#include <stdlib.h> //for abs
#include <string.h> //for memset
#include <set>

#define MAX_RUN 4

using namespace Dasher;

PPMLanguageModel::PPMLanguageModel(int numOfSymbols, int maxOrder) :
		numOfSymbols(numOfSymbols), maxOrder(maxOrder), root(new PPMNode(-1)),
		contextAllocator(1024), nodeAllocator(8192) {
	rootContext=contextAllocator.allocate();
	rootContext->head=root;
	rootContext->order=0;
}

PPMLanguageModel::Context PPMLanguageModel::createEmptyContext() {
	PPMContext* allocatedContext = contextAllocator.allocate();
	*allocatedContext=*rootContext;
	setOfContexts.insert(allocatedContext);
	return (Context) allocatedContext;
}

void PPMLanguageModel::releaseContext(Context release) {
	setOfContexts.erase(setOfContexts.find((PPMContext*) release));
	contextAllocator.free((PPMContext*) release);
}

//Update context with symbol 'symbol'
void PPMLanguageModel::enterSymbol(Context c, Symbol symbol) {
	if (symbol==0) return;
	//DASHER_ASSERT(symbol>=0 && symbol<GetSize());
	PPMContext& context = *(PPMContext*) c;
	while (context.head!=NULL) {
		if (context.order<maxOrder) { //Only try to extend the context if it's not going to make it too long
			PPMNode* find = context.head->findSymbol(symbol);
			if (find!=NULL) {
				context.order++;
				context.head=find;
				//printf("found context %x order %d\n", head, order);
				return;
			}
		}
		//If we can't extend the current context, follow vine pointer to shorten it and try again
		context.order--;
		context.head=context.head->vine;
	}
	if (context.head==NULL) {
		context.head=root;
		context.order=0;
	}
}

//Add symbol to the context
//Creates new nodes, updates counts, and leaves 'context' at the new context.
void PPMLanguageModel::learnSymbol(Context c, Symbol symbol) {
	if (symbol==0) return;
	//DASHER_ASSERT(symbol>=0 && symbol<GetSize());
	PPMContext& context = *(PPMContext*) c;
	PPMNode* node = addSymbolToNode(context.head, symbol);
	//DASHER_ASSERT(node==context.head->findSymbol(symbol));
	context.head=node;
	context.order++;
	while (context.order>maxOrder) {
		context.head=context.head->vine;
		context.order--;
	}
}

//Get the probability distribution at the context
void PPMLanguageModel::getProbs(Context context, std::vector<unsigned int>& probs, int alpha, int beta, int uniform) const {
	//adapted from CAlphabetManager::GetProbs
	static const int NORMALIZATION = 1<<16; //from CDasherModel
	int uniformAdd = std::max(1, (NORMALIZATION*uniform/1000)/numOfSymbols);
	int norm = NORMALIZATION-numOfSymbols*uniformAdd; //non-uniform norm
	//
	const PPMContext* ppmContext = (const PPMContext*) context;
	//DASHER_ASSERT(isValidContext(context)); //method removed, simply checked whether setOfContexts contains context
	probs.assign(numOfSymbols+1, 0);
	unsigned int toSpend = norm;
	for (PPMNode* temp = ppmContext->head; temp!=NULL; temp=temp->vine) {
		int total = 0;
		for (ChildIterator symbolIterator = temp->children(); symbolIterator!=temp->end(); symbolIterator.next()) {
			total+=(*symbolIterator)->count;
		}
		if (total!=0) {
			unsigned int sizeOfSlice = toSpend;
			for (ChildIterator symbolIterator = temp->children(); symbolIterator!=temp->end(); symbolIterator.next()) {
				unsigned int p = static_cast<int64>(sizeOfSlice)*(100*(*symbolIterator)->count-beta)/(100*total+alpha);
				probs[(*symbolIterator)->symbol]+=p;
				toSpend-=p;
				//printf("symbol %u counts %d p %u toSpend %u \n", symbol, s->count, p, toSpend);
			}
		}
	}
	unsigned int sizeOfSlice = toSpend;
	for (int i = 1; i<=numOfSymbols; i++) {
		unsigned int p = sizeOfSlice/numOfSymbols;
		probs[i]+=p;
		toSpend-=p;
	}
	int left = numOfSymbols;
	for (int i = 1; i<=numOfSymbols; i++) {
		unsigned int p = toSpend/left;
		probs[i]+=p+uniformAdd; //Note: Adding the uniform distribution ("Smoothing") is not part of
		                        //the language model in the Dasher sources, but is done afterwards
		                        //in CAlphabetManager::GetProbs
		left--;
		toSpend-=p;
	}
	//DASHER_ASSERT(toSpend==0);
}

PPMLanguageModel::PPMNode* PPMLanguageModel::makeNode(Symbol symbol) {
	PPMNode* res = nodeAllocator.allocate();
	res->symbol=symbol;
	return res;
}

PPMLanguageModel::PPMNode* PPMLanguageModel::addSymbolToNode(PPMNode* node, Symbol symbol) {
	PPMNode* returnVal = node->findSymbol(symbol);
	if (returnVal!=NULL) {
		returnVal->count++;
	} else {
		//symbol does not exist at this level
		returnVal=makeNode(symbol); //count initialized to 1 but no vine pointer
		node->addChild(returnVal, numOfSymbols+1);
		returnVal->vine=(node==root ? root : addSymbolToNode(node->vine, symbol));
	}
	return returnVal;
}

PPMLanguageModel::PPMNode::PPMNode(Symbol symbol) :
		symbol(symbol), vine(NULL), count(1), numOfChildSlots(0), childrenArray(NULL) {
	//empty
}

PPMLanguageModel::PPMNode::~PPMNode() {
	//single child = is direct pointer to node, not array...
	if (numOfChildSlots!=1) delete[] childrenArray;
}

PPMLanguageModel::ChildIterator PPMLanguageModel::PPMNode::children() const {
	//if numOfChildSlots = 0 / 1, 'childrenArray' is direct pointer, else pointer to array (of pointers)
	PPMNode *const *ppChild = (numOfChildSlots==0 || numOfChildSlots==1) ? &child : childrenArray;
	return ChildIterator(ppChild+abs(numOfChildSlots), ppChild-1);
}

const PPMLanguageModel::ChildIterator PPMLanguageModel::PPMNode::end() const {
	//if numOfChildSlots = 0 / 1, 'childrenArray' is direct pointer, else pointer to array (of pointers)
	PPMNode *const *ppChild = (numOfChildSlots==0 || numOfChildSlots==1) ? &child : childrenArray;
	return ChildIterator(ppChild, ppChild-1);
}

void PPMLanguageModel::PPMNode::addChild(PPMNode* newChild, int numSymbols) {
	if (numOfChildSlots<0) {
		childrenArray[newChild->symbol]=newChild;
	} else {
		if (numOfChildSlots==0) {
			numOfChildSlots=1;
			child=newChild;
			return;
		} else if (numOfChildSlots==1) {
			//no room, have to resize...
		} else if (numOfChildSlots<=MAX_RUN) {
			for (int i = 0; i<numOfChildSlots; i++)
				if (childrenArray[i]==NULL) {
					childrenArray[i]=newChild;
					return;
				}
		} else {
			Symbol start = newChild->symbol;
			//find length of run (including to-be-inserted element)...
			while (childrenArray[start=(start+numOfChildSlots-1)%numOfChildSlots]!=NULL);
			Symbol idx = newChild->symbol;
			while (childrenArray[idx%=numOfChildSlots]) idx++;
			//found NULL
			Symbol stop = idx;
			while (childrenArray[stop=(stop+1)%numOfChildSlots]!=NULL);
			//start and idx point to NULLs (with inserted element somewhere in between)
			int runLen = (numOfChildSlots+stop-(start+1))%numOfChildSlots;
			if (runLen<=MAX_RUN) {
				//ok, maintain size
				childrenArray[idx]=newChild;
				return;
			}
		}
		//resize!
		PPMNode** oldChildrenArray = childrenArray;
		int oldSlots = numOfChildSlots;
		int newNumElems;
		if (numOfChildSlots>=numSymbols/4) {
			numOfChildSlots=-numSymbols; //negative = "use direct indexing"
			newNumElems=numSymbols;
		} else {
			numOfChildSlots+=numOfChildSlots+1;
			newNumElems=numOfChildSlots;
		}
		childrenArray=new PPMNode*[newNumElems]; //null terminator
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

PPMLanguageModel::PPMNode* PPMLanguageModel::PPMNode::findSymbol(Symbol symbolToFind) const {
	//see if symbol is a child of node
	if (numOfChildSlots<0) //negative to mean "full alphabet", use direct indexing
		return childrenArray[symbolToFind];
	if (numOfChildSlots==1) {
		if (child->symbol==symbolToFind) return child;
		return NULL;
	}
	if (numOfChildSlots<=MAX_RUN) {
		for (int i = 0; i<numOfChildSlots && childrenArray[i]!=NULL; i++)
			if (childrenArray[i]->symbol==symbolToFind) return childrenArray[i];
		return NULL;
	}
	//printf("finding symbol %d at node %d\n", symbol, node->id);
	for (int i = symbolToFind;; i++) { //search through elements which have overflowed into subsequent slots
		PPMNode* found = this->childrenArray[i%numOfChildSlots]; //wrap round
		if (!found) return NULL; //null element
		if (found->symbol==symbolToFind) return found;
	}
	return NULL;
}
