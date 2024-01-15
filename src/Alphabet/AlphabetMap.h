#ifndef ALPHABET_MAP_INCLUDED
#define ALPHABET_MAP_INCLUDED

#include "../Common/DasherTypes.h"
#include <vector>
#include <string>

namespace Dasher {
	class AlphabetMap {
		public:
			AlphabetMap(unsigned int initialTableSize = 255);
			~AlphabetMap();
			//Adds a symbol to the map
			//key: text of the symbol; must not be present already
			//value: symbol number to which that text should be mapped
			void add(const std::string& key, Symbol value);
			//Returns the symbol associated with 'key' or Undefined.
			Symbol get(const std::string& key) const;
			Symbol getSingleChar(char key) const;
		private:
			class Entry;
			std::vector<Entry> entries;
			std::vector<Entry*> hashTable;
			Symbol* singleChars;
			unsigned int hash(const std::string& input) const;
			class Entry {
				public:
					Entry(std::string key, Symbol symbol, Entry* next) :
							key(key), symbol(symbol), next(next) {
						//empty
					}
					std::string key;
					Symbol symbol;
					Entry* next;
			};
	};
}

#endif
