// Application to handle translating FBX files into a format that TexTools 
// can easily read and work with.




// FBX API
#include <fbxsdk.h>

// SQLite3
#include <external/sqlite3.h>

// Core
#include <iostream>
#include <string>
#include <fstream>
#include <memory>

const char* initScript = "SQL/CreateDB.SQL";
const char* dbPath = "result.db";

/**
 * Main function - loads the hard-coded fbx file,
 * and prints its contents in an xml format to stdout.
 */
int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "No file path supplied.\n");
		return(1);
	}
	else {
		fprintf(stdout, "Attempting to process FBX: %s\n", argv[1]);
	}
	const char* fbxFilePath = argv[1];

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

	std::unique_ptr<char> v1 = std::make_unique<char>();


    rc = sqlite3_open(dbPath, &db);
    if( rc ){
      fprintf(stderr, "Failed to create database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return(13);
    }

	
	
	/*
    rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
     sqlite3_free(zErrMsg);
    }*/
    sqlite3_close(db);
    //return 0;



	// Create the FBX SDK manager
	FbxManager* lSdkManager = FbxManager::Create();

	// Create an IOSettings object.
	FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// ... Configure the FbxIOSettings object ...
	/*(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_MATERIAL, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_TEXTURE, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_LINK, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_SHAPE, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_GOBO, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_ANIMATION, false);
	(*(lSdkManager->GetIOSettings())).SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);*/

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");

	// Declare the path and filename of the fidle containing the scene.
	// In this case, we are assuming the file is in the same directory as the executable.

	// Initialize the importer.
	bool lImportStatus = lImporter->Initialize(fbxFilePath, -1, lSdkManager->GetIOSettings());

	// Use the first argument as the filename for the importer.
	if (!lImportStatus) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported; so get rid of the importer.
	lImporter->Destroy();

	// Destroy the SDK manager and all the other objects it was handling.
	lSdkManager->Destroy();

	char z;
	//std::cin >> z;
	return 0;
}