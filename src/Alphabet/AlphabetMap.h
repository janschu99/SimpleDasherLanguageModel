#ifndef __AlphabetMap_h__
#define __AlphabetMap_h__

#include "../Common/DasherTypes.h"
#include <vector>
#include <string>

namespace Dasher {
	class CAlphabetMap;
}

/// Class used for fast conversion from training text (i.e. catenated
/// non-display text of symbols; Mandarin / Super-PinYin is a bit more
/// complicated but still uses one!) into Dasher's internal "symbol" indices.
/// One of these is created for the alphabet (CAlphInfo) currently in use,
/// tho there are no restrictions on creation of CAlphabetMaps in other places
/// (Mandarin!) - or modification, if you have a non-const pointer!
///
/// Ian clearly had reservations about this system, as follows; and I'd add
/// that much of the fun comes from supporting single unicode characters
/// which are multiple octets, as we use  std::string (which works in octets)
/// for everything...note that we do *not* support multi-unicode-character
/// symbols (such as the "asdf" suggested below) except in the case of "\r\n"
/// for the paragraph symbol.
///
/// Note that in 2010 we did indeed tailor this to the alphabet more closely,
/// fast-casing single-octet characters to avoid using a hash etc. - this makes
/// many common alphabets substantially faster!
///
/// Anyway, Ian writes:
///
/// If I were just using GCC, which comes with the CGI "STL" implementation, I would
/// use hash_map (which isn't part of the ANSI/ISO standard C++ STL, but hey it's nice).
/// Using a plain map is just too slow for training on large files (or it is with certain
/// STL implementations). I'm sure training could be made much faster still, but that's
/// another matter...
/// 
/// While I could (and probably should) get a hash_map for VC++ from
/// http://www.stlport.org I thought it would be nicer if people didn't have
/// to download extra stuff and then have to get it working alongside the STL
/// with VC++, especially for just one small part of Dasher.
/// 
/// The result is this:
/// ***************************************************
/// very much thrown together to get Dasher out ASAP.
/// ***************************************************
/// It is deliberately not like an STL container.
/// However, as it has a tiny interface, it should still be easy to replace.
/// Sorry if this seems really unprofressional.
/// 
/// Replacing it might be a good idea. On the other hand it could be customised
/// to the needs of the alphabet, so that it works faster.
///
/// You can't remove items once they are added as Dasher has no need for that.
/// 
/// IAM 08/2002

class Dasher::CAlphabetMap {
	public:
		~CAlphabetMap();
		// Return the symbol associated with Key or Undefined.
		symbol Get(const std::string &Key) const;
		symbol GetSingleChar(char key) const;
		class SymbolStream {
			public:
				///pMsgs used for reporting errors in utf8 encoding
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
				char buf[1024];
				off_t pos, len;
				std::istream &in;
		};
		// Fills Symbols with the symbols corresponding to Input. {{{ Note that this
		// is not necessarily reversible by repeated use of GetText. Some text
		// may not be recognised; any such will be turned into symbol number 0.}}}
		void GetSymbols(std::vector<symbol> &Symbols,
				const std::string &Input) const;
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

#endif /* #ifndef __AlphabetMap_h__ */
