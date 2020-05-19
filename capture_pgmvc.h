#pragma once

#include <SOP/SOP_Node.h>

class SOP_capture_pgmvc : public SOP_Node
{
  public:
    SOP_capture_pgmvc(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_capture_pgmvc();


    static PRM_Template    myTemplateList[];
    static OP_Node    *myConstructor(OP_Network*, const char *,
                                     OP_Operator *);


  protected:
    virtual unsigned      disableParms();
    virtual const char          *inputLabel(unsigned int) const;

    // Method to cook geometry for the SOP
    virtual OP_ERROR       cookMySop(OP_Context &context);

    float THRES(float t)       { return evalFloat(0, 0, t); }
    int   SAMPLES(float t)     { return evalInt(1 /*index*/, 0, t); } 

    void F(UT_Vector3, const GEO_Primitive*, float, std::vector<float>&);

    inline int myMod(int a, int b) { return (a<0?b+a:a); }


  private:
    float tolerance;
    void showSphere(GU_Detail *, std::vector<UT_Vector3>&);
    const GU_Detail     *cage;
    UT_Vector3 cageCenter;


};
