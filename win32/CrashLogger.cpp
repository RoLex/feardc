/*
* Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"
#include "CrashLogger.h"

#include <dcpp/Util.h>
#include <dcpp/version.h>
#include "WinUtil.h"

namespace {

FILE* f;

#if defined(__MINGW32__)

/* All MinGW variants (even x64 SEH ones) store debug information as DWARF. We use libdwarf to
parse it. */

#include <imagehlp.h>

#include <dwarf.h>
#include <libdwarf.h>

/* The following functions are called by libdwarf to get information about the sections in our
file. libdwarf was originally designed for ELF, but it fortunately provides abstract methods one
can use to load sections in a different format, such as PE/COFF in our case.
Section indexes are offset to 1 because the first section (index 0) is reserved in libdwarf.
The "obj" parameter passed to our callbacks is a PLOADED_IMAGE. */

int get_section_info(void* obj, Dwarf_Half section_index, Dwarf_Obj_Access_Section* return_section, int* error) {
	if(section_index == 0) {
		// simulate the ELF empty section at index 0, as recommended in various comments in dwarf_init_finish.c.
		return_section->addr = 0;
		return_section->size = 0;
		static char emptyStr = '\0';
		return_section->name = &emptyStr;
		return_section->link = 0;
		return DW_DLV_OK;
	}

	auto image = reinterpret_cast<PLOADED_IMAGE>(obj);
	auto section = image->Sections + section_index - 1;
	if(!section) {
		return DW_DLV_ERROR;
	};

	return_section->addr = 0; // 0 is ok for non-ELF as per the comments in dwarf_opaque.h.
	return_section->size = section->Misc.VirtualSize;
	return_section->name = reinterpret_cast<const char*>(section->Name);
	if(return_section->name[0] == '/') {
		/* This is a long string (> 8 characters) in the format "/x", where "x" is an offset of the
		actual string in the COFF string table. As documented in the PE/COFF doc, the COFF string
		table is immediately following the COFF symbol table.
		The "18" multiplier is the size of a COFF symbol. */
		auto& header = image->FileHeader->FileHeader;
		return_section->name = reinterpret_cast<const char*>(image->MappedAddress +
			header.PointerToSymbolTable + header.NumberOfSymbols * 18 + atoi(return_section->name + 1));
	}
	return_section->link = 0;
	return DW_DLV_OK;
}

Dwarf_Endianness get_byte_order(void*) {
	return DW_OBJECT_LSB; // always little-endian on Windows
}

Dwarf_Small get_length_size(void*) {
	return 4;
}

Dwarf_Small get_pointer_size(void*) {
	return 4;
}

Dwarf_Unsigned get_section_count(void* obj) {
	return reinterpret_cast<PLOADED_IMAGE>(obj)->NumberOfSections + 1;
}

int load_section(void* obj, Dwarf_Half section_index, Dwarf_Small** return_data, int* error) {
	if(section_index == 0) {
		return DW_DLV_NO_ENTRY;
	}

	auto image = reinterpret_cast<PLOADED_IMAGE>(obj);
	auto section = image->Sections + section_index - 1;
	if(!section) {
		return DW_DLV_ERROR;
	};

	*return_data = image->MappedAddress + section->PointerToRawData;
	return DW_DLV_OK;
}

