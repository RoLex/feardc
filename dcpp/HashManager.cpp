/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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
#include "HashManager.h"

#include <boost/scoped_array.hpp>

#include "File.h"
#include "FileReader.h"
#include "LogManager.h"
#include "ScopedFunctor.h"
#include "SimpleXML.h"
#include "SFVReader.h"
#include "ZUtils.h"

namespace dcpp {

using std::swap;

using boost::none;

/* Version history:
- Version 1: DC++ 0.307 to 0.68.
- Version 2: DC++ 0.670 to DC++ 0.802. Improved efficiency.
- Version 3: from DC++ 0.810 on. Changed the file registry to be case-sensitive. */
#define HASH_FILE_VERSION_STRING "3"
static const uint32_t HASH_FILE_VERSION = 3;
const int64_t HashManager::MIN_BLOCK_SIZE = 64 * 1024;

optional<TTHValue> HashManager::getTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) noexcept {
	Lock l(cs);
	auto tth = store.getTTH(aFileName, aSize, aTimeStamp);
	if(!tth) {
		hasher.hashFile(aFileName, aSize);
	}
	return tth;
}

bool HashManager::getTree(const TTHValue& root, TigerTree& tt) {
	Lock l(cs);
	return store.getTree(root, tt);
}

int64_t HashManager::getBlockSize(const TTHValue& root) {
	Lock l(cs);
	return store.getBlockSize(root);
}

void HashManager::hashDone(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, int64_t speed, int64_t size) {
	try {
		Lock l(cs);
		store.addFile(aFileName, aTimeStamp, tth, true);
	} catch (const Exception& e) {
		LogManager::getInstance()->message(str(F_("Hashing failed: %1%") % e.getError()));
		return;
	}

	fire(HashManagerListener::TTHDone(), aFileName, tth.getRoot());

	if(speed > 0) {
		LogManager::getInstance()->message(str(F_("Finished hashing: %1% (%2% at %3%/s)") % Util::addBrackets(aFileName) %
			Util::formatBytes(size) % Util::formatBytes(speed)));
	} else if(size >= 0) {
		LogManager::getInstance()->message(str(F_("Finished hashing: %1% (%2%)") % Util::addBrackets(aFileName) %
			Util::formatBytes(size)));
	} else {
		LogManager::getInstance()->message(str(F_("Finished hashing: %1%") % Util::addBrackets(aFileName)));
	}
}

void HashManager::HashStore::addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed) {
	addTree(tth);

	auto fname = Util::getFileName(aFileName), fpath = Util::getFilePath(aFileName);

	auto& fileList = fileIndex[fpath];

	auto j = find(fileList.begin(), fileList.end(), fname);
	if (j != fileList.end()) {
		fileList.erase(j);
	}

	fileList.emplace_back(fname, tth.getRoot(), aTimeStamp, aUsed);
	dirty = true;
}

bool HashManager::HashStore::addTree(const TigerTree& tt) noexcept {
	if (treeIndex.find(tt.getRoot()) == treeIndex.end()) {
		try {
			File f(getDataFile(), File::READ | File::WRITE, File::OPEN);
			int64_t index = saveTree(f, tt);
			treeIndex.emplace(tt.getRoot(), TreeInfo(tt.getFileSize(), index, tt.getBlockSize()));
			dirty = true;
		} catch (const FileException& e) {
			LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
			return false;
		}
	}

	return true;
}

int64_t HashManager::HashStore::saveTree(File& f, const TigerTree& tt) {
	if (tt.getLeaves().size() == 1)
		return SMALL_TREE;

	f.setPos(0);
	int64_t pos = 0;
	size_t n = sizeof(pos);
	if (f.read(&pos, n) != sizeof(pos))
		throw FileException(_("Unable to read hash data file"));

	// Check if we should grow the file, we grow by a meg at a time...
	int64_t datsz = f.getSize();
	if ((pos + (int64_t) (tt.getLeaves().size() * TTHValue::BYTES)) >= datsz) {
		f.setPos(datsz + 1024 * 1024);
		f.setEOF();
	}
	f.setPos(pos);dcassert(tt.getLeaves().size()> 1);
	f.write(tt.getLeaves()[0].data, (tt.getLeaves().size() * TTHValue::BYTES));
	int64_t p2 = f.getPos();
	f.setPos(0);
	f.write(&p2, sizeof(p2));
	return pos;
}

