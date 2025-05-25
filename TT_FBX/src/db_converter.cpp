#include <db_converter.h>

bool _UseColor2Channel = true;
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
int DBConverter::Init(std::wstring dbFilePath) {

	char* zErrMsg = 0;
	int rc;
	fprintf(stdout, "Attempting to process DB File: %s\n", dbFilePath);

	// Create and connect to the database file.
	rc = sqlite3_open16(dbFilePath.c_str(), &db);
	if (rc) {
		fprintf(stderr, "Failed to connect to database: %ls\n", sqlite3_errmsg(db));
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
		if (bone->ParentName == "") {
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
		if (bone->ParentName == root->Name) {
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

	// Meta Values
	sqlite3_stmt* query = MakeSqlStatement("select key, value name from meta");
	while (GetRow(query)) {
		std::string key = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 0)));
		std::string value = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 1)));
		
		if (value == "") {
			continue;
		}

		if (key == "unit") {
			ttModel->Units = value;
		}
		else if (key == "name") {
			// Use the old name field if we have one.
			ttModel->RootName = value;
		}
		else if (key == "root_name") {
			ttModel->RootName = value;
		}
		else if (key == "up") {
			ttModel->Up = value[0];
		}
		else if (key == "front") {
			ttModel->Front = value[0];
		}
		else if (key == "handedness") {
			ttModel->Handedness = value[0];
		}
		else if (key == "version") {
			ttModel->Version = value;
		}
		else if (key == "application") {
			ttModel->Application = value;
		}
		else if (key == "for_3ds_max") {
			_UseColor2Channel = value == "1" ? false : true;
		}
	}
	sqlite3_finalize(query);

	// Models (Really just model names for now)
	query = MakeSqlStatement("select model, name from models");
	while (GetRow(query)) {
		int ModelNameId = sqlite3_column_int(query, 0);

		std::string name = "";
		auto s = (char*)sqlite3_column_text(query, 1);
		if (s != NULL) {
			name = std::string(s);
		}

		while (ModelNameId >= ttModel->ModelNames.size()) {
			// Root name is used as default.
			ttModel->ModelNames.push_back(ttModel->RootName);
		}

		ttModel->ModelNames[ModelNameId] = name;
	}

	// Meshes
	query = MakeSqlStatement("select mesh, name, material_id, model from meshes");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);

		std::string name = "";
		auto s = (char*)sqlite3_column_text(query, 1);
		if (s != NULL) {
			name = std::string(s);
		}

		int materialId = sqlite3_column_int(query, 2);
		int modelId = sqlite3_column_int(query, 3);

		// Safety fallback.
		while (modelId >= ttModel->ModelNames.size()) {
			// Root name is used as default.
			ttModel->ModelNames.push_back(ttModel->RootName);
		}

		// Create mesh groups as needed.
		while (meshId >= ttModel->MeshGroups.size()) {
			ttModel->MeshGroups.push_back(new TTMeshGroup());
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->Model = ttModel;
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->MeshId = ttModel->MeshGroups.size() - 1;
		}
		ttModel->MeshGroups[meshId]->MaterialId = materialId;
		ttModel->MeshGroups[meshId]->ModelNameId = modelId;
		ttModel->MeshGroups[meshId]->Name = name;
	}
	sqlite3_finalize(query);

	// Meshes and Parts
	query = MakeSqlStatement("select mesh, part, name from parts");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int partId = sqlite3_column_int(query, 1);

		std::string name = "";
		auto s = (char*)sqlite3_column_text(query, 2);
		if (s != NULL) {
			name = std::string(s);
		}

		// Create mesh groups as needed.
		while (meshId >= ttModel->MeshGroups.size()) {
			ttModel->MeshGroups.push_back(new TTMeshGroup());
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->Model = ttModel;
			ttModel->MeshGroups[ttModel->MeshGroups.size() - 1]->MeshId = ttModel->MeshGroups.size() - 1;
		}

		// Create parts as needed.
		while (partId >= ttModel->MeshGroups[meshId]->Parts.size()) {
			ttModel->MeshGroups[meshId]->Parts.push_back(new TTPart());
			TTPart* part = ttModel->MeshGroups[meshId]->Parts[ttModel->MeshGroups[meshId]->Parts.size() - 1];
			part->MeshGroup = ttModel->MeshGroups[meshId];
			part->PartId = ttModel->MeshGroups[meshId]->Parts.size() - 1;
		}

		ttModel->MeshGroups[meshId]->Parts[partId]->Name = name;
	}
	sqlite3_finalize(query);

	// Materials
	query = MakeSqlStatement("select material_id, diffuse, normal, specular, opacity, emissive, name from materials order by material_id asc");
	int matId = 0;
	while (GetRow(query)) {

		int material_id = sqlite3_column_int(query, 0);
		while (ttModel->Materials.size() < material_id + 1) {
			ttModel->Materials.push_back(new TTMaterial);
		}

		// We don't actually care if any of these fail particularly.
		auto s = (char*)sqlite3_column_text(query, 1);
		if (s != NULL) {
			ttModel->Materials[material_id]->Diffuse = std::string(s);
		}
		s = (char*)sqlite3_column_text(query, 2);
		if (s != NULL) {
			ttModel->Materials[material_id]->Normal = std::string(s);
		}
		s = (char*)sqlite3_column_text(query, 3);
		if (s != NULL) {
			ttModel->Materials[material_id]->Specular = std::string(s);
		}
		s = (char*)sqlite3_column_text(query, 4);
		if (s != NULL) {
			ttModel->Materials[material_id]->Opacity = std::string(s);
		}
		s = (char*)sqlite3_column_text(query, 5);
		if (s != NULL) {
			ttModel->Materials[material_id]->Emissive = std::string(s);
		}
		s = (char*)sqlite3_column_text(query, 6);
		if (s != NULL) {
			ttModel->Materials[material_id]->Name = std::string(s);
		}
		else {
			ttModel->Materials[material_id]->Name = std::string("Material " + std::to_string(matId));
		}
		matId++;
	}
	sqlite3_finalize(query);

	// Skeleton
	query = MakeSqlStatement("select name, parent, matrix_0, matrix_1, matrix_2, matrix_3, matrix_4, matrix_5, matrix_6, matrix_7, matrix_8, matrix_9, matrix_10, matrix_11, matrix_12, matrix_13, matrix_14, matrix_15 from skeleton order by name asc");
	while (GetRow(query)) {
		TTBone* bone = new TTBone();

		bone->Name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 0)));

		auto s = (char*)sqlite3_column_text(query, 1);
		if (s != NULL) {
			 bone->ParentName = std::string(s);
		}
		else {
			bone->ParentName = "";
		}

		Eigen::Transform<double, 3, Eigen::Affine> matrix;
		Eigen::Matrix4d baseMatrix = matrix.matrix();

		baseMatrix(0, 0) = sqlite3_column_double(query, 2);
		baseMatrix(0, 1) = sqlite3_column_double(query, 3);
		baseMatrix(0, 2) = sqlite3_column_double(query, 4);
		baseMatrix(0, 3) = sqlite3_column_double(query, 5);

		baseMatrix(1, 0) = sqlite3_column_double(query, 6);
		baseMatrix(1, 1) = sqlite3_column_double(query, 7);
		baseMatrix(1, 2) = sqlite3_column_double(query, 8);
		baseMatrix(1, 3) = sqlite3_column_double(query, 9);

		baseMatrix(2, 0) = sqlite3_column_double(query, 10);
		baseMatrix(2, 1) = sqlite3_column_double(query, 11);
		baseMatrix(2, 2) = sqlite3_column_double(query, 12);
		baseMatrix(2, 3) = sqlite3_column_double(query, 13);

		baseMatrix(3, 0) = sqlite3_column_double(query, 14);
		baseMatrix(3, 1) = sqlite3_column_double(query, 15);
		baseMatrix(3, 2) = sqlite3_column_double(query, 16);
		baseMatrix(3, 3) = sqlite3_column_double(query, 17);

		matrix.matrix() = baseMatrix;
		bone->PoseMatrix = matrix;

		bones.push_back(bone);
	}

	// Bones
	query = MakeSqlStatement("select mesh, bone_id, name from bones order by mesh asc, bone_id asc");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int boneId = sqlite3_column_int(query, 1);
		std::string name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 2)));

		// Fill in missing mesh groups (This shouldn't really ever happen, but safety)
		while (meshId >= ttModel->MeshGroups.size()) {
			ttModel->MeshGroups.push_back(new TTMeshGroup());
		}

		// Fill in missing bones as needed in case we read them out of order.
		while (boneId >= ttModel->MeshGroups[meshId]->Bones.size()) {
			ttModel->MeshGroups[meshId]->Bones.push_back("");
		}

		ttModel->MeshGroups[meshId]->Bones[boneId] = name;
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


	query = MakeSqlStatement("select mesh, part, vertex_id, position_x, position_y, position_z, normal_x, normal_y, normal_z, color_r, color_g, color_b, color_a, color2_r, color2_g, color2_b, color2_a, uv_1_u, uv_1_v, uv_2_u, uv_2_v, bone_1_id, bone_1_weight, bone_2_id, bone_2_weight, bone_3_id, bone_3_weight, bone_4_id, bone_4_weight, bone_5_id, bone_5_weight, bone_6_id, bone_6_weight, bone_7_id, bone_7_weight, bone_8_id, bone_8_weight, binormal_x, binormal_y, binormal_z, tangent_x, tangent_y, tangent_z, uv_3_u, uv_3_v, flow_u, flow_v from vertices order by mesh asc, part asc, vertex_id asc");
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

		v.VertexColor2[0] = sqlite3_column_double(query, 13);
		v.VertexColor2[1] = sqlite3_column_double(query, 14);
		v.VertexColor2[2] = sqlite3_column_double(query, 15);
		v.VertexColor2[3] = sqlite3_column_double(query, 16);

		v.UV1[0] = sqlite3_column_double(query, 17);
		v.UV1[1] = sqlite3_column_double(query, 18);

		v.UV2[0] = sqlite3_column_double(query, 19);
		v.UV2[1] = sqlite3_column_double(query, 20);

		v.WeightSet.Weights[0].BoneId = sqlite3_column_int(query, 21);
		v.WeightSet.Weights[0].Weight = sqlite3_column_double(query, 22);

		v.WeightSet.Weights[1].BoneId = sqlite3_column_int(query, 23);
		v.WeightSet.Weights[1].Weight = sqlite3_column_double(query, 24);

		v.WeightSet.Weights[2].BoneId = sqlite3_column_int(query, 25);
		v.WeightSet.Weights[2].Weight = sqlite3_column_double(query, 26);

		v.WeightSet.Weights[3].BoneId = sqlite3_column_int(query, 27);
		v.WeightSet.Weights[3].Weight = sqlite3_column_double(query, 28);

		v.WeightSet.Weights[4].BoneId = sqlite3_column_int(query, 29);
		v.WeightSet.Weights[4].Weight = sqlite3_column_double(query, 30);

		v.WeightSet.Weights[5].BoneId = sqlite3_column_int(query, 31);
		v.WeightSet.Weights[5].Weight = sqlite3_column_double(query, 32);

		v.WeightSet.Weights[6].BoneId = sqlite3_column_int(query, 33);
		v.WeightSet.Weights[6].Weight = sqlite3_column_double(query, 34);

		v.WeightSet.Weights[7].BoneId = sqlite3_column_int(query, 35);
		v.WeightSet.Weights[7].Weight = sqlite3_column_double(query, 36);

		v.Binormal[0] = sqlite3_column_double(query, 37);
		v.Binormal[1] = sqlite3_column_double(query, 38);
		v.Binormal[2] = sqlite3_column_double(query, 39);

		v.Tangent[0] = sqlite3_column_double(query, 40);
		v.Tangent[1] = sqlite3_column_double(query, 41);
		v.Tangent[2] = sqlite3_column_double(query, 42);

		v.UV3[0] = sqlite3_column_double(query, 43);
		v.UV3[1] = sqlite3_column_double(query, 44);

		v.VertexColor3[0] = (sqlite3_column_double(query, 45) + 1) / 2.0f;
		v.VertexColor3[1] = (sqlite3_column_double(query, 46) + 1) / 2.0f;
		v.VertexColor3[2] = 1;
		v.VertexColor3[3] = 1;


		ttModel->MeshGroups[meshId]->Parts[partId]->Vertices.push_back(v);
	}
	sqlite3_finalize(query);

	query = MakeSqlStatement("select mesh, part, shape, vertex_id, position_x, position_y, position_z from shape_vertices order by mesh, part, shape, vertex_id");
	while (GetRow(query)) {
		int meshId = sqlite3_column_int(query, 0);
		int partId = sqlite3_column_int(query, 1);
		std::string name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(query, 2)));
		int vertexId = sqlite3_column_int(query, 3);

		auto part = ttModel->MeshGroups[meshId]->Parts[partId];
		if (part->Shapes.count(name) == 0) {
			auto shp = new TTShapePart();
			shp->Name = name;
			part->Shapes.insert({ name, shp });
		}

		auto shape = part->Shapes[name];

		// Assign new position data.
		TTVertex v;
		v.Position[0] = sqlite3_column_double(query, 4);
		v.Position[1] = sqlite3_column_double(query, 5);
		v.Position[2] = sqlite3_column_double(query, 6);

		auto rep = part->Vertices[vertexId];

		// Copy over other values for convenience.
		v.Normal = rep.Normal;
		v.Binormal = rep.Binormal;
		v.Tangent = rep.Tangent;
		v.VertexColor = rep.VertexColor;
		v.VertexColor2 = rep.VertexColor2;
		v.VertexColor3 = rep.VertexColor3;
		v.UV1 = rep.UV1;
		v.UV2 = rep.UV2;
		v.UV3 = rep.UV3;

		for (int i = 0; i < _TTW_Max_Weights; i++) {
			v.WeightSet.Weights[i] = rep.WeightSet.Weights[i];
			if (i < 4) {
				v.VertexColor[i] = rep.VertexColor[i];
				v.VertexColor2[i] = rep.VertexColor2[i];
				v.VertexColor3[i] = rep.VertexColor3[i];
			}
		}

		shape->VertexReplacements.insert({vertexId, v});
	}


	int z = 0;
	z++;

}

