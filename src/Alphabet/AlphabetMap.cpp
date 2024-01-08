#include "AlphabetMap.h"
#include <limits>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace Dasher;

#define UNKNOWN_SYMBOL 0

AlphabetMap::SymbolStream::SymbolStream(std::istream &in) :
		pos(0), len(0), in(in) {
	readMore();
}

Symbol AlphabetMap::SymbolStream::next(const AlphabetMap *map) {
	int numChars = findNext();
	if (numChars==0) return -1; //EOF
	if (numChars==1) {
		if (map->paragraphSymbol!=UNKNOWN_SYMBOL && buf[pos]=='\r') {
			//DASHER_ASSERT(pos+1<len || len<1024); //there are more characters
			//(we should have read utf8...max_length), or else input is exhausted
			if (pos+1<len && buf[pos+1]=='\n') {
				pos += 2;
				return map->paragraphSymbol;
			}
		}
		return map->getSingleChar(buf[pos++]);
	}
	Symbol sym = map->get(std::string(&buf[pos], numChars));
	pos += numChars;
	return sym;
}

inline int AlphabetMap::SymbolStream::findNext() {
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

void AlphabetMap::SymbolStream::readMore() {
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

int AlphabetMap::SymbolStream::getUtf8Count(int pos) {
	if (pos<=0x7f) return 1;
	if (pos<=0xc1) return 0;
	if (pos<=0xdf) return 2;
	if (pos<=0xef) return 3;
	if (pos<=0xf4) return 4;
	return 0;
}

AlphabetMap::AlphabetMap(unsigned int initialTableSize) :
		hashTable(initialTableSize<<1), paragraphSymbol(UNKNOWN_SYMBOL) {
	entries.reserve(initialTableSize);
	// TODO: fix the code so it works if char is signed.
	const int numChars = std::numeric_limits<char>::max()+1;
	singleChars = new Symbol[numChars];
	for (int i = 0; i<numChars; i++)
		singleChars[i] = UNKNOWN_SYMBOL;
}

AlphabetMap::~AlphabetMap() {
	delete[] singleChars;
}

void AlphabetMap::add(const std::string &key, Symbol value) {
	//Only single unicode-characters should be added...
	//DASHER_ASSERT(getUtf8Count(key[0])==key.length());
	if (key.length()==1) {
		//DASHER_ASSERT(singleChars[key[0]]==UNKNOWN_SYMBOL);
		//DASHER_ASSERT(key[0]!='\r' || paragraphSymbol==UNKNOWN_SYMBOL);
		singleChars[key[0]] = value;
		return;
	}
	Entry *&hashEntry = hashTable[hash(key)];
	//Loop through entries with the correct hash value,
	//to check the key is not already present
	//for (Entry *i = HashEntry; i; i = i->next) {
		//DASHER_ASSERT(i->key != key);
	//}
	//When hash table gets 1/2 full...
	//(no I haven't optimized when to resize)
	if (entries.size()<<1>=hashTable.size()) {
		//Double up all the storage
		hashTable.clear();
		hashTable.resize(entries.size()<<2);
		entries.reserve(entries.size()<<1);
		//Rehash as the pointers will all be mangled.
		for (unsigned int j = 0; j<entries.size(); j++) {
			Entry *&hashEntry2 = hashTable[hash(entries[j].key)];
			entries[j].next = hashEntry2;
			hashEntry2 = &entries[j];
		}
		//Have to recall this function as the key's hash needs recalculating
		add(key, value);
		return;
	}
	entries.push_back(Entry(key, value, hashEntry));
	hashEntry = &entries.back();
}

void AlphabetMap::addParagraphSymbol(Symbol value) {
	//DASHER_ASSERT (paragraphSymbol==UNKNOWN_SYMBOL);
	//DASHER_ASSERT (singleChars['\r'] == UNKNOWN_SYMBOL);
	//DASHER_ASSERT (singleChars['\n'] == UNKNOWN_SYMBOL);
	singleChars['\n'] = paragraphSymbol = value;
}

Symbol AlphabetMap::get(const std::string &key) const {
	if (paragraphSymbol!=UNKNOWN_SYMBOL && key=="\r\n") return paragraphSymbol;
	//DASHER_ASSERT(m_utf8_count_array[key[0]]==key.length());
	if (key.length()==1) return getSingleChar(key[0]);
	//Loop through entries with the correct hash value.
	for (Entry *i = hashTable[hash(key)]; i; i = i->next) {
		if (i->key==key) return i->symbol;
	}
	return UNKNOWN_SYMBOL;
}

Symbol AlphabetMap::getSingleChar(char key) const {
	return singleChars[key];
}
