
#include <fbx_importer.h>

const char* initScript = "SQL/CreateDB.SQL";
const char* dbPath = "result.db";
const std::regex meshRegex(".*[_ ^][0-9][\\.\\-]?[0-9]?$");
const std::regex extractMeshInfoRegex(".*[_ ^]([0-9])[\\.\\-]?([0-9])?$");


inline bool FBXImporter::file_exists(const std::string& name) {

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
int FBXImporter::Init(const char* fbxFilePath, sqlite3** database, FbxManager** manager, FbxScene** scene) {

	char* zErrMsg = 0;
	int rc;
	fprintf(stdout, "Attempting to process FBX: %s\n", fbxFilePath);

	if (file_exists(dbPath)) {
		int failure = remove(dbPath);
		if (failure != 0) {
			fprintf(stderr, "Unable to remove existing database.\n");
			return(102);
		}

	}

	// Create and connect to the database file.
	rc = sqlite3_open(dbPath, database);
	if (rc) {
		fprintf(stderr, "Failed to create database: %s\n", sqlite3_errmsg(*database));
		sqlite3_close(*database);
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
	rc = sqlite3_exec(*database, fullSql.c_str(), NULL, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Database creation SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(*database);
		return 104;
	}



	// Create the FBX SDK manager
	*manager = FbxManager::Create();

	// Create an IOSettings object.
	FbxIOSettings* ios = FbxIOSettings::Create(*manager, IOSROOT);
	(*manager)->SetIOSettings(ios);

	// Create an importer.
	FbxImporter* importer = FbxImporter::Create(*manager, "");

	// Declare the path and filename of the fidle containing the scene.
	// In this case, we are assuming the file is in the same directory as the executable.

	// Initialize the importer.
	bool success = importer->Initialize(fbxFilePath, -1, (*manager)->GetIOSettings());

	// Use the first argument as the filename for the importer.
	if (!success) {
		fprintf(stderr, "Unable to load FBX file.");
		sqlite3_close(*database);
		(*manager)->Destroy();
		return 105;
	}

	// Create a new scene so that it can be populated by the imported file.
	(*scene) = FbxScene::Create((*manager), "root");

	// Import the contents of the file into the scene.
	importer->Import((*scene));

	// The file is imported; so get rid of the importer.
	importer->Destroy();


	auto unit = (*scene)->GetGlobalSettings().GetSystemUnit();

	// Convert the scene to meters.
	FbxSystemUnit::m.ConvertScene(*scene);


	return 0;
}


/**
 * Shuts down the system gracefully.
 */
void FBXImporter::Shutdown(int code, const char* errorMessage) {

	if (errorMessage != NULL) {
		fprintf(stderr, "\nCritical Error: %s\n", errorMessage);
	}

	// Destroying the manger destroys the scene with it.
	int whatever = manager->GetDocumentCount();
	manager->Destroy();

	// Good night DB.
	sqlite3_close(db);


	// Time for sleep.
	exit(code);
}


int FBXImporter::GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxVector4>* layerElement, int index_id) {

	if (layerElement == NULL) {
		return -1;
	}

	FbxLayerElement::EMappingMode mapMode = layerElement->GetMappingMode();
	FbxLayerElement::EReferenceMode refMode = layerElement->GetReferenceMode();
	// Pick which index we're using.
	int index = 0;
	if (mapMode == FbxLayerElement::eByControlPoint) {
		index = mesh->GetPolygonVertex(index_id / 3, index_id % 3);
	}
	else if (mapMode == FbxLayerElement::eByPolygonVertex) {
		index = index_id;
	}
	else {
		return -1;
	}


	FbxVector4 Normal;
	// Run it through the appropriate direct/indirect
	if (refMode == FbxLayerElement::eDirect) {
		index = index;
	}
	else if (refMode == FbxLayerElement::eIndexToDirect) {
		index = layerElement->GetIndexArray().GetAt(index);
	}
	else {
		return -1;
	}
	return index;
}

int FBXImporter::GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxVector2>* layerElement, int index_id) {

	if (layerElement == NULL) {
		return -1;
	}

	FbxLayerElement::EMappingMode mapMode = layerElement->GetMappingMode();
	FbxLayerElement::EReferenceMode refMode = layerElement->GetReferenceMode();
	// Pick which index we're using.
	int index = 0;
	if (mapMode == FbxLayerElement::eByControlPoint) {
		index = mesh->GetPolygonVertex(index_id / 3, index_id % 3);
	}
	else if (mapMode == FbxLayerElement::eByPolygonVertex) {
		index = index_id;
	}
	else {
		return -1;
	}


	FbxVector4 Normal;
	// Run it through the appropriate direct/indirect
	if (refMode == FbxLayerElement::eDirect) {
		index = index;
	}
	else if (refMode == FbxLayerElement::eIndexToDirect) {
		index = layerElement->GetIndexArray().GetAt(index);
	}
	else {
		return -1;
	}
	return index;
}

