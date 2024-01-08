#ifndef __PPMLanguageModel_h__
#define __PPMLanguageModel_h__

#include "../Common/DasherTypes.h"
#include "../Common/NoClones.h"
#include "../Common/PooledAlloc.h"
#include "stdlib.h"
#include <vector>
#include <set>

namespace Dasher {
	
	/// Common superclass for both PPM and PPMY language models. Implements the PPM tree,
	/// inc. fast hashing of child nodes by symbol number; and entering and learning symbols
	/// in a context, i.e. navigating and updating the tree, with update exclusion according
	/// to LP_LM_UPDATE_EXCLUSION
	/// Subclasses must implement CLanguageModel::GetProbs and a makeNode() method (perhaps
	/// using a pooled allocator).
	class CAbstractPPM: private NoClones {
		protected:
			class ChildIterator;
			class CPPMnode {
				private:
					union {
						CPPMnode **m_ppChildren;
						CPPMnode *m_pChild;
					};
					///Elements in above array, including nulls, as follows:
					/// (a) negative -> absolute value is number of elems in m_ppChildren, but use direct indexing
					/// (b) 1 -> use m_pChild as direct pointer to CPPMnode (no array)
					/// (c) 2-MAX_RUN -> m_ppChildren is unordered array of that many elems
					/// (d) >MAX_RUN ->  m_ppChildren is an inline hash (overflow to next elem) with that many slots
					int m_iNumChildSlots;
					friend class CPPMLanguageModel;
				public:
					ChildIterator children() const;
					const ChildIterator end() const;
					void AddChild(CPPMnode *pNewChild, int numSymbols);
					CPPMnode* find_symbol(symbol sym) const;
					CPPMnode *vine;
					unsigned short int count;
					symbol sym;
					CPPMnode(symbol sym);
					CPPMnode();
					virtual ~CPPMnode();
			};
			class ChildIterator {
				private:
					void nxt() {
						if (m_ppChild==m_ppStop) return;
						while ((--m_ppChild)!=m_ppStop)
							if (*m_ppChild) break;
					}
				public:
					bool operator==(const ChildIterator &other) const {
						return m_ppChild==other.m_ppChild && m_ppStop==other.m_ppStop;
					}
					bool operator!=(const ChildIterator &other) const {
						return m_ppChild!=other.m_ppChild || m_ppStop!=other.m_ppStop;
					}
					CPPMnode* operator*() const {
						return (m_ppChild==m_ppStop) ? NULL : *m_ppChild;
					}
					ChildIterator operator++(int) {
						ChildIterator temp(*this);
						nxt();
						return temp;
					}
					ChildIterator(CPPMnode *const*ppChild, CPPMnode *const*ppStop) :
							m_ppChild(ppChild), m_ppStop(ppStop) {
						nxt();
					}
				private:
					CPPMnode *const*m_ppChild, *const*m_ppStop;
			};
			
			class CPPMContext {
				public:
					CPPMContext(CPPMContext const &input) {
						head = input.head;
						order = input.order;
					}
					CPPMContext(CPPMnode *_head = 0, int _order = 0) :
							head(_head), order(_order) {
					};
					~CPPMContext() {
						//empty
					};
					CPPMnode *head;
					int order;
			};
			
