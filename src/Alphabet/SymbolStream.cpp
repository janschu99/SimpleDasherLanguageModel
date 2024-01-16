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

SymbolStream::SymbolStream(std::istream& in) :
		pos(0), len(0), in(in) {
	readMore();
}

Symbol SymbolStream::next(const AlphabetMap* map) {
	int numChars = findNext();
	if (numChars==0) return -1; //EOF
	if (numChars==1) return map->getSingleChar(buf[pos++]);
	Symbol sym = map->get(std::string(&buf[pos], numChars));
	pos+=numChars;
	return sym;
}

int SymbolStream::findNext() {
	for (;;) {
		if (pos+4>len) { //4 is max length of an UTF-8 char
			//may need more bytes for next char
			if (pos) {
				//shift remaining bytes to beginning
				len-=pos; //len of them
				memmove(buf, &buf[pos], len);
				pos=0;
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

void SymbolStream::readMore() {
	//len is first unfilled byte
	in.read(&buf[len], ARRAY_LENGTH(buf)-len);
	if (in.good()) {
		//DASHER_ASSERT(in.gcount()==ARRAY_LENGTH(buf)-len);
		len=ARRAY_LENGTH(buf);
	} else {
		len+=in.gcount();
		//DASHER_ASSERT(len<ARRAY_LENGTH(buf));
		//next attempt to read more will fail.
	}
}

int SymbolStream::getUtf8Count(int pos) {
	if (pos<=0x7f) return 1;
	if (pos<=0xc1) return 0;
	if (pos<=0xdf) return 2;
	if (pos<=0xef) return 3;
	if (pos<=0xf4) return 4;
	return 0;
}
