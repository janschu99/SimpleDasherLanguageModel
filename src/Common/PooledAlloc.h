#ifndef __PooledAlloc_h__
#define __PooledAlloc_h__

#include "SimplePooledAllocator.h"

// PooledAllocator allocates objects T in fixed-size blocks (specified in the constructor) 
// allocate returns an uninitialized T*
// free returns an object to the pool
template<typename T>
class PooledAllocator {
	public:
		//Construct with given block size
		PooledAllocator(size_t blockSize);
		//Return an uninitialized object
		T* allocate();
		//Return an object to the pool
		void free(T *elementToFree);
	private:
		//Use simple pooled alloc for the blocked allocation
		SimplePooledAllocator<T> simpleAllocator;
		//The free list
		std::vector<T*> freeList;
};

template<typename T>
PooledAllocator<T>::PooledAllocator(size_t blockSize) :
		simpleAllocator(blockSize) {
	//empty
}

template<typename T>
T* PooledAllocator<T>::allocate() {
	if (freeList.size()>0) {
		T *lastElement = freeList.back();
		freeList.pop_back();
		return lastElement;
	}
	return simpleAllocator.allocate();
}

template<typename T>
void PooledAllocator<T>::free(T *elementToFree) {
	freeList.push_back(elementToFree);
}

#endif