int FBXImporter::GetDirectIndex(FbxMesh* mesh, FbxLayerElementTemplate<FbxColor>* layerElement, int index_id) {

	if (layerElement == NULL) {
		return -1;
	}

	FbxLayerElement::EMappingMode mapMode = layerElement->GetMappingMode();
	FbxLayerElement::EReferenceMode refMode = layerElement->GetReferenceMode();
	// Pick which index we're using.
	int index = 0;
	if (mapMode == FbxLayerElement::eByControlPoint) {
		index = mesh->GetPolygonVertex(index_id / 3, index_id % 3);
	}
	else if (mapMode == FbxLayerElement::eByPolygonVertex) {
		index = index_id;
	}
	else {
		return -1;
	}


	FbxVector4 Normal;
	// Run it through the appropriate direct/indirect
	if (refMode == FbxLayerElement::eDirect) {
		index = index;
	}
	else if (refMode == FbxLayerElement::eIndexToDirect) {
		index = layerElement->GetIndexArray().GetAt(index);
	}
	else {
		return -1;
	}
	return index;
}

// Get the raw position value for a triangle index.
FbxVector4 FBXImporter::GetPosition(FbxMesh* const mesh, int index_id) {
	FbxVector4 def = FbxVector4(0, 0, 0, 0);
	if (mesh->GetLayerCount() < 1) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(0);
	int vertex_id = mesh->GetPolygonVertex(index_id / 3, index_id % 3);
	FbxVector4 position = mesh->GetControlPointAt(vertex_id);
	return position;
}

// Get the raw normal value for a triangle index.
FbxVector4 FBXImporter::GetNormal(FbxMesh* const mesh, int index_id) {
	FbxVector4 def = FbxVector4(0, 0, 0, 1.0);
	if (mesh->GetLayerCount() < 1) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(0);
	FbxLayerElementNormal* layerElement = layer->GetNormals();
	int index = GetDirectIndex(mesh, layerElement, index_id);
	return index == -1 ? def : layerElement->GetDirectArray().GetAt(index);
}

// Get the raw uv1 value for a triangle index.
FbxVector2 FBXImporter::GetUV1(FbxMesh* const mesh, int index_id) {
	FbxVector2 def = FbxVector2(0, 0);
	if (mesh->GetLayerCount() < 1) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(0);
	FbxLayerElementUV* uvs = layer->GetUVs();
	int index = GetDirectIndex(mesh, uvs, index_id);
	return index == -1 ? def : uvs->GetDirectArray().GetAt(index);
}

// Get the raw uv2 value for a triangle index.
FbxVector2 FBXImporter::GetUV2(FbxMesh* const mesh, int index_id) {
	FbxVector2 def = FbxVector2(0, 0);
	if (mesh->GetLayerCount() < 2) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(1);
	FbxLayerElementUV* uvs = layer->GetUVs();
	int index = GetDirectIndex(mesh, uvs, index_id);
	return index == -1 ? def : uvs->GetDirectArray().GetAt(index);
}