			///Makes a new node, of whatever kind (subclass of CPPMnode, perhaps with extra info)
			/// is required by the subclass, for the specified symbol. (Initial count will be 1.)
			virtual CPPMnode* makeNode(int sym)=0;
			/// \param iMaxOrder max order of model; anything <0 means to use LP_LM_MAX_ORDER.
			CAbstractPPM(int iNumSyms, CPPMnode *pRoot, int iMaxOrder = -1);
			CPPMContext *m_pRootContext;
			CPPMnode *m_pRoot;
			/// Cache parameters that don't make sense to adjust during the life of a language model...
			const int m_iMaxOrder;
			const bool bUpdateExclusion;
			///Return the number of symbols over which we are making predictions, plus one
			/// (to leave space for an initial 0).
			int GetSize() const {
				return m_iNumSyms+1;
			}
			const int m_iNumSyms;
		public:
			/// Index of registered context
			typedef size_t Context;
			virtual ~CAbstractPPM() {
				//empty
			};
			Context CreateEmptyContext();
			void ReleaseContext(Context context);
			Context CloneContext(Context context);
			virtual void EnterSymbol(Context context, int Symbol);
			virtual void LearnSymbol(Context context, int Symbol);
			bool isValidContext(const Context c) const;
		private:
			CPPMnode* AddSymbolToNode(CPPMnode *pNode, symbol sym);
			CPooledAlloc<CPPMContext> m_ContextAlloc;
			std::set<const CPPMContext*> m_setContexts;
	};
	
	///"Standard" PPM language model: GetProbs uses counts in PPM child nodes,
	/// universal alpha+beta values read from LP_LM_ALPHA and LP_LM_BETA,
	/// max order from LP_LM_MAX_ORDER.
	class CPPMLanguageModel: public CAbstractPPM {
		public:
			CPPMLanguageModel(int iNumSyms);
			virtual void GetProbs(Context context, std::vector<unsigned int> &Probs, int norm, int iUniform) const;
		protected:
			/// Makes a standard CPPMnode, but using a pooled allocator (m_NodeAlloc) - faster!
			virtual CPPMnode* makeNode(int sym);
		private:
			int NodesAllocated;
			mutable CSimplePooledAlloc<CPPMnode> m_NodeAlloc;
	};
	
	inline CAbstractPPM::ChildIterator CPPMLanguageModel::CPPMnode::children() const {
		//if m_iNumChildSlots = 0 / 1, m_ppChildren is direct pointer, else ptr to array (of pointers)
		CPPMnode *const*ppChild =
				(m_iNumChildSlots==0||m_iNumChildSlots==1) ?
						&m_pChild : m_ppChildren;
		return ChildIterator(ppChild+abs(m_iNumChildSlots), ppChild-1);
	}
	
	inline const CAbstractPPM::ChildIterator CPPMLanguageModel::CPPMnode::end() const {
		//if m_iNumChildSlots = 0 / 1, m_ppChildren is direct pointer, else ptr to array (of pointers)
		CPPMnode *const*ppChild = (m_iNumChildSlots==0||m_iNumChildSlots==1) ? &m_pChild : m_ppChildren;
		return ChildIterator(ppChild, ppChild-1);
	}
	
	inline Dasher::CAbstractPPM::CPPMnode::CPPMnode(symbol _sym) :
			sym(_sym) {
		vine = 0;
		m_iNumChildSlots = 0;
		m_ppChildren = NULL;
		count = 1;
	}
	
	inline CAbstractPPM::CPPMnode::CPPMnode() {
		vine = 0;
		m_iNumChildSlots = 0;
		m_ppChildren = NULL;
		count = 1;
	}
	
	inline CAbstractPPM::CPPMnode::~CPPMnode() {
		//single child = is direct pointer to node, not array...
		if (m_iNumChildSlots!=1) delete[] m_ppChildren;
	}
	
	inline CAbstractPPM::Context CAbstractPPM::CreateEmptyContext() {
		CPPMContext *pCont = m_ContextAlloc.Alloc();
		*pCont = *m_pRootContext;
		m_setContexts.insert(pCont);
		return (Context) pCont;
	}
	
	inline CAbstractPPM::Context CAbstractPPM::CloneContext(Context Copy) {
		CPPMContext *pCont = m_ContextAlloc.Alloc();
		CPPMContext *pCopy = (CPPMContext*) Copy;
		*pCont = *pCopy;
		m_setContexts.insert(pCont);
		return (Context) pCont;
	}
	
	inline void CAbstractPPM::ReleaseContext(Context release) {
		m_setContexts.erase(m_setContexts.find((CPPMContext*) release));
		m_ContextAlloc.Free((CPPMContext*) release);
	}
}

#endif
