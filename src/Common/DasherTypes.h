#ifndef DASHER_TYPES_INCLUDED
#define DASHER_TYPES_INCLUDED

//from int.h:
#ifdef _WIN32
	typedef __int64 int64;
#else
	typedef long long int int64;
#endif

namespace Dasher {
	
	//Using a signed symbol type allows "Out of band" i.e. negative
	//values to be used to flag non-symbol data. For example commands
	//in dasher nodes.
	typedef int Symbol;
}

#endif
