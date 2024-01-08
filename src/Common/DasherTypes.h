#ifndef __DasherTypes_h__
#define __DasherTypes_h__

//from int.h:
#ifdef _WIN32
typedef __int64 int64;
#else
typedef long long int int64;
#endif

namespace Dasher {
	
	// Using a signed symbol type allows "Out of band" ie negative
	// values to be used to flag non-symbol data. For example commands
	// in dasher nodes.
	typedef int symbol;
}

#endif /* #ifndef __DasherTypes_h__ */
