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
	for(std::size_t i = 0; i<size; i++) {
		sum+=v[i];
	}
	std::cout << "Size: " << size << ", Sum: " << sum << "\n";
	for(std::size_t i = 0; i<size; i++) {
		std::cout << v[i] << "\n";
	}
}

//Reading alphabet definitions from xml files has been left out because it is irrelevant for the core data structure
//and algorithm. For testing purposes, simply hardcode a small alphabet.
AlphabetMap* getDefaultAlphabetMap() {
	AlphabetMap* map = new AlphabetMap();
	map->add("a", 1);
	map->add("b", 2);
	map->add("c", 3);
	map->add("d", 4);
	return map;
}

AlphabetMap* getLargeAlphabetMap() {
	AlphabetMap* map = new AlphabetMap();
	map->add("a", 1);
	map->add("b", 2);
	map->add("c", 3);
	map->add("d", 4);
	map->add("e", 5);
	map->add("f", 6);
	map->add("g", 7);
	map->add("h", 8);
	map->add("i", 9);
	map->add("j", 10);
	map->add("k", 11);
	map->add("l", 12);
	map->add("m", 13);
	map->add("n", 14);
	map->add("o", 15);
	map->add("p", 16);
	map->add("q", 17);
	map->add("r", 18);
	map->add("s", 19);
	map->add("t", 20);
	map->add("u", 21);
	map->add("v", 22);
	map->add("w", 23);
	map->add("x", 24);
	map->add("y", 25);
	map->add("z", 26);
	map->add("A", 27);
	map->add("B", 28);
	map->add("C", 29);
	map->add("D", 30);
	map->add("E", 31);
	map->add("F", 32);
	map->add("G", 33);
	map->add("H", 34);
	map->add("I", 35);
	map->add("J", 36);
	map->add("K", 37);
	map->add("L", 38);
	map->add("M", 39);
	map->add("N", 40);
	map->add("O", 41);
	map->add("P", 42);
	map->add("Q", 43);
	map->add("R", 44);
	map->add("S", 45);
	map->add("T", 46);
	map->add("U", 47);
	map->add("V", 48);
	map->add("W", 49);
	map->add("X", 50);
	map->add("Y", 51);
	map->add("Z", 52);
	map->add("0", 53);
	map->add("1", 54);
	map->add("2", 55);
	map->add("3", 56);
	map->add("4", 57);
	map->add("5", 58);
	map->add("6", 59);
	map->add("7", 60);
	map->add("8", 61);
	map->add("9", 62);
	return map;
}

//construct a AlphabetMap::SymbolStream by passing a std::istream to its constructor (see CTrainer::Parse)
//adapted from CTrainer::Train
void train(PPMLanguageModel* model, AlphabetMap* alphabetMap, AlphabetMap::SymbolStream &syms) {
	PPMLanguageModel::Context sContext = model->createEmptyContext();
	for (Symbol sym; (sym=syms.next(alphabetMap))!=-1;) {
		model->learnSymbol(sContext, sym);
	}
	model->releaseContext(sContext);
}

int main() {
	int alpha = 49;
	int beta = 77;
	int uniform = 80;
	unsigned int numOfSymbols = 4;
	PPMLanguageModel lm(numOfSymbols, 5, true);
	AlphabetMap* alphabetMap = getDefaultAlphabetMap();
	std::ifstream trainingTextStream;
	trainingTextStream.open("training.txt");
	AlphabetMap::SymbolStream symStream(trainingTextStream);
	train(&lm, alphabetMap, symStream);
	trainingTextStream.close();
	delete alphabetMap;
	
	PPMLanguageModel::Context context;
	std::vector<unsigned int> probs;
	
	std::cout << "Entering 'b':\n";
	context = lm.createEmptyContext();
	lm.enterSymbol(context, 2);
	lm.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lm.releaseContext(context);
	
	std::cout << "\nEntering 'a':\n";
	context = lm.createEmptyContext();
	lm.enterSymbol(context, 1);
	lm.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lm.releaseContext(context);
	
	std::cout << "\nEntering 'bc':\n";
	context = lm.createEmptyContext();
	lm.enterSymbol(context, 2);
	lm.enterSymbol(context, 3);
	lm.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lm.releaseContext(context);
	
	std::cout << "\nEntering 'ddc':\n";
	context = lm.createEmptyContext();
	lm.enterSymbol(context, 4);
	lm.enterSymbol(context, 4);
	lm.enterSymbol(context, 3);
	lm.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lm.releaseContext(context);
	
	std::cout << "\nEntering 'bdca':\n";
	context = lm.createEmptyContext();
	lm.enterSymbol(context, 2);
	lm.enterSymbol(context, 4);
	lm.enterSymbol(context, 3);
	lm.enterSymbol(context, 1);
	lm.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lm.releaseContext(context);
	
	std::cout << "\nLoading large training file...\n";
	unsigned int numOfSymbolsLarge = 62;
	PPMLanguageModel lmLarge(numOfSymbolsLarge, 5, true);
	AlphabetMap* alphabetMapLarge = getLargeAlphabetMap();
	std::ifstream trainingTextStreamLarge;
	trainingTextStreamLarge.open("trainingLarge3.txt");
	AlphabetMap::SymbolStream symStreamLarge(trainingTextStreamLarge);
	train(&lmLarge, alphabetMapLarge, symStreamLarge);
	trainingTextStreamLarge.close();
	delete alphabetMapLarge;
	
	std::cout << "\nEntering 'bdca' in large:\n";
	context = lmLarge.createEmptyContext();
	lmLarge.enterSymbol(context, 2);
	lmLarge.enterSymbol(context, 4);
	lmLarge.enterSymbol(context, 3);
	lmLarge.enterSymbol(context, 1);
	lmLarge.getProbs(context, probs, alpha, beta, uniform);
	printVector(probs);
	lmLarge.releaseContext(context);
	
	return 0;
}
