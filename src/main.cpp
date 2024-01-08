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

CAlphabetMap* getLargeAlphabetMap() {
	CAlphabetMap* map = new CAlphabetMap();
	map->Add("a", 1);
	map->Add("b", 2);
	map->Add("c", 3);
	map->Add("d", 4);
	map->Add("e", 5);
	map->Add("f", 6);
	map->Add("g", 7);
	map->Add("h", 8);
	map->Add("i", 9);
	map->Add("j", 10);
	map->Add("k", 11);
	map->Add("l", 12);
	map->Add("m", 13);
	map->Add("n", 14);
	map->Add("o", 15);
	map->Add("p", 16);
	map->Add("q", 17);
	map->Add("r", 18);
	map->Add("s", 19);
	map->Add("t", 20);
	map->Add("u", 21);
	map->Add("v", 22);
	map->Add("w", 23);
	map->Add("x", 24);
	map->Add("y", 25);
	map->Add("z", 26);
	map->Add("A", 27);
	map->Add("B", 28);
	map->Add("C", 29);
	map->Add("D", 30);
	map->Add("E", 31);
	map->Add("F", 32);
	map->Add("G", 33);
	map->Add("H", 34);
	map->Add("I", 35);
	map->Add("J", 36);
	map->Add("K", 37);
	map->Add("L", 38);
	map->Add("M", 39);
	map->Add("N", 40);
	map->Add("O", 41);
	map->Add("P", 42);
	map->Add("Q", 43);
	map->Add("R", 44);
	map->Add("S", 45);
	map->Add("T", 46);
	map->Add("U", 47);
	map->Add("V", 48);
	map->Add("W", 49);
	map->Add("X", 50);
	map->Add("Y", 51);
	map->Add("Z", 52);
	map->Add("0", 53);
	map->Add("1", 54);
	map->Add("2", 55);
	map->Add("3", 56);
	map->Add("4", 57);
	map->Add("5", 58);
	map->Add("6", 59);
	map->Add("7", 60);
	map->Add("8", 61);
	map->Add("9", 62);
	return map;
}

//construct a CAlphabetMap::SymbolStream by passing a std::istream to its constructor (see CTrainer::Parse)
//adapted from CTrainer::Train
void train(CPPMLanguageModel* model, CAlphabetMap* alphabetMap, CAlphabetMap::SymbolStream &syms) {
	CPPMLanguageModel::Context sContext = model->CreateEmptyContext();
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
	
	CPPMLanguageModel::Context context;
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
	
	std::cout << "\nLoading large training file...\n";
	unsigned int numOfSymbolsLarge = 62;
	unsigned long normLarge = getNorm(numOfSymbolsLarge);

	CPPMLanguageModel lmLarge(numOfSymbolsLarge);
	CAlphabetMap* alphabetMapLarge = getLargeAlphabetMap();
	std::ifstream trainingTextStreamLarge;
	trainingTextStreamLarge.open("trainingLarge3.txt");
	CAlphabetMap::SymbolStream symStreamLarge(trainingTextStreamLarge);
	train(&lmLarge, alphabetMapLarge, symStreamLarge);
	trainingTextStreamLarge.close();
	delete alphabetMapLarge;

	std::cout << "\nEntering 'bdca' in large:\n";
	context = lmLarge.CreateEmptyContext();
	lmLarge.EnterSymbol(context, 2);
	lmLarge.EnterSymbol(context, 4);
	lmLarge.EnterSymbol(context, 3);
	lmLarge.EnterSymbol(context, 1);
	lmLarge.GetProbs(context, probs, normLarge, 0);
	printVector(probs);
	lmLarge.ReleaseContext(context);

	return 0;
}