// Create the FBX scene.
void DBConverter::CreateScene() {
	scene = FbxScene::Create(manager, "TexTools FBX Export");
	FbxNode* root = scene->GetRootNode();

	// FFXIV stuff comes in Y Up, Z Front, Right Handed, for reference.
	auto up = FbxAxisSystem::EUpVector::eYAxis;
	auto front = FbxAxisSystem::EFrontVector::eParityOdd;
	auto handedness = FbxAxisSystem::eRightHanded;

	if (ttModel->Up == 'z') {

		// Z UP
		FbxAxisSystem::EUpVector::eZAxis;
		if (ttModel->Front == 'x') {
			front = FbxAxisSystem::EFrontVector::eParityEven;
		}
		else {
			front = FbxAxisSystem::EFrontVector::eParityOdd;
		}
	}
	else if(ttModel->Up == 'x') {
		// X UP
		FbxAxisSystem::EUpVector::eXAxis;
		if (ttModel->Front == 'y') {
			front = FbxAxisSystem::EFrontVector::eParityEven;
		}
		else {
			front = FbxAxisSystem::EFrontVector::eParityOdd;
		}
	}
	else {
		// Y UP
		FbxAxisSystem::EUpVector::eYAxis;
		if (ttModel->Front == 'x') {
			front = FbxAxisSystem::EFrontVector::eParityEven;
		}
		else {
			front = FbxAxisSystem::EFrontVector::eParityOdd;
		}
	}

	if (ttModel->Handedness == 'l') {
		handedness = FbxAxisSystem::eLeftHanded;
	}
	else {
		handedness = FbxAxisSystem::eRightHanded;
	}

	FbxAxisSystem dbAxis(up, front, handedness);
	dbAxis.ConvertScene(scene);

	// FFXIV uses Meters for internal units.
	scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);

	FbxNode* firstNode = FbxNode::Create(manager, ttModel->RootName.c_str());
	FbxDouble3 rootScale = firstNode->LclScaling.Get();
	root->AddChild(firstNode);
	ttModel->Node = firstNode;

	FbxPose* pose = FbxPose::Create(manager, "Bindpose");
	pose->SetIsBindPose(true);
	AddBoneToScene(ttModel->FullSkeleton, pose);
	scene->AddPose(pose);

	CreateMaterials();

	std::vector<FbxNode*> meshGroupNodes;
	for (int i = 0; i < ttModel->MeshGroups.size(); i++) {
		// Mesh group headings derive their names from their parent model.
		std::string modelName = ttModel->ModelNames[ttModel->MeshGroups[i]->ModelNameId];
		FbxNode* node = FbxNode::Create(manager, std::string(modelName + " Group " + std::to_string(i)).c_str());
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

void DBConverter::CreateMaterials() {

	int i;
	
	for (int i = 0; i < ttModel->Materials.size(); i++)
	{
		auto* mat = ttModel->Materials[i];
		FbxString lMaterialName = mat->Name.c_str();
		FbxDouble3 white = FbxDouble3(1.0f, 1.0f, 1.0f);
		FbxDouble3 black = FbxDouble3(0.f, 0.f, 0.f);
		FbxSurfacePhong* lMaterial = FbxSurfacePhong::Create(scene, lMaterialName.Buffer());


		// Generate primary and secondary colors.
		lMaterial->Emissive.Set(black);
		lMaterial->Diffuse.Set(white);
		lMaterial->TransparencyFactor.Set(0.0);
		lMaterial->ShadingModel.Set("Phong");
		lMaterial->Shininess.Set(0.5);

		if (mat->Diffuse != "") {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, std::string(mat->Name + " Diffuse").c_str());
			texture->SetFileName(mat->Diffuse.c_str()); // Resource file is in current directory.
			texture->SetTextureUse(FbxTexture::eStandard);
			texture->SetMappingType(FbxTexture::eUV);
			texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			texture->Alpha.Set(1.0);
			lMaterial->Diffuse.ConnectSrcObject(texture);
		}

		if (mat->Specular != "") {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, std::string(mat->Name + " Specular").c_str());
			texture->SetFileName(mat->Specular.c_str()); // Resource file is in current directory.
			texture->SetTextureUse(FbxTexture::eStandard);
			texture->SetMappingType(FbxTexture::eUV);
			texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			texture->Alpha.Set(1.0);
			lMaterial->Specular.ConnectSrcObject(texture);
		}

		if (mat->Normal != "") {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, std::string(mat->Name + " Normal").c_str());
			texture->SetFileName(mat->Normal.c_str()); // Resource file is in current directory.
			texture->SetTextureUse(FbxTexture::eStandard);
			texture->SetMappingType(FbxTexture::eUV);
			texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			texture->Alpha.Set(1.0);
			lMaterial->NormalMap.ConnectSrcObject(texture);
		}

		if (mat->Emissive != "") {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, std::string(mat->Name + " Emissive").c_str());
			texture->SetFileName(mat->Emissive.c_str()); // Resource file is in current directory.
			texture->SetTextureUse(FbxTexture::eStandard);
			texture->SetMappingType(FbxTexture::eUV);
			texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			texture->Alpha.Set(1.0);
			lMaterial->Emissive.ConnectSrcObject(texture);
		}

		if (mat->Opacity != "") {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, std::string(mat->Name + " Opacity").c_str());
			texture->SetFileName(mat->Opacity.c_str()); // Resource file is in current directory.
			texture->SetTextureUse(FbxTexture::eStandard);
			texture->SetMappingType(FbxTexture::eUV);
			texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
			texture->Alpha.Set(1.0);
			lMaterial->TransparentColor.ConnectSrcObject(texture);
			lMaterial->TransparencyFactor.ConnectSrcObject(texture);
		}
		else {
			lMaterial->TransparentColor.Set(FbxDouble3(1.0f, 1.0f, 1.0f));
			lMaterial->TransparencyFactor.Set(1.0f);
		}


		mat->Material = lMaterial;
	}
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
	if (bone == NULL) return;

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

