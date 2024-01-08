#include "PPMLanguageModel.h"

#include <math.h>
#include <string.h>
#include <stack>
#include <sstream>
#include <iostream>

using namespace Dasher;
using namespace std;

CAbstractPPM::CAbstractPPM(int iNumSyms, CPPMnode *pRoot, int iMaxOrder) :
		m_iNumSyms(iNumSyms), m_pRoot(pRoot), m_iMaxOrder(
				iMaxOrder<0 ?
						5/*GetLongParameter(LP_LM_MAX_ORDER)*/: iMaxOrder), bUpdateExclusion(
				1/*GetLongParameter(LP_LM_UPDATE_EXCLUSION)*/!=0), m_ContextAlloc(
				1024) {
	m_pRootContext = m_ContextAlloc.Alloc();
	m_pRootContext->head = m_pRoot;
	m_pRootContext->order = 0;
}

bool CAbstractPPM::isValidContext(const Context context) const {
	return m_setContexts.count((const CPPMContext*) context)>0;
}

// Get the probability distribution at the context
void CPPMLanguageModel::GetProbs(Context context,
		std::vector<unsigned int> &probs, int norm, int iUniform) const {
	const CPPMContext *ppmcontext = (const CPPMContext*) (context);
	//DASHER_ASSERT(isValidContext(context));
	int iNumSymbols = GetSize();
	probs.resize(iNumSymbols);
	std::vector<bool> exclusions(iNumSymbols);
	unsigned int iToSpend = norm;
	unsigned int iUniformLeft = iUniform;
	// TODO: Sort out zero symbol case
	probs[0] = 0;
	exclusions[0] = false;
	for (int i = 1; i<iNumSymbols; i++) {
		probs[i] = iUniformLeft/(iNumSymbols-i);
		iUniformLeft -= probs[i];
		iToSpend -= probs[i];
		exclusions[i] = false;
	}
	//DASHER_ASSERT(iUniformLeft == 0);
	//  bool doExclusion = GetLongParameter( LP_LM_ALPHA );
	bool doExclusion = 0; //FIXME
	int alpha = 49; //GetLongParameter( LP_LM_ALPHA );
	int beta = 77; //GetLongParameter( LP_LM_BETA );
	for (CPPMnode *pTemp = ppmcontext->head; pTemp; pTemp = pTemp->vine) {
		int iTotal = 0;
		for (ChildIterator pSymbol = pTemp->children(); pSymbol!=pTemp->end();
				pSymbol++) {
			symbol sym = (*pSymbol)->sym;
			if (!(exclusions[sym]&&doExclusion)) iTotal += (*pSymbol)->count;
		}
		if (iTotal) {
			unsigned int size_of_slice = iToSpend;
			for (ChildIterator pSymbol = pTemp->children();
					pSymbol!=pTemp->end(); pSymbol++) {
				if (!(exclusions[(*pSymbol)->sym]&&doExclusion)) {
					exclusions[(*pSymbol)->sym] = 1;
					
					unsigned int p = static_cast<int64>(size_of_slice)
							*(100*(*pSymbol)->count-beta)/(100*iTotal+alpha);
					
					probs[(*pSymbol)->sym] += p;
					iToSpend -= p;
				}
				// Usprintf(debug,TEXT("sym %u counts %d p %u tospend %u \n"),sym,s->count,p,tospend);
				// DebugOutput(debug);
			}
		}
	}
	unsigned int size_of_slice = iToSpend;
	int symbolsleft = 0;
	for (int i = 1; i<iNumSymbols; i++)
		if (!(exclusions[i]&&doExclusion)) symbolsleft++;
	for (int i = 1; i<iNumSymbols; i++) {
		if (!(exclusions[i]&&doExclusion)) {
			unsigned int p = size_of_slice/symbolsleft;
			probs[i] += p;
			iToSpend -= p;
		}
	}
	int iLeft = iNumSymbols-1;
	for (int i = 1; i<iNumSymbols; i++) {
		unsigned int p = iToSpend/iLeft;
		probs[i] += p;
		--iLeft;
		iToSpend -= p;
	}
	//DASHER_ASSERT(iToSpend == 0);
}

// Update context with symbol 'Symbol'
void CAbstractPPM::EnterSymbol(Context c, int Symbol) {
	if (Symbol==0) return;
	//DASHER_ASSERT(Symbol >= 0 && Symbol < GetSize());
	CPPMContext &context = *(CPPMContext*) (c);
	while (context.head) {
		if (context.order<m_iMaxOrder) { // Only try to extend the context if it's not going to make it too long
			if (CPPMnode *find = context.head->find_symbol(Symbol)) {
				context.order++;
				context.head = find;
				//      Usprintf(debug,TEXT("found context %x order %d\n"),head,order);
				//      DebugOutput(debug);
				//      std::cout << context.order << std::endl;
				return;
			}
		}
		// If we can't extend the current context, follow vine pointer to shorten it and try again
		context.order--;
		context.head = context.head->vine;
	}
	if (context.head==0) {
		context.head = m_pRoot;
		context.order = 0;
	}
	// std::cout << context.order << std::endl;

}

