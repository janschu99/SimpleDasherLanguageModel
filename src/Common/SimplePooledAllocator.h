#ifndef __SimplePooledAlloc_h__
#define __SimplePooledAlloc_h__

#include <vector>

//SimplePooledAllocator allocates objects T in fixed-size blocks (specified) 
//allocate returns a default-constructed T*
//Memory is only freed on destruction of the allocator
template<typename T>
class SimplePooledAllocator {
	public:
		//Construct with given block size
		SimplePooledAllocator(size_t blockSize);
		~SimplePooledAllocator();
		//Return an uninitialized object
		T* allocate();
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
};

template<typename T>
SimplePooledAllocator<T>::SimplePooledAllocator(size_t blockSize) :
		blockSize(blockSize), currentPos(0) {
	pool.push_back(new Pool(blockSize));
}

template<typename T>
SimplePooledAllocator<T>::~SimplePooledAllocator() {
	for (size_t i = 0; i<pool.size(); i++)
		delete pool[i];
}

template<typename T>
T* SimplePooledAllocator<T>::allocate() {
	T *p = pool[currentPos]->allocate();
	if (p) return p;
	pool.push_back(new Pool(blockSize));
	currentPos++;
	return pool.back()->allocate();
}

#endif
