#include <db_converter.h>

/**
 * Shuts down the system gracefully.
 */
void DBConverter::Shutdown(int code, const char* errorMessage) {

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

// Write a non-critical warning message to stdout/stderr.
void DBConverter::WriteLog(std::string message, bool warning) {
	if (warning) {
		fprintf(stderr, "Warning: %s\n", message.c_str());
	}
	else {
		fprintf(stdout, "%s\n", message.c_str());
	}
}

// Makes an sqlite statement from a string.
sqlite3_stmt* DBConverter::MakeSqlStatement(std::string query) {
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
	return stmt;
}
/**
 * Attempts to initialize the SQLite Database and FBX scene.
 * Returns 0 on success, non-zero on error.
 */
int DBConverter::Init(const char* dbFilePath) {

	char* zErrMsg = 0;
	int rc;
	fprintf(stdout, "Attempting to process DB File: %s\n", dbFilePath);

	// Create and connect to the database file.
	rc = sqlite3_open(dbFilePath, &db);
	if (rc) {
		fprintf(stderr, "Failed to connect to database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return(103);
	}

	// Create the FBX SDK manager
	manager = FbxManager::Create();

	// Create a new scene so that it can be populated by the imported file.
	(scene) = FbxScene::Create((manager), "root");

	return 0;
}

// Handles running and error checking the first sqlite step of a statement.
bool DBConverter::GetRow(sqlite3_stmt* statement) {

	int result = sqlite3_step(statement);
	if (result == SQLITE_ROW) {
		return true;
	}
	else if (result == SQLITE_DONE) {
		return false;
	} else  {
		std::string err = sqlite3_errmsg(db);
		fprintf(stderr, "SQLite Error: %s", err.c_str());
		sqlite3_finalize(statement);
		Shutdown(201, "SQLite Error.");
	}
}

// Assembles the bone list into a heirarchical skeleton.
void DBConverter::BuildSkeleton(std::vector<TTBone*> bones) {

	TTBone* root = NULL;

	// First, find the root;
	for (int i = 0; i < bones.size(); i++) {
		TTBone* bone = bones[i];
		if (bone->ParentId == -1) {
			root = bone;
			root->Parent = NULL;
			break;
		}
	}

	if (root == NULL) {
		return;
	}

	ttModel->FullSkeleton = root;
	AssignChildren(root, bones);


}

void DBConverter::AssignChildren(TTBone* root, std::vector<TTBone*> bones) {
	for (int i = 0; i < bones.size(); i++) {
		TTBone* bone = bones[i];
		if (bone->ParentId == root->Id) {
			bone->Parent = root;
			root->Children.push_back(bone);
			AssignChildren(bone, bones);
		}
	}
}


void DBConverter::ReadDB() {

	ttModel = new TTModel();

	std::vector<TTBone*> bones;

	// Meshes and Parts
	sqlite3_stmt* query = MakeSqlStatement("select mesh, part, name from parts");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int partId = sqlite3_column_int(query, 1);
		std::string name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 2)));

		while (meshId >= ttModel->MeshGroups.size()) {
			ttModel->MeshGroups.push_back(new TTMeshGroup());
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->Model = ttModel;
		}
		while (partId >= ttModel->MeshGroups[meshId]->Parts.size()) {
			ttModel->MeshGroups[meshId]->Parts.push_back(new TTPart());
			TTPart* part = ttModel->MeshGroups[meshId]->Parts[ttModel->MeshGroups[meshId]->Parts.size() - 1];
			part->MeshGroup = ttModel->MeshGroups[meshId];
		}
		ttModel->MeshGroups[meshId]->Parts[partId]->Name = name;
	}
	sqlite3_finalize(query);

	// Bones
	query = MakeSqlStatement("select mesh, bone_id, parent_id, name, matrix_0, matrix_1, matrix_2, matrix_3, matrix_4, matrix_5, matrix_6, matrix_7, matrix_8, matrix_9, matrix_10, matrix_11, matrix_12, matrix_13, matrix_14, matrix_15 from bones order by mesh asc, bone_id asc");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int boneId = sqlite3_column_int(query, 1);
		std::string name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 3)));
		if (meshId == -1) {
			// This is part of the full skeleton list, not a mesh bone.
			int parentId = sqlite3_column_int(query, 2);
			TTBone* bone = new TTBone();
			bone->Id = boneId;
			bone->Name = name;
			bone->ParentId = parentId;
			bone->PoseMatrix = FbxMatrix(sqlite3_column_double(query, 3), sqlite3_column_double(query, 4), sqlite3_column_double(query, 5), sqlite3_column_double(query, 6),
				sqlite3_column_double(query, 7), sqlite3_column_double(query, 8), sqlite3_column_double(query, 9), sqlite3_column_double(query, 10),
				sqlite3_column_double(query, 11), sqlite3_column_double(query, 12), sqlite3_column_double(query, 13), sqlite3_column_double(query, 14),
				sqlite3_column_double(query, 15), sqlite3_column_double(query, 16), sqlite3_column_double(query, 17), sqlite3_column_double(query, 18));

			bones.push_back(bone);
		}
		else {
			while (meshId >= ttModel->MeshGroups.size()) {
				ttModel->MeshGroups.push_back(new TTMeshGroup());
			}
			while (boneId >= ttModel->MeshGroups[meshId]->Bones.size()) {
				ttModel->MeshGroups[meshId]->Bones.push_back("");
			}
			ttModel->MeshGroups[meshId]->Bones[boneId] = name;
		}
	}
	sqlite3_finalize(query);

	BuildSkeleton(bones);

	// Indices
	query = MakeSqlStatement("select mesh, part, index_id, vertex_id from indices order by mesh asc, part asc, index_id asc");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int partId = sqlite3_column_int(query, 1);
		int indexId = sqlite3_column_int(query, 2);
		int vertexId= sqlite3_column_int(query, 3);

		ttModel->MeshGroups[meshId]->Parts[partId]->Indices.push_back(vertexId);
	}
	sqlite3_finalize(query);


	query = MakeSqlStatement("select mesh, part, vertex_id, position_x, position_y, position_z, normal_x, normal_y, normal_z, color_r, color_g, color_b, color_a, uv_1_u, uv_1_v, uv_2_u, uv_2_v, bone_1_id, bone_1_weight, bone_2_id, bone_2_weight, bone_3_id, bone_3_weight, bone_4_id, bone_4_weight from vertices order by mesh asc, part asc, vertex_id asc");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int partId = sqlite3_column_int(query, 1);
		int vertexId = sqlite3_column_int(query, 2);

		TTVertex v;
		v.Position[0] = sqlite3_column_double(query, 3);
		v.Position[1] = sqlite3_column_double(query, 4);
		v.Position[2] = sqlite3_column_double(query, 5);

		v.Normal[0] = sqlite3_column_double(query, 6);
		v.Normal[1] = sqlite3_column_double(query, 7);
		v.Normal[2] = sqlite3_column_double(query, 8);

		v.VertexColor[0] = sqlite3_column_double(query, 9);
		v.VertexColor[1] = sqlite3_column_double(query, 10);
		v.VertexColor[2] = sqlite3_column_double(query, 11);
		v.VertexColor[3] = sqlite3_column_double(query, 12);

		v.UV1[0] = sqlite3_column_double(query, 13);
		v.UV1[1] = sqlite3_column_double(query, 14);

		v.UV2[0] = sqlite3_column_double(query, 15);
		v.UV2[1] = sqlite3_column_double(query, 16);

		v.WeightSet.Weights[0].BoneId = sqlite3_column_int(query, 17);
		v.WeightSet.Weights[0].Weight = sqlite3_column_double(query, 18);

		v.WeightSet.Weights[1].BoneId = sqlite3_column_int(query, 19);
		v.WeightSet.Weights[1].Weight = sqlite3_column_double(query, 20);

		v.WeightSet.Weights[2].BoneId = sqlite3_column_int(query, 21);
		v.WeightSet.Weights[2].Weight = sqlite3_column_double(query, 22);

		v.WeightSet.Weights[3].BoneId = sqlite3_column_int(query, 23);
		v.WeightSet.Weights[3].Weight = sqlite3_column_double(query, 24);

		ttModel->MeshGroups[meshId]->Parts[partId]->Vertices.push_back(v);
	}
	sqlite3_finalize(query);


	int z = 0;
	z++;

}

