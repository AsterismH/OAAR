/**@file   probdata_OAAR.c
 * @brief  Problem data for OAAR problem
 * @author He Xingqiu
 *
 * This file mainly provide functions that used for manipulating probdata.
 **/

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#define SCIP_DEBUG

#include <string.h>

#include "probdata_OAAR.h"
#include "vardata_OAAR.h"
#include "pricer_OAAR.h"
#include "scip/cons_setppc.h"
#include "scip/cons_knapsack.h"
#include "OAARdataStructure.h"

#include "scip/scip.h"

/** @brief Problem data which is accessible in all places
 *
 * This problem data is used to store the input of the OAAR, all variables which are created, and all
 * constrsaints.
 */
struct SCIP_ProbData
{
   SCIP_VAR**            vars;         /**< all exiting variables in the problem */
   SCIP_CONS**           conss;        /**< all constraints */
   OAARNode*             Nodes;        /**< Nodes array */
   OAARLink*             Links;        /**< Links array */
   OAARFlow*             Flows;        /**< Flow array storing all user defined flow infomation */ 
   int                   nNodes;       /**< Number of Nodes */
   int                   nOpticalNodes;/**< Number of Optical Nodes */
   int                   nLinks;       /**< Number of Links */
   int                   nOpticalLinks;/**< NUmber of Optical Links */
   int                   nFlows;       /**< Number of flows */
   int                   nvars;        /**< number of generated variables */
   int                   nCons;        /**< number of constraints */
   int*                  nFlowSol;     /**< how many vars for each flow */
   int                   varssize;     /**< size of the variable array */
};


/**@name Event handler properties
 *
 * @{
 */

#define EVENTHDLR_NAME         "addedvar"
#define EVENTHDLR_DESC         "event handler for catching added variables"

/**@} */

/**@name Callback methods of event handler
 *
 * @{
 */

/** execution method of event handler */
static
SCIP_DECL_EVENTEXEC(eventExecAddedVar)
{  /*lint --e{715}*/
   assert(eventhdlr != NULL);
   assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);
   assert(event != NULL);
   assert(SCIPeventGetType(event) == SCIP_EVENTTYPE_VARADDED);

   SCIPdebugMessage("exec method of event handler for added variable to probdata\n");

   /* add new variable to probdata */
   SCIP_CALL( SCIPprobdataAddVar(scip, SCIPgetProbData(scip), SCIPeventGetVar(event)) );

   return SCIP_OKAY;
}

/**@} */


/**@name Local methods
 *
 * @{
 */

/** creates problem data */
static
SCIP_RETCODE probdataCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA**       probdata,           /**< pointer to problem data */
   SCIP_VAR**            vars,               /**< all exist variables */
   SCIP_CONS**           conss,              /**< set partitioning constraints for each job exactly one */
   OAARNode*             Nodes,              /**< Nodes array */
   OAARLink*             Links,              /**< Links array */
   OAARFlow*             Flows,              /**< Flow information */
   int                   nNodes,             /**< number of Nodes */
   int                   nOpticalNodes,      /**< number of optical nodes */
   int                   nLinks,             /**< number of links */
   int                   nOpticalLinks,      /**< number of optical links */
   int                   nFlows,             /**< number of flows */
   int                   nvars,              /**< number of variables */
   int                   nCons,              /**< number of constraints */
   int*                  nFlowSol
   )
{
   int i;
   assert(scip != NULL);
   assert(probdata != NULL);

   /* allocate memory */
   SCIP_CALL( SCIPallocMemory(scip, probdata) );

   if( nvars > 0 )
   {
      /* copy variable array */
      SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->vars, vars, nvars) );
   }
   else
      (*probdata)->vars = NULL;

   /* duplicate arrays */
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->conss, conss, nCons) ); 
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->Nodes, Nodes, nNodes) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->Links, Links, nLinks) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->Flows, Flows, nFlows) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->nFlowSol, nFlowSol, nFlows) );
   for( i = 0; i < nNodes; i++ )
   {
      SCIP_CALL( SCIPduplicateMemoryArray(scip, &((*probdata)->Nodes[i].ConnLinks), 
         Nodes[i].ConnLinks, Nodes[i].nConnLinks) );
   }

   (*probdata)->nvars = nvars;
   (*probdata)->varssize = nvars;
   (*probdata)->nCons = nCons;
   (*probdata)->nNodes = nNodes;
   (*probdata)->nOpticalNodes = nOpticalNodes;
   (*probdata)->nLinks = nLinks;
   (*probdata)->nOpticalLinks = nOpticalLinks;
   (*probdata)->nFlows = nFlows;

   return SCIP_OKAY;
}

