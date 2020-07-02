#include <UT/UT_DSOVersion.h>
#include <PRM/PRM_Include.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <UT/UT_Interrupt.h>
#include <SOP/SOP_Node.h>

#include <SYS/SYS_Math.h>

#include<GEO/GEO_Vertex.h>
#include<GEO/GEO_AttributeHandle.h>  // this is deprecated anyway, but still available

#include<GA/GA_AIFBlob.h>

#include "capture_pgmvc.h"
#include "sparse_data.h"

#include <cassert>
#include <vector>
#include <numeric>  // iota

#include <tbb/tbb.h>
using namespace tbb;

void
newSopOperator( OP_OperatorTable *table )
{
    table->addOperator
        (
            new OP_Operator
            (
                "capture_pgmvc_all",             // Internal name
                "capture_pgmvc_all",             // UI name
                SOP_capture_pgmvc::myConstructor,  //
                SOP_capture_pgmvc::myTemplateList, //
                2,                             // Min Inputs
                2,                             // Max Inputs
                0                              // Local Variables
                //OP_FLAG_*                    // Additional Flag
                )

            // Examples of additional flags include, OP_FLAG_GENERATOR, OP_FLAG_UNORDERED.
            // See OP_Operator.h for more details.

            );
}

static PRM_Name names[] =
{
    PRM_Name( "thres", "Threshold" ),
    PRM_Name( "samples", "Sphere samples"),
    PRM_Name( "sparseweight", "Sparse weights"),
    PRM_Name( 0 )
};

static PRM_Range PRMsmallRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_UI, 0.01);
static PRM_Range PRMsampleRange(PRM_RANGE_RESTRICTED, 1, PRM_RANGE_UI, 16);
static PRM_Default maxsamplDefault(16,"");
static PRM_Default small(0.0001,"");
static PRM_Default zero(0,"");

PRM_Template
SOP_capture_pgmvc::myTemplateList[] =
{
    PRM_Template(PRM_FLT_J, 1, &names[0], &small, 0, &PRMsmallRange),
    PRM_Template(PRM_INT_J, 1, &names[1], &maxsamplDefault, 0, &PRMsampleRange),
    PRM_Template(PRM_TOGGLE_J, 1, &names[2], &zero, 0),
    PRM_Template(),
};

const char *
SOP_capture_pgmvc::inputLabel( unsigned int input) const
{
    switch (input) {
    case 0:
        return "Input geometry";
        break;
    case 1:
        return "Cage geometry";
        break;
    default:
        return "Something";
        break;
    }
}

//overriding constructor
OP_Node *
SOP_capture_pgmvc::myConstructor( OP_Network *net, const char *name, OP_Operator *op )
{
    return new SOP_capture_pgmvc( net, name, op );
}

SOP_capture_pgmvc::SOP_capture_pgmvc( OP_Network *net, const char *name, OP_Operator *op )
    : SOP_Node( net, name, op )
{

}

//destructor
SOP_capture_pgmvc::~SOP_capture_pgmvc() {}

//don't even disable or enable anything
unsigned
SOP_capture_pgmvc::disableParms()
{
    return 0;
}



//utility function

/// return a unit sphere of n samples
void sphereSamples(std::vector<UT_Vector3>& samples, UT_Vector3 origin = UT_Vector3(0,0,0) ){

    int numSamples = samples.size();
    assert (numSamples > 0);

    float off = 2.0 / (float) numSamples;
    float inc = 3.14159265 * (3.0 - sqrt(5.0));
    for(int i = 0; i<numSamples; ++i){

        float y = (i * off) - 1.0 + (off / 2.0);
        float r = sqrt(1.0 - y*y);
        float phi = i * inc;
        samples[i].assign( cos(phi)*r + origin.x(),
                           y + origin.y(),
                           sin(phi)*r + origin.z());

    }
}
/// just test point sphere creation
void
SOP_capture_pgmvc::showSphere(GU_Detail* gdp, std::vector<UT_Vector3> &samples){
    int numSamples = 16;
    // std::vector<UT_Vector3> samples(numSamples, UT_Vector3(0,0,0));
    UT_BoundingBox bbox;
    cage->getBBox(&bbox);
    cageCenter.assign(bbox.centerX(), bbox.centerY(), bbox.centerZ());
    sphereSamples(samples, cageCenter);

    gdp->clear();

    // add  normal attribute
    GA_RWHandleV3 n_h(gdp, GA_ATTRIB_POINT, "N");  // ReadWrite attribute
    if (!n_h.isValid()){
        n_h = GA_RWHandleV3(gdp->addFloatTuple(GA_ATTRIB_POINT,  GEO_STD_ATTRIB_NORMAL, 3));
    }


    GA_Offset off;
    for (auto &sample : samples){
        off = gdp->appendPoint();
        gdp->setPos3(off, sample);

        // calculate normal
        auto n = sample - cageCenter;
        n_h.set(off,n);
    }
}

