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
			void add(const std::string &key, Symbol value);
			//Returns the symbol associated with 'key' or Undefined.
			Symbol get(const std::string &key) const;
			Symbol getSingleChar(char key) const;
		private:
			class Entry {
				public:
					Entry(std::string key, Symbol symbol, Entry *next) :
							key(key), symbol(symbol), next(next) {
						//empty
					}
					std::string key;
					Symbol symbol;
					Entry *next;
			};
			std::vector<Entry> entries;
			std::vector<Entry*> hashTable;
			Symbol *singleChars;
			// A standard hash -- could try and research something specific.
			inline unsigned int hash(const std::string &input) const {
				unsigned int result = 0;
				std::string::const_iterator cur = input.begin();
				std::string::const_iterator end = input.end();
				while (cur!=end)
					result = (result<<1)^*cur++;
				result %= hashTable.size();
				return result;
				/*
				 if (input.size()==1) return Input[0]; //Speedup for ASCII text
				 for (int i = 0; i<input.size(); i++)
				 result = (result<<1)^input[i];
				 return result%hashTable.size();
				 */
			}
	};
}

#endif
