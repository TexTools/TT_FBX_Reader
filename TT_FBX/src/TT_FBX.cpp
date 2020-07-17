// Application for converting FBX files to/from Textools' DB format.

// Core
#include <iostream>
#include <string>
#include <regex>

// Custom
#include <fbx_importer.h>
#include <db_converter.h>

//using namespace FbxSdk;

const std::regex dbRegex(".*\\.db$");

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