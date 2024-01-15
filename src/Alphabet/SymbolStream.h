#include "../Common/DasherTypes.h"
#include "AlphabetMap.h"
#include <iostream>

#ifndef SYMBOL_STREAM_INCLUDED
#define SYMBOL_STREAM_INCLUDED

namespace Dasher {
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
			int findNext();
			void readMore();
			int getUtf8Count(int pos);
	};
}

#endif
