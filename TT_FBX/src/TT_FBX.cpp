// Application to handle translating FBX files into a format that TexTools 
// can easily read and work with.




// FBX API
#include <fbxsdk.h>

// SQLite3
#include <sqlite3.h>

// Core
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <memory>
#include <exception>
#include <vector>
#include <map>
#include <regex>

// Custom
#include <tt_vertex.h>
#include <fbx_importer.h>
#include <db_converter.h>

//using namespace FbxSdk;

const std::regex dbRegex(".*\\.db$");


int ConvertDB(const char* dbPath) {
	return 0;
}

/**
 * Program entry point, yaaaay.
 */
int main(int argc, char** argv) {

	if (argc < 2) {
		fprintf(stderr, "No file path supplied.\n");
		return(101);
	}


	std::cmatch m;
	bool success = std::regex_match(argv[1], m, dbRegex);
	if (!success) {
		FBXImporter* fbxImporter = new FBXImporter();
		return fbxImporter->ImportFBX(argv[1]);
	}
	else {
		DBConverter* dbConverter = new DBConverter();
		return dbConverter->ConvertDB(argv[1]);
	}
}