// Create the FBX scene.
void DBConverter::CreateScene() {
	scene = FbxScene::Create(manager, "TexTools FBX Export");
	FbxNode* root = scene->GetRootNode();

	FbxNode* node = FbxNode::Create(manager, "TexTools Export");
	root->AddChild(node);
	ttModel->Node = node;

	AddBoneToScene(ttModel->FullSkeleton);

	std::vector<FbxNode*> meshGroupNodes;
	for (int i = 0; i < ttModel->MeshGroups.size(); i++) {
		FbxNode* node = FbxNode::Create(manager, "Group");
		meshGroupNodes.push_back(node);
		root->AddChild(node);
		ttModel->MeshGroups[i]->Node = node;

		for (int pi = 0; pi < ttModel->MeshGroups[i]->Parts.size(); pi++) {
			AddPartToScene(ttModel->MeshGroups[i]->Parts[pi], node);
		}
	}


}

void DBConverter::AddBoneToScene(TTBone* bone) {
	FbxNode* parentNode;
	if (bone->Parent == NULL) {
		parentNode = ttModel->Node;
	}
	else {
		parentNode = bone->Parent->Node;
	}

	FbxNode* node = FbxNode::Create(manager, bone->Name.c_str());
	parentNode->AddChild(node);
	bone->Node = node;

	for (int i = 0; i < bone->Children.size(); i++) {
		AddBoneToScene(bone->Children[i]);
	}
}