bool HashManager::HashStore::loadTree(File& f, const TreeInfo& ti, const TTHValue& root, TigerTree& tt) {
	if (ti.getIndex() == SMALL_TREE) {
		tt = TigerTree(ti.getSize(), ti.getBlockSize(), root);
		return true;
	}
	try {
		f.setPos(ti.getIndex());
		size_t datalen = TigerTree::calcBlocks(ti.getSize(), ti.getBlockSize()) * TTHValue::BYTES;
		boost::scoped_array<uint8_t> buf(new uint8_t[datalen]);
		f.read(&buf[0], datalen);
		tt = TigerTree(ti.getSize(), ti.getBlockSize(), &buf[0]);
		if (!(tt.getRoot() == root))
			return false;
	} catch (const Exception&) {
		return false;
	}

	return true;
}

bool HashManager::HashStore::getTree(const TTHValue& root, TigerTree& tt) {
	auto i = treeIndex.find(root);
	if (i == treeIndex.end())
		return false;
	try {
		File f(getDataFile(), File::READ, File::OPEN);
		return loadTree(f, i->second, root, tt);
	} catch (const Exception&) {
		return false;
	}
}

int64_t HashManager::HashStore::getBlockSize(const TTHValue& root) const {
	auto i = treeIndex.find(root);
	return i == treeIndex.end() ? 0 : i->second.getBlockSize();
}

optional<TTHValue> HashManager::HashStore::getTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) noexcept {
	auto fname = Util::getFileName(aFileName), fpath = Util::getFilePath(aFileName);

	auto i = fileIndex.find(fpath);
	if (i != fileIndex.end()) {
		auto j = find(i->second.begin(), i->second.end(), fname);
		if (j != i->second.end()) {
			FileInfo& fi = *j;
			const auto& root = fi.getRoot();
			auto ti = treeIndex.find(root);
			if(ti != treeIndex.end() && ti->second.getSize() == aSize && fi.getTimeStamp() == aTimeStamp) {
				fi.setUsed(true);
				return root;
			}

			// the file size or the timestamp has changed
			i->second.erase(j);
			dirty = true;
		}
	}
	return none;
}

void HashManager::HashStore::rebuild() {
	try {
		decltype(fileIndex) newFileIndex;
		decltype(treeIndex) newTreeIndex;

		for (auto& i: fileIndex) {
			for (auto& j: i.second) {
				if (!j.getUsed())
					continue;

				auto k = treeIndex.find(j.getRoot());
				if (k != treeIndex.end()) {
					newTreeIndex[j.getRoot()] = k->second;
				}
			}
		}

		auto tmpName = getDataFile() + ".tmp";
		auto origName = getDataFile();

		createDataFile(tmpName);

		{
			File in(origName, File::READ, File::OPEN);
			File out(tmpName, File::READ | File::WRITE, File::OPEN);

			for (auto i = newTreeIndex.begin(); i != newTreeIndex.end();) {
				TigerTree tree;
				if (loadTree(in, i->second, i->first, tree)) {
					i->second.setIndex(saveTree(out, tree));
					++i;
				} else {
					newTreeIndex.erase(i++);
				}
			}
		}

		for (auto& i: fileIndex) {
			decltype(fileIndex)::mapped_type newFileList;

			for (auto& j: i.second) {
				if (newTreeIndex.find(j.getRoot()) != newTreeIndex.end() && j.getUsed()) {
					newFileList.push_back(j);
				}
			}

			if(!newFileList.empty()) {
				newFileIndex[i.first] = move(newFileList);
			}
		}

		File::deleteFile(origName);
		File::renameFile(tmpName, origName);
		treeIndex = newTreeIndex;
		fileIndex = newFileIndex;
		dirty = true;
		save();
	} catch (const Exception& e) {
		LogManager::getInstance()->message(str(F_("Hash data rebuilding failed: %1%") % e.getError()));
	}
}

