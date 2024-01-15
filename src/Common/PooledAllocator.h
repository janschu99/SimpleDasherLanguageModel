#ifndef POOLED_ALLOCATOR_INCLUDED
#define POOLED_ALLOCATOR_INCLUDED

#include <vector>

//PooledAllocator allocates objects T in fixed-size blocks (specified in the constructor)
template<typename T>
class PooledAllocator {
	public:
		PooledAllocator(size_t blockSize);
		~PooledAllocator();
		T* allocate(); //Returns a default-constructed T*
		void free(T* elementToFree); //Returns an object to the pool
	private:
		class Pool;
		std::vector<Pool*> pools; //list of pools (=blocks)
		size_t blockSize;
		int currentPos;
		std::vector<T*> freeList; //list of freed entries (in all pools combined)
		class Pool {
			public:
				Pool(size_t poolSize) : currentPos(0), poolSize(poolSize) {
					data=new T[poolSize];
				}
				~Pool() {
					delete[] data;
				}
				T* allocate() const {
					if (currentPos<poolSize) return &data[currentPos++];
					return NULL;
				}
			private:
				mutable size_t currentPos;
				size_t poolSize;
				T* data;
		};
};

template<typename T>
PooledAllocator<T>::PooledAllocator(size_t blockSize) :
		blockSize(blockSize), currentPos(0) {
	pools.push_back(new Pool(blockSize)); //create first pool
}

template<typename T>
PooledAllocator<T>::~PooledAllocator() {
	for (size_t i = 0; i<pools.size(); i++) //delete all pools
		delete pools[i];
}

template<typename T>
T* PooledAllocator<T>::allocate() {
	if (freeList.size()>0) { //try to reuse freed entries (doesn't matter in which pool)
		T* lastElement = freeList.back();
		freeList.pop_back();
		return lastElement;
	}
	//no more free entries, allocate a new one in the current pool
	T* p = pools[currentPos]->allocate();
	if (p!=NULL) return p; //allocation successful, return
	//allocation failed since current pool is full, create new pool and allocate there
	pools.push_back(new Pool(blockSize));
	currentPos++;
	return pools.back()->allocate();
}

template<typename T>
void PooledAllocator<T>::free(T* elementToFree) {
	freeList.push_back(elementToFree); //add entry to freeList, to be reused later
}

#endif
