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

	FBXSDK_printf("Autodesk FBX SDK version %s\n", manager->GetVersion());
	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);

	FbxString lPath = FbxGetApplicationDirectory();
	manager->LoadPluginsDirectory(lPath.Buffer());

	// Create a new scene so that it can be populated by the imported file.
	(scene) = FbxScene::Create((manager), "TT Export");

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


// Reads the raw SQLite DB file and populates a TTModel object from it.
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
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->MeshId = ttModel->MeshGroups.size() - 1;
		}
		while (partId >= ttModel->MeshGroups[meshId]->Parts.size()) {
			ttModel->MeshGroups[meshId]->Parts.push_back(new TTPart());
			TTPart* part = ttModel->MeshGroups[meshId]->Parts[ttModel->MeshGroups[meshId]->Parts.size() - 1];
			part->MeshGroup = ttModel->MeshGroups[meshId];
			part->PartId = ttModel->MeshGroups[meshId]->Parts.size() - 1;
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
			Eigen::Transform<double, 3, Eigen::Affine> matrix;
			Eigen::Matrix4d baseMatrix = matrix.matrix();

			baseMatrix(0, 0) = sqlite3_column_double(query, 4);
			baseMatrix(0, 1) = sqlite3_column_double(query, 5);
			baseMatrix(0, 2) = sqlite3_column_double(query, 6);
			baseMatrix(0, 3) = sqlite3_column_double(query, 7);

			baseMatrix(1, 0) = sqlite3_column_double(query, 8);
			baseMatrix(1, 1) = sqlite3_column_double(query, 9);
			baseMatrix(1, 2) = sqlite3_column_double(query, 10);
			baseMatrix(1, 3) = sqlite3_column_double(query, 11);

			baseMatrix(2, 0) = sqlite3_column_double(query, 12);
			baseMatrix(2, 1) = sqlite3_column_double(query, 13);
			baseMatrix(2, 2) = sqlite3_column_double(query, 14);
			baseMatrix(2, 3) = sqlite3_column_double(query, 15);

			baseMatrix(3, 0) = sqlite3_column_double(query, 16);
			baseMatrix(3, 1) = sqlite3_column_double(query, 17);
			baseMatrix(3, 2) = sqlite3_column_double(query, 18);
			baseMatrix(3, 3) = sqlite3_column_double(query, 19);

			matrix.matrix() = baseMatrix;
			bone->PoseMatrix = matrix;

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

	FbxAxisSystem directXAxisSys(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityOdd, FbxAxisSystem::eRightHanded);
	directXAxisSys.ConvertScene(scene);

	FbxNode* firstNode = FbxNode::Create(manager, "TexTools Export");
	FbxDouble3 rootScale = firstNode->LclScaling.Get();
	root->AddChild(firstNode);
	ttModel->Node = firstNode;

	FbxPose* pose = FbxPose::Create(manager, "Bindpose");
	pose->SetIsBindPose(true);
	AddBoneToScene(ttModel->FullSkeleton, pose);
	scene->AddPose(pose);


	std::vector<FbxNode*> meshGroupNodes;
	for (int i = 0; i < ttModel->MeshGroups.size(); i++) {
		FbxNode* node = FbxNode::Create(manager, std::string("Group " + std::to_string(i)).c_str());
		meshGroupNodes.push_back(node);
		firstNode->AddChild(node);

		ttModel->MeshGroups[i]->Node = node;

		pose->Add(node, node->EvaluateGlobalTransform());

		for (int pi = 0; pi < ttModel->MeshGroups[i]->Parts.size(); pi++) {
			AddPartToScene(ttModel->MeshGroups[i]->Parts[pi], node);
		}
	}
	
	pose->Add(firstNode, firstNode->EvaluateGlobalTransform());

}

