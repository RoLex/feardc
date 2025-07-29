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

#ifndef DCPLUSPLUS_DCPP_CRYPTO_MANAGER_H
#define DCPLUSPLUS_DCPP_CRYPTO_MANAGER_H

#include "typedefs.h"

#include "SettingsManager.h"

#include "Exception.h"
#include "Singleton.h"

#include "SSL.h"

namespace dcpp {

using std::pair;
using std::string;

STANDARD_EXCEPTION(CryptoException);

class CryptoManager : public Singleton<CryptoManager>
{
public:
	typedef pair<bool, string> SSLVerifyData;

	enum SSLContext {
		SSL_CLIENT,
		SSL_SERVER
	};

	string makeKey(const string& aLock);
	const string& getLock() { return lock; }
	const string& getPk() { return pk; }
	bool isExtended(const string& aLock) { return strncmp(aLock.c_str(), "EXTENDEDPROTOCOL", 16) == 0; }

	void decodeBZ2(const uint8_t* is, size_t sz, string& os);

	string encryptSUDP(const uint8_t* aKey, const string& aCmd);
	bool decryptSUDP(const uint8_t* aKey, const uint8_t* aData, size_t aDataLen, string& result_);

	SSL_CTX* getSSLContext(SSLContext wanted);

	void loadCertificates() noexcept;
	void generateCertificate();
	bool checkCertificate(int minValidityDays = 0) noexcept;
	const ByteVector& getKeyprint() const noexcept;

	bool TLSOk() const noexcept;

	static void locking_function(int mode, int n, const char* /*file*/, int /*line*/);
	static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx);

	static string keyprintToString(const ByteVector& kp) noexcept;

	static int idxVerifyData;

private:

	friend class Singleton<CryptoManager>;

	CryptoManager();
	virtual ~CryptoManager();

	ssl::SSL_CTX clientContext;
	ssl::SSL_CTX serverContext;

	void sslRandCheck();

	bool certsLoaded;

	static CriticalSection* cs;
	static char idxVerifyDataName[];
	static SSLVerifyData trustedKeyprint;

	ByteVector keyprint;
	const string lock;
	const string pk;

	string keySubst(const uint8_t* aKey, size_t len, size_t n);
	bool isExtra(uint8_t b) {
		return (b == 0 || b==5 || b==124 || b==96 || b==126 || b==36);
	}

	static string formatError(X509_STORE_CTX *ctx, const string& message);
	static string getNameEntryByNID(X509_NAME* name, int nid) noexcept;

	void loadKeyprint(const string& file) noexcept;
};

} // namespace dcpp

#endif // !defined(CRYPTO_MANAGER_H)