/** frees the memory of the given problem data */
static
SCIP_RETCODE probdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA**       probdata            /**< pointer to problem data */
   )
{
   int i;

   assert(scip != NULL);
   assert(probdata != NULL);

   /* release all variables */
   for( i = 0; i < (*probdata)->nvars; ++i )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &(*probdata)->vars[i]) );
   }

   /* release all constraints */
   for( i = 0; i < (*probdata)->nCons; ++i )
   {
      SCIP_CALL( SCIPreleaseCons(scip, &(*probdata)->conss[i]) );
   }

   // free memory of ConnLinks array 
   for( i = 0; i < (*probdata)->nNodes; i++ )
   {
      SCIPfreeMemoryArray(scip, &((*probdata)->Nodes[i].ConnLinks));
   }

   /* free memory of arrays */
   SCIPfreeMemoryArray(scip, &(*probdata)->vars);
   SCIPfreeMemoryArray(scip, &(*probdata)->conss);
   SCIPfreeMemoryArray(scip, &(*probdata)->Nodes);
   SCIPfreeMemoryArray(scip, &(*probdata)->Links);
   SCIPfreeMemoryArray(scip, &(*probdata)->Flows);
   SCIPfreeMemoryArray(scip, &(*probdata)->nFlowSol);

   /* free probdata */
   SCIPfreeMemory(scip, probdata);

   return SCIP_OKAY;
}

/** create initial columns */
static
SCIP_RETCODE createInitialColumns(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   SCIP_CONS** conss;
   SCIP_VARDATA* vardata;
   SCIP_VAR* var;
   char name[SCIP_MAXSTRLEN];

   int nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows;
   int nvars, nCons;

   OAARNode* Nodes;
   OAARLink* Links;
   OAARFlow* Flows;

   int i;
   int tempConsIds[2];
   OAARLink tempLink;
   OAARNode tempHeadNode;
   OAARFlow tempFlow;
   double tempDelay, tempJitter, tempPathCost, obj;
   int* oriFlowVars;
   int nOriFlowVars;

   conss = probdata->conss;
   Nodes = probdata->Nodes;
   Links = probdata->Links;
   Flows = probdata->Flows;
   nNodes = probdata->nNodes;
   nOpticalNodes = probdata->nOpticalNodes;
   nLinks = probdata->nLinks;
   nOpticalLinks = probdata->nOpticalLinks;
   nFlows = probdata->nFlows;
   nvars = probdata->nvars;
   nCons = probdata->nCons;

   
   /* create start solution using the artificial links */
   for( i = 0; i < nFlows; i++ )
   {
      // lambda_i_0 stands for the initial solution for i-th flow
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "lambda_%d_0", i);

      SCIPdebugMessage("create variable for flow %d\n", i);

      /* create variable for the artificial links */
      // this function is defined in vardata_OAAR.c 
      // get the i-th artificial link
      tempLink = Links[nLinks - nFlows + i]; 
      tempHeadNode = Nodes[tempLink.Head];
      tempFlow = Flows[i];
      // compute obj
      tempDelay = tempHeadNode.ProcDelay + tempHeadNode.QueueDelay + 
         tempLink.PropDelay + tempLink.TransDelay; 
      tempJitter = tempHeadNode.Jitter;
      tempPathCost = tempLink.BandCost;
      obj = tempFlow.Priority * (tempDelay * tempFlow.DelayPrice + 
         tempJitter * tempFlow.JitterPrice + tempPathCost * tempFlow.BandWidth);
      SCIP_CALL( SCIPcreateVarOAAR(scip, &var, name, obj, TRUE, TRUE, NULL) );

      /* add variable to the problem */
      SCIP_CALL( SCIPaddVar(scip, var) );

      /* store variable in the problme data */
      SCIP_CALL( SCIPprobdataAddVar(scip, probdata, var) );

      /* add variable to corresponding constraint */
      // Cons1
      SCIP_CALL( SCIPaddCoefSetppc(scip, conss[i], var) );
      // Cons2. the initial vars do not appear in Cons3
      // add codfficient for 
      // nFlow(number of Cons1)+(nLinks-nOpticalLinks)(number of ElecLinks)-nFlow+i
      SCIP_CALL( SCIPaddCoefKnapsack(scip, conss[nLinks - nOpticalLinks + i], var, tempFlow.BandWidth) );
      // check if the previous deduction is correct
      if( SCIPgetCapacityKnapsack(scip, conss[nLinks-nOpticalLinks+i]) != tempLink.Capacity ) 
         SCIPdebugMessage("Initial column error!\n");

      /* create the variable data for the variable;  */
      // vardata contains the cons that the variable appears
      tempConsIds[0] = i; tempConsIds[1] = nLinks-nOpticalLinks+i;
      nOriFlowVars = nLinks + 2 * nOpticalLinks * nWaveLength;
      SCIPallocBufferArray(scip, &oriFlowVars, nOriFlowVars);
      memset(oriFlowVars, 0, nOriFlowVars);
      oriFlowVars[nLinks-nFlows+i] = 1;
      SCIP_CALL( SCIPvardataCreateOAAR(scip, &vardata, tempConsIds, 2, oriFlowVars, nOriFlowVars) );

      /* add the variable data to the variable */
      SCIPvarSetData(var, vardata);

      /* change the upper bound of the binary variable to lazy since the upper bound is already enforced
       * due to the objective function the set covering constraint;
       * The reason for doing is that, is to avoid the bound of x <= 1 in the LP relaxation since this bound
       * constraint would produce a dual variable which might have a positive reduced cost
       */
      // such setting is suitable for my problem since the cons1 also force lambda <= 1
      SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

      /* release variable */
      SCIP_CALL( SCIPreleaseVar(scip, &var) );

      SCIPfreeBufferArray(scip, &oriFlowVars);
   }

   return SCIP_OKAY;
}

