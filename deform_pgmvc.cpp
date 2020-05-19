#include <UT/UT_DSOVersion.h>
#include <UT/UT_Matrix3.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <PRM/PRM_Include.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <SOP/SOP_Guide.h>
#include <UT/UT_ErrorManager.h>
#include <OP/OP_Director.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Interrupt.h>
#include <GEO/GEO_Detail.h>
#include <GEO/GEO_Point.h>
#include <GEO/GEO_Vertex.h>

#include <SYS/SYS_Math.h>

#include "deform_pgmvc.h"

void
newSopOperator( OP_OperatorTable *table )
{
    table->addOperator
        (
            new OP_Operator
            (
                "deform_pgmvc_all",               // Internal name
                "deform_pgmvc_all",               // UI name
                SOP_PGMVC_deform::myConstructor,  //
                SOP_PGMVC_deform::myTemplateList, //
                2,                             // Min Inputs
                2,                             // Max Inputs
                0                              // Local Variables
                //OP_FLAG_*                    // Additional Flag
                )
            );
}

static PRM_Name names[] =
{
    PRM_Name( "delatt", "Delete capture attributes" ),
    PRM_Name( 0 )
};

// How to make choices for a menu.

PRM_Template
SOP_PGMVC_deform::myTemplateList[] =
{
    PRM_Template(PRM_TOGGLE,    1, &names[0]),
    PRM_Template(),
};

const char *
SOP_PGMVC_deform::inputLabel( unsigned int input) const
{
    switch (input) {
    case 0:
        return "Geometry To Deform";
        break;
    case 1:
        return "Deformed Mesh";
        break;
    default:
        return "Something";
        break;
    }
}

//overriding constructor
OP_Node *
SOP_PGMVC_deform::myConstructor( OP_Network *net, const char *name, OP_Operator *op )
{
    return new SOP_PGMVC_deform( net, name, op );
}

SOP_PGMVC_deform::SOP_PGMVC_deform( OP_Network *net, const char *name, OP_Operator *op )
    : SOP_Node( net, name, op )
{

    mySopFlags.setNeedGuide1( 1 );

}

//destructor
SOP_PGMVC_deform::~SOP_PGMVC_deform() {}

//don't even disable or enable anything
unsigned
SOP_PGMVC_deform::disableParms()
{
    return 0;
}


//cook method

OP_ERROR
SOP_PGMVC_deform::cookMySop( OP_Context &context )
{
    int                   i;
    float                 weight;
    UT_Vector3            new_pos;
    GA_Offset             off;
    UT_Vector3            pos;
    const GU_Detail       *deform;

    // Before we do anything, we must lock our inputs.  Before returning,
    // we have to make sure that the inputs get unlocked.
    if( lockInputs( context ) >= UT_ERROR_ABORT ) return error();
    if( (deform =   inputGeo(1,context)) == NULL ) return error();

    duplicateSource( 0, context );

    GA_RWHandleF wCapt_gah(gdp, GA_ATTRIB_POINT, "PGMVCweights");

    if (!wCapt_gah.isValid()) {
        addError(SOP_ERR_INVALID_SRC , "input 1 is not captured");
        unlockInputs();
        return error();
    }

    GA_Offset cage_off;
    GA_FOR_ALL_PTOFF(gdp, off) {
        new_pos = UT_Vector3(0,0,0);
        
        i = 0;
        GA_FOR_ALL_PTOFF(deform, cage_off)  {
            weight = wCapt_gah.get(off, i);
            if (weight > 0)  {
                pos = deform->getPos3(cage_off);
                new_pos += weight * pos;
            }
            i++;
        }
        gdp->setPos3(off, new_pos);
    }

    if (DELATT()) {
        if (wCapt_gah.isValid()) {
            gdp->destroyPointAttrib("PGMVCweights");
        }
    }

    unlockInputs();

    return error();

}

OP_ERROR
SOP_PGMVC_deform::cookMyGuide1( OP_Context &context )
{
  if ( lockInputs( context ) >= UT_ERROR_ABORT ) return error();

  GU_Detail *in_gdp = (GU_Detail *)inputGeo(1, context);
  if (in_gdp)
    myGuide1->copy(*in_gdp);

  unlockInputs();
  return error();
}