/* this recursive function browses through the children and siblings of a DIE, looking for the one
DIE that specifically describes the enclosing function of the given address. */
Dwarf_Die browseDIE(Dwarf_Debug dbg, Dwarf_Addr addr, Dwarf_Die die, Dwarf_Error& error) {

	/* only care about DIEs that represent functions (section 3.3 of the DWARF 4 spec). */
	Dwarf_Half tag;
	if(dwarf_tag(die, &tag, &error) == DW_DLV_OK && (tag == DW_TAG_subprogram || tag == DW_TAG_inlined_subroutine)) {

		/* get the low_pc & high_pc attributes of this DIE to see if it contains the address we are
		looking for. can't use dwarf_highpc here because that function only accepts absolute
		high_pc values; however, section 2.17.2 of the DWARF 4 spec mandates that the high_pc value
		may also be a constant relative to low_pc. */
		Dwarf_Addr lowpc, highpc;
		Dwarf_Attribute high_attr;
		if(dwarf_lowpc(die, &lowpc, &error) == DW_DLV_OK && addr >= lowpc &&
			dwarf_attr(die, DW_AT_high_pc, &high_attr, &error) == DW_DLV_OK)
		{

			Dwarf_Half form;
			if(dwarf_whatform(high_attr, &form, &error) == DW_DLV_OK) {
				bool got_high = false;
				if(form == DW_FORM_addr) { // absolute ref, we can use dwarf_highpc.
					got_high = dwarf_highpc(die, &highpc, &error) == DW_DLV_OK;
				} else { // relative ref.
					Dwarf_Unsigned offset;
					if(dwarf_formudata(high_attr, &offset, &error) == DW_DLV_OK) {
						highpc = lowpc + offset;
						got_high = true;
					}
				}

				if(got_high && addr < highpc) {
					dwarf_dealloc(dbg, high_attr, DW_DLA_ATTR);
					return die;
				}
			}

			dwarf_dealloc(dbg, high_attr, DW_DLA_ATTR);
		}
	}

	// flow to the next DIE. start with children then move to siblings.
	Dwarf_Die next;

	if(dwarf_child(die, &next, &error) == DW_DLV_OK) {
		Dwarf_Die ret = browseDIE(dbg, addr, next, error);
		if(ret) {
			if(next != ret) {
				dwarf_dealloc(dbg, next, DW_DLA_DIE);
			}
			return ret;
		}
	}

	if(dwarf_siblingof(dbg, die, &next, &error) == DW_DLV_OK) {
		Dwarf_Die ret = browseDIE(dbg, addr, next, error);
		if(ret) {
			if(next != ret) {
				dwarf_dealloc(dbg, next, DW_DLA_DIE);
			}
			return ret;
		}
	}

	return 0;
}

// utility function that retrieves the name of a DIE.
bool getName(Dwarf_Debug dbg, Dwarf_Die die, string& ret, Dwarf_Error& error) {
	char* name;
	if(dwarf_diename(die, &name, &error) == DW_DLV_OK) {
		ret = name;
		dwarf_dealloc(dbg, name, DW_DLA_STRING);
		return true;
	}
	return false;
}

// utility function that follows a reference-type attribute (one that points to another DIE).
Dwarf_Die followRef(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half attribute, Dwarf_Error& error) {
	Dwarf_Die ret = 0;
	Dwarf_Attribute attr;
	if(dwarf_attr(die, attribute, &attr, &error) == DW_DLV_OK) {

		Dwarf_Half form;
		if(dwarf_whatform(attr, &form, &error) == DW_DLV_OK && form == DW_FORM_ref_sig8) {
			// DWARF 4 pointer to a type DIE in the .debug_types section.
			Dwarf_Sig8 sig;
			if(dwarf_formsig8(attr, &sig, &error) == DW_DLV_OK) {

				/* browse through available type DIEs, looking for the one type DIE whose 8-byte
				signature matches "sig". */
				Dwarf_Sig8 cu_sig;
				Dwarf_Unsigned cu_offset = 0, type_offset, next = 0;
				while(dwarf_next_cu_header_c(dbg, 0, 0, 0, 0, 0, 0, 0, &cu_sig, &type_offset, &next, &error) == DW_DLV_OK) {
					if(!memcmp(&cu_sig, &sig, sizeof(Dwarf_Sig8))) {
						dwarf_offdie_b(dbg, cu_offset + type_offset, 0, &ret, &error);
						break;
					}
					cu_offset = next;
				}
			}

		} else {
			Dwarf_Off offset;
			if(dwarf_global_formref(attr, &offset, &error) == DW_DLV_OK) {
				dwarf_offdie_b(dbg, offset, 1, &ret, &error);
			}
		}

		dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
	}
	return ret;
}

