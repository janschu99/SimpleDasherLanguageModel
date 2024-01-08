#ifndef __DasherTypes_h__
#define __DasherTypes_h__

// We use our own version of hungarian notation to indicate
// the type of variables:
//    i       - integer and enumerated types 
//    c       - char
//    str     - STL string
//    sz      - char* string
//    b       - boolean
//    p       - pointer (to a primative type or to an object)
//    pp      - pointer to a pointer (and so on)
//    v       - STL vector
//    map     - STL map
//    d       - float or double
//    s       - structure 
//    o       - object
//    h       - HANDLE type in Windows
// Class member variables and global variables should 
// have the additional prefixes:
//    m_      - member variables 
//    g_      - global variablse
//    s_      - static member variables
// Variables names (local and member) should capitalize each 
// new word and don't use underscores (except as above).

//from int.h:
#ifdef _WIN32
typedef __int64 int64;
#else
typedef long long int int64;
#endif

namespace Dasher {

	// Using a signed symbol type allows "Out of band" ie negative {{{
	// values to be used to flag non-symbol data. For example commands
	// in dasher nodes.
	//typedef unsigned int symbol; // }}}
	typedef int symbol;
}

#endif /* #ifndef __DasherTypes_h__ */
