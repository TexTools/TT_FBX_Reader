#pragma once

#include <fbxsdk.h>
#include <string>
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