/**@} */

/**@name Callback methods of problem data
 *
 * @{
 */

/** frees user data of original problem (called when the original problem is freed) */
static
SCIP_DECL_PROBDELORIG(probdelorigOAAR)
{
   SCIPdebugMessage("free original problem data\n");

   SCIP_CALL( probdataFree(scip, probdata) );

   return SCIP_OKAY;
}

/** creates user data of transformed problem by transforming the original user problem data
 *  (called after problem was transformed) */
static
SCIP_DECL_PROBTRANS(probtransOAAR)
{
   /* create transform probdata */
   SCIP_CALL( probdataCreate(scip, targetdata, sourcedata->vars, sourcedata->conss, sourcedata->Nodes,
         sourcedata->Links, sourcedata->Flows, sourcedata->nNodes, sourcedata->nOpticalNodes, 
	 sourcedata->nLinks, sourcedata->nOpticalLinks, sourcedata->nFlows, 
	 sourcedata->nvars, sourcedata->nCons, sourcedata->nFlowSol) );

   /* transform all constraints */
   SCIP_CALL( SCIPtransformConss(scip, (*targetdata)->nCons, (*targetdata)->conss, (*targetdata)->conss) );

   /* transform all variables */
   SCIP_CALL( SCIPtransformVars(scip, (*targetdata)->nvars, (*targetdata)->vars, (*targetdata)->vars) );

   return SCIP_OKAY;
}

/** frees user data of transformed problem (called when the transformed problem is freed) */
static
SCIP_DECL_PROBDELTRANS(probdeltransOAAR)
{
   SCIPdebugMessage("free transformed problem data\n");

   SCIP_CALL( probdataFree(scip, probdata) );

   return SCIP_OKAY;
}

/** solving process initialization method of transformed data (called before the branch and bound process begins) */
static
SCIP_DECL_PROBINITSOL(probinitsolOAAR)
{
   SCIP_EVENTHDLR* eventhdlr;

   assert(probdata != NULL);

   /* catch variable added event */
   eventhdlr = SCIPfindEventhdlr(scip, "addedvar");
   assert(eventhdlr != NULL);

   SCIP_CALL( SCIPcatchEvent(scip, SCIP_EVENTTYPE_VARADDED, eventhdlr, NULL, NULL) );

   return SCIP_OKAY;
}

/** solving process deinitialization method of transformed data (called before the branch and bound data is freed) */
static
SCIP_DECL_PROBEXITSOL(probexitsolOAAR)
{
   SCIP_EVENTHDLR* eventhdlr;

   assert(probdata != NULL);

   /* drop variable added event */
   eventhdlr = SCIPfindEventhdlr(scip, "addedvar");
   assert(eventhdlr != NULL);

   SCIP_CALL( SCIPdropEvent(scip, SCIP_EVENTTYPE_VARADDED, eventhdlr, NULL, -1) );


   return SCIP_OKAY;
}

/**@} */