// Get the raw uv2 value for a triangle index.
FbxVector2 FBXImporter::GetUV3(FbxMesh* const mesh, int index_id) {
	FbxVector2 def = FbxVector2(1, 0);
	if (mesh->GetLayerCount() < 3) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(2);
	FbxLayerElementUV* uvs = layer->GetUVs();
	int index = GetDirectIndex(mesh, uvs, index_id);
	return index == -1 ? def : uvs->GetDirectArray().GetAt(index);
}

// Gets the raw vertex color value for a triangle index.
FbxColor FBXImporter::GetVertexColor(FbxMesh* const mesh, int index_id) {
	FbxColor def = FbxColor(1, 1, 1, 1);
	if (mesh->GetLayerCount() < 1) {
		return def;
	}
	FbxLayer* layer = mesh->GetLayer(0);
	FbxLayerElementVertexColor* layerElement = layer->GetVertexColors();
	int index = GetDirectIndex(mesh, layerElement, index_id);
	return index == -1 ? def : layerElement->GetDirectArray().GetAt(index);
}

// Retreives the shared bone Id for a given bone (added to the bone Id list if needed)
int FBXImporter::GetBoneId(int mesh, std::string boneName) {
	while (boneNames.size() <= (unsigned int)mesh) {
		std::vector<std::string> n;
		boneNames.push_back(n);
	}

	// Get
	int boneIdx = -1;
	for (unsigned int ni = 0; ni < boneNames[mesh].size(); ni++) {
		std::string bName = boneNames[mesh][ni];
		if (bName.compare(boneName) == 0) {
			boneIdx = ni;
			break;
		}
	}

	if (boneIdx == -1) {
		boneNames[mesh].push_back(boneName);
		boneIdx = boneNames[mesh].size() - 1;
	}
	return boneIdx;
}

// Gets the first Skin element in a mesh.
FbxSkin* FBXImporter::GetSkin(FbxMesh* mesh) {

	int count = mesh->GetDeformerCount();
	for (int i = 0; i < count; i++) {
		FbxDeformer* d = mesh->GetDeformer(i);
		FbxDeformer::EDeformerType dType = d->GetDeformerType();
		if (dType == FbxDeformer::eSkin) {
			return (FbxSkin*)d;
		}
	}
	return NULL;
}

// Gets the first Skin element in a mesh.
FbxBlendShape* FBXImporter::GetMorpher(FbxMesh* mesh) {

	int count = mesh->GetDeformerCount();
	for (int i = 0; i < count; i++) {
		FbxDeformer* d = mesh->GetDeformer(i);
		FbxDeformer::EDeformerType dType = d->GetDeformerType();
		if (dType == FbxDeformer::eBlendShape) {
			return (FbxBlendShape*)d;
		}
	}
	return NULL;
}

// Runs a simple pass/fail query with no results.
void FBXImporter::RunSql(std::string query) {

	char* err;
	int result = sqlite3_exec(db, query.c_str(), NULL, 0, &err);
	if (result != SQLITE_OK) {
		fprintf(stderr, "SQLite Error: %s", err);
		sqlite3_free(err);
		Shutdown(201, "SQLite Error.");
	}
}

// Runs a simple pass/fail query with no results.
void FBXImporter::RunSql(sqlite3_stmt* statement) {

	int result = sqlite3_step(statement);
	if (result != SQLITE_DONE) {
		std::string err = sqlite3_errmsg(db);
		fprintf(stderr, "SQLite Error: %s", err.c_str());
		sqlite3_finalize(statement);
		Shutdown(201, "SQLite Error.");
	}
	sqlite3_reset(statement);
	sqlite3_clear_bindings(statement);
	if (result != SQLITE_DONE) {
		std::string err = sqlite3_errmsg(db);
		fprintf(stderr, "SQLite Error: %s", err.c_str());
		sqlite3_finalize(statement);
		Shutdown(201, "SQLite Error.");
	}
}

// Makes an Sqlite3 statement object from a query string.
sqlite3_stmt* FBXImporter::MakeSqlStatement(std::string query) {
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
	return stmt;
}

