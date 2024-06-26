/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "Thread.h"

#include "format.h"

namespace dcpp {

#ifdef _WIN32
void Thread::start() {
	join();
	if( (threadHandle = CreateThread(NULL, 0, &starter, this, 0, &threadId)) == NULL) {
		throw ThreadException(_("Unable to create thread"));
	}
}

#else
void Thread::start() {
	join();
	if(pthread_create(&threadHandle, NULL, &starter, this) != 0) {
		throw ThreadException(_("Unable to create thread"));
	}
}
#endif

} // namespace dcpp