// add symbol to the context
// creates new nodes, updates counts
// and leaves 'context' at the new context
void CAbstractPPM::LearnSymbol(Context c, int Symbol) {
	if (Symbol==0) return;
	//DASHER_ASSERT(Symbol >= 0 && Symbol < GetSize());
	CPPMContext &context = *(CPPMContext*) (c);
	CPPMnode *n = AddSymbolToNode(context.head, Symbol);
	//DASHER_ASSERT ( n == context.head->find_symbol(Symbol));
	context.head = n;
	context.order++;
	while (context.order>m_iMaxOrder) {
		context.head = context.head->vine;
		context.order--;
	}
}

#define MAX_RUN 4

CAbstractPPM::CPPMnode* CAbstractPPM::CPPMnode::find_symbol(symbol sym) const {
	// see if symbol is a child of node
	if (m_iNumChildSlots<0) //negative to mean "full alphabet", use direct indexing
	return m_ppChildren[sym];
	if (m_iNumChildSlots==1) {
		if (m_pChild->sym==sym) return m_pChild;
		return 0;
	}
	if (m_iNumChildSlots<=MAX_RUN) {
		for (int i = 0; i<m_iNumChildSlots&&m_ppChildren[i]; i++)
			if (m_ppChildren[i]->sym==sym) return m_ppChildren[i];
		return 0;
	}
	//  printf("finding symbol %d at node %d\n",sym,node->id);
	for (int i = sym;; i++) { //search through elements which have overflowed into subsequent slots
		CPPMnode *found = this->m_ppChildren[i%m_iNumChildSlots]; //wrap round
		if (!found) return 0; //null element
		if (found->sym==sym) return found;
	}
	return 0;
}

void CAbstractPPM::CPPMnode::AddChild(CPPMnode *pNewChild, int numSymbols) {
	if (m_iNumChildSlots<0) {
		m_ppChildren[pNewChild->sym] = pNewChild;
	} else {
		if (m_iNumChildSlots==0) {
			m_iNumChildSlots = 1;
			m_pChild = pNewChild;
			return;
		} else if (m_iNumChildSlots==1) {
			//no room, have to resize...
		} else if (m_iNumChildSlots<=MAX_RUN) {
			for (int i = 0; i<m_iNumChildSlots; i++)
				if (!m_ppChildren[i]) {
					m_ppChildren[i] = pNewChild;
					return;
				}
		} else {
			int start = pNewChild->sym;
			//find length of run (including to-be-inserted element)...
			while (m_ppChildren[start = (start+m_iNumChildSlots-1)%m_iNumChildSlots]);
			int idx = pNewChild->sym;
			while (m_ppChildren[idx %= m_iNumChildSlots]) ++idx;
			//found NULL
			int stop = idx;
			while (m_ppChildren[stop = (stop+1)%m_iNumChildSlots]);
			//start and idx point to NULLs (with inserted element somewhere inbetween)
			int runLen = (m_iNumChildSlots+stop-(start+1))%m_iNumChildSlots;
			if (runLen<=MAX_RUN) {
				//ok, maintain size
				m_ppChildren[idx] = pNewChild;
				return;
			}
		}
		//resize!
		CPPMnode **oldChildren = m_ppChildren;
		int oldSlots = m_iNumChildSlots;
		int newNumElems;
		if (m_iNumChildSlots>=numSymbols/4) {
			m_iNumChildSlots = -numSymbols; // negative = "use direct indexing"
			newNumElems = numSymbols;
		} else {
			m_iNumChildSlots += m_iNumChildSlots+1;
			newNumElems = m_iNumChildSlots;
		}
		m_ppChildren = new CPPMnode*[newNumElems]; //null terminator
		memset(m_ppChildren, 0, sizeof(CPPMnode*)*newNumElems);
		if (oldSlots==1) AddChild((CPPMnode*) oldChildren, numSymbols);
		else {
			while (oldSlots-->0)
				if (oldChildren[oldSlots]) AddChild(oldChildren[oldSlots],
						numSymbols);
			delete[] oldChildren;
		}
		AddChild(pNewChild, numSymbols);
	}
}

CAbstractPPM::CPPMnode* CAbstractPPM::AddSymbolToNode(CPPMnode *pNode, symbol sym) {
	CPPMnode *pReturn = pNode->find_symbol(sym);
	// std::cout << sym << ",";
	if (pReturn!=NULL) {
		pReturn->count++;
		if (!bUpdateExclusion) {
			//update vine contexts too. Guaranteed to exist if child does!
			for (CPPMnode *v = pReturn->vine; v; v = v->vine) {
				//DASHER_ASSERT(v == m_pRoot || v->sym == sym);
				v->count++;
			}
		}
	} else {
		//symbol does not exist at this level
		pReturn = makeNode(sym); //count initialized to 1 but no vine pointer
		pNode->AddChild(pReturn, GetSize());
		pReturn->vine =
				(pNode==m_pRoot) ? m_pRoot : AddSymbolToNode(pNode->vine, sym);
	}
	return pReturn;
}

CPPMLanguageModel::CPPMLanguageModel(int iNumSyms) :
		CAbstractPPM(iNumSyms, new CPPMnode(-1)), NodesAllocated(0), m_NodeAlloc(8192) {
	//empty
}

CAbstractPPM::CPPMnode* CPPMLanguageModel::makeNode(int sym) {
	CPPMnode *res = m_NodeAlloc.Alloc();
	res->sym = sym;
	++NodesAllocated;
	return res;
}
