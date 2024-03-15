/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#include <dwt/util/RegKey.h>

namespace dwt { namespace util {

RegKey::RegKey(void)
{
	m_hKeyHandle = NULL;
	m_dwDisposition = 0;
	m_pAccess = KEY_ALL_ACCESS;
	memset(&REGKEYINFO,0,sizeof(_REGKEYINFO));
	REGKEYINFO.cchClassName = MAX_PATH;
}

RegKey::RegKey(HKEY hKey, tstring lpSubKey,REGSAM samDesired,bool createIfNotExists)
{
	m_hKeyHandle = NULL;
	m_dwDisposition = 0;
	m_pAccess = KEY_ALL_ACCESS;
	memset(&REGKEYINFO,0,sizeof(_REGKEYINFO));
	REGKEYINFO.cchClassName = MAX_PATH;
	if(!open(hKey,lpSubKey,samDesired,createIfNotExists)) {
		xCeptionSmartUtilities x( "Couldn't open registry key" );
		throw x;
	}
}

RegKey::~RegKey(void)
{
	if(m_hKeyHandle != NULL) close();
}

bool RegKey::open(HKEY hKey, tstring lpSubKey,REGSAM samDesired,bool createIfNotExists)
{
	if(m_hKeyHandle != NULL) close();
	LONG lResult;
	m_pAccess = samDesired;
	if(createIfNotExists)
		lResult = RegCreateKeyEx(hKey,lpSubKey.c_str(),0,NULL,REG_OPTION_NON_VOLATILE, samDesired, NULL, &m_hKeyHandle, &m_dwDisposition);
	else {
		m_dwDisposition = REG_OPENED_EXISTING_KEY;
		lResult = RegOpenKeyEx(hKey, lpSubKey.c_str(), 0, samDesired, &m_hKeyHandle);
	}
	if(lResult != ERROR_SUCCESS) return false;
	lResult = RegQueryInfoKey(
		m_hKeyHandle,						// key handle
		REGKEYINFO.achClass,                // buffer for class name
		&REGKEYINFO.cchClassName,           // size of class string
		NULL,								// reserved
		&REGKEYINFO.cSubKeys,               // number of subkeys
		&REGKEYINFO.cbMaxSubKey,            // longest subkey size
		&REGKEYINFO.cchMaxClass,            // longest class string
		&REGKEYINFO.cValues,                // number of values for this key
		&REGKEYINFO.cchMaxValue,            // longest value name
		&REGKEYINFO.cbMaxValueData,         // longest value data
		&REGKEYINFO.cbSecurityDescriptor,   // security descriptor
		&REGKEYINFO.ftLastWriteTime);       // last write time
	return true;
}

void RegKey::close(void)
{
	if(m_hKeyHandle != NULL) {
		RegCloseKey(m_hKeyHandle);
		m_hKeyHandle = NULL;
		m_dwDisposition = 0;
		m_pAccess = KEY_ALL_ACCESS;
		memset(&REGKEYINFO,0,sizeof(_REGKEYINFO));
		REGKEYINFO.cchClassName = MAX_PATH;
	}
}

bool RegKey::deleteSubkey(tstring lpSubKey, bool recursiv)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x(  "No open reg key!" );
		throw x;
	}
	if(lpSubKey.length() == 0) {
		xCeptionSmartUtilities x(  "No subkey specified!" );
		throw x;
	}
	LONG lResult;
	if(recursiv)
		lResult = SHDeleteKey(m_hKeyHandle,lpSubKey.c_str());
	else
		lResult = SHDeleteEmptyKey(m_hKeyHandle,lpSubKey.c_str());
	return (lResult == ERROR_SUCCESS);
}

bool RegKey::deleteValue(tstring lpValue)
{
	return (ERROR_SUCCESS == RegDeleteValue(m_hKeyHandle,lpValue.c_str()));
}

std::vector<tstring> RegKey::getValues(void)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x(  "No open reg key!" );
		throw x;
	}
	if(!(m_pAccess & KEY_ENUMERATE_SUB_KEYS)) {
		xCeptionSmartUtilities x("You need to have KEY_ENUMERATE_SUB_KEYS access rights!" );
		throw x;
	}

	DWORD cbName;
	LONG lResult;
	std::vector<tstring> v;
	if(REGKEYINFO.cValues) {
		TCHAR* tmp = (TCHAR*)malloc((REGKEYINFO.cchMaxValue + 1) * sizeof(TCHAR));
		for(unsigned int i=0; i<REGKEYINFO.cValues; i++) {
			memset(tmp,0,(REGKEYINFO.cchMaxValue + 1) * sizeof(TCHAR));
			cbName = 16383;
			lResult = RegEnumValue(m_hKeyHandle, i,
                 tmp,
                 &cbName,
                 NULL,
                 NULL,
                 NULL,
                 NULL);
			if(lResult == ERROR_SUCCESS) v.push_back(tmp);
		}
		free(tmp);
	}
	return v;
}

