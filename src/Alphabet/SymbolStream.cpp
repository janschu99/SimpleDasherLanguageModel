#include "SymbolStream.h"
#include "AlphabetMap.h"
#include <iostream>
#include <string>
#include <cstring> //for memmove

//Note: The following macro doesn't work for arrays passed as function parameter
//because those "decay" into a simple pointer and the size information is lost.
//Also, the array must actually be declared as an array and not as a simple pointer
//(which is usually equivalent, but not in this case):
// char data1[] = "Test"; will work correctly (and also count the terminating 0-char), but
// char* data2 = "Test"; won't work (and ARRAY_LENGTH(data2) should result in a compiler warning).
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*(array)))

using namespace Dasher;

SymbolStream::SymbolStream(std::istream& in) : pos(0), validBufferLength(0), in(in) {
	readMore();
}

Symbol SymbolStream::next(const AlphabetMap* map) {
	int utf8Length = findNext();
	if (utf8Length==0) return -1; //EOF
	if (utf8Length==1) return map->getSingleChar(buffer[pos++]);
	Symbol symbol = map->get(std::string(&buffer[pos], utf8Length));
	pos+=utf8Length;
	return symbol;
}

int SymbolStream::findNext() {
	while (true) {
		if (pos+4>validBufferLength) { //4 is max length of an UTF-8 char
			//may need more bytes for next char, so...
			if (pos>0) { //...shift remaining bytes to the beginning of buffer...
				validBufferLength-=pos; //length of them
				memmove(buffer, &buffer[pos], validBufferLength);
				pos=0;
			}
			
			readMore(); //...and look for more
		}
		if (pos==validBufferLength) return 0; //still don't have any chars after attempting to read more, EOF
		if (int utf8Length = getUTF8Length(buffer[pos])) {
			if (pos+utf8Length>validBufferLength) {
				//no more bytes in file (would have tried to read earlier), but not enough for char
				printf("File ends with incomplete UTF-8 character beginning 0x%x (expecting %i bytes but only %li)\n",
						static_cast<unsigned int>(buffer[pos]&0xff), utf8Length, validBufferLength-pos);
				return 0;
			}
			return utf8Length;
		}
		printf("Read invalid UTF-8 character 0x%x\n", static_cast<unsigned int>(buffer[pos]&0xff));
		pos++;
	}
}

void SymbolStream::readMore() {
	//'validBufferLength' is first unfilled byte
	in.read(&buffer[validBufferLength], ARRAY_LENGTH(buffer)-validBufferLength);
	if (in.good()) { //read full buffer
		//DASHER_ASSERT(in.gcount()==ARRAY_LENGTH(buffer)-validBufferLength);
		validBufferLength=ARRAY_LENGTH(buffer);
	} else { //couldn't fill whole buffer, next attempt to read more will fail
		validBufferLength+=in.gcount();
		//DASHER_ASSERT(validBufferLength<ARRAY_LENGTH(buffer));
	}
}

int SymbolStream::getUTF8Length(int firstByte) {
	if (firstByte<=0x7f) return 1;
	if (firstByte<=0xc1) return 0;
	if (firstByte<=0xdf) return 2;
	if (firstByte<=0xef) return 3;
	if (firstByte<=0xf4) return 4;
	return 0;
}
