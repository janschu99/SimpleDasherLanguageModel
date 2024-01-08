#include "LanguageModelling/LanguageModel.h"
#include "LanguageModelling/PPMLanguageModel.h"
#include "Alphabet/AlphabetMap.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace Dasher;

template <typename T>
static void printVector(std::vector<T> v) {
	std::size_t size = v.size();
	T sum = 0;
	for(std::size_t i = 0; i < size; ++i) {
		sum+=v[i];
	}
	std::cout << "Size: " << size << ", Sum: " << sum << "\n";
	for(std::size_t i = 0; i < size; ++i) {
		std::cout << v[i] << "\n";
	}
}

static unsigned long getNorm(unsigned int numOfSymbols) {
	//adapted from CAlphabetManager::GetProbs
	const unsigned int iSymbols = numOfSymbols; //m_pBaseGroup->iEnd-1;
	static const unsigned int NORMALIZATION = 1<<16; //from CDasherModel
	//const unsigned long iNorm(CDasherModel::NORMALIZATION-iControlSpace/*m_pNCManager->GetAlphNodeNormalization()*/); //from CNodeCreationManager::CreateControlBox
	const unsigned long iNorm(NORMALIZATION-0);
	const unsigned int iUniformAdd = std::max(1ul, ((iNorm * 80/*GetLongParameter(LP_UNIFORM)*/) / 1000) / iSymbols);
	const unsigned long iNonUniformNorm = iNorm - iSymbols * iUniformAdd;
	return iNonUniformNorm;
}

//Reading alphabet definitions from xml files has been left out because it is irrelevant for the core data structure
//and algorithm. For testing purposes, simply hardcode a small alphabet.
CAlphabetMap* getDefaultAlphabetMap() {
	CAlphabetMap* map = new CAlphabetMap();
	map->Add("a", 1);
	map->Add("b", 2);
	map->Add("c", 3);
	map->Add("d", 4);
	return map;
}

//construct a CAlphabetMap::SymbolStream by passing a std::istream to its constructor (see CTrainer::Parse)
//adapted from CTrainer::Train
void train(CLanguageModel* model, CAlphabetMap* alphabetMap, CAlphabetMap::SymbolStream &syms) {
	CLanguageModel::Context sContext = model->CreateEmptyContext();
	for(symbol sym; (sym=syms.next(alphabetMap))!=-1;) {
		model->LearnSymbol(sContext, sym);
	}
	model->ReleaseContext(sContext);
}

int main() {
	unsigned int numOfSymbols = 4;
	unsigned long norm = getNorm(numOfSymbols);
	
	CPPMLanguageModel lm(numOfSymbols);
	CAlphabetMap* alphabetMap = getDefaultAlphabetMap();
	std::ifstream trainingTextStream;
	trainingTextStream.open("training.txt");
	CAlphabetMap::SymbolStream symStream(trainingTextStream);
	train(&lm, alphabetMap, symStream);
	trainingTextStream.close();
	delete alphabetMap;
	
	
	
	CLanguageModel::Context context;
	std::vector<unsigned int> probs;
	
	std::cout << "Entering 'b':\n";
	context = lm.CreateEmptyContext();
	lm.EnterSymbol(context, 2);
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	lm.ReleaseContext(context);
	
	std::cout << "\nEntering 'a':\n";
	context = lm.CreateEmptyContext();
	lm.EnterSymbol(context, 1);
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	lm.ReleaseContext(context);
	
	std::cout << "\nEntering 'bc':\n";
	context = lm.CreateEmptyContext();
	lm.EnterSymbol(context, 2);
	lm.EnterSymbol(context, 3);
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	lm.ReleaseContext(context);
	
	std::cout << "\nEntering 'ddc':\n";
	context = lm.CreateEmptyContext();
	lm.EnterSymbol(context, 4);
	lm.EnterSymbol(context, 4);
	lm.EnterSymbol(context, 3);
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	lm.ReleaseContext(context);
	
	std::cout << "\nEntering 'bdca':\n";
	context = lm.CreateEmptyContext();
	lm.EnterSymbol(context, 2);
	lm.EnterSymbol(context, 4);
	lm.EnterSymbol(context, 3);
	lm.EnterSymbol(context, 1);
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	lm.ReleaseContext(context);
	
	return 0;
}