FbxDouble3 MatrixToScale(Eigen::Transform<double, 3, Eigen::Affine> affineMatrix) {

	Eigen::Matrix4d m = affineMatrix.matrix();
	
	FbxVector4 row1 = FbxVector4(m(0, 0), m(1, 0), m(2, 0));
	FbxVector4 row2 = FbxVector4(m(0, 1), m(1, 1), m(2, 1));
	FbxVector4 row3 = FbxVector4(m(0, 2), m(1, 2), m(2, 2));
	FbxDouble3 ret;
	ret[0] = row1.Length();
	ret[1] = row2.Length();
	ret[2] = row3.Length();
	return ret;

}

void DBConverter::AddBoneToScene(TTBone* bone, FbxPose* bindPose) {
	FbxNode* parentNode;
	if (bone->Parent == NULL) {
		parentNode = ttModel->Node;
	}
	else {
		parentNode = bone->Parent->Node;
	}

	FbxNode* node = FbxNode::Create(manager, bone->Name.c_str());
	bone->Node = node;

	FbxSkeleton* skeletonAttribute = FbxSkeleton::Create(scene, "Skeleton");

	if (bone->Parent == NULL)
		skeletonAttribute->SetSkeletonType(FbxSkeleton::eRoot);
	else
		skeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);


	skeletonAttribute->LimbLength.Set(1.0);
	skeletonAttribute->Size.Set(1.0);
	node->SetNodeAttribute(skeletonAttribute);

	// Assign transformation vars.

	Eigen::Block<Eigen::Matrix4d, 3, 1, true> t = bone->PoseMatrix.translation();
	FbxDouble3 translation = FbxDouble3(t.x(), t.y(), t.z());

	// I have absoultely no fucking clue how these euler angle values are decided, but these are what works.
	Eigen::Vector3d rot = bone->PoseMatrix.rotation().eulerAngles(2, 1, 0);
	FbxDouble3 degRot = FbxDouble3(FBXSDK_RAD_TO_DEG * rot[2], FBXSDK_RAD_TO_DEG * rot[1], FBXSDK_RAD_TO_DEG * rot[0]);

	// The bones seem to always be scaled at 1.0, so I'm not sure this is actually useful, but whatever.
	FbxDouble3 scale = MatrixToScale(bone->PoseMatrix);

	node->LclTranslation.Set(translation);
	node->LclRotation.Set(degRot);
	node->LclScaling.Set(scale);


	parentNode->AddChild(node);
	bindPose->Add(node, node->EvaluateGlobalTransform());

	for (int i = 0; i < bone->Children.size(); i++) {
		AddBoneToScene(bone->Children[i], bindPose);
	}
}

