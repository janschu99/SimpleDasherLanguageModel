#ifndef DASHER_TYPES_INCLUDED
#define DASHER_TYPES_INCLUDED

namespace Dasher {
	
	//Using a signed symbol type allows "Out of band" i.e. negative
	//values to be used to flag non-symbol data. For example commands
	//in dasher nodes.
	typedef int Symbol;
}

#endif
