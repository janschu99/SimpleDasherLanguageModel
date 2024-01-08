#include "LanguageModel.h"
#include "PPMLanguageModel.h"
#include <vector>
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

int main() {
	unsigned int numOfSymbols = 10;
	unsigned long norm = getNorm(numOfSymbols);
	
	CPPMLanguageModel lm(numOfSymbols);
	CLanguageModel::Context context;
	context = lm.CreateEmptyContext();
	lm.LearnSymbol(context, 5);
	lm.LearnSymbol(context, 8);
	lm.LearnSymbol(context, 4);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 4);
	lm.LearnSymbol(context, 2);
	lm.LearnSymbol(context, 5);
	lm.LearnSymbol(context, 1);
	lm.LearnSymbol(context, 1);
	lm.LearnSymbol(context, 7);
	lm.LearnSymbol(context, 6);
	lm.LearnSymbol(context, 5);
	lm.LearnSymbol(context, 4);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 2);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 2);
	lm.LearnSymbol(context, 6);
	lm.LearnSymbol(context, 8);
	lm.LearnSymbol(context, 3);
	lm.LearnSymbol(context, 4);
	
	std::vector<unsigned int> probs;
	
	
	lm.GetProbs(context, probs, norm, 0);
	printVector(probs);
	return 0;
}


//from Src/Test/LanguageModelling/main.cpp - outdated, doesn't work anymore because some classes don't even exist
/*string userlocation = "/usr/local/share/dasher/";
string filename = "alphabet.english.xml";
vector < string > vFileNames;
vFileNames.push_back(filename);

// Set up the CAlphIO
std::auto_ptr < CAlphIO > ptrAlphIO(new CAlphIO("", userlocation, vFileNames));

string strID = "English alphabet with lots of punctuation";
const CAlphIO::AlphInfo & AlphInfo = ptrAlphIO->GetInfo(strID);

// Create the Alphabet that converts plain text to symbols
std::auto_ptr < CAlphabet > ptrAlphabet(new CAlphabet(AlphInfo));

// DJW - add some functionality to CAlphabet to get the CSymbolAlphabet
CSymbolAlphabet alphabet(ptrAlphabet->GetNumberSymbols());
alphabet.SetSpaceSymbol(ptrAlphabet->GetSpaceSymbol());

CPPMLanguageModel lm(alphabet);*/