FbxMesh* DBConverter::MakeMesh(std::vector<TTVertex> vertices, std::vector<int> indices, std::string meshName, FbxNode* parent, FbxSurfaceMaterial* material) {
	
	FbxMesh* mesh = FbxMesh::Create(manager, meshName.c_str());
	// set the shading mode to view texture
	parent->SetShadingMode(FbxNode::eTextureShading);

	parent->AddMaterial(material);
	parent->SetNodeAttribute(mesh);
	FbxGeometryElementMaterial* lMaterialElement = mesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(FbxGeometryElement::eAllSame);


	mesh->InitControlPoints(vertices.size());
	mesh->InitNormals(vertices.size());
	mesh->InitBinormals(vertices.size());
	mesh->InitTangents(vertices.size());

	//binEl[0] 

	// LAYER 0
	auto* layer0 = mesh->GetLayer(0);

	// Layer 1
	auto* layer1 = mesh->GetLayer(mesh->CreateLayer());

	// Layer 2
	auto* layer2 = mesh->GetLayer(mesh->CreateLayer());

	// Layer 3
	auto* layer3 = mesh->GetLayer(mesh->CreateLayer());

	// Layer 3
	auto* layer4 = mesh->GetLayer(mesh->CreateLayer());

	auto* colorLayer = FbxLayerElementVertexColor::Create(mesh, "vc0");
	colorLayer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer0->SetVertexColors(colorLayer);

	auto* uvLayer = FbxLayerElementUV::Create(mesh, "uv0");
	uvLayer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer0->SetUVs(uvLayer);

	auto* uv2Layer = FbxLayerElementUV::Create(mesh, "uv1");
	uv2Layer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer1->SetUVs(uv2Layer);

	auto* uv3Layer = FbxLayerElementUV::Create(mesh, "uv2");
	uv3Layer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer2->SetUVs(uv3Layer);


	FbxLayerElementVertexColor* color2Layer = 0;
	FbxLayerElementVertexColor* color3Layer = 0;
	FbxLayerElementUV* color2Layer_rg = 0;
	FbxLayerElementUV* color2Layer_ba = 0;
	if (_UseColor2Channel) {
		color2Layer = FbxLayerElementVertexColor::Create(mesh, "vc1");
		color2Layer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
		layer1->SetVertexColors(color2Layer);

		color3Layer = FbxLayerElementVertexColor::Create(mesh, "vc2");
		color3Layer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
		layer2->SetVertexColors(color3Layer);
	}

	auto* binormalLayer = FbxLayerElementBinormal::Create(mesh, "bn");
	binormalLayer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer0->SetBinormals(binormalLayer);

	auto* tangentLayer = FbxLayerElementTangent::Create(mesh, "bn");
	tangentLayer->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
	layer0->SetTangents(tangentLayer);

	for (int i = 0; i < vertices.size(); i++) {
		TTVertex v = vertices[i];

		mesh->SetControlPointAt(v.Position, v.Normal, i);
		binormalLayer->GetDirectArray().Add(v.Binormal);
		tangentLayer->GetDirectArray().Add(v.Tangent);
		colorLayer->GetDirectArray().Add(v.VertexColor);
		uvLayer->GetDirectArray().Add(FbxVector2(v.UV1[0], v.UV1[1]));
		uv2Layer->GetDirectArray().Add(FbxVector2(v.UV2[0], v.UV2[1]));
		uv3Layer->GetDirectArray().Add(FbxVector2(v.UV3[0], v.UV3[1]));

		if (_UseColor2Channel) {
			color2Layer->GetDirectArray().Add(v.VertexColor2);
			color3Layer->GetDirectArray().Add(v.VertexColor3);
		}
	}

	for (int i = 0; i < indices.size(); i += 3) {
		mesh->BeginPolygon();
		mesh->AddPolygon(indices[i]);
		mesh->AddPolygon(indices[i + 1]);
		mesh->AddPolygon(indices[i + 2]);
		mesh->EndPolygon();

		TTVertex vert1 = vertices[indices[i]];
		TTVertex vert2 = vertices[indices[i + 1]];
		TTVertex vert3 = vertices[indices[i + 2]];


	}

	return mesh;
}