// Write a non-critical warning message to the DB and stdout.
void FBXImporter::WriteWarning(std::string warning) {
	fprintf(stderr, "Warning: %s\n", warning.c_str());

	// Load the triangle indicies into the SQLite DB.
	std::string insertStatement = "insert into warnings (text) values (?1)";
	sqlite3_stmt* query = MakeSqlStatement(insertStatement);
	sqlite3_bind_text(query, 1, warning.c_str(), warning.length(), NULL);
	RunSql(query);
	sqlite3_finalize(query);
}

bool FBXImporter::MeshGroupExists(int mesh) {
	std::map<int, std::map<int, std::string>>::iterator it = meshParts.find(mesh);
	if (it == meshParts.end()) return false;
	return true;
}

// Checks if a mesh part already exists.
bool FBXImporter::MeshPartExists(int mesh, int part) {

	// Does mesh exist?
	std::map<int, std::map<int, std::string>>::iterator it = meshParts.find(mesh);
	if (it == meshParts.end()) return false;

	std::map<int, std::string> partList = it->second;

	// Does part exist?
	std::map<int, std::string>::iterator it2 = partList.find(part);
	if (it2 == partList.end()) return false;


	return true;
}


// Adds a mesh part to the mesh parts dictionary
void FBXImporter::MakeMeshPart(int mesh, int part, std::string name, std::string parentName) {

	std::map<int, std::string> partList;

	// Create mesh entry if needed.
	std::map<int, std::map<int, std::string>>::iterator it = meshParts.find(mesh);
	if (it == meshParts.end()) {
		partList = std::map<int, std::string>();
		meshParts.insert(make_pair(mesh, partList));

		// Pop the name and entry into the DB too.
		// We don't really care about having an accurate material ID here, as TexTools doesn't read it on
		// import anyways.
		std::string insertStatement = "insert into meshes (mesh, name, material_id) values (?1, ?2, 0)";
		sqlite3_stmt* query = MakeSqlStatement(insertStatement);
		sqlite3_bind_int(query, 1, mesh);
		sqlite3_bind_text(query, 2, parentName.c_str(), parentName.length(), NULL);
		RunSql(query);
		sqlite3_finalize(query);
	}
	else {
		partList = it->second;
	}

	// Insert mesh name
	partList.insert(make_pair(part, name));
	meshParts[mesh] = partList;

	// Insert the part into the SQlite DB.

	// Load the Part into the SQLite DB.
	std::string insertStatement = "insert into parts (mesh, part, name) values (?1, ?2, ?3)";
	sqlite3_stmt* query = MakeSqlStatement(insertStatement);
	sqlite3_bind_int(query, 1, mesh);
	sqlite3_bind_int(query, 2, part);
	sqlite3_bind_text(query, 3, name.c_str(), name.length(), NULL);
	RunSql(query);
	sqlite3_finalize(query);
}

/**
 * Saves the given node to the SQLite DB.
 */
