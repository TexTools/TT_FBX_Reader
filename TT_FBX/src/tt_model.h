#pragma once

#include <fbxsdk.h>
#include <string>

#include <eigen>
#define _TTW_Max_Weights 8

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
        int usedWeights = 0;
        for (int i = 0; i < _TTW_Max_Weights; i++) {
            if (Weights[i].BoneId == -1) {
                idx = i;
                break;
            }
            usedWeights++;
        }

        // We are already at max weights.  We must decide if one should be replaced.
        if (usedWeights == 4) {

            int minWeightIdx = -1;
            double minWeight = 999;

            for (int i = 0; i < _TTW_Max_Weights; i++) {
                if (Weights[i].Weight < minWeight || minWeightIdx == -1) {
                    minWeight = Weights[i].Weight;
                    minWeightIdx = i;
                }
            }

            // Our new weight is heavier than the lightest weight.  Replace the lightest.
            if (weight > minWeight) {
                idx = minWeightIdx;
            }
        }

        if (idx < 0) {
            return;
        }

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
    FbxVector4 Binormal;
    FbxVector4 Tangent;
    FbxVector2 UV1;
    FbxVector2 UV2;
    FbxVector2 UV3;
    FbxColor VertexColor;
    FbxColor VertexColor2;
    FbxColor VertexColor3;
    TTWeightSet WeightSet;

    int UV1Index;
    int UV2Index;
    int UV3Index;


    // Overload equality to test memberwise.
    friend bool operator== (const TTVertex& a, const TTVertex& b) {
        if (a.Position != b.Position) return false;
        if (a.Normal != b.Normal) return false;
        if (a.Binormal != b.Binormal) return false;
        if (a.Tangent != b.Tangent) return false;
        if (a.UV1 != b.UV1) return false;
        if (a.UV2 != b.UV2) return false;
        if (a.UV3 != b.UV3) return false;
        if (a.VertexColor != b.VertexColor) return false;
        if (a.VertexColor2 != b.VertexColor2) return false;
        if (a.VertexColor3 != b.VertexColor3) return false;
        if (a.WeightSet != b.WeightSet) return false;

        // Compare UVs by index as well.
        if (a.UV1Index != b.UV1Index) return false;
        if (a.UV2Index != b.UV2Index) return false;
        if (a.UV3Index != b.UV3Index) return false;



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

class TTShapePart {
public:
    std::string Name;

    // Maps original part vertex ID to new replacement vertex.
    std::map<int, TTVertex> VertexReplacements;
};

class TTPart {
public:
    std::string Name;
    int PartId;
    std::map<std::string, TTShapePart*> Shapes;
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

    // Basic hack workaround for changing child name prefixes.
    // Not sure if we'll ever need more than one TTModel object
    // in a scene, but if we do this will be changed to key to the 
    // actual TTModel entry, and a TTScene parent class for
    // TTModel should be generated.
    int ModelNameId;
};

class TTModel {
public:
    std::vector<TTMeshGroup*> MeshGroups;
    std::vector<TTMaterial*> Materials;

    // Slightly hacky model name workaround for exporting
    // multiple models that share skeletons
    std::vector<std::string> ModelNames;

    // The overall root name that should be used for FBX scene.
    std::string RootName;

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