FbxShape* DBConverter::MakeShape(std::vector<TTVertex> vertices, std::string meshName) {
	FbxShape* shapeMesh = FbxShape::Create(manager, meshName.c_str());

	shapeMesh->InitControlPoints(vertices.size());
	shapeMesh->InitNormals(vertices.size());

	shapeMesh->InitControlPoints(vertices.size());
	shapeMesh->InitNormals(vertices.size());

	FbxVector4* cps = shapeMesh->GetControlPoints();

	for (int i = 0; i < vertices.size(); i++) {
		TTVertex v = vertices[i];

		shapeMesh->SetControlPointAt(v.Position, v.Normal, i);
	}

	return shapeMesh;
}


void DBConverter::AddPartToScene(TTPart* part, FbxNode* parent) {
	// Create a mesh object

	std::string modelName = ttModel->ModelNames[part->MeshGroup->ModelNameId];
	std::string partName = std::string(modelName + " Part " + std::to_string(part->MeshGroup->MeshId) + "." + std::to_string(part->PartId));


	// Set the mesh as the node attribute of the node
	FbxNode* node = FbxNode::Create(manager, partName.c_str());
	part->Node = node;
	parent->AddChild(node);


	auto material = ttModel->Materials[part->MeshGroup->MaterialId]->Material;
	auto mesh = MakeMesh(part->Vertices, part->Indices, std::string(partName + " Mesh Attribute"), node, material);
	

	if (part->Shapes.size() > 0) {
		// Add the morpher element.
		auto blendShape = FbxBlendShape::Create(scene, std::string(partName + " Blend Shapes").c_str());
		mesh->AddDeformer(blendShape);

		for (auto it = part->Shapes.begin(); it != part->Shapes.end(); ++it)
		{
			auto shape = it->second;
			auto channel = FbxBlendShapeChannel::Create(blendShape, std::string("channel_" + shape->Name).c_str());

			// Need to make our modified vertex array here.
			std::vector<TTVertex> shapeVertices;

			for (int i = 0; i < part->Vertices.size(); i++) {
				auto it = shape->VertexReplacements.find(i);
				if (it != shape->VertexReplacements.end()) {
					auto vert = it->second;
					shapeVertices.push_back(vert);
				}
				else {
					shapeVertices.push_back(part->Vertices[i]);
				}
			}

			auto shapeMesh = MakeShape(shapeVertices, shape->Name);
			channel->SetMultiLayer(false);
			channel->AddTargetShape(shapeMesh);
		}

		//RepopulateMesh(mesh, part->Vertices, part->Indices, std::string(partName + " Mesh Attribute"), node, material);
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
		if (bone == NULL) continue;

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
			for (int wi = 0; wi < _TTW_Max_Weights; wi++) {
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

	// Convert the scene format to the incorrect Legacy method.


	// Create an IOSettings object.
	auto ios = (manager->GetIOSettings());

	// ... Configure the FbxIOSettings object here ...
	ios->SetBoolProp(EXP_FBX_MATERIAL, true);
	ios->SetBoolProp(EXP_FBX_TEXTURE, true);
	ios->SetBoolProp(EXP_FBX_EMBEDDED, true);
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

	// File is always piped to this directory currently.
	const char* lFilename = "result.fbx";

	ios->SetBoolProp(EXP_FBX_ANIMATION, true);

	// Initialize the exporter.
	manager->SetIOSettings(ios);
	bool exportStatus = exporter->Initialize(lFilename, -1, ios);
	if (!exportStatus) {
		printf("Call to FbxExporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
		Shutdown(800);
	}


	exporter->Export(scene);

	// Destroy the exporter.
	exporter->Destroy();
}

int DBConverter::ConvertDB(std::wstring dbFile) {
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
