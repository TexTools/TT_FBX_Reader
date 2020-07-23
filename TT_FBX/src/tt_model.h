#pragma once

#include <fbxsdk.h>
#include <string>

#include <eigen>
#define _TTW_Max_Weights 4

// An individual TTWeight
struct TTWeight {
    int BoneId;
    double Weight;
};

// Struct representing a set of weights on a TTVertex
struct TTWeightSet {
public:
    TTWeight Weights[_TTW_Max_Weights];

    friend bool operator==(const TTWeightSet& lhs, const TTWeightSet& rhs)
    {
        for (int i = 0; i < _TTW_Max_Weights; i++) {
            if (lhs.Weights[i].Weight != 0) {
                if (lhs.Weights[i].Weight != rhs.Weights[i].Weight) return false;
                if (lhs.Weights[i].BoneId != rhs.Weights[i].BoneId) return false;
            }
        }
        return true;
    }

    void Add(int boneId, double weight) {
        int idx = -1;
        for (int i = 0; i < _TTW_Max_Weights; i++) {
            if (Weights[i].BoneId == -1) {
                idx = i;
                break;
            }
        }

        if (idx == -1) return;

        Weights[idx].BoneId = boneId;
        Weights[idx].Weight = weight;

    }

    TTWeightSet() {
        for (int i = 0; i < _TTW_Max_Weights; i++) {
            Weights[i].BoneId = -1;
            Weights[i].Weight = 0;
        }
    }


    friend bool operator!=(const TTWeightSet& lhs, const TTWeightSet& rhs)
    {
        return !(lhs == rhs);
    }
};

// A fully qualified TT Vertex.
class TTVertex {
public:

    FbxVector4 Position;
    FbxVector4 Normal;
    FbxVector2 UV1;
    FbxVector2 UV2;
    FbxColor VertexColor;
    TTWeightSet WeightSet;


    // Overload equality to test memberwise.
    friend bool operator== (const TTVertex& a, const TTVertex& b) {
        if (a.Position != b.Position) return false;
        if (a.Normal != b.Normal) return false;
        if (a.UV1 != b.UV1) return false;
        if (a.UV2 != b.UV2) return false;
        if (a.VertexColor != b.VertexColor) return false;
        if (a.WeightSet != b.WeightSet) return false;



        return true;
    };
};

class TTMeshPart;
class TTMeshGroup;
class TTModel;

// Model Level Skeleton Information
class TTBone {
public:
    std::string Name;
    std::string ParentName;

    TTBone* Parent;
    std::vector<TTBone*> Children;

    Eigen::Transform<double, 3, Eigen::Affine> PoseMatrix;
    FbxNode* Node;
};

class TTMaterial {
public:
    std::string Name;
    std::string Normal;
    std::string Specular;
    std::string Emissive;
    std::string Opacity;
    std::string Diffuse;
    FbxSurfaceMaterial* Material;
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
    std::string Name;
    std::vector<TTPart*> Parts;
    std::vector<std::string> Bones;
    int MeshId;
    int MaterialId;
    FbxNode* Node;
    TTModel* Model;
};

class TTModel {
public:
    std::vector<TTMeshGroup*> MeshGroups;
    std::vector<TTMaterial*> Materials;
    TTBone* FullSkeleton;
    FbxNode* Node;

    // Unit information  'meter', 'inch', 'centimeter' etc.
    std::string Units;

    // Axis, ex z, y
    char Up = 'y';
    
    // Axis, ex z, y
    char Front = 'z';

    // Right(r) or Left(l)
    char Handedness = 'r';

    // Application that created the underlying DB file.
    std::string Application;

    // Application version number
    std::string Version;

    // Name of the model
    std::string Name;

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