/* retrieve the name of the DIE pointed to by the "type" attribute of a DIE. for a function DIE,
this is the return value of the function (section 3.3.2 of the DWARF 2 spec). for an object DIE,
this is the type of that object (section 4.1.4 of the DWARF 2 spec). */
string getType(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Error& error, bool recursing = false) {
	string ret;

	// follow section 5 of the DWARF 2 spec. multiple type DIEs may be chained together.
	if(recursing && getName(dbg, die, ret, error)) {
		return ret;
	}
	Dwarf_Die type_die = followRef(dbg, die, DW_AT_type, error);
	if(type_die) {
		ret = getType(dbg, type_die, error, true);
		Dwarf_Half tag;
		if(dwarf_tag(die, &tag, &error) == DW_DLV_OK) {
			switch(tag) {
			case DW_TAG_const_type: ret += " const"; break;
			case DW_TAG_pointer_type: ret += '*'; break;
			case DW_TAG_reference_type: ret += '&'; break;
			case DW_TAG_volatile_type: ret += " volatile"; break;
			}
		}
		dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
	}

	if(ret.empty())
		ret = "void/unknown";

	return ret;
}

void getDebugInfo(string path, DWORD addr, string& file, int& line, int& column, string& function) {
	if(path.empty())
		return;
	// replace the extension by "pdb".
	auto dot = path.rfind('.');
	if(dot != string::npos)
		path.replace(dot, path.size() - dot, ".pdb");

	auto image = ImageLoad(&path[0], 0);
	if(!image) {
		fprintf(f, "[Failed to load the debugging data into memory (error: %lu)] ", GetLastError());
		return;
	}

	Dwarf_Debug dbg;
	Dwarf_Error error = 0;

	/* initialize libdwarf using the "custom" interface that allows one to read the DWARF
	information contained in non-ELF files (PE/COFF in our case). */
	Dwarf_Obj_Access_Methods methods = {
		get_section_info,
		get_byte_order,
		get_length_size,
		get_pointer_size,
		get_section_count,
		load_section
	};
	Dwarf_Obj_Access_Interface access = { image, &methods };
	if(dwarf_object_init(&access, 0, 0, &dbg, &error) == DW_DLV_OK) {

		/* use the ".debug_aranges" DWARF section to pinpoint the CU (Compilation Unit) that
		corresponds to the address we want to find information about. */
		Dwarf_Arange* aranges;
		Dwarf_Signed arange_count;
		if(dwarf_get_aranges(dbg, &aranges, &arange_count, &error) == DW_DLV_OK) {

			Dwarf_Arange arange;
			if(dwarf_get_arange(aranges, arange_count, addr, &arange, &error) == DW_DLV_OK) {

				/* great, got a range that matches. let's find the CU it describes, and the DIE
				(Debugging Information Entry) related to that CU. */
				Dwarf_Off die_offset;
				if(dwarf_get_cu_die_offset(arange, &die_offset, &error) == DW_DLV_OK) {

					Dwarf_Die cu_die;
					if(dwarf_offdie_b(dbg, die_offset, 1, &cu_die, &error) == DW_DLV_OK) {

						/* inside this CU, find the exact statement (DWARF calls it a "line") that
						corresponds to the address we want to find information about. */
						Dwarf_Line* lines;
						Dwarf_Signed line_count;
						if(dwarf_srclines(cu_die, &lines, &line_count, &error) == DW_DLV_OK) {

							/* skim through all available statements to find the one that fits best
							(with an address <= "addr", as close as possible to "addr"). */
							Dwarf_Line best = 0;

							int delta = 65;
							for(Dwarf_Signed i = 0; i < line_count; ++i) {
								auto& l = lines[i];
								Dwarf_Addr lineaddr;
								if(dwarf_lineaddr(l, &lineaddr, &error) == DW_DLV_OK) {
									int d = addr - lineaddr;
									if(d >= 0 && d < delta) {
										best = l;
										if(d == 0) // found a perfect match.
											break;
										delta = d;
									}
								}
							}

							if(best) {
								// get the source file behind this statement.
								char* linesrc;
								if(dwarf_linesrc(best, &linesrc, &error) == DW_DLV_OK) {
									file = linesrc;
									dwarf_dealloc(dbg, linesrc, DW_DLA_STRING);

									// get the line number inside that source file.
									Dwarf_Unsigned lineno;
									if(dwarf_lineno(best, &lineno, &error) == DW_DLV_OK) {
										line = lineno;

										// get the column number as well if available.
										Dwarf_Signed lineoff;
										if(dwarf_lineoff(best, &lineoff, &error) == DW_DLV_OK) {
											column = lineoff;
										}
									}
								}
							}

							dwarf_srclines_dealloc(dbg, lines, line_count);
						}

						if(file.empty()) {
							/* could not get a precise statement within this CU; resort to showing
							the global name of this CU's DIE which, according to section 3.1 of the
							DWARF 2 spec, is almost what we want. */
							getName(dbg, cu_die, file, error);
						}

						/* to find the enclosing function, skim through the children and siblings
						of this CU's DIE to find the one DIE that contains the frame address. */
						Dwarf_Die die = browseDIE(dbg, addr, cu_die, error);
						if(die) {

							/* check if the DIE has a "specification" attribute which, according to
							section 5.5.5 of the DWARF 2 spec, is a pointer to the DIE that
							actually describes the function. */
							Dwarf_Die spec_die = followRef(dbg, die, DW_AT_specification, error);

							/* as specified in section 3.3.1 of the DWARF 2 spec, the name of this
							function-representing DIE should be what we are looking for. */
							getName(dbg, spec_die ? spec_die : die, function, error);

							if(!function.empty()) {
								/* according to section 3.3.2 of the DWARF 2 spec, the type of this
								function-representing DIE is the return value of the function. */
								function = getType(dbg, spec_die ? spec_die : die, error) + ' ' + function + '(';

								/* now get the parameters of the function. each parameter is
								represented by a DIE that is a child of the function-representing
								DIE (section 3.3.4 of the DWARF 2 spec). */
								Dwarf_Die child;
								if(dwarf_child(die, &child, &error) == DW_DLV_OK) {
									Dwarf_Die next = 0;
									string name;
									while(true) {
										if(getName(dbg, child, name, error)) {
											if(next) // not the first iteration
												function += ", ";
											function += getType(dbg, child, error) + ' ' + name;
										}
										bool ok = dwarf_siblingof(dbg, child, &next, &error) == DW_DLV_OK;
										dwarf_dealloc(dbg, child, DW_DLA_DIE);
										if(ok) {
											child = next;
										} else {
											break;
										}
									}
								}

								function += ')';
							}

							if(spec_die) {
								dwarf_dealloc(dbg, spec_die, DW_DLA_DIE);
							}
							dwarf_dealloc(dbg, die, DW_DLA_DIE);
						}

						dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
					}

				}

			}

			for(Dwarf_Signed i = 0; i < arange_count; ++i) {
				dwarf_dealloc(dbg, aranges[i], DW_DLA_ARANGE);
			}
			dwarf_dealloc(dbg, aranges, DW_DLA_LIST);
		}

		dwarf_object_finish(dbg, &error);
	}

	ImageUnload(image);

	if(error) {
		fprintf(f, "[libdwarf error: %s] ", dwarf_errmsg(error));
	}
}

