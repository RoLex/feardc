/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DCPLUSPLUS_DCPP_SINGLETON_H
#define DCPLUSPLUS_DCPP_SINGLETON_H

#include <boost/core/noncopyable.hpp>

#include "debug.h"

namespace dcpp {

template<typename T>
class Singleton : boost::noncopyable {
public:
	Singleton() { }
	virtual ~Singleton() { }

	static T* getInstance() {
		dcassert(instance);
		return instance;
	}

	static void newInstance() {
		if(instance)
			delete instance;

		instance = new T();
	}

	static void deleteInstance() {
		if(instance)
			delete instance;
		instance = NULL;
	}
protected:
	static T* instance;
};

template<class T> T* Singleton<T>::instance = NULL;

} // namespace dcpp

#endif // DCPLUSPLUS_DCPP_SINGLETON_H