std::vector<tstring> RegKey::getSubkeys(void)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x(  "No open reg key!"  );
		throw x;
	}
	if(!(m_pAccess & KEY_ENUMERATE_SUB_KEYS)) {
		xCeptionSmartUtilities x( "You need to have KEY_ENUMERATE_SUB_KEYS access rights!" );
		throw x;
	}

	DWORD cbName;
	LONG lResult;
	std::vector<tstring> v;
	if(REGKEYINFO.cSubKeys) {
		TCHAR* tmp = (TCHAR*)malloc((REGKEYINFO.cbMaxSubKey + 1) * sizeof(TCHAR));
		FILETIME ftLastWriteTime;
		for(unsigned int i=0; i<REGKEYINFO.cSubKeys; i++) {
			memset(tmp,0,(REGKEYINFO.cbMaxSubKey + 1) * sizeof(TCHAR));
			cbName = 255;
			lResult = RegEnumKeyEx(m_hKeyHandle, i,
                 tmp,
                 &cbName,
                 NULL,
                 NULL,
                 NULL,
                 &ftLastWriteTime);
			if(lResult == ERROR_SUCCESS) v.push_back(tmp);
		}
		free(tmp);
	}
	return v;
}

bool RegKey::writeDword(tstring lpValueName, DWORD dwValueDword)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x(  "No open reg key!"  );
		throw x;
	}
	return (RegSetValueEx(m_hKeyHandle,lpValueName.c_str(),0,REG_DWORD,(LPBYTE)dwValueDword,sizeof(DWORD)) == ERROR_SUCCESS);
}

bool RegKey::writeString(tstring lpValueName, tstring dwValueString)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x( "No open reg key!" );
		throw x;
	}
	return (RegSetValueEx(m_hKeyHandle,lpValueName.c_str(),0,REG_SZ,(LPBYTE)dwValueString.c_str(),(dwValueString.length() + 1) * sizeof(TCHAR)) == ERROR_SUCCESS);
}

bool RegKey::writeBinary(tstring lpValueName, void* pValueBinary, int size)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x(  "No open reg key!" );
		throw x;
	}
	return (RegSetValueEx(m_hKeyHandle,lpValueName.c_str(),0,REG_BINARY,(LPBYTE)pValueBinary,size) == ERROR_SUCCESS);
}

DWORD RegKey::readDword(tstring lpValueName)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x("No open reg key!");
		throw x;
	}
	DWORD dwRegTyp = REG_DWORD;
	DWORD dwLen = sizeof(DWORD);
	LONG lResult = ERROR_SUCCESS;
	lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, 0, &dwLen);
	DWORD v = 0;
	if(lResult == ERROR_SUCCESS) lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, (LPBYTE)&v, &dwLen);
	if(lResult != ERROR_SUCCESS) {
		xCeptionSmartUtilities x( "Error while retreiving value!" );
		throw x;
	}
	return v;
}

tstring RegKey::readString(tstring lpValueName)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x( "No open reg key!" );
		throw x;
	}
	DWORD dwRegTyp = REG_SZ;
	DWORD dwLen = 524289 * sizeof(TCHAR);
	TCHAR tmp[524289];
	memset(tmp,0,dwLen);
	LONG lResult = ERROR_SUCCESS;
	lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, 0, &dwLen);
	if(lResult == ERROR_SUCCESS) lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, (LPBYTE)tmp, &dwLen);
	if(lResult != ERROR_SUCCESS) {
		xCeptionSmartUtilities x( "Error while retreiving value!" );
		throw x;
	}
	tmp[524288] = '\0';
	tstring v = tmp;
	return v;
}

void RegKey::readBinary(tstring lpValueName,void* pValueData, int size)
{
	if(m_hKeyHandle == NULL) {
		xCeptionSmartUtilities x( "No open reg key!" );
		throw x;
	}
	DWORD dwRegTyp = REG_BINARY;
	DWORD dwLen = size;
	LONG lResult = ERROR_SUCCESS;
	lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, 0, &dwLen);
	if(lResult == ERROR_SUCCESS) lResult = RegQueryValueEx(m_hKeyHandle, lpValueName.c_str(), 0, &dwRegTyp, (LPBYTE)pValueData, &dwLen);
	if(lResult != ERROR_SUCCESS) {
		xCeptionSmartUtilities x( "Error while retreiving value!" );
		throw x;
	}
}

} }