void FBXImporter::SaveNode(FbxNode* node) {
	FbxMesh* mesh = node->GetMesh();
	std::string meshName = node->GetName();

	int numVertices = mesh->GetControlPointsCount();
	int numIndices = mesh->GetPolygonVertexCount();
	if (numIndices == 0 || numVertices == 0) {
		// Mesh does not actually have any tris.
		WriteWarning("Ignored mesh: " + meshName + " - Mesh had no vertices/triangles.");
		return;
	}
	FbxSkin* skin = GetSkin(mesh);
	if (skin == NULL) {
		// Mesh does not actually have a skin.
		WriteWarning("Ignored mesh: " + meshName + " - Mesh did not have a valid skin element.");
		return;
	}

	auto worldTransform = node->EvaluateGlobalTransform();

	//boost::match_results<std::string::const_iterator> results;

	std::smatch m;

	bool success = std::regex_match(meshName, m, extractMeshInfoRegex);

	// Somehow we got here with a badly named mesh.
	if (!success) return;

	std::string meshMatch = m[1];
	std::string partMatch = m[2];


	int meshNum = std::atoi(meshMatch.c_str());;
	int partNum = 0;
	if (partMatch != "") {
		partNum = std::atoi(partMatch.c_str());;
	}

	if (MeshPartExists(meshNum, partNum)) {
		// Mesh part already exists.
		WriteWarning("Ignored mesh: " + meshName + " - Mesh " + std::to_string(meshNum) + " Part " + std::to_string(partNum) + " already exists.");
		return;
	}

	std::string parentName = "Group " + std::to_string(meshNum);
	if (node->GetParent() != NULL && node->GetParent()->GetName() != NULL) {
		parentName = node->GetParent()->GetName();
	}

	// Add the mesh part.
	MakeMeshPart(meshNum, partNum, meshName, parentName);

	// Create a vector the side of the control point array to store the weights.
	std::vector<TTWeightSet> weightSets;
	weightSets.resize(mesh->GetControlPointsCount());

	// Vector of [control point index] => [Set of tri indexes that reference it.
	std::vector<std::vector<int>> controlToPolyArray;
	controlToPolyArray.resize(mesh->GetControlPointsCount());
	int polys = mesh->GetPolygonCount();
	if (polys != (numIndices / 3.0f)) {
		Shutdown(500, "FBX is not fully triangulated.  Please export from your 3D modeling program with Triangulate option enabled.");
	}

	// Loop all tri indices and add them to the appropriate array.
	for (int i = 0; i < numIndices; i++) {
		int controlPointIndex = mesh->GetPolygonVertex(i / 3, i % 3);
		controlToPolyArray[controlPointIndex].push_back(i);
	}

	int numClusters = skin->GetClusterCount();
	// Loop all the clusters and populate the weight sets.
	for (int i = 0; i < numClusters; i++) {
		FbxCluster::ELinkMode mode = skin->GetCluster(i)->GetLinkMode();
		std::string name = skin->GetCluster(i)->GetLink()->GetName();
		int affectedVertCount = skin->GetCluster(i)->GetControlPointIndicesCount();

		int boneIdx = GetBoneId(meshNum, name);


		for (int vi = 0; vi < affectedVertCount; vi++) {
			int cpIndex = skin->GetCluster(i)->GetControlPointIndices()[vi];
			double weight = skin->GetCluster(i)->GetControlPointWeights()[vi];
			weightSets[cpIndex].Add(boneIdx, weight);
		}
	}


	FbxBlendShape* morpher = GetMorpher(mesh);
	if (morpher != NULL) {
		fprintf(stderr, "%s contains an uncollapsed morpher.  Morpher data will be ignored.\n", meshName.c_str());
		int channelCount = morpher->GetBlendShapeChannelCount();
		for (int i = 0; i < channelCount; i++) {
			FbxBlendShapeChannel* channel = morpher->GetBlendShapeChannel(i);
			FbxSubDeformer::EType subtype = channel->GetSubDeformerType();
			int shapeCount = channel->GetTargetShapeCount();
			double pct = channel->DeformPercent;


			for (int s = 0; s < shapeCount; s++) {
				FbxShape* shape = channel->GetTargetShape(s);
				int indices = shape->GetControlPointIndicesCount();
				int z = 0;
				z++;
			}
		}
	}


	std::vector<TTVertex> ttVerticies;
	std::vector<int> ttTriIndexes;
	ttTriIndexes.resize(numIndices);

	// Time to convert all the data to TTVertices.
	// Start by looping over the groups of shared vertices.
	unsigned int vertCount = controlToPolyArray.size();
	for (unsigned int cpi = 0; cpi < vertCount; cpi++) {
		unsigned int sharedIndexCount = controlToPolyArray[cpi].size();
		unsigned int oldSize = ttVerticies.size();

		// No indices, this is an orphaned control point, skip it.
		if (sharedIndexCount == 0) continue;

		// Setup vertex list
		std::vector<TTVertex> sharedVerts;


		// Loop all tri indices in that point to that control point.
		for (unsigned ti = 0; ti < sharedIndexCount; ti++) {

			int indexId = controlToPolyArray[cpi][ti];

			// Build our own vertex.
			TTVertex myVert;
			int controlPointIndex = mesh->GetPolygonVertex(indexId / 3, indexId % 3);
			auto vertWorldPosition = worldTransform.MultT(GetPosition(mesh, indexId));
			//auto vertWorldNormal = worldTransform.MultT(GetNormal(mesh, indexId));

			myVert.Position = vertWorldPosition;
			myVert.Normal = GetNormal(mesh, indexId);
			myVert.VertexColor = GetVertexColor(mesh, indexId);
			myVert.UV1 = GetUV1(mesh, indexId);
			myVert.UV2 = GetUV2(mesh, indexId);
			myVert.WeightSet = weightSets[controlPointIndex];

			// Loop through the shared vertices to see if we already have an identical entry.
			int sharedVertToUse = -1;
			for (unsigned int svi = 0; svi < sharedVerts.size(); svi++) {
				TTVertex other = sharedVerts[svi];
				if (myVert == other) {
					sharedVertToUse = svi;
					break;
				}
			}

			// We are a unique vertex.
			if (sharedVertToUse == -1)
			{
				sharedVerts.push_back(myVert);
				sharedVertToUse = sharedVerts.size() - 1;
			}

			// Assign the triangle index the correct new tt_vertex index to use.
			ttTriIndexes[indexId] = sharedVertToUse + oldSize;

		}

		// Push all our new vertices into the main list.
		ttVerticies.resize(oldSize + sharedVerts.size());
		for (unsigned int svi = 0; svi < sharedVerts.size(); svi++) {
			ttVerticies[oldSize + svi] = sharedVerts[svi];
		}
	}

	// We now have a fully populated TT Vertex list
	// And a fully populated triangle Index list that references it.
	int asdf = 4;
	asdf++;

	// Start by writing the tri indexes.
	const std::string startTransaction = "BEGIN TRANSACTION;";
	const std::string endTransaction = "COMMIT;";

	// Load the triangle indicies into the SQLite DB.
	std::string insertStatement = "insert into indices (mesh, part, index_id, vertex_id) values (?1,?2,?3,?4)";
	RunSql(startTransaction);
	sqlite3_stmt* query = MakeSqlStatement(insertStatement);
	for (unsigned int i = 0; i < ttTriIndexes.size(); i++) {
		sqlite3_bind_int(query, 1, meshNum);
		sqlite3_bind_int(query, 2, partNum);
		sqlite3_bind_int(query, 3, i);
		sqlite3_bind_int(query, 4, ttTriIndexes[i]);
		RunSql(query);
	}
	sqlite3_finalize(query);

	// Load the Vertices into the SQLite DB.
	insertStatement = "insert into vertices (mesh, part, vertex_id, position_x, position_y, position_z, normal_x, normal_y, normal_z, color_r, color_g, color_b, color_a, uv_1_u, uv_1_v, uv_2_u, uv_2_v, bone_1_id, bone_1_weight, bone_2_id, bone_2_weight, bone_3_id, bone_3_weight, bone_4_id, bone_4_weight)";
	insertStatement += "			 values(   ?1,   ?2,        ?3,         ?4,         ?5,         ?6,       ?7,       ?8,       ?9,     ?10,     ?11,     ?12,     ?13,    ?14,    ?15,    ?16,    ?17,       ?18,           ?19,       ?20,           ?21,       ?22,           ?23,       ?24,           ?25)";
	query = MakeSqlStatement(insertStatement);
	for (unsigned int i = 0; i < ttVerticies.size(); i++) {
		sqlite3_bind_int(query, 1, meshNum);
		sqlite3_bind_int(query, 2, partNum);
		sqlite3_bind_int(query, 3, i);

		sqlite3_bind_double(query, 4, ttVerticies[i].Position[0]);
		sqlite3_bind_double(query, 5, ttVerticies[i].Position[1]);
		sqlite3_bind_double(query, 6, ttVerticies[i].Position[2]);

		sqlite3_bind_double(query, 7, ttVerticies[i].Normal[0]);
		sqlite3_bind_double(query, 8, ttVerticies[i].Normal[1]);
		sqlite3_bind_double(query, 9, ttVerticies[i].Normal[2]);

		sqlite3_bind_double(query, 10, ttVerticies[i].VertexColor.mRed);
		sqlite3_bind_double(query, 11, ttVerticies[i].VertexColor.mGreen);
		sqlite3_bind_double(query, 12, ttVerticies[i].VertexColor.mBlue);
		sqlite3_bind_double(query, 13, ttVerticies[i].VertexColor.mAlpha);

		sqlite3_bind_double(query, 14, ttVerticies[i].UV1[0]);
		sqlite3_bind_double(query, 15, ttVerticies[i].UV1[1]);

		sqlite3_bind_double(query, 16, ttVerticies[i].UV2[0]);
		sqlite3_bind_double(query, 17, ttVerticies[i].UV2[1]);

		if (ttVerticies[i].WeightSet.Weights[0].BoneId >= 0) {
			sqlite3_bind_int(query, 18, ttVerticies[i].WeightSet.Weights[0].BoneId);
			sqlite3_bind_double(query, 19, ttVerticies[i].WeightSet.Weights[0].Weight);
		}

		if (ttVerticies[i].WeightSet.Weights[1].BoneId >= 0) {
			sqlite3_bind_int(query, 20, ttVerticies[i].WeightSet.Weights[1].BoneId);
			sqlite3_bind_double(query, 21, ttVerticies[i].WeightSet.Weights[1].Weight);
		}

		if (ttVerticies[i].WeightSet.Weights[2].BoneId >= 0) {
			sqlite3_bind_int(query, 22, ttVerticies[i].WeightSet.Weights[2].BoneId);
			sqlite3_bind_double(query, 23, ttVerticies[i].WeightSet.Weights[2].Weight);
		}

		if (ttVerticies[i].WeightSet.Weights[3].BoneId >= 0) {
			sqlite3_bind_int(query, 24, ttVerticies[i].WeightSet.Weights[3].BoneId);
			sqlite3_bind_double(query, 25, ttVerticies[i].WeightSet.Weights[3].Weight);
		}


		RunSql(query);
	}
	sqlite3_finalize(query);

	RunSql(endTransaction);


}

