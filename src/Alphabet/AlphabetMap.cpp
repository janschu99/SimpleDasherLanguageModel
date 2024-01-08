#include "AlphabetMap.h"
#include <limits>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace Dasher;
using namespace std;

#define UNKNOWN_SYMBOL 0

int CAlphabetMap::SymbolStream::getUtf8Count(int pos) {
	if (pos<=0x7f) return 1;
	if (pos<=0xc1) return 0;
	if (pos<=0xdf) return 2;
	if (pos<=0xef) return 3;
	if (pos<=0xf4) return 4;
	return 0;
}

CAlphabetMap::SymbolStream::SymbolStream(std::istream &_in) :
		pos(0), len(0), in(_in) {
	readMore();
}

void CAlphabetMap::SymbolStream::readMore() {
	//len is first unfilled byte
	in.read(&buf[len], 1024-len);
	if (in.good()) {
		//DASHER_ASSERT(in.gcount() == 1024-len);
		len = 1024;
	} else {
		len += in.gcount();
		//DASHER_ASSERT(len<1024);
		//next attempt to read more will fail.
	}
}

inline int CAlphabetMap::SymbolStream::findNext() {
	for (;;) {
		if (pos+4>len) { //4 is max length of an UTF-8 char
			//may need more bytes for next char
			if (pos) {
				//shift remaining bytes to beginning
				len -= pos; //len of them
				memmove(buf, &buf[pos], len);
				pos = 0;
			}
			//and look for more
			readMore();
		}
		//if still don't have any chars after attempting to read more...EOF!
		if (pos==len) return 0; //EOF
		if (int numChars = getUtf8Count(buf[pos])) {
			if (pos+numChars>len) {
				//no more bytes in file (would have tried to read earlier), but not enough for char
				printf("File ends with incomplete UTF-8 character beginning 0x%x (expecting %i bytes but only %li)\n",
						static_cast<unsigned int>(buf[pos]&0xff), numChars, len-pos);
				return 0;
			}
			return numChars;
		}
		printf("Read invalid UTF-8 character 0x%x\n", static_cast<unsigned int>(buf[pos]&0xff));
		++pos;
	}
}

symbol CAlphabetMap::SymbolStream::next(const CAlphabetMap *map) {
	int numChars = findNext();
	if (numChars==0) return -1; //EOF
	if (numChars==1) {
		if (map->m_ParagraphSymbol!=UNKNOWN_SYMBOL && buf[pos]=='\r') {
			//DASHER_ASSERT(pos+1<len || len<1024); //there are more characters (we should have read utf8...max_length), or else input is exhausted
			if (pos+1<len && buf[pos+1]=='\n') {
				pos += 2;
				return map->m_ParagraphSymbol;
			}
		}
		return map->GetSingleChar(buf[pos++]);
	}
	int sym = map->Get(string(&buf[pos], numChars));
	pos += numChars;
	return sym;
}

void CAlphabetMap::GetSymbols(std::vector<symbol> &Symbols, const std::string &Input) const {
	std::istringstream in(Input);
	SymbolStream syms(in);
	for (symbol sym; (sym = syms.next(this))!=-1;)
		Symbols.push_back(sym);
}

CAlphabetMap::CAlphabetMap(unsigned int InitialTableSize) :
		HashTable(InitialTableSize<<1), m_ParagraphSymbol(UNKNOWN_SYMBOL) {
	Entries.reserve(InitialTableSize);
	// TODO: fix the code so it works if char is signed.
	const int numChars = numeric_limits<char>::max()+1;
	m_pSingleChars = new symbol[numChars];
	for (int i = 0; i<numChars; i++)
		m_pSingleChars[i] = UNKNOWN_SYMBOL;
}

CAlphabetMap::~CAlphabetMap() {
	delete[] m_pSingleChars;
}

void CAlphabetMap::AddParagraphSymbol(symbol Value) {
	//DASHER_ASSERT (m_ParagraphSymbol==UNKNOWN_SYMBOL);
	//DASHER_ASSERT (m_pSingleChars['\r'] == UNKNOWN_SYMBOL);
	//DASHER_ASSERT (m_pSingleChars['\n'] == UNKNOWN_SYMBOL);
	m_pSingleChars['\n'] = m_ParagraphSymbol = Value;
}

void CAlphabetMap::Add(const std::string &Key, symbol Value) {
	//Only single unicode-characters should be added...
	//DASHER_ASSERT(m_utf8_count_array[Key[0]]==Key.length());
	if (Key.length()==1) {
		//DASHER_ASSERT(m_pSingleChars[Key[0]]==UNKNOWN_SYMBOL);
		//DASHER_ASSERT(Key[0]!='\r' || m_ParagraphSymbol==UNKNOWN_SYMBOL);
		m_pSingleChars[Key[0]] = Value;
		return;
	}
	Entry *&HashEntry = HashTable[Hash(Key)];
	//Loop through Entries with the correct Hash value,
	// to check the key is not already present
	//for (Entry *i = HashEntry; i; i = i->Next) {
		//DASHER_ASSERT(i->Key != Key);
	//}
	// When hash table gets 1/2 full...
	// (no I haven't optimised when to resize)
	if (Entries.size()<<1>=HashTable.size()) {
		// Double up all the storage
		HashTable.clear();
		HashTable.resize(Entries.size()<<2);
		Entries.reserve(Entries.size()<<1);
		// Rehash as the pointers will all be mangled.
		for (unsigned int j = 0; j<Entries.size(); j++) {
			Entry *&HashEntry2 = HashTable[Hash(Entries[j].Key)];
			Entries[j].Next = HashEntry2;
			HashEntry2 = &Entries[j];
		}
		// Have to recall this function as the key's hash needs recalculating
		Add(Key, Value);
		return;
	}
	Entries.push_back(Entry(Key, Value, HashEntry));
	HashEntry = &Entries.back();
}

symbol CAlphabetMap::Get(const std::string &Key) const {
	if (m_ParagraphSymbol!=UNKNOWN_SYMBOL && Key=="\r\n") return m_ParagraphSymbol;
	//DASHER_ASSERT(m_utf8_count_array[Key[0]]==Key.length());
	if (Key.length()==1) return GetSingleChar(Key[0]);
	// Loop through Entries with the correct Hash value.
	for (Entry *i = HashTable[Hash(Key)]; i; i = i->Next) {
		if (i->Key==Key) return i->Symbol;
	}
	return UNKNOWN_SYMBOL;
}

symbol CAlphabetMap::GetSingleChar(char key) const {
	return m_pSingleChars[key];
}