void
SOP_capture_pgmvc::F(UT_Vector3 hitP, const GEO_Primitive *prim, float distScale, std::vector<float> &weights)
{

    // get cageVets attribute from input 1
    short cageNumPrims = cage->getNumPrimitives();
    short vertsCount = prim->getVertexCount();

    // create list same as number of cage vers
    weights.resize(vertsCount, 0.0);

    // create temp veritices arrays
    std::vector<UT_Vector3> sdists(vertsCount); // undefined at this point;
    std::vector<float> ri(vertsCount);

    // 1. check if hit the vertex
    for (auto i=0; i<vertsCount; ++i){
        GA_Offset off = prim->getPointOffset(i);
        auto vertPos = cage->getPos3(off);    // <-------from cage

        UT_Vector3 val = vertPos - hitP;
        if (val.length() < tolerance){
            val = UT_Vector3(0,0,0);
        }
        sdists[i] = val;
        ri[i] = sdists[i].length();
        if (ri[i] < tolerance){
            weights[i] = distScale;
            ri[i] = 0;
            return;
        }
    }

    // 2. check if hit the edge

    std::vector<float> Ai(vertsCount);
    std::vector<float> Di(vertsCount);
    for (auto i=0; i<vertsCount; ++i){
        // GA_Offset off = prim->getPointOffset(i);
        auto next_idx = (i+1) % vertsCount;

        // GA_Offset off_next = prim->getPointOffset( next_idx );
        // auto vertPos = gdp->getPos3(off);
        // auto nextVertPos = gdp->getPos3(off_next);

        Ai[i] = (cross(sdists[i], sdists[next_idx]).length()) / 2.0;
        Di[i] = dot(sdists[i], sdists[next_idx]);

        if ( (Ai[i] < tolerance) && (Di[i] < 0.0) ){
            weights[i] = (ri[next_idx] / (ri[i] + ri[next_idx]) ) * distScale;
            weights[next_idx] = (ri[i] / (ri[i] + ri[next_idx]) ) * distScale;
            return;
        }
    }

    // 3.

    float w = 0.0;
    float W = 0.0;
    for (auto i=0; i<vertsCount; ++i){
        auto next_idx = (i+1) % vertsCount;
        auto prev_idx = (i-1 == -1) ? vertsCount -1 : i-1;
        w = 0;
        if (Ai[prev_idx] != 0.0){
            w += (ri[prev_idx] - (Di[prev_idx] / ri[i])) / Ai[prev_idx];
        }
        if (Ai[i] != 0.0){
            w += (ri[next_idx] - (Di[i] / ri[i])) / Ai[i];
        }
        W += w;
        weights[i] = w;
    }

    short i = 0;
    for (const auto& w : weights){
        weights[i] = (w / W) * distScale;
        ++i;
    }
}