/* although the 64-bit functions should just map to the 32-bit ones on a 32-bit OS, this seems to
fail on XP. */
#ifndef _WIN64
#define DWORD64 DWORD
#define IMAGEHLP_LINE64 IMAGEHLP_LINE
#define IMAGEHLP_MODULE64 IMAGEHLP_MODULE
#define IMAGEHLP_SYMBOL64 IMAGEHLP_SYMBOL
#define STACKFRAME64 STACKFRAME
#define StackWalk64 StackWalk
#define SymFunctionTableAccess64 SymFunctionTableAccess
#define SymGetLineFromAddr64 SymGetLineFromAddr
#define SymGetModuleBase64 SymGetModuleBase
#define SymGetModuleInfo64 SymGetModuleInfo
#define SymGetSymFromAddr64 SymGetSymFromAddr
#endif

#elif defined(_MSC_VER)

#include <dbghelp.h>

// MSVC uses SEH. Nothing special to add besides that include file.

#else

#define NO_BACKTRACE

#endif

inline void writeAppInfo() {
	fputs(Util::formatTime(APPNAME " has crashed on %Y-%m-%d at %H:%M:%S.\n", time(0)).c_str(), f);
	fputs("Please report this data to the " APPNAME " team for further investigation.\n\n", f);

	fprintf(f, APPNAME " version: %s\n", fullVersionString.c_str());
	fprintf(f, "TTH: %S\n", WinUtil::tth.c_str());

	// see also AboutDlg.cpp for similar tests.
#ifdef __MINGW32__
#ifdef HAVE_OLD_MINGW
	fputs("Compiled with MinGW's GCC " __VERSION__, f);
#else
	fputs("Compiled with MinGW-w64's GCC " __VERSION__, f);
#endif
#elif defined(_MSC_VER)
	fprintf(f, "Compiled with MS Visual Studio %d", _MSC_VER);
#else
	fputs(f, "Compiled with an unknown compiler");
#endif
#ifdef _DEBUG
	fputs(" (debug)", f);
#endif
#ifdef _WIN64
	fputs(" (x64)", f);
#endif
	fputs("\n", f);
}

