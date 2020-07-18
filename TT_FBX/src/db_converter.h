#pragma once

// FBX API
#include <fbxsdk.h>

// SQLite3
#include <sqlite3.h>

// Eigen
#include <eigen>

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
#include <tt_model.h>


class DBConverter {
	sqlite3* db;
	FbxManager* manager;
	FbxScene* scene;

	TTModel* ttModel;

	void CreateMaterials();

	void Shutdown(int code, const char* errorMessage = NULL);
	void WriteLog(std::string message, bool warning = false);
	int Init(const char* dbFilePath);
	
	void ReadDB();
	void CreateScene();
	void ExportScene();

	void AddPartToScene(TTPart* part, FbxNode* parent);
	void BuildSkeleton(std::vector<TTBone*> bones);
	void AssignChildren(TTBone* root, std::vector<TTBone*> bones);
	void AddBoneToScene(TTBone* bone, FbxPose* bindPose);

	sqlite3_stmt* MakeSqlStatement(std::string query);
	bool GetRow(sqlite3_stmt* statement);

public:
	int ConvertDB(const char* dbFile);
};