
#ifndef __SOP_PGMVC_h__
#define __SOP_PGMVC_h__

#include <SOP/SOP_Node.h>

class SOP_PGMVC_deform : public SOP_Node
{
public:
    SOP_PGMVC_deform(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_PGMVC_deform();


    static PRM_Template    myTemplateList[];
    static OP_Node    *myConstructor(OP_Network*, const char *,
                                     OP_Operator *);


protected:
    virtual unsigned      disableParms();
    virtual const char          *inputLabel(unsigned int) const;

    // Method to cook geometry for the SOP
    virtual OP_ERROR       cookMySop(OP_Context &context);
    // This method is used to generate geometry for a "guide".
    virtual OP_ERROR             cookMyGuide1(OP_Context &context);

    int              DELATT()     { return evalInt  (0, 0, 0); }


};

#endif
