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
#include <tt_vertex.h>

class TTMeshPart;
class TTMeshGroup;
class TTModel;

class TTBone {
public:
	std::string Name;
	int Id;
	int ParentId;
	TTBone* Parent;
	std::vector<TTBone*> Children;
	Eigen::Transform<double, 3, Eigen::Affine> PoseMatrix;
	FbxNode* Node;
};

class TTPart {
public:
	std::string Name;
	int PartId;
	std::vector<TTVertex> Vertices;
	std::vector<int> Indices;
	FbxNode* Node;
	TTMeshGroup* MeshGroup;
};

class TTMeshGroup {
public:
	std::vector<TTPart*> Parts;
	std::vector<std::string> Bones;
	int MeshId;
	FbxNode* Node;
	TTModel* Model;
};

class TTModel {
public:
	std::vector<TTMeshGroup*> MeshGroups;
	TTBone* FullSkeleton;
	FbxNode* Node;

	TTBone* GetBone(std::string name, TTBone* parent = NULL) {
		if (parent == NULL) {
			parent = FullSkeleton;
		}

		if (parent->Name == name) {
			return parent;
		}

		for (int i = 0; i < parent->Children.size(); i++) {
			TTBone* r = GetBone(name, parent->Children[i]);
			if (r != NULL) {
				return r;
			}
		}

		return NULL;

	}
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
	void AddBoneToScene(TTBone* bone, FbxPose* bindPose);

	sqlite3_stmt* MakeSqlStatement(std::string query);
	bool GetRow(sqlite3_stmt* statement);

public:
	int ConvertDB(const char* dbFile);
};