void HashManager::HashStore::save() {
	if (dirty) {
		try {
			File ff(getIndexFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			BufferedOutputStream<false> f(&ff);

			string tmp;
			string b32tmp;

			f.write(SimpleXML::utf8Header);
			f.write(LIT("<HashStore Version=\"" HASH_FILE_VERSION_STRING "\">\r\n"));

			f.write(LIT("\t<Trees>\r\n"));

			for (auto& i: treeIndex) {
				const TreeInfo& ti = i.second;
				f.write(LIT("\t\t<Hash Type=\"TTH\" Index=\""));
				f.write(Util::toString(ti.getIndex()));
				f.write(LIT("\" BlockSize=\""));
				f.write(Util::toString(ti.getBlockSize()));
				f.write(LIT("\" Size=\""));
				f.write(Util::toString(ti.getSize()));
				f.write(LIT("\" Root=\""));
				b32tmp.clear();
				f.write(i.first.toBase32(b32tmp));
				f.write(LIT("\"/>\r\n"));
			}

			f.write(LIT("\t</Trees>\r\n\t<Files>\r\n"));

			for (auto& i: fileIndex) {
				const string& dir = i.first;
				for (auto& fi: i.second) {
					f.write(LIT("\t\t<File Name=\""));
					f.write(SimpleXML::escape(dir + fi.getFileName(), tmp, true));
					f.write(LIT("\" TimeStamp=\""));
					f.write(Util::toString(fi.getTimeStamp()));
					f.write(LIT("\" Root=\""));
					b32tmp.clear();
					f.write(fi.getRoot().toBase32(b32tmp));
					f.write(LIT("\"/>\r\n"));
				}
			}
			f.write(LIT("\t</Files>\r\n</HashStore>"));
			f.flush();
			ff.close();
			File::deleteFile( getIndexFile());
			File::renameFile(getIndexFile() + ".tmp", getIndexFile());

			dirty = false;
		} catch (const FileException& e) {
			LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
		}
	}
}

string HashManager::HashStore::getIndexFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "HashIndex.xml"; }
string HashManager::HashStore::getDataFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "HashData.dat"; }

class HashLoader: public SimpleXMLReader::CallBack {
public:
	HashLoader(HashManager::HashStore& s, const CountedInputStream<false>& countedStream, uint64_t fileSize, function<void (float)> progressF) :
		store(s),
		countedStream(countedStream),
		streamPos(0),
		fileSize(fileSize),
		progressF(progressF),
		version(HASH_FILE_VERSION),
		inTrees(false),
		inFiles(false),
		inHashStore(false)
	{ }
	void startTag(const string& name, StringPairList& attribs, bool simple);

private:
	HashManager::HashStore& store;

	const CountedInputStream<false>& countedStream;
	uint64_t streamPos;
	uint64_t fileSize;
	function<void (float)> progressF;

	int version;
	string file;

	bool inTrees;
	bool inFiles;
	bool inHashStore;
};

void HashManager::HashStore::load(function<void (float)> progressF) {
	try {
		Util::migrate(getIndexFile());

		File f(getIndexFile(), File::READ, File::OPEN);
		CountedInputStream<false> countedStream(&f);
		HashLoader l(*this, countedStream, f.getSize(), progressF);
		SimpleXMLReader(&l).parse(countedStream);
	} catch (const Exception&) {
		// ...
	}
}

namespace {
/* version 2 files were stored in lower-case; carry the file registration over only if the file can
be found, and if it has no case-insensitive duplicate. */

#ifdef _WIN32

bool upgradeFromV2(string& file) {
	WIN32_FIND_DATA data;
	// FindFirstFile does a case-insensitive search by default
	auto handle = ::FindFirstFile(Text::toT(file).c_str(), &data);
	if(handle == INVALID_HANDLE_VALUE) {
		// file not found
		return false;
	}
	if(::FindNextFile(handle, &data)) {
		// found a dupe
		::FindClose(handle);
		return false;
	}
	::FindClose(handle);

	// don't use dcpp::File as that would be case-sensitive
	handle = ::CreateFile((Text::toT(Util::getFilePath(file)) + data.cFileName).c_str(),
		GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if(handle == INVALID_HANDLE_VALUE) {
		return false;
	}

	// use GetFinalPathNameByHandle to retrieve a properly cased path from the
	// lower-case one that the version 2 file registry has provided us with.
	wstring buf(file.size() * 2, 0);
	buf.resize(::GetFinalPathNameByHandle(handle, &buf[0], buf.size(), 0));

	::CloseHandle(handle);

	if(buf.empty()) {
		return false;
	}
	// GetFinalPathNameByHandle prepends "\\?\"; remove it.
	if(buf.size() >= 4 && buf.substr(0, 4) == L"\\\\?\\") {
		buf.erase(0, 4);
	}

	auto buf8 = Text::fromT(buf);
	if(Text::toLower(buf8) == file) {
		file = move(buf8);
		return true;
	}

	return false;
}

#else

bool upgradeFromV2(string& file) {
	/// @todo implement this on Linux; by default, force re-hashing.
	return false;
}

#endif

}

static const string sHashStore = "HashStore";
static const string sversion = "version"; // Oops, v1 was like this
static const string sVersion = "Version";
static const string sTrees = "Trees";
static const string sFiles = "Files";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sHash = "Hash";
static const string sType = "Type";
static const string sTTH = "TTH";
static const string sIndex = "Index";
static const string sBlockSize = "BlockSize";
static const string sTimeStamp = "TimeStamp";
static const string sRoot = "Root";

void HashLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	ScopedFunctor([this] {
		auto readBytes = countedStream.getReadBytes();
		if(readBytes != streamPos) {
			streamPos = readBytes;
			progressF(static_cast<float>(readBytes) / static_cast<float>(fileSize));
		}
	});

