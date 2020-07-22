// Application for converting FBX files to/from Textools' DB format.

// Core
#include <iostream>
#include <string>
#include "tchar.h"
#include <regex>

// Blegh.  Need to replace this later with a better UTF8 converter.
#include <windows.h>

// Custom
#include <fbx_importer.h>
#include <db_converter.h>

//using namespace FbxSdk;

const std::wregex dbRegex(L".*\\.db$");


/**
 * Program entry point, yaaaay.
 */
int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	if (argc < 2) {
		fprintf(stderr, "No file path supplied.\n");
		return(101);
	}

	wchar_t* base = argv[1];

	std::wcmatch m;
	std::wstring arg = argv[1];
	bool success = std::regex_match(arg.c_str(), m, dbRegex);
	if (!success) {
		FBXImporter* fbxImporter = new FBXImporter();
		return fbxImporter->ImportFBX(arg);
	}
	else {
		DBConverter* dbConverter = new DBConverter();
		return dbConverter->ConvertDB(arg);
	}
}