inline void writePlatformInfo() {
	OSVERSIONINFOEX ver = { sizeof(OSVERSIONINFOEX) };
	if(!::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver)))
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if(::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver))) {
		fprintf(f, "Windows version: major = %lu, minor = %lu, build = %lu, SP = %u, type = %u\n",
			ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber, ver.wServicePackMajor, ver.wProductType);

	} else {
		fputs("Windows version: unknown\n", f);
	}

	SYSTEM_INFO info;
	::GetNativeSystemInfo(&info);
	fprintf(f, "Processors: %lu * %s\n", info.dwNumberOfProcessors,
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" :
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) ? "x86" :
		(info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ? "ia64" :
		"[unknown architecture]");

	MEMORYSTATUSEX memoryStatusEx = { sizeof(MEMORYSTATUSEX) };
	::GlobalMemoryStatusEx(&memoryStatusEx);
	fprintf(f, "System memory installed: %s", Util::formatBytes(memoryStatusEx.ullTotalPhys).c_str());
}

#ifndef NO_BACKTRACE

inline void writeBacktrace(LPCONTEXT context) {
	HANDLE const process = GetCurrentProcess();
	HANDLE const thread = GetCurrentThread();

#if defined(__MINGW32__)
	SymSetOptions(SYMOPT_DEFERRED_LOADS);
#else
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
#endif

	if(!SymInitialize(process, 0, TRUE)) {
		fprintf(f, "Failed to initialize the symbol handler (error: %lu)\n", GetLastError());
		return;
	}

	STACKFRAME64 frame;
	memset(&frame, 0, sizeof(frame));

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

#ifdef _WIN64
	frame.AddrPC.Offset = context->Rip;
	frame.AddrFrame.Offset = context->Rbp;
	frame.AddrStack.Offset = context->Rsp;

#define WALK_ARCH IMAGE_FILE_MACHINE_AMD64

#else
	frame.AddrPC.Offset = context->Eip;
	frame.AddrFrame.Offset = context->Ebp;
	frame.AddrStack.Offset = context->Esp;

#define WALK_ARCH IMAGE_FILE_MACHINE_I386

#endif

	char symbolBuf[sizeof(IMAGEHLP_SYMBOL64) + 255];

	for(uint8_t step = 0; step < 128; ++step) { // 128 steps max to avoid too long traces

		/* in case something unexpected happens when reading the next address, we want to at least
		record the information that has been gathered so far. */
		fflush(f);

		if(!StackWalk64(WALK_ARCH, process, thread, &frame, context,
			0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) { break; }

		string file;
		int line = -1;
		int column = -1;
		string function;

		IMAGEHLP_MODULE64 module = { sizeof(IMAGEHLP_MODULE64) };
		if(!SymGetModuleInfo64(process, frame.AddrPC.Offset, &module))
			continue;
		fprintf(f, "%s: ", module.ModuleName);

#if defined(__MINGW32__)
		// read DWARF debugging info if available.
		if(module.LoadedImageName[0] ||
			// LoadedImageName is not always correctly filled in XP...
			::GetModuleFileNameA(reinterpret_cast<HMODULE>(module.BaseOfImage), module.LoadedImageName, sizeof(module.LoadedImageName)))
		{
			getDebugInfo(module.LoadedImageName, frame.AddrPC.Offset, file, line, column, function);
		}
#endif

		/* this is the usual Windows PDB reading method. we try it on MinGW too if reading DWARF
		data has failed, just in case Windows can extract some information. */
		if(file.empty()) {
			IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(symbolBuf);
			symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
			symbol->MaxNameLength = 254;
			if(SymGetSymFromAddr64(process, frame.AddrPC.Offset, 0, symbol)) {
				file = symbol->Name;
			}

			IMAGEHLP_LINE64 info = { sizeof(IMAGEHLP_LINE64) };
			DWORD col;
			if(SymGetLineFromAddr64(process, frame.AddrPC.Offset, &col, &info)) {
				function = file;
				file = info.FileName;
				line = info.LineNumber;
				column = col;
			}
		}

		// write the data collected about this frame to the file.
		fprintf(f, "%s", file.empty() ? "?" : file.c_str());
		if(line >= 0) {
			fprintf(f, " (%d", line);
			if(column >= 0) {
				fprintf(f, ":%d", column);
			}
			fputs(")", f);
		}
		if(!function.empty()) {
			fprintf(f, ", function: %s", function.c_str());
		}
		fputs("\n", f);
	}

	SymCleanup(process);
}

#endif // NO_BACKTRACE

LONG WINAPI exceptionFilter(LPEXCEPTION_POINTERS info) {
	if(f) {
		// only log the first exception.
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	f = _wfopen(CrashLogger::getPath().c_str(), L"w");
	if(f) {
		writeAppInfo();

		fprintf(f, "Exception code: %lx\n", info->ExceptionRecord->ExceptionCode);

		writePlatformInfo();

#ifdef NO_BACKTRACE
		fputs("\nStack trace unavailable: this program hasn't been compiled with backtrace support\n", f);
#else
		fputs("\nWriting the stack trace...\n\n", f);
		writeBacktrace(info->ContextRecord);
#endif

		fputs("\nInformation about the crash has been written.\n", f);
		fclose(f);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

LPTOP_LEVEL_EXCEPTION_FILTER prevFilter;

} // unnamed namespace

CrashLogger::CrashLogger() {
	prevFilter = SetUnhandledExceptionFilter(exceptionFilter);
}

CrashLogger::~CrashLogger() {
	SetUnhandledExceptionFilter(prevFilter);
}

tstring CrashLogger::getPath() {
	return Text::toT(Util::getPath(Util::PATH_USER_LOCAL)) + _T("CrashLog.txt");
}
