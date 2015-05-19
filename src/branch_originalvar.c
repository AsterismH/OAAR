/**@file   branch_originalvar.c
 * @ingroup BRANCHINGRULES
 * @brief  branch on the original variable
 * @author He Xingqiu
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#define SCIP_DEBUG

#include <assert.h>
#include <string.h>

#include "branch_originalvar.h"
#include "cons_zeroone.h"
#include "probdata_OAAR.h"
#include "vardata_OAAR.h"
#include "OAARdataStructure.h"

/**@name Branching rule properties
 *
 * @{
 */

#define BRANCHRULE_NAME            "originalvar"
#define BRANCHRULE_DESC            "Braching on the original variables"
#define BRANCHRULE_PRIORITY        50000
#define BRANCHRULE_MAXDEPTH        -1
#define BRANCHRULE_MAXBOUNDDIST    1.0

/**@} */

/**@name Callback methods
 *
 * @{
 */

/** branching execution method for fractional LP solutions */
static
SCIP_DECL_BRANCHEXECLP(branchExeclpOriginalvar)
{  /*lint --e{715}*/
   SCIP_PROBDATA* probdata;

   SCIP_VAR** lpcands;
   SCIP_Real* lpcandsfrac;
   int nlpcands;

   SCIP_NODE* childzero;
   SCIP_NODE* childone;
   SCIP_CONS* conszero;
   SCIP_CONS* consone;

   SCIP_VARDATA* vardata;
   int* consids;
   int nconsids;
   int* oriFlowVars;
   int nOriFlowVars;

   SCIP_VAR** lambda_i;
   int nlambda_i;
   int nread;
   int tempI, tempJ;
   char* tempStr;

   int nLinks, nOpticalLinks, nNodes, nOpticalNodes;
   int nFlows;

   int i,j,k;
   int index1, index2;
   double fracOriVal;

   assert(scip != NULL);
   assert(branchrule != NULL);
   assert(strcmp(SCIPbranchruleGetName(branchrule), BRANCHRULE_NAME) == 0);
   assert(result != NULL);

   SCIPdebugMessage("start branching at node %"SCIP_LONGINT_FORMAT", depth %d\n", SCIPgetNNodes(scip), SCIPgetDepth(scip));

   *result = SCIP_DIDNOTRUN;

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);

   //nitems = SCIPprobdataGetNItems(probdata);
   nNodes = SCIPprobdataGetNNodes(probdata);
   nOpticalNodes = SCIPprobdataGetNOpticalNodes(probdata);
   nLinks = SCIPprobdataGetNLinks(probdata);
   nOpticalLinks = SCIPprobdataGetNOpticalLinks(probdata);
   nFlows = SCIPprobdataGetNFlows(probdata);

   /* get fractional LP candidates */
   SCIP_CALL( SCIPgetLPBranchCands(scip, &lpcands, NULL, &lpcandsfrac, NULL, &nlpcands, NULL) );
   assert(nlpcands > 0);

   //compute the first fractional original variable
   for(i = 0; i < nFlows; i++)
   {
      //get lambda_i
      fracOriVal = 0;
      SCIP_CALL( SCIPduplicateMemoryArray(scip, &lambda_i, lpcands, nlpcands) );
      for(j = 0; j < nlpcands; j++)
      {
	 tempStr = SCIPvarGetName(lambda_i[j]);
         nread = sscanf(tempStr, "lambda_%d_%d", &tempI, &tempJ);
	 if(nread == 0) 
	 {
	    SCIPwarningMessage(scip, "branch_originalvar.c, error when parsing lambda name\n");
	 }
	 //if current lambda does not belong to flow i
	 if(tempI != i) 
	 {
	    lambda_i[j] = NULL;
	 }
      }
      for(j = 0; j < nLinks+nOpticalLinks*nWaveLength*2; j++)
      {
         for(k = 0; k < nlpcands; k++)
	 {
	    if(lambda_i[k] != NULL) 
	    {
	       vardata = SCIPvarGetData(lambda_i[k]);
	       oriFlowVars = SCIPvardataGetOriFlowVars(vardata);
	       fracOriVal += lpcandsfrac[k] * oriFlowVars[j];
	    }
	 }
	 if(SCIPisFeasEQ(scip, fracOriVal, 1.0) || SCIPisFeasEQ(scip, fracOriVal, 0.0))
	 {
	    continue;
	 }
	 else
	 {
	    index1 = i;
	    index2 = j;
	    goto label;
	 }
      }
      SCIPfreeMemoryArray(scip, &lambda_i);
   }

   label: 
   SCIPdebugMessage("branch on original variable x_%d_%d\n", index1, index2);
   SCIPdebugMessage("current value of x_%d_%d is %lf \n", index1, index2, fracOriVal);

   /* create the branch-and-bound tree child nodes of the current node */
   SCIP_CALL( SCIPcreateChild(scip, &childzero, 0.0, SCIPgetLocalTransEstimate(scip)) );
   SCIP_CALL( SCIPcreateChild(scip, &childone, 0.0, SCIPgetLocalTransEstimate(scip)) );

   /* create corresponding constraints */
   SCIP_CALL( SCIPcreateConsZeroone(scip, &conszero, "zero", index1, 
      index2, ZERO, childzero, TRUE) );
   SCIP_CALL( SCIPcreateConsZeroone(scip, &consone, "one", index1, 
      index2, ONE, childone, TRUE) );

  /* add constraints to nodes */
   SCIP_CALL( SCIPaddConsNode(scip, childzero, conszero, NULL) );
   SCIP_CALL( SCIPaddConsNode(scip, childone, consone, NULL) );

   /* release constraints */
   SCIP_CALL( SCIPreleaseCons(scip, &conszero) );
   SCIP_CALL( SCIPreleaseCons(scip, &consone) );

  *result = SCIP_BRANCHED;

   return SCIP_OKAY;
}

/**@} */

/**@name Interface methods
 *
 * @{
 */

/** creates the ryan foster branching rule and includes it in SCIP */
SCIP_RETCODE SCIPincludeBranchruleOriginalvar(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_BRANCHRULEDATA* branchruledata;
   SCIP_BRANCHRULE* branchrule;

   /* create ryan foster branching rule data */
   branchruledata = NULL;
   branchrule = NULL;
   /* include branching rule */
   SCIP_CALL( SCIPincludeBranchruleBasic(scip, &branchrule, BRANCHRULE_NAME, BRANCHRULE_DESC, BRANCHRULE_PRIORITY, BRANCHRULE_MAXDEPTH,
         BRANCHRULE_MAXBOUNDDIST, branchruledata) );
   assert(branchrule != NULL);

   SCIP_CALL( SCIPsetBranchruleExecLp(scip, branchrule, branchExeclpOriginalvar) );

   return SCIP_OKAY;
}

/**@} */
