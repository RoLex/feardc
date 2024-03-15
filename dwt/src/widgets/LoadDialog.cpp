/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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

#include <dwt/widgets/LoadDialog.h>

namespace dwt {

bool LoadDialog::openImpl(OPENFILENAME& ofn) {
	ofn.Flags |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	return ::GetOpenFileName(&ofn);
}

bool LoadDialog::openMultiple(std::vector<tstring>& files, unsigned flags)
{
	// get the current directory and restore it later to avoid directory locks
	TCHAR buf[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, buf);

	OPENFILENAME ofn;
	getOFN(ofn);
	ofn.lpstrFile = files.empty() ? 0 : const_cast<LPTSTR>(files[0].c_str());
	ofn.Flags = flags | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT;

	bool ret = false;
	if( ::GetOpenFileName(&ofn) )
	{
		// If a single file is selected, the lpstrFile string is just the path terminated by TWO null bytes
		// If multiple files are selected, the format of the string returned is:
		// DIRECTORY_PATH + '\0' + FILE_NAME_1 + '\0' + FILE_NAME_2 + '\0' + ... + FILE_NAME_N + '\0' + '\0'
		// (Note the last file name is terminated by two null bytes)

		tstring fileName;
		tstring filePath;
		tstring directory;
		directory = ofn.lpstrFile; // tstring ends at first null
		TCHAR *array_p = ofn.lpstrFile + directory.length() + 1; // set pointer to one position past null
		fileName = array_p; // fileName is substring from array_p to next null
		if (fileName.length() == 0) // only one file was selected
			files.push_back(directory); // string 'directory' contains full path
		else
		{
			while (fileName.length() > 0)
			{
				filePath = directory + _T("\\") + fileName;
				files.push_back(filePath);
				array_p = array_p + fileName.length() + 1; // set pointer one position past null
				fileName = array_p; // fileName is substring from array_p to next null
			}
		}
		ret = true;
	}

	::SetCurrentDirectory(buf);
	return ret;
}

}
