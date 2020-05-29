#pragma once

#include<GA/GA_ATIBlob.h>
#include<GA/GA_BlobData.h>

#include <unordered_map>

class SparseData : public GA_BlobData {
public:
    SparseData() {}
    virtual ~SparseData() {}

    // Implementation of methods on GA_Blob
    uint hash() const override {return weights.size();}
    bool isEqual(const GA_BlobData &blob) const override{
        const SparseData *s;
        s = dynamic_cast<const SparseData*>(&blob);
        return s && (s->weights == weights);
    }

    int64 getMemoryUsage(bool inclusive) const override {return 1;}  // TODO
    void countMemory(UT_MemoryCounter&, bool) const override {} // TODO
    
    void set_weight(int index, float weight){
        weights[index] = weight;
    }
    // inline float get_weight(int index){
    //     weights[index] = weight;
    // }

    // TODO: make private once we figute interface
    // cage index point and weight 
    std::unordered_map<int, float> weights;

private:
};