	if (!inHashStore && name == sHashStore) {
		version = Util::toInt(getAttrib(attribs, sVersion, 0));
		if (version == 0) {
			version = Util::toInt(getAttrib(attribs, sversion, 0));
		}
		inHashStore = !simple;
	} else if (inHashStore && (version == 2 || version == 3)) {
		if (inTrees && name == sHash) {
			const string& type = getAttrib(attribs, sType, 0);
			int64_t index = Util::toInt64(getAttrib(attribs, sIndex, 1));
			int64_t blockSize = Util::toInt64(getAttrib(attribs, sBlockSize, 2));
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 3));
			const string& root = getAttrib(attribs, sRoot, 4);
			if (!root.empty() && type == sTTH && (index >= 8 || index == HashManager::SMALL_TREE) && blockSize >= 1024) {
				store.treeIndex[TTHValue(root)] = HashManager::HashStore::TreeInfo(size, index, blockSize);
			}
		} else if (inFiles && name == sFile) {
			file = getAttrib(attribs, sName, 0);
			auto timeStamp = Util::toUInt32(getAttrib(attribs, sTimeStamp, 1));
			const auto& root = getAttrib(attribs, sRoot, 2);
			if(!file.empty() && timeStamp > 0 && !root.empty() && (version != 2 || upgradeFromV2(file))) {
				auto fname = Util::getFileName(file), fpath = Util::getFilePath(file);
				store.fileIndex[fpath].emplace_back(fname, TTHValue(root), timeStamp, false);
			}
		} else if (name == sTrees) {
			inTrees = !simple;
		} else if (name == sFiles) {
			inFiles = !simple;
		}
	}
}

HashManager::HashStore::HashStore() :
	dirty(false) {

	Util::migrate(getDataFile());

	if (File::getSize(getDataFile()) <= static_cast<int64_t> (sizeof(int64_t))) {
		try {
			createDataFile( getDataFile());
		} catch (const FileException&) {
			// ?
		}
	}
}

/**
 * Creates the data files for storing hash values.
 * The data file is very simple in its format. The first 8 bytes
 * are filled with an int64_t (little endian) of the next write position
 * in the file counting from the start (so that file can be grown in chunks).
 * We start with a 1 mb file, and then grow it as needed to avoid fragmentation.
 * To find data inside the file, use the corresponding index file.
 * Since file is never deleted, space will eventually be wasted, so a rebuild
 * should occasionally be done.
 */
void HashManager::HashStore::createDataFile(const string& name) {
	try {
		File dat(name, File::WRITE, File::CREATE | File::TRUNCATE);
		dat.setPos(1024 * 1024);
		dat.setEOF();
		dat.setPos(0);
		int64_t start = sizeof(start);
		dat.write(&start, sizeof(start));

	} catch (const FileException& e) {
		LogManager::getInstance()->message(str(F_("Error creating hash data file: %1%") % e.getError()));
	}
}

void HashManager::Hasher::hashFile(const string& fileName, int64_t size) noexcept {
	Lock l(cs);
	if(w.insert(make_pair(fileName, size)).second && idle) {
		s.signal();
	}
}

bool HashManager::Hasher::pause() noexcept {
	Lock l(cs);
	return paused++;
}

void HashManager::Hasher::resume() noexcept {
	Lock l(cs);
	dcassert(paused > 0);
	if(--paused == 0)
		s.signal();
}

bool HashManager::Hasher::isPaused() const noexcept {
	Lock l(cs);
	return paused > 0;
}

void HashManager::Hasher::stopHashing(const string& baseDir) {
	Lock l(cs);
	for(auto i = w.begin(); i != w.end();) {
		if(strncmp(baseDir.c_str(), i->first.c_str(), baseDir.size()) == 0) {
			w.erase(i++);
		} else {
			++i;
		}
	}
}

