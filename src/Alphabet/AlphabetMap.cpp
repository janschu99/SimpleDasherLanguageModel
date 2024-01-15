#include "AlphabetMap.h"
#include <limits>
#include <sstream>

using namespace Dasher;

#define UNKNOWN_SYMBOL 0

AlphabetMap::AlphabetMap(unsigned int initialTableSize) : hashTable(initialTableSize<<1) {
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

Symbol AlphabetMap::get(const std::string &key) const {
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
