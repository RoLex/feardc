/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DWT_RegKey_H
#define DWT_RegKey_H

#include "../WindowsHeaders.h"
#include "../tstring.h"
#include "xCeptionSmartUtilities.h"
#include <vector>

/// Contains utility class for reading and editing the registry
namespace dwt { namespace util {

class RegKey
{
private:
	HKEY m_hKeyHandle;
	DWORD m_dwDisposition;
	REGSAM m_pAccess;

	struct _REGKEYINFO {
		TCHAR achClass[MAX_PATH];				// buffer for class name
		DWORD cchClassName;						// size of class string
		DWORD cSubKeys;							// number of subkeys
		DWORD cbMaxSubKey;						// longest subkey size
		DWORD cchMaxClass;						// longest class string
		DWORD cValues;							// number of values for key
		DWORD cchMaxValue;						// longest value name
		DWORD cbMaxValueData;					// longest value data
		DWORD cbSecurityDescriptor;				// size of security descriptor
		FILETIME ftLastWriteTime;				// last write time
	} REGKEYINFO;
public:
	/// Normal constructor
	/** Constructs a new instanc of this class
	  */
	RegKey(void);

	/// Constructor, with parameters to open/create a reg key
	/** Constructor taking a hKey and tstring to specify the key to open. <br>
	  * hKey could be HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE or HKEY_USERS
	  */
	RegKey(HKEY hKey, tstring lpSubKey,REGSAM samDesired = KEY_ALL_ACCESS,bool createIfNotExists = false);

	/// Deconstructor
	/** Deconstructor, close keys if they are still open
	  */
	~RegKey(void);

	/// Open or create a reg key
	/** Open a reg key or create before <br>
	  * hKey could be HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE or HKEY_USERS
	  */
	bool open(HKEY hKey, tstring lpSubKey,REGSAM samDesired = KEY_ALL_ACCESS,bool createIfNotExists = false);

	/// Close open key
	/** Close open key
	  */
	void close(void);

	/// Check for an open key
	/** Check for an open key
	  */
	bool isOpen(void) { return (m_hKeyHandle != NULL); }

	/// Check for a new key
	/** Call this if you are sure that you have an open key!
	  * It returns true if the open key was created by open or false if that key alredy exists.
	  */
	bool isNewKey(void) { return (m_dwDisposition == REG_CREATED_NEW_KEY); }

	/// Delete the specific subkey of the current open key
	/** Delete the specific subkey of the current open key. The key must be empty or set recursiv=true to delete all content, too.
	  */
	bool deleteSubkey(tstring lpSubKey, bool recursiv = false);

	/// Delete the specific value of the current open key
	/** Delete the specific value of the current open key
	  */
	bool deleteValue(tstring lpValue);

	/// Get value names
	/** Get value names
	  */
	std::vector<tstring> getValues(void);

	/// Get subkeys
	/** Get subkeys
	  */
	std::vector<tstring> getSubkeys(void);

	/// Set DWORD
	/** Writes a DWORD value
	  */
	bool writeDword(tstring lpValueName, DWORD dwValueDword);

	/// Set string
	/** Writes a string value
	  */
	bool writeString(tstring lpValueName, tstring dwValueString);

	/// Set binary
	/** Writes a binary value
	  */
	bool writeBinary(tstring lpValueName, void* pValueBinary, int length);

	/// Read DWORD
	/** Read a DWORD value
	  */
	DWORD readDword(tstring lpValueName);

	/// Read string
	/** Read a string value
	  */
	tstring readString(tstring lpValueName);

	/// Read binary
	/** Read a binary value
	  */
	void readBinary(tstring lpValueName,void* pValueData, int size);
};

} }

#endif
