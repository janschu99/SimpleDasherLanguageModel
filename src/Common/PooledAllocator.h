#ifndef POOLED_ALLOCATOR_INCLUDED
#define POOLED_ALLOCATOR_INCLUDED

#include <vector>

//PooledAllocator allocates objects T in fixed-size blocks (specified in the constructor);
//allocate returns an uninitialized T*;
//free returns an object to the pool
template<typename T>
class PooledAllocator {
	public:
		//Construct with given block size
		PooledAllocator(size_t blockSize);
		~PooledAllocator();
		//Return an uninitialized object
		T* allocate();
		//Return an object to the pool
		void free(T *elementToFree);
	private:
		class Pool {
			public:
				Pool(size_t poolSize) :
						currentPos(0), poolSize(poolSize) {
					data = new T[poolSize];
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
				T *data;
		};
		std::vector<Pool*> pool;
		size_t blockSize;
		int currentPos;
		//The free list
		std::vector<T*> freeList;
};

template<typename T>
PooledAllocator<T>::PooledAllocator(size_t blockSize) :
		blockSize(blockSize), currentPos(0) {
	pool.push_back(new Pool(blockSize));
}

template<typename T>
PooledAllocator<T>::~PooledAllocator() {
	for (size_t i = 0; i<pool.size(); i++)
		delete pool[i];
}

template<typename T>
T* PooledAllocator<T>::allocate() {
	if (freeList.size()>0) {
		T *lastElement = freeList.back();
		freeList.pop_back();
		return lastElement;
	}
	T *p = pool[currentPos]->allocate();
	if (p) return p;
	pool.push_back(new Pool(blockSize));
	currentPos++;
	return pool.back()->allocate();
}

template<typename T>
void PooledAllocator<T>::free(T *elementToFree) {
	freeList.push_back(elementToFree);
}

#endif