/**@name Interface methods
 *
 * @{
 */

/** sets up the problem data */
SCIP_RETCODE SCIPprobdataCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           probname,           /**< problem name */
   OAARNode*             Nodes,              /**< Nodes array */
   OAARLink*             Links,              /**< Links array */
   OAARFlow*             Flows,              /**< Flows array */
   int                   nNodes,           
   int                   nOpticalNodes,
   int                   nLinks,
   int                   nOpticalLinks,
   int                   nFlows,
   int                   nCons
   )
{
   SCIP_PROBDATA* probdata;
   SCIP_CONS** conss;
   char name[SCIP_MAXSTRLEN];
   int i,j;
   int* nFlowSol;

   assert(scip != NULL);

   /* create event handler if it does not exist yet */
   if( SCIPfindEventhdlr(scip, EVENTHDLR_NAME) == NULL )
   {
      SCIP_CALL( SCIPincludeEventhdlrBasic(scip, NULL, EVENTHDLR_NAME, EVENTHDLR_DESC, eventExecAddedVar, NULL) );
   }

   /* create problem in SCIP and add non-NULL callbacks via setter functions */
   SCIP_CALL( SCIPcreateProbBasic(scip, probname) );

   SCIP_CALL( SCIPsetProbDelorig(scip, probdelorigOAAR) );
   SCIP_CALL( SCIPsetProbTrans(scip, probtransOAAR) );
   SCIP_CALL( SCIPsetProbDeltrans(scip, probdeltransOAAR) );
   SCIP_CALL( SCIPsetProbInitsol(scip, probinitsolOAAR) );
   SCIP_CALL( SCIPsetProbExitsol(scip, probexitsolOAAR) );

   /* set objective sense */
   SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE) );

   /* tell SCIP that the objective will be always integral */
   // The objective is not necessarily integral in my problem
   //SCIP_CALL( SCIPsetObjIntegral(scip) );

   SCIP_CALL( SCIPallocBufferArray(scip, &conss, nCons) );
   SCIP_CALL( SCIPallocBufferArray(scip, &nFlowSol, nFlows) );
   for(i = 0; i < nFlows; i++)
      nFlowSol[i] = 0;
   for(i = 0; i < nFlows; i++)
      SCIPdebugMessage("nFlowSol[%d]:%d \n", i, nFlowSol[i]);

   // Number of electronic links
   int nElecLinks;
   nElecLinks = nLinks - nOpticalLinks;
   int TempCapacity;

   // Set Cons1
   for( i = 0; i < nFlows; i++ )
   {
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "Cons1_%d", i);

      SCIP_CALL( SCIPcreateConsBasicSetpart(scip, &conss[i], name, 0, NULL) );

      /* declare constraint modifiable for adding variables during pricing */
      SCIP_CALL( SCIPsetConsModifiable(scip, conss[i], TRUE) );
      SCIP_CALL( SCIPaddCons(scip, conss[i]) );   
   }   
   // Set Cons2
   for( i = 0; i < nElecLinks; i++ )
   {
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "Cons2_%d", i);

      TempCapacity = Links[nOpticalLinks+i].Capacity;
      SCIP_CALL( SCIPcreateConsBasicKnapsack(scip, &conss[i+nFlows], name, 0, NULL, NULL, TempCapacity)  );

      // declare modifiable
      SCIP_CALL( SCIPsetConsModifiable(scip, conss[i+nFlows], TRUE) );
      SCIP_CALL( SCIPaddCons(scip, conss[i+nFlows]) );
   }
   // Set Cons3
   for( i = 0; i < nOpticalLinks; i++ )
   {
      for( j = 0; j < nWaveLength; j++ )
      {
         (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "Cons3_%d_%d", i, j);

	 SCIP_CALL( SCIPcreateConsBasicSetpack(scip, &conss[nFlows+nElecLinks+(i*nWaveLength)+j],
	    name, 0, NULL) );

	 // declare modifiable
	 SCIP_CALL( SCIPsetConsModifiable(scip, conss[nFlows+nElecLinks+(i*nWaveLength)+j], TRUE) );
	 SCIP_CALL( SCIPaddCons(scip, conss[nFlows+nElecLinks+(i*nWaveLength)+j]) );
      }
   }
   
   /* create problem data */
   // no variable, and varssize is 0
   SCIP_CALL( probdataCreate(scip, &probdata, NULL, conss, Nodes, Links, Flows, nNodes, nOpticalNodes,
      nLinks, nOpticalLinks, nFlows, 0, nCons, nFlowSol) );

   SCIP_CALL( createInitialColumns(scip, probdata) );

   for(i = 0; i < nFlows; i++)
      SCIPdebugMessage("nFlowSol[%d]:%d\n", i, probdata->nFlowSol[i]);

   /*
   printf("DEBUG: print probdata after initialization.\n");
   printNodes(probdata->Nodes, probdata->nNodes);
   printLinks(probdata->Links, probdata->nLinks);
   printFlows(probdata->Flows, probdata->nFlows);
   printf("nNodes:%d, nOpticalNodes:%d, nLinks:%d, nOpticalLinks:%d, nFlows:%d, nvars:%d\n", 
      probdata->nNodes, probdata->nOpticalNodes, probdata->nLinks, probdata->nOpticalLinks, 
      probdata->nFlows, probdata->nvars);
   printf("nFlowSol:");
   for(i = 0; i < nFlows; i++)
      printf("%d, ", probdata->nFlowSol[i]);
   printf("\n");
   */

   /* set user problem data */
   SCIP_CALL( SCIPsetProbData(scip, probdata) );

   SCIP_CALL( SCIPpricerOAARActivate(scip, conss, Nodes, Links, Flows, nNodes, nOpticalNodes,
      nLinks, nOpticalLinks, nFlows, nCons) );

   /* free local buffer arrays */
   SCIPfreeBufferArray(scip, &conss);
   SCIPfreeBufferArray(scip, &nFlowSol);

   return SCIP_OKAY;
}