void DBConverter::AddPartToScene(TTPart* part, FbxNode* parent) {
	// Create a mesh object
	FbxMesh* mesh = FbxMesh::Create(manager, "");

	// Set the mesh as the node attribute of the node
	FbxNode* node = FbxNode::Create(manager, part->Name.c_str());
	parent->AddChild(node);
	node->SetNodeAttribute(mesh);

	// set the shading mode to view texture
	node->SetShadingMode(FbxNode::eTextureShading);

	mesh->InitControlPoints(part->Vertices.size());
	//mesh->InitNormals(part->Vertices.size());
	//mesh->InitTextureUV(part->Vertices.size());
	FbxVector4* cps = mesh->GetControlPoints();
	//FbxVector4* cps = mesh->Getnorm;

	for (int i = 0; i < part->Vertices.size(); i++) {
		TTVertex v = part->Vertices[i];
		cps[i] = v.Position;
	}

	for (int i = 0; i < part->Indices.size(); i += 3) {
		mesh->BeginPolygon();
		mesh->AddPolygon(part->Indices[i]);
		mesh->AddPolygon(part->Indices[i + 1]);
		mesh->AddPolygon(part->Indices[i + 2]);
		mesh->EndPolygon();
	}

	part->Node = node;
}

void DBConverter::ExportScene() {

	// Create an IOSettings object.
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);

	// ... Configure the FbxIOSettings object here ...

	// Create an exporter.
	FbxExporter* exporter = FbxExporter::Create(manager, "");

	// Declare the path and filename of the file to which the scene will be exported.
	// In this case, the file will be in the same directory as the executable.
	const char* lFilename = "result.fbx";

	// Initialize the exporter.
	bool exportStatus = exporter->Initialize(lFilename, -1, manager->GetIOSettings()); 
	if (!exportStatus) {
		printf("Call to FbxExporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
		Shutdown(800);
	}

	exporter->Export(scene);

	// Destroy the exporter.
	exporter->Destroy();
}

int DBConverter::ConvertDB(const char* dbFile) {
	int ret = Init(dbFile);
	if (ret != 0) {
		return ret;
	}
	
	// Load data from the DB file itself.
	ReadDB();

	// Create the FBX Scene.
	CreateScene();

	// Export the FBX Scene.
	ExportScene();
	
	Shutdown(0);
	return 0;
}