void HashManager::Hasher::getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const {
	Lock l(cs);
	curFile = currentFile;
	filesLeft = w.size();
	if (running)
		filesLeft++;
	bytesLeft = 0;
	for (auto& i: w) {
		bytesLeft += i.second;
	}
	bytesLeft += currentSize;
}

void HashManager::Hasher::instantPause() {
	bool wait = false;
	{
		Lock l(cs);
		if(paused > 0 && !stop) {
			wait = true;
		}
	}
	if(wait)
		s.wait();
}

void HashManager::Hasher::scheduleRebuild() {
	rebuild = true; 
	{
		Lock l(cs);
		if(idle) {
			s.signal();
			return;
		}
	}

	LogManager::getInstance()->message(_("Hash database rebuild has been scheduled"));
}

int HashManager::Hasher::run() {
	setThreadPriority(Thread::IDLE);

	string fname;

	for(;;) {
		s.wait();
		if(stop)
			break;
		if(rebuild) {
			HashManager::getInstance()->doRebuild();
			rebuild = false;
			LogManager::getInstance()->message(_("Hash database rebuilt"));
			s.signal();
			continue;
		}
		{
			Lock l(cs);
			if(!w.empty()) {
				currentFile = fname = w.begin()->first;
				currentSize = w.begin()->second;
				w.erase(w.begin());
				idle = false;
			} else {
				fname.clear();
				idle = true;
				continue;
			}
		}

		running = true;
		if(!fname.empty()) {
			try {
				auto start = GET_TICK();

				File f(fname, File::READ, File::OPEN);
				auto size = f.getSize();
				auto timestamp = f.getLastModified();

				auto sizeLeft = size;
				auto bs = max(TigerTree::calcBlockSize(size, 10), MIN_BLOCK_SIZE);

				TigerTree tt(bs);

				CRC32Filter crc32;
				SFVReader sfv(fname);
				CRC32Filter* xcrc32 = 0;
				if(sfv.hasCRC())
					xcrc32 = &crc32;

				auto lastRead = GET_TICK();

				FileReader fr(true);

				fr.read(fname, [&](const void* buf, size_t n) -> bool {
					if(SETTING(MAX_HASH_SPEED)> 0) {
						uint64_t now = GET_TICK();
						uint64_t minTime = n * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
						if(lastRead + minTime> now) {
							Thread::sleep(minTime - (now - lastRead));
						}
						lastRead = lastRead + minTime;
					} else {
						lastRead = GET_TICK();
					}

					tt.update(buf, n);
					if(xcrc32)
						(*xcrc32)(buf, n);

					{
						Lock l(cs);
						currentSize = max(static_cast<uint64_t>(currentSize - n), static_cast<uint64_t>(0));
					}
					sizeLeft -= n;

					instantPause();
					return !stop;
				});

				f.close();
				tt.finalize();
				uint64_t end = GET_TICK();
				int64_t speed = 0;
				if(end > start) {
					speed = size * 1000 / (end - start);
				}

				if(xcrc32 && xcrc32->getValue() != sfv.getCRC()) {
					LogManager::getInstance()->message(str(F_("%1% not shared; calculated CRC32 does not match the one found in SFV file.") % Util::addBrackets(fname)));
				} else if(sizeLeft != 0) {
					LogManager::getInstance()->message(str(F_("%1% not shared; hashing did not complete.") % Util::addBrackets(fname)));
				} else {
					HashManager::getInstance()->hashDone(fname, timestamp, tt, speed, size);
				}
			} catch(const FileException& e) {
				LogManager::getInstance()->message(str(F_("Error hashing %1%: %2%") % Util::addBrackets(fname) % e.getError()));
			}
		}
		{
			Lock l(cs);
			currentFile.clear();
			currentSize = 0;
		}
		running = false;
		s.signal();
	}
	return 0;
}

HashManager::HashPauser::HashPauser() {
	HashManager::getInstance()->pauseHashing();
}

HashManager::HashPauser::~HashPauser() {
	HashManager::getInstance()->resumeHashing();
}

bool HashManager::pauseHashing() noexcept {
	Lock l(cs);
	return hasher.pause();
}

void HashManager::resumeHashing() noexcept {
	Lock l(cs);
	hasher.resume();
}

bool HashManager::isHashingPaused() const noexcept {
	Lock l(cs);
	return hasher.isPaused();
}

} // namespace dcpp
