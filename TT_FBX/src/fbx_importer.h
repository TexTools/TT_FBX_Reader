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

class FBXImporter {
	sqlite3* db;
	FbxManager* manager;
	FbxScene* scene;

	std::vector<std::vector<std::string>> boneNames;
	std::map<int, std::map<int, std::string>> meshParts;


	inline bool file_exists(const std::string& name);
	int Init(const char* fbxFilePath, sqlite3** database, FbxManager** manager, FbxScene** scene);
	void Shutdown(int code, const char* errorMessage = NULL);
	int GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxVector4>* layerElement, int index_id);
	int GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxVector2>* layerElement, int index_id);
	int GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxColor>* layerElement, int index_id);
	FbxVector4 GetPosition(FbxMesh* const mesh, int index_id);
	FbxVector4 GetNormal(FbxMesh* const mesh, int index_id);
	FbxVector2 GetUV1(FbxMesh* const mesh, int index_id);
	FbxVector2 GetUV2(FbxMesh* const mesh, int index_id);
	FbxVector2 GetUV3(FbxMesh* const mesh, int index_id);
	FbxColor GetVertexColor(FbxMesh* const mesh, int index_id);
	int GetBoneId(int mesh, std::string boneName);
	FbxSkin* GetSkin(FbxMesh* mesh);
	FbxBlendShape* GetMorpher(FbxMesh* mesh);
	void RunSql(sqlite3_stmt* statement);
	void RunSql(std::string query);

	sqlite3_stmt* MakeSqlStatement(std::string query);
	bool MeshPartExists(int mesh, int part);
	void MakeMeshPart(int mesh, int part, std::string name);
	void TestNode(FbxNode* pNode);
	void SaveNode(FbxNode* node);
	void WriteWarning(std::string warning);

public:
	int ImportFBX(const char* fbxFile);
};