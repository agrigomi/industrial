#ifndef __STL_MAP_H__
#define __STL_MAP_H__

#include <map>
#include "tAllocator.h"

template <typename key, typename T>
class tSTLMap: public std::map<key, T, std::less<key>, tAllocator<T>> {
public:
	void insert(key _k, T _v) {
		std::map<key, T, std::less<key>, tAllocator<T>>::insert(std::pair<key, T>(_k, _v));
	}
};

#endif
