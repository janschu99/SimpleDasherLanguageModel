#ifndef __AlphabetMap_h__
#define __AlphabetMap_h__

#include "../Common/DasherTypes.h"
#include <vector>
#include <string>

namespace Dasher {
	class CAlphabetMap {
		public:
			~CAlphabetMap();
			// Return the symbol associated with Key or Undefined.
			symbol Get(const std::string &Key) const;
			symbol GetSingleChar(char key) const;
			class SymbolStream {
				public:
					SymbolStream(std::istream &_in);
					///Gets the next symbol in the stream, using the specified AlphabetMap
					/// to convert unicode characters to symbols.
					/// \return 0 for unknown symbol (not in map); -1 for EOF; else symbol#.
					symbol next(const CAlphabetMap *map);
				private:
					///Finds beginning of next unicode character, at position 'pos' or later,
					/// filling buffer and skipping invalid characters as necessary.
					/// Leaves 'pos' pointing at beginning of said character.
					/// \return the number of octets representing the next character, or 0 for EOF
					/// (inc. where the file ends with an incomplete character)
					inline int findNext();
					void readMore();
					int getUtf8Count(int pos);
					char buf[1024];
					off_t pos, len;
					std::istream &in;
			};
			// Fills Symbols with the symbols corresponding to Input. {{{ Note that this
			// is not necessarily reversible by repeated use of GetText. Some text
			// may not be recognised; any such will be turned into symbol number 0.}}}
			void GetSymbols(std::vector<symbol> &Symbols, const std::string &Input) const;
			CAlphabetMap(unsigned int InitialTableSize = 255);
			void AddParagraphSymbol(symbol Value);
			///Add a symbol to the map
			/// \param Key text of the symbol; must not be present already
			/// \param Value symbol number to which that text should be mapped
			void Add(const std::string &Key, symbol Value);
		private:
			class Entry {
				public:
					Entry(std::string Key, symbol Symbol, Entry *Next) :
							Key(Key), Symbol(Symbol), Next(Next) {
						//empty
					}
					std::string Key;
					symbol Symbol;
					Entry *Next;
			};
			// A standard hash -- could try and research something specific.
			inline unsigned int Hash(const std::string &Input) const {
				unsigned int Result = 0;
				std::string::const_iterator Cur = Input.begin();
				std::string::const_iterator end = Input.end();
				while (Cur!=end)
					Result = (Result<<1)^*Cur++;
				Result %= HashTable.size();
				return Result;
				/*
				 if (Input.size()==1) // Speedup for ASCII text
				 return Input[0];
				 for (int i=0; i<Input.size(); i++)
				 Result = (Result<<1)^Input[i];
				 return Result%HashTable.size();
				 */
			}
			std::vector<Entry> Entries;
			std::vector<Entry*> HashTable;
			symbol *m_pSingleChars;
			/// both "\r\n" and "\n" are mapped to this (if not Undefined).
			/// This is the only case where >1 character can map to a symbol.
			symbol m_ParagraphSymbol;
	};
}

#endif /* #ifndef __AlphabetMap_h__ */