/**
 * Recursively scans the node tree for nodes that match our Regex, then pipes them to the save function.
 */
void FBXImporter::TestNode(FbxNode* pNode) {
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();


	bool show = pNode->Show.Get();
	if (regex_match(nodeName, meshRegex) && pNode->GetMesh() != NULL && show) {

		// Save the node to the db.
		SaveNode(pNode);
	}

	// Continue scanning the tree.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		TestNode(pNode->GetChild(j));
}

int FBXImporter::ImportFBX(const char* fbxfilepath) {
	// Try to load all the things.
	int result = Init(fbxfilepath, &db, &manager, &scene);
	if (result != 0) {
		return result;
	}
	// We're now ready to actually do some work.


	// Print the nodes of the scene and their attributes recursively.
	// Note that we are not printing the root node because it should
	// not contain any attributes.
	FbxNode* root = scene->GetRootNode();
	if (root) {
		for (int i = 0; i < root->GetChildCount(); i++) {
			TestNode(root->GetChild(i));
		}
	}

	// Save bones to the SQLite DB
	std::string insertStatement = "insert into bones (mesh, bone_id, name) values (?1,?2,?3)";
	sqlite3_stmt* query = MakeSqlStatement(insertStatement);
	for (unsigned int mi = 0; mi < boneNames.size(); mi++) {
		for (unsigned int bi = 0; bi < boneNames[mi].size(); bi++) {
			sqlite3_bind_int(query, 1, mi);
			sqlite3_bind_int(query, 2, bi);
			sqlite3_bind_text(query, 3, boneNames[mi][bi].c_str(), boneNames[mi][bi].length(), NULL);
			RunSql(query);
		}
	}
	sqlite3_finalize(query);


	fprintf(stdout, "Successfully processed FBX File.\n");
	// Successs~
	Shutdown(0);
}
