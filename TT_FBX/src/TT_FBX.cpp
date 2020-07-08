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

const char* initScript = "SQL/CreateDB.SQL";
const char* dbPath = "result.db";


inline bool file_exists(const std::string& name) {

	std::fstream fs;
	fs.open(name, std::fstream::in);
	if (fs.fail()) {
		return false;
	}
	fs.close();
	return true;
}


/**
 * Attempts to initialize the SQLite Database and FBX scene.
 * Returns 0 on success, non-zero on error.
 */
int Init(const int argc,  char** const argv, sqlite3** db, FbxManager** manager, FbxScene** scene) {

	if (argc < 2) {
		fprintf(stderr, "No file path supplied.\n");
		return(101);
	}
	else {
		fprintf(stdout, "Attempting to process FBX: %s\n", argv[1]);
	}
	const char* fbxFilePath = argv[1];

	char *zErrMsg = 0;
	int rc;

	if (file_exists(dbPath)) {
		int failure = remove(dbPath);
		if (failure != 0) {
			fprintf(stderr, "Unable to remove existing database.\n");
			return(102);
		}

	}

	// Create and connect to the database file.
	rc = sqlite3_open(dbPath, db);
	if (rc) {
		fprintf(stderr, "Failed to create database: %s\n", sqlite3_errmsg(*db));
		sqlite3_close(*db);
		return(103);
	}


	// Read the DB Creation file.
	std::ifstream file(initScript);
	std::string str;
	std::string fullSql = "";
	while (std::getline(file, str))
	{
		fullSql += str + "\n";
	}
	file.close();

	// Create the DB Schema.
	rc = sqlite3_exec(*db, fullSql.c_str(), NULL, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Database creation SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(*db);
		return 104;
	}



	// Create the FBX SDK manager
	*manager = FbxManager::Create();

	// Create an IOSettings object.
	FbxIOSettings * ios = FbxIOSettings::Create(*manager, IOSROOT);
	(*manager)->SetIOSettings(ios);

	// Create an importer.
	FbxImporter* importer  = FbxImporter::Create(*manager, "");

	// Declare the path and filename of the fidle containing the scene.
	// In this case, we are assuming the file is in the same directory as the executable.

	// Initialize the importer.
	bool success = importer->Initialize(fbxFilePath, -1, (*manager)->GetIOSettings());

	// Use the first argument as the filename for the importer.
	if (!success) {
		fprintf(stderr, "Unable to load FBX file.");
		sqlite3_close(*db);
		(*manager)->Destroy();
		return 105;
	}

	// Create a new scene so that it can be populated by the imported file.
	(*scene) = FbxScene::Create((*manager), "root");

	// Import the contents of the file into the scene.
	importer->Import((*scene));

	// The file is imported; so get rid of the importer.
	importer->Destroy();

	return 0;
}


/**
 * Shuts down the system gracefully.
 */
void Shutdown(int code, sqlite3* db, FbxManager* manager, const char* errorMessage = NULL) {

	if (errorMessage != NULL) {
		fprintf(stderr, "Critical Error: %s\n", errorMessage);
	}

	// Destroying the manger destroys the scene with it.
	manager->Destroy();

	// Good night DB.
	sqlite3_close(db);

	
	// Time for sleep.
	exit(code);
}



/**
 * Print a node, its attributes, and all its children recursively.
 */
void PrintNode(FbxNode* pNode) {
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	// Print the contents of the node.
	printf("Node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);

	// Print the node's attributes.

	// Recursively print the children.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		PrintNode(pNode->GetChild(j));
}

/**
 * Program entry point, yaaaay.
 */
int main(int argc, char** argv) {

	FbxManager* manager = NULL;
	FbxScene* scene = NULL;
	sqlite3* db = NULL;

	// Try to load all the things.
	int result = Init(argc, argv, &db, &manager, &scene);
	if (result != 0) {
		return result;
	}

	// We're now ready to actually do some work.


	// Print the nodes of the scene and their attributes recursively.
	// Note that we are not printing the root node because it should
	// not contain any attributes.
	FbxNode* root = scene->GetRootNode();
	if (root) {
		for (int i = 0; i < root->GetChildCount(); i++)
			PrintNode(root->GetChild(i));
	}





	// Successs~
	Shutdown(0, db, manager);
}