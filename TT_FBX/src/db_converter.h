#pragma once

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

class TTMeshPart;
class TTMeshGroup;
class TTModel;

class TTPart {
public:
	std::string Name;
	std::vector<TTVertex> Vertices;
	std::vector<int> Indices;
	FbxNode* Node;
	TTMeshGroup* MeshGroup;
};

class TTMeshGroup {
public:
	std::vector<TTPart*> Parts;
	std::vector<std::string> Bones;
	FbxNode* Node;
	TTModel* Model;
};

class TTBone {
public:
	std::string Name;
	int Id;
	int ParentId;
	TTBone* Parent;
	std::vector<TTBone*> Children;
	FbxMatrix PoseMatrix;
	FbxMatrix InverseMatrix;
	FbxNode* Node;
};

class TTModel {
public:
	std::vector<TTMeshGroup*> MeshGroups;
	TTBone* FullSkeleton;
	FbxNode* Node;
};

class DBConverter {
	sqlite3* db;
	FbxManager* manager;
	FbxScene* scene;

	TTModel* ttModel;

	void Shutdown(int code, const char* errorMessage = NULL);
	void WriteLog(std::string message, bool warning = false);
	int Init(const char* dbFilePath);
	
	void ReadDB();
	void CreateScene();
	void ExportScene();

	void AddPartToScene(TTPart* part, FbxNode* parent);
	void BuildSkeleton(std::vector<TTBone*> bones);
	void AssignChildren(TTBone* root, std::vector<TTBone*> bones);
	void AddBoneToScene(TTBone* bone);

	sqlite3_stmt* MakeSqlStatement(std::string query);
	bool GetRow(sqlite3_stmt* statement);

public:
	int ConvertDB(const char* dbFile);
};