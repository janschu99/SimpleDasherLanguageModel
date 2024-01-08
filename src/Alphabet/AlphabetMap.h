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
			void addParagraphSymbol(Symbol value);
			//Returns the symbol associated with 'key' or Undefined.
			Symbol get(const std::string &key) const;
			Symbol getSingleChar(char key) const;
			class SymbolStream {
				public:
					SymbolStream(std::istream &in);
					//Gets the next symbol in the stream, using the specified AlphabetMap
					//to convert unicode characters to symbols.
					//Returns 0 for unknown symbol (not in map); -1 for EOF; else symbol#.
					Symbol next(const AlphabetMap *map);
				private:
					char buf[1024];
					off_t pos, len;
					std::istream &in;
					//Finds beginning of next unicode character, at position 'pos' or later,
					//filling buffer and skipping invalid characters as necessary.
					//Leaves 'pos' pointing at beginning of said character.
					//Returns the number of octets representing the next character, or 0 for EOF
					//(including where the file ends with an incomplete character)
					inline int findNext();
					void readMore();
					int getUtf8Count(int pos);
			};
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
			//Both "\r\n" and "\n" are mapped to this (if not Undefined).
			//This is the only case where >1 character can map to a symbol.
			Symbol paragraphSymbol;
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