OP_ERROR
SOP_capture_pgmvc::cookMySop( OP_Context &context )
{
    if (lockInputs(context) >= UT_ERROR_ABORT) return error();
    if( (cage =   inputGeo(1,context)) == NULL ) return error();

    duplicateSource(0, context);

    float now = context.getTime();

    int numSamples = SAMPLES(now);
    tolerance = THRES(now);
    bool  useSpareWeight = SPARSEWEIGHTS(now);

    float area = 4 * M_PI;
    area /= numSamples;

    int cageNumPoints = cage->getNumPoints();

    GA_RWAttributeRef blob_gah;
    GA_Attribute *blob_attr;
    const GA_AIFBlob *aif;
    GA_RWHandleF weights_gah(gdp->findFloatTuple(GA_ATTRIB_POINT, "PGMVCweights", cageNumPoints));

    if ((!weights_gah.isValid()) && (useSpareWeight == 0))  {
        weights_gah = GA_RWHandleF(gdp->addFloatTuple(GA_ATTRIB_POINT, "PGMVCweights", cageNumPoints));
    }

    else {
        blob_gah = gdp->createAttribute(GA_ATTRIB_POINT,
                                        GA_SCOPE_PUBLIC,
                                        "blob",
                                        nullptr /*create args*/,
                                        nullptr /*attribute options*/,
                                        "blob");
        blob_attr = blob_gah.getAttribute();
        aif = blob_attr->getAIFBlob();
    }



    std::vector<UT_Vector3> sphere_samples(numSamples, UT_Vector3(0,0,0));
    UT_BoundingBox bbox;
    cage->getBBox(&bbox);
    cageCenter.assign(bbox.centerX(), bbox.centerY(), bbox.centerZ());
    sphereSamples(sphere_samples);

    // showSphere(gdp, sphere_samples);


    // Option 3 - OpenMP
    // #pragma omp parallel for ordered schedule(dynamic)
    // for (auto pt_index = 0; pt_index < gdp->getNumPoints(); ++pt_index){
    //     GA_Offset ptoff = gdp->pointOffset(pt_index);

    // Option 2 - TBB
    std::vector<GA_Size> pt_indices(gdp->getNumPoints());
    std::iota(pt_indices.begin(), pt_indices.end(), 0);
    tbb::parallel_for( size_t(0), pt_indices.size(), [&]( size_t pt_index ) {
            GA_Offset ptoff = gdp->pointOffset(pt_index);

    // Option 1 - serial
    // GA_Offset ptoff;
    // GA_FOR_ALL_PTOFF(gdp, ptoff){

        std::vector<float> captureweights(cageNumPoints, 0.0);


        float total_captureweights = 0.0;
        int sampleIndex = 0;

        // Option 2 - TBB inner loop
        // tbb::parallel_for( size_t(0), sphere_samples.size(), [&]( size_t i ) {
        //     auto direction = sphere_samples[i];

        // Option 1 - serial inner loop
        for (const auto & direction : sphere_samples){

            const GEO_Primitive *prim;            
            UT_Vector3 meshP =  gdp->getPos3(ptoff);     // <------ get pos from mesh
            GA_FOR_ALL_PRIMITIVES  (cage, prim) {
                float distance = 0;
                UT_Vector3 hitP;
                UT_Vector3 nml;
                float u;
                float v;
                int hit = prim->intersectRay( meshP - direction*1E-6F, /* move inward a bit*/
                                              direction,
                                              1E17F,  /*tmax*/
                                              1E-12F, /*tol*/
                                              &distance,
                                              &hitP, /*projected point*/
                                              &nml,
                                              0,  /*accurate*/
                                              &u,
                                              &v,
                                              1  /*ignoretrim*/
                    );

                if (hit == 1){
                    // std::cout << "sample:" << sampleIndex << " hit prim Number: " << prim->getMapIndex() << std::endl;

                    // preview intersection points
                    // GA_Offset off = gdp->appendPoint();
                    // gdp->setPos3(off, hitP);

                    float hitDist = distance;
                    hitDist = std::max<float>(tolerance, hitDist);
                    float distScale = (1.0/hitDist) * area;

                    // sample_weights
                    std::vector<float> sample_weights;
                    F(hitP, prim, distScale, sample_weights);  // cage is implicit

                    // for each vertex in ppolyon get point index
                    for (auto i=0; i<prim->getVertexCount(); ++i){
                        GA_Index cage_index = prim->getPointIndex(i);
                        auto weight = sample_weights[i];
                        captureweights[cage_index] += weight;
                        total_captureweights += weight;
                    }
                    break;
                } // end of hit
            }  // end of prim loop

            sampleIndex++;  // not used

         // }); // end of Option 2 - TBB - samples
        } // end of Option 1 - serial inner loop - samples

        if (total_captureweights > tolerance){
            for (auto i = 0; i<cageNumPoints; ++i){
                float normalweight = captureweights[i] / total_captureweights;
                if (normalweight > tolerance){
                    captureweights[i] = normalweight;
                }
                else{
                    captureweights[i] = 0.0;
                }
            }
        }
        // Set regular attribute weights
        if ((weights_gah.isValid()) && (useSpareWeight == 0)){
            for (auto i=0; i< cageNumPoints; ++i){
                weights_gah.set (ptoff, i, captureweights[i] );
            }
        }
        // Set blob value with sparse weights
        else{
            auto aa = new SparseData();
            for (auto i=0; i<cageNumPoints; ++i){
                if (captureweights[i] > 0){
                    aa->weights[i] = captureweights[i];
                }
            }
            GA_BlobRef      strblob(aa);
            aif->setBlob(blob_attr, strblob, ptoff);  // TODO: this may require lock?
            aif->compactStorage (blob_attr);
        }

    // } // end of Option 3 - OpenMP
    });  // end of Option 2 - TBB - parallel_for point offsets
    // } // end of Option 1 - serial - GA_FOR_ALL_PTOFF
    
    unlockInputs();

    return error();

}