/** returns array of Nodes */
OAARNode* SCIPprobdataGetNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->Nodes;
}

/** returns array of Links */
OAARLink* SCIPprobdataGetLinks(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->Links;
}

/** returns array of Flows */
OAARFlow* SCIPprobdataGetFlows(
   SCIP_PROBDATA*       probdata
   )
{
   return probdata->Flows;
}

/** returns number of Nodes */
int SCIPprobdataGetNNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->nNodes;
}

/** returns number of Optical Nodes */
int SCIPprobdataGetNOpticalNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->nOpticalNodes;
}

/** returns number of Links */
int SCIPprobdataGetNLinks(
   SCIP_PROBDATA*        probdata
   )
{
   return probdata->nLinks;
}

/** reuturns number of Optical Links */
int SCIPprobdataGetNOpticalLinks(
   SCIP_PROBDATA*        probdata
   )
{
   return probdata->nOpticalLinks;
}

/** returns number of Flows */
int SCIPprobdataGetNFlows(
   SCIP_PROBDATA*        probdata
   )
{
   return probdata->nFlows;
}

/** returns array of all variables itemed in the way they got generated */
SCIP_VAR** SCIPprobdataGetVars(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->vars;
}

/** returns number of variables */
int SCIPprobdataGetNVars(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->nvars;
}

/** returns array of constraints */
SCIP_CONS** SCIPprobdataGetConss(
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
   return probdata->conss;
}

/** returns number of constraints */
int SCIPprobdataGetNCons(
   SCIP_PROBDATA*        probdata
   )
{
   return probdata->nCons;
}

int* SCIPprobdataGetNFlowSol(
   SCIP_PROBDATA*       probdata
   )
{
   return probdata->nFlowSol;
}

/** adds given variable to the problem data */
SCIP_RETCODE SCIPprobdataAddVar(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata,           /**< problem data */
   SCIP_VAR*             var                 /**< variables to add */
   )
{
   char* varname;
   int varSup, varSub;
   
   /* check if enough memory is left */
   if( probdata->varssize == probdata->nvars )
   {
      probdata->varssize = MAX(100, probdata->varssize * 2);
      SCIP_CALL( SCIPreallocMemoryArray(scip, &probdata->vars, probdata->varssize) );
   }

   /* caputure variables */
   SCIP_CALL( SCIPcaptureVar(scip, var) );

   probdata->vars[probdata->nvars] = var;
   probdata->nvars++;

   varname = SCIPvarGetName(var);
   sscanf(varname, "lambda_%d_%d", &varSup, &varSub);
   
   if(varSub != probdata->nFlowSol[varSup])
   {
      SCIPdebugMessage("The subscript of the new variable lambda_%d_%d is incorrect!\n", 
         varSup, varSub);
   }
   probdata->nFlowSol[varSup]++;


   SCIPdebugMessage("added variable %s to probdata; nvars = %d\n", varname, probdata->nvars);

   return SCIP_OKAY;
}

/**@} */