void DBConverter::AddPartToScene(TTPart* part, FbxNode* parent) {
	// Create a mesh object

	std::string partName = std::string("Part_" + std::to_string(part->MeshGroup->MeshId) + "." + std::to_string(part->PartId));
	FbxMesh* mesh = FbxMesh::Create(manager, std::string(partName + " Mesh Attribute").c_str());

	// Set the mesh as the node attribute of the node
	FbxNode* node = FbxNode::Create(manager, partName.c_str());
	part->Node = node;
	parent->AddChild(node);
	node->SetNodeAttribute(mesh);

	// set the shading mode to view texture
	node->SetShadingMode(FbxNode::eTextureShading);

	mesh->InitControlPoints(part->Vertices.size());
	mesh->InitNormals(part->Vertices.size());

	FbxGeometryElementVertexColor* colorElement = mesh->CreateElementVertexColor();
	colorElement->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);

	FbxGeometryElementUV* uvElement = mesh->CreateElementUV("set");
	uvElement->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);

	FbxVector4* cps = mesh->GetControlPoints();

	for (int i = 0; i < part->Vertices.size(); i++) {
		TTVertex v = part->Vertices[i];

		mesh->SetControlPointAt(v.Position, v.Normal, i);
		uvElement->GetDirectArray().Add(FbxVector2(v.UV1[0], v.UV1[1]));
		colorElement->GetDirectArray().Add(v.VertexColor);
	}

	for (int i = 0; i < part->Indices.size(); i += 3) {
		mesh->BeginPolygon();
		mesh->AddPolygon(part->Indices[i]);
		mesh->AddPolygon(part->Indices[i + 1]);
		mesh->AddPolygon(part->Indices[i + 2]);
		mesh->EndPolygon();
	}


	// Add the skin element.
	FbxSkin* skin = FbxSkin::Create(scene, std::string(partName + " Skin Attribute").c_str());
	auto typeName = skin->GetTypeName();
	auto defType = skin->GetDeformerType();

	// Setting this to eBlend will crash 3DS Max on loading the file.
	skin->SetSkinningType(FbxSkin::eLinear);

	mesh->AddDeformer(skin);

	// For every possible bone
	for (int bi = 0; bi < part->MeshGroup->Bones.size(); bi++) {
		auto boneName = part->MeshGroup->Bones[bi];
		TTBone* bone = ttModel->GetBone(boneName);

		// Create a cluster for it.
		FbxCluster* cluster = FbxCluster::Create(scene, std::string(partName + " " + boneName + " Cluster").c_str());

		// Link and Link Mode.
		cluster->SetLink(bone->Node);
		cluster->SetLinkMode(FbxCluster::ELinkMode::eNormalize);

		// Set Matrices
		cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
		cluster->SetTransformLinkMatrix(bone->Node->EvaluateGlobalTransform());


		// For every Vertex.
		for (int vi = 0; vi < mesh->GetControlPointsCount(); vi++) {
			auto v = part->Vertices[vi];

			// For every weight set.
			for (int wi = 0; wi < 4; wi++) {
				auto set = v.WeightSet.Weights[wi];

				// If we're the same bone and we have weight.
				if (set.BoneId == bi && set.Weight > 0) {

					// Add the weight to this bone's weight list.
					cluster->AddControlPointIndex(vi, set.Weight);
				}
			}
		}

		// Add the cluster if we have any weights.
		if (cluster->GetControlPointIndicesCount() > 0) {
			skin->AddCluster(cluster);
			assert(cluster->GetLink() == bone->Node);
			assert(cluster->GetSubDeformerType() == FbxSubDeformer::eCluster);
			
		}
		else {
			cluster->Destroy();
		}
	}

	// Add the part node to the bind pose.
		// Probably overkill.
	FbxPose* pose = scene->GetPose(0);
	FbxMatrix bind = node->EvaluateGlobalTransform();
	pose->Add(node, bind);

}

void DBConverter::ExportScene() {

	// Create an IOSettings object.
	auto ios = (manager->GetIOSettings());

	// ... Configure the FbxIOSettings object here ...

	ios->SetBoolProp(EXP_FBX_MATERIAL, true);
	ios->SetBoolProp(EXP_FBX_TEXTURE, true);
	ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
	ios->SetBoolProp(EXP_FBX_SHAPE, true);
	ios->SetBoolProp(EXP_FBX_GOBO, true);
	ios->SetBoolProp(EXP_FBX_ANIMATION, true);
	ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);


	int lMajor, lMinor, lRevision;
	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

	// Create an exporter.
	FbxExporter* exporter = FbxExporter::Create(manager, "");
	//exporter->SetFileExportVersion(FBX_2010_00_COMPATIBLE);

	// Declare the path and filename of the file to which the scene will be exported.
	// In this case, the file will be in the same directory as the executable.
	const char* lFilename = "result.fbx";

	// Write in fall back format in less no ASCII format found
	auto pFileFormat = -1;

	//Try to export in ASCII if possible
	/*int lFormatIndex, lFormatCount = manager->GetIOPluginRegistry()->GetWriterFormatCount();

	for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
	{
		if (manager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
		{
			FbxString lDesc = manager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
			const char* lASCII = "ascii";
			if (lDesc.Find(lASCII) >= 0)
			{
				pFileFormat = lFormatIndex;
				break;
			}
		}
	}*/
	ios->SetBoolProp(EXP_FBX_ANIMATION, true);
	// GetIOSettings().SetBoolProp(EXP_FBX_ANIMATION, true);

	// Initialize the exporter.
	manager->SetIOSettings(ios);
	bool exportStatus = exporter->Initialize(lFilename, pFileFormat, ios);
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
