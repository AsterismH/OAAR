/**@file   pricer_OAAR.c
 * @brief  OAAR variable pricer
 * @author He Xingqiu
 *
 * This file implements the variable pricer which check if variables exist with negative reduced cost. 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

//#define SCIP_DEBUG

#include <assert.h>
#include <string.h>

#include "scip/cons_knapsack.h"
#include "scip/cons_setppc.h"
#include "scip/scipdefplugins.h"

#include "cons_zeroone.h" 
#include "pricer_OAAR.h"
#include "probdata_OAAR.h"
#include "vardata_OAAR.h"

/**@name Pricer properties
 *
 * @{
 */

#define PRICER_NAME            "OAAR"
#define PRICER_DESC            "pricer for OAAR"
#define PRICER_PRIORITY        0
#define PRICER_DELAY           TRUE     /* only call pricer if all problem variables have non-negative reduced costs */

/**@} */


/*
 * Data structures
 */

struct SCIP_PricerData
{
   SCIP_CONSHDLR*        conshdlr;           /**< comstraint handler for zeroone constraints */
   SCIP_CONS**           conss;              
   OAARNode*             Nodes;
   OAARLink*             Links;
   OAARFlow*             Flows;
   int                   nNodes;
   int                   nOpticalNodes;
   int                   nLinks;
   int                   nOpticalLinks;
   int                   nFlows;
   int                   nCons;
   //int*                  nFlowSol;
};



/**@name Local methods
 *
 * @{
 */

/** add branching decisions constraints to the sub SCIP */
static
SCIP_RETCODE addBranchingDecisionConss(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP*                 subscip,            /**< pricing SCIP data structure */
   SCIP_VAR**            vars,               /**< variable array of the subscuip oder variables */
   SCIP_CONSHDLR*        conshdlr,           /**< constraint handler for branching data */
   int                   k
   )
{
   SCIP_CONS** conss;
   SCIP_CONS* cons;
   int nconss;
   CONSTYPE type;

   SCIP_Bool infeasible, fixed;

   int index1, index2;
   int c;

   assert( scip != NULL );
   assert( subscip != NULL );
   assert( conshdlr != NULL );

   /* collect all branching decision constraints */
   conss = SCIPconshdlrGetConss(conshdlr);
   nconss = SCIPconshdlrGetNConss(conshdlr);

   /* loop over all branching decision constraints and apply the branching decision if the corresponding constraint is
    * active
    */
   for( c = 0; c < nconss; ++c )
   {
      cons = conss[c];

      /* ignore constraints which are not active since these are not laying on the current active path of the search
       * tree
       */
      if( !SCIPconsIsActive(cons) )
         continue;

      /* collect the two item ids and the branching type (SAME or DIFFER) on which the constraint branched */
      index1 = SCIPgetIndex1Zeroone(scip, cons);
      index2 = SCIPgetIndex2Zeroone(scip, cons);
      type = SCIPgetTypeZeroone(scip, cons);

      if(index1 != k)
         continue;

      SCIPdebugMessage("set variable x_%d_%d to %d\n", index1, index2, type == ZERO ? 0 : 1);

      /* depending on the branching type select the correct left and right hand side for the linear constraint which
       * enforces this branching decision in the pricing problem MIP
       */
      if( type == ZERO )
      {
         SCIP_CALL( SCIPfixVar(subscip, vars[index2], 0.0, &infeasible, &fixed) );
      }
      else if( type == ONE )
      {
         SCIP_CALL( SCIPfixVar(subscip, vars[index2], 1.0, &infeasible, &fixed) );
      }
      else
      {
         SCIPerrorMessage("unknow constraint type <%d>\n, type");
         return SCIP_INVALIDDATA;
      }

      //SCIPdebugPrintCons(subscip, cons, NULL);

      //SCIP_CALL( SCIPaddCons(subscip, cons) );
      //SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
   }

   return SCIP_OKAY;
}

/** avoid to generate columns which are fixed to zero; therefore add for each variable which is fixed to zero a
 *  corresponding logicor constraint to forbid this column
 *
 * @note variable which are fixed locally to zero should not be generated again by the pricing MIP
 */

// we do not implement this function here
static
SCIP_RETCODE addFixedVarsConss(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP*                 subscip,            /**< pricing SCIP data structure */
   SCIP_VAR**            vars,               /**< variable array of the subscuip */
   SCIP_CONS**           conss,              /**< array of setppc constraint for each item one */
   int                   nitems              /**< number of items */
   )
{
   SCIP_VAR** origvars;
   int norigvars;

   SCIP_CONS* cons;
   int* consids;
   int nconsids;
   int consid;
   int nvars;

   SCIP_VAR** logicorvars;
   SCIP_VAR* var;
   SCIP_VARDATA* vardata;
   SCIP_Bool needed;
   int nlogicorvars;

   int v;
   int c;
   int o;

   /* collect all variable which are currently existing */
   origvars = SCIPgetVars(scip);
   norigvars = SCIPgetNVars(scip);

   /* loop over all these variables and check if they are fixed to zero */
   for( v = 0; v < norigvars; ++v )
   {
      assert(SCIPvarGetType(origvars[v]) == SCIP_VARTYPE_BINARY);

      /* if the upper bound is smaller than 0.5 if follows due to the integrality that the binary variable is fixed to zero */
      if( SCIPvarGetUbLocal(origvars[v]) < 0.5 )
      {
         SCIPdebugMessage("variable <%s> glb=[%.15g,%.15g] loc=[%.15g,%.15g] is fixed to zero\n",
            SCIPvarGetName(origvars[v]), SCIPvarGetLbGlobal(origvars[v]), SCIPvarGetUbGlobal(origvars[v]),
            SCIPvarGetLbLocal(origvars[v]), SCIPvarGetUbLocal(origvars[v]) );

         /* coolect the constraints/items the variable belongs to */
         vardata = SCIPvarGetData(origvars[v]);
         nconsids = SCIPvardataGetNConsids(vardata);
         consids = SCIPvardataGetConsids(vardata);
         needed = TRUE;

         SCIP_CALL( SCIPallocBufferArray(subscip, &logicorvars, nitems) );
         nlogicorvars = 0;
         consid = consids[0];
         nvars = 0;

         /* loop over these items and create a linear (logicor) constraint which forbids this item combination in the
          * pricing problem; thereby check if this item combination is already forbidden
          */
         for( c = 0, o = 0; o < nitems && needed; ++o )
         {
            assert(o <= consid);
            cons = conss[o];

            if( SCIPconsIsEnabled(cons) )
            {
               assert( SCIPgetNFixedonesSetppc(scip, cons) == 0 );

               var = vars[nvars];
               nvars++;
               assert(var != NULL);

               if( o == consid )
               {
                  SCIP_CALL( SCIPgetNegatedVar(subscip, var, &var) );
               }

               logicorvars[nlogicorvars] = var;
               nlogicorvars++;
            }
            else if( o == consid )
               needed = FALSE;

            if( o == consid )
            {
               c++;
               if ( c == nconsids )
                  consid = nitems + 100;
               else
               {
                  assert(consid < consids[c]);
                  consid = consids[c];
               }
            }
         }

         if( needed )
         {
            SCIP_CALL( SCIPcreateConsBasicLogicor(subscip, &cons, SCIPvarGetName(origvars[v]), nlogicorvars, logicorvars) );
            SCIP_CALL( SCIPsetConsInitial(subscip, cons, FALSE) );

            SCIP_CALL( SCIPaddCons(subscip, cons) );
            SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
         }

         SCIPfreeBufferArray(subscip, &logicorvars);
      }
   }

   return SCIP_OKAY;
}

/** initializes the pricing problem for the given capacity */
static
SCIP_RETCODE initPricing(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRICERDATA*      pricerdata,         /**< pricer data */
   SCIP*                 subscip,            /**< pricing SCIP data structure */
   SCIP_VAR**            vars,               /**< variable array  */
   int                   k,                  /**< index of current subproblem */
   double*               alpha,
   double*               beta,
   double*               gamma
   )
{
   SCIP_CONS** conss;
   SCIP_CONS* cons;
   SCIP_VAR* var;
   SCIP_PROBDATA* probdata;

   int nvars;
   int i,j,l;

   OAARNode* Nodes;
   OAARLink* Links;
   OAARFlow* Flows;
   int nNodes, nOpticalNodes, nElecNodes;
   int nLinks, nOpticalLinks, nElecLinks;
   int nFlows;
   int nCons;
   int* nFlowSol;

   double tempDelay, tempJitter, tempBandCost, tempC;
   char tempName[SCIP_MAXSTRLEN];
   OAARNode tempNode;
   OAARLink tempLink;

   assert( SCIPgetStage(subscip) == SCIP_STAGE_PROBLEM );
   assert(pricerdata != NULL);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);

   conss = pricerdata->conss;
   Nodes = pricerdata->Nodes;
   Links = pricerdata->Links;
   Flows = pricerdata->Flows;
   nNodes = pricerdata->nNodes;
   nOpticalNodes = pricerdata->nOpticalNodes;
   nLinks = pricerdata->nLinks;
   nOpticalLinks = pricerdata->nOpticalLinks;
   nFlows = pricerdata->nFlows;
   nCons = pricerdata->nCons;
   nFlowSol = SCIPprobdataGetNFlowSol(probdata);
   nvars = 0;
   nElecLinks = nLinks - nOpticalLinks;
   nElecNodes = nNodes - nOpticalNodes;
   //create variables
   //create x 
   for(i = 0; i < nLinks; i++)
   {
      tempLink = Links[i];
      tempNode = Nodes[tempLink.Head];
      tempDelay = tempNode.ProcDelay + tempNode.QueueDelay + tempLink.PropDelay + tempLink.TransDelay;
      tempJitter = tempNode.Jitter;
      tempBandCost = tempLink.BandCost;
      tempC = Flows[k].DelayPrice * tempDelay + Flows[k].JitterPrice * tempJitter + 
                 Flows[k].BandWidth * tempBandCost;
      if(i >= nOpticalLinks)
      {
         tempC -= alpha[i-nOpticalLinks] * Flows[k].BandWidth;
      }
      (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "PricerVarX_%d", i);
      SCIP_CALL( SCIPcreateVarBasic(subscip, &var, tempName, 0.0, 1.0, -tempC,
         SCIP_VARTYPE_BINARY) );
      SCIPdebugMessage("Create variable %s with objective coef %lf \n", tempName, -tempC);
      SCIP_CALL( SCIPaddVar(subscip, var) );
      vars[i] = var;
      SCIP_CALL( SCIPreleaseVar(subscip, &var) );
   }
   //create y
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "PricerVarY_%d_%d", i, j);
         SCIP_CALL( SCIPcreateVarBasic(subscip, &var, tempName, 0.0, 1.0, beta[i*nWaveLength+j],
	    SCIP_VARTYPE_BINARY) );
	 SCIPdebugMessage("Create variable %s with objective coef %lf \n", tempName, beta[i*nWaveLength+j]);
	 SCIP_CALL( SCIPaddVar(subscip, var) );
	 vars[nLinks+i*nWaveLength+j] = var;
	 SCIP_CALL( SCIPreleaseVar(subscip, &var) );
      }
   }
   //create z
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "PricerVarZ_%d_%d", i, j);
	 SCIP_CALL( SCIPcreateVarBasic(subscip, &var, tempName, 0.0, 1.0, 0, SCIP_VARTYPE_BINARY) );
	 SCIPdebugMessage("Create variable %s with objective coef 0 \n", tempName);
	 SCIP_CALL( SCIPaddVar(subscip, var) );
	 vars[nLinks+nOpticalLinks*nWaveLength+i*nWaveLength+j] = var;
	 SCIP_CALL( SCIPreleaseVar(subscip, &var) );
      }
   }


   //create subCons1
   SCIP_CALL( SCIPcreateConsBasicSetpart(subscip, &cons, "subCons1", 0, NULL) );
   SCIP_CALL( SCIPaddCons(subscip, cons) );
   for(i = 0; i < nLinks; i++)
   {
      if(Links[i].Head == Flows[k].Source)
      {
         SCIP_CALL( SCIPaddCoefSetppc(subscip, cons, vars[i]) );
      }
   }
   SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

   //create subCons2
   for(i = 0; i < nNodes; i++)
   {
      if(i == Flows[k].Source || i == Flows[k].Destination) continue;
      (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "subCons2_%d", i);
      SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, tempName, 0, NULL, NULL, 0, 0) );
      SCIP_CALL( SCIPaddCons(subscip, cons) );
      //add vars to the cons
      for(j = 0; j < nLinks; j++)
      {
         if(Links[j].Head == i)
	 {
	    SCIP_CALL( SCIPaddCoefLinear(subscip, cons, vars[j], 1) );
	 }
	 else if(Links[j].Tail == i)
	 {
	    SCIP_CALL( SCIPaddCoefLinear(subscip, cons, vars[j], -1) );
	 }
      }
      SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
   }

   //create subCons3
   SCIP_CALL( SCIPcreateConsBasicSetpart(subscip, &cons, "subCons3", 0, NULL) );
   SCIP_CALL( SCIPaddCons(subscip, cons) );
   for(i = 0; i < nLinks; i++)
   {
      if(Links[i].Tail == Flows[k].Destination)
      { 
         SCIP_CALL( SCIPaddCoefSetppc(subscip, cons, vars[i]) );
      }
   }
   SCIP_CALL( SCIPreleaseCons(subscip, &cons) );

   //create subCons4
   for(i = 0; i < nOpticalLinks; i++)
   {
      (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "subCons4_%d", i);
      SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, tempName, 0, NULL, NULL, 0, SCIPinfinity(subscip)) );
      SCIP_CALL( SCIPaddCons(subscip, cons) );
      SCIP_CALL( SCIPaddCoefLinear(subscip, cons, vars[i], -Flows[k].BandWidth) );
      for(j = 0; j < nWaveLength; j++)
      {
         SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	    vars[nLinks+nOpticalLinks*nWaveLength+i*nWaveLength+j], WaveLengthBand) );
      }
      SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
   }

   //create subCons5
   for(i = 0; i < nOpticalNodes; i++)
   {
      if(i == Flows[k].Destination) continue;
      for(j = 0; j < nWaveLength; j++)
      {
         (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "subCons5_%d_%d", i, j);
	 SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, tempName, 0, NULL, NULL, 0, 0) );
	 SCIP_CALL( SCIPaddCons(subscip, cons) );
	 for(l = 0; l < nOpticalLinks; l++)
	 {
	    if(Links[l].Tail == i)
	    {
	       SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	          vars[nLinks+nOpticalLinks*nWaveLength+l*nWaveLength+j], 1) );
	    }
	    else if(Links[l].Head == i)
	    {
	       SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	          vars[nLinks+nOpticalLinks*nWaveLength+l*nWaveLength+j], -1) );
	    }
	 }
	 SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
      }
   }

   //create subCons6
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "subCons6_%d_%d", i, j);
	 SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, tempName, 0, NULL, NULL, 0, SCIPinfinity(subscip)) );
	 SCIP_CALL( SCIPaddCons(subscip, cons) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons, vars[i], 1) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	    vars[nLinks+i*nWaveLength+j], 1) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	    vars[nLinks+nOpticalLinks*nWaveLength+i*nWaveLength+j], -2) );
	 SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
      }
   }

   //create subCons7
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         (void)SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "subCons7_%d_%d", i, j);
	 SCIP_CALL( SCIPcreateConsBasicLinear(subscip, &cons, tempName, 0, NULL, NULL, -1, SCIPinfinity(subscip)) );
	 SCIP_CALL( SCIPaddCons(subscip, cons) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons, vars[i], -1) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons, 
	    vars[nLinks+i*nWaveLength+j], -1) );
	 SCIP_CALL( SCIPaddCoefLinear(subscip, cons,
	    vars[nLinks+nOpticalLinks*nWaveLength+i*nWaveLength+j], 1) );
	 SCIP_CALL( SCIPreleaseCons(subscip, &cons) );
      }
   }

   /* add constraint of the branching decisions */
   SCIP_CALL( addBranchingDecisionConss(scip, subscip, vars, pricerdata->conshdlr, k) );

   /* avoid to generate columns which are fixed to zero */
   //SCIP_CALL( addFixedVarsConss(scip, subscip, vars, conss, nitems) );

   //SCIPfreeBufferArray(subscip, &vals);

   return SCIP_OKAY;

}

/**@} */

/**name Callback methods
 *
 * @{
 */

/** destructor of variable pricer to free user data (called when SCIP is exiting) */
static
SCIP_DECL_PRICERFREE(pricerFreeOAAR)
{
   SCIP_PRICERDATA* pricerdata;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);

   if( pricerdata != NULL)
   {
      /* free memory */
      SCIPfreeMemoryArrayNull(scip, &pricerdata->conss);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->Nodes);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->Links);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->Flows);

      SCIPfreeMemory(scip, &pricerdata);
   }

   return SCIP_OKAY;
}


/** initialization method of variable pricer (called after problem was transformed) */
static
SCIP_DECL_PRICERINIT(pricerInitOAAR)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;
   SCIP_CONS* cons;
   int c;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   /* get transformed constraints */
   for( c = 0; c < pricerdata->nCons; ++c )
   {
      cons = pricerdata->conss[c];

      /* release original constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &pricerdata->conss[c]) );

      /* get transformed constraint */
      SCIP_CALL( SCIPgetTransformedCons(scip, cons, &pricerdata->conss[c]) );

      /* capture transformed constraint */
      SCIP_CALL( SCIPcaptureCons(scip, pricerdata->conss[c]) );
   }

   return SCIP_OKAY;
}


/** solving process deinitialization method of variable pricer (called before branch and bound process data is freed) */
static
SCIP_DECL_PRICEREXITSOL(pricerExitsolOAAR)
{
   SCIP_PRICERDATA* pricerdata;
   int c;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   /* get release constraints */
   for( c = 0; c < pricerdata->nCons; ++c )
   {
      /* release constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &(pricerdata->conss[c])) );
   }

   return SCIP_OKAY;
}


/** reduced cost pricing method of variable pricer for feasible LPs */
static
SCIP_DECL_PRICERREDCOST(pricerRedcostOAAR)
{  /*lint --e{715}*/
   SCIP* subscip;
   SCIP_PRICERDATA* pricerdata;
   SCIP_PROBDATA* probdata;
   SCIP_CONS** conss;
   SCIP_CONS* cons;
   SCIP_VAR** vars;
   SCIP_Bool addvar;

   SCIP_SOL** sols;
   int nsols;
   int s;
   int i,j;

   int nNodes, nLinks, nFlows;
   int nOpticalNodes, nOpticalLinks;
   int nElecLinks;
   int* nFlowSol;
   OAARNode* Nodes;
   OAARLink* Links;
   OAARFlow* Flows;

   double* alpha;
   double* beta;
   double* gamma;

   int k; //current index of subproblem
   int nAddedColumn;

   char name[SCIP_MAXSTRLEN];


   SCIP_Real timelimit;
   SCIP_Real memorylimit;

   assert(scip != NULL);
   assert(pricer != NULL);

   (*result) = SCIP_DIDNOTRUN;

   /* get the pricer data */
   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);

   conss = pricerdata->conss;
   Nodes = pricerdata->Nodes;
   Links = pricerdata->Links;
   Flows = pricerdata->Flows;
   nNodes = pricerdata->nNodes;
   nOpticalNodes = pricerdata->nOpticalNodes;
   nLinks = pricerdata->nLinks;
   nOpticalLinks = pricerdata->nOpticalLinks;
   nFlows = pricerdata->nFlows;
   nFlowSol = SCIPprobdataGetNFlowSol(probdata);
   nElecLinks = nLinks - nOpticalLinks;

   SCIP_CALL( SCIPallocBufferArray(scip, &alpha, nElecLinks) );
   SCIP_CALL( SCIPallocBufferArray(scip, &beta, nOpticalLinks*nWaveLength) );
   SCIP_CALL( SCIPallocBufferArray(scip, &gamma, nFlows) );

   //get dual values
   //gamma for FlowCons1
   for(i = 0; i < nFlows; i++)
   {
      cons = conss[i];

      //TODO: check constraint handler

      gamma[i] = SCIPgetDualsolSetppc(scip, cons);
   }
   //alpha for ElecCons2
   for(i = 0; i < nElecLinks; i++)
   {
      cons = conss[i+nFlows];
      alpha[i] = SCIPgetDualsolKnapsack(scip, cons);
   }
   //beta for OpticalCons3
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         cons = conss[i*nWaveLength+j+nFlows+nElecLinks];
         beta[i*nWaveLength+j] = SCIPgetDualsolSetppc(scip, cons);
      }
   }

   SCIPdebugMessage("Print dual values:\n");
   for(i = 0; i < nFlows; i++)
      SCIPdebugPrintf("gamma[%d]:%lf ", i, gamma[i]);
   SCIPdebugPrintf("\n");
   for(i = 0; i < nElecLinks; i++)
      SCIPdebugPrintf("alpha[%d]:%lf ", i, alpha[i]);
   SCIPdebugPrintf("\n");
   for(i = 0; i < nOpticalLinks; i++)
   {
      for(j = 0; j < nWaveLength; j++)
      {
         SCIPdebugPrintf("beta[%d][%d]: %lf ", i, j, beta[i*nWaveLength+j]);
      }
      SCIPdebugPrintf("\n");
   }



   for(k = 0; k < nFlows; k++)
   {

      /* get the remaining time and memory limit */
      SCIP_CALL( SCIPgetRealParam(scip, "limits/time", &timelimit) );
      if( !SCIPisInfinity(scip, timelimit) )
	 timelimit -= SCIPgetSolvingTime(scip);
      SCIP_CALL( SCIPgetRealParam(scip, "limits/memory", &memorylimit) );
      if( !SCIPisInfinity(scip, memorylimit) )
	 memorylimit -= SCIPgetMemUsed(scip)/1048576.0;

      /* initialize SCIP */
      SCIP_CALL( SCIPcreate(&subscip) );
      SCIP_CALL( SCIPincludeDefaultPlugins(subscip) );

      /* create problem in sub SCIP */
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "pricing_%d", k);
      SCIP_CALL( SCIPcreateProbBasic(subscip, name) );
      SCIP_CALL( SCIPsetObjsense(subscip, SCIP_OBJSENSE_MAXIMIZE) );

      /* do not abort subproblem on CTRL-C */
      SCIP_CALL( SCIPsetBoolParam(subscip, "misc/catchctrlc", FALSE) );

      /* disable output to console */
      SCIP_CALL( SCIPsetIntParam(subscip, "display/verblevel", 0) );

      /* set time and memory limit */
      SCIP_CALL( SCIPsetRealParam(subscip, "limits/time", timelimit) );
      SCIP_CALL( SCIPsetRealParam(subscip, "limits/memory", memorylimit) );

      //nx(nLinks) + ny(nOpticalLinks*nWavelength) + nz(nOpticalLinks*nWavelength)
      SCIP_CALL( SCIPallocMemoryArray(subscip, &vars, nLinks + 2*nOpticalLinks*nWaveLength) );

      /* initialization local pricing problem */
      SCIP_CALL( initPricing(scip, pricerdata, subscip, vars, k, alpha, beta, gamma) );

      SCIPdebugMessage("solve pricer problem %d\n", k);

      /* solve sub SCIP */
      SCIP_CALL( SCIPsolve(subscip) );

      sols = SCIPgetSols(subscip);
      nsols = SCIPgetNSols(subscip);
      addvar = FALSE;

      if(nsols > 3) 
         nAddedColumn = 3;
      else
         nAddedColumn = nsols;

      /* loop over all solutions and create the corresponding column to master if the reduced cost are negative for master,
       * that is the objective value i greater than gamma_k
       */
      for( s = 0; s < nAddedColumn; ++s )
      {
	 SCIP_Bool feasible;
	 SCIP_SOL* sol;

	 /* the soultion should be sorted w.r.t. the objective function value */
	 assert(s == 0 || SCIPisFeasGE(subscip, SCIPgetSolOrigObj(subscip, sols[s-1]), SCIPgetSolOrigObj(subscip, sols[s])));

	 sol = sols[s];
	 assert(sol != NULL);

	 /* check if solution is feasible in original sub SCIP */
	 SCIP_CALL( SCIPcheckSolOrig(subscip, sol, &feasible, FALSE, FALSE ) );

	 if( !feasible )
	 {
	    SCIPwarningMessage(scip, "solution in pricing problem %d is infeasible\n", k);
	    continue;
	 }

	 /* check if the solution has a value greater than gamma_k */
	 if( SCIPisFeasGT(subscip, SCIPgetSolOrigObj(subscip, sol), -gamma[k]) )
	 {
	    SCIP_VAR* var;
	    SCIP_VARDATA* vardata;
	    int* consids;
	    int nconsids;
	    char strtmp[SCIP_MAXSTRLEN];
	    char tempName[SCIP_MAXSTRLEN];
	    int nconss;

	    OAARNode tempNode;
	    OAARLink tempLink;
	    OAARFlow tempFlow;
	    double tempJitter;
	    double tempDelay;
	    double tempBandCost;
	    double tempObj;

	    int* oriFlowVars;
	    int nOriFlowVars;

	    SCIPdebug( SCIP_CALL( SCIPprintSol(subscip, sol, NULL, FALSE) ) );

	    nconss = 0;

	    //1(cons1) + nElecLinks(cons2) + nOpticalLinks*nWaveLength(cons3)
	    SCIP_CALL( SCIPallocBufferArray(scip, &consids, 1+nElecLinks+nOpticalLinks*nWaveLength) );

	    //construct consids
	    consids[0] = k; nconss++;
	    for(i = 0; i < nElecLinks; i++)
	    {
	       if( SCIPgetSolVal(subscip, sol, vars[nOpticalLinks+i]) > 0.5)
	       {
	          consids[nconss] = nFlows + i;
		  nconss++;
	       }
	       else
	       {
	          assert( SCIPisFeasEQ(subscip, SCIPgetSolVal(subscip, sol, 
		     vars[nOpticalLinks+i]), 0.0) );
	       }
	    }
	    for(i = 0; i < nOpticalLinks; i++)
	    {
	       for(j = 0; j < nWaveLength; j++)
	       {
	          if( SCIPgetSolVal(subscip, sol, vars[nLinks+i*nWaveLength+j]) > 0.5 )
		  {
		     consids[nconss] = nFlows + nElecLinks + i*nWaveLength + j;
		     nconss++;
		  }
		  else
		  {
		     assert( SCIPisFeasEQ(subscip, SCIPgetSolVal(subscip, sol, 
		        vars[nLinks+i*nWaveLength+j]), 0.0) );
		  }
	       }
	    }

            nOriFlowVars = nLinks+2*nOpticalLinks*nWaveLength;
	    SCIP_CALL( SCIPallocBufferArray(scip, &oriFlowVars, nOriFlowVars) );
	    for(i = 0; i < nOriFlowVars; i++)
	    {
	       if( SCIPgetSolVal(subscip, sol, vars[i]) > 0.5  )
	       {
	          oriFlowVars[i] = 1;
	       }
	       else
	       {
	          oriFlowVars[i] = 0;
	       }
	    }

	    SCIP_CALL( SCIPvardataCreateOAAR(scip, &vardata, consids, nconss, oriFlowVars, 
	       nOriFlowVars) );

	    (void) SCIPsnprintf(tempName, SCIP_MAXSTRLEN, "lambda_%d_%d", k, nFlowSol[k]);

	    /* create variable for a new column */
	    tempFlow = Flows[k];
	    tempDelay = 0; tempJitter = 0; tempBandCost = 0;
	    for(i = 0; i < nLinks; i++)
	    {
	       if( SCIPgetSolVal(subscip, sol, vars[i]) > 0.5 )
	       {
	          tempNode = Nodes[Links[i].Head];
	          tempLink = Links[i];
		  tempDelay += tempNode.ProcDelay + tempNode.QueueDelay + 
		     tempLink.PropDelay + tempLink.TransDelay;
		  tempJitter += tempNode.Jitter;
		  tempBandCost += tempLink.BandCost;
		  SCIPdebugMessage("Include link %d\n", i);
	       }
	    }
	    tempObj = tempFlow.Priority * (tempFlow.DelayPrice * tempDelay + 
	       tempFlow.JitterPrice * tempJitter + tempFlow.BandWidth * tempBandCost);
	    SCIP_CALL( SCIPcreateVarOAAR(scip, &var, tempName, tempObj, FALSE, TRUE, vardata) );
	    SCIPdebugMessage("Add variable %s with obj %lf\n", tempName, tempObj);
	    SCIPdebugMessage("tempDelay:%lf, tempJitter:%lf, tempBandCost:%lf\n",
	       tempDelay, tempJitter, tempBandCost);

	    /* add the new variable to the pricer store */
	    SCIP_CALL( SCIPaddPricedVar(scip, var, 1.0) );
	    addvar = TRUE;

	    SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

	    for( i = 0; i < nconss; i++ )
	    {
	       //assert(SCIPconsIsEnabled(conss[consids[i]]));
	       if(i == 0)
	       {
	          SCIP_CALL( SCIPaddCoefSetppc(scip, conss[consids[i]], var) );
	       }
	       else if( consids[i] < nFlows + nElecLinks )
	       {
	          SCIP_CALL( SCIPaddCoefKnapsack(scip, conss[consids[i]], var, tempFlow.BandWidth) );
	       }
	       else
	       {
	          SCIP_CALL( SCIPaddCoefSetppc(scip, conss[consids[i]], var) );
	       }
	    }

	    SCIPdebug(SCIPprintVar(scip, var, NULL) );
	    SCIP_CALL( SCIPreleaseVar(scip, &var) );

	    SCIPfreeBufferArray(scip, &consids);
	    SCIPfreeBufferArray(scip, &oriFlowVars);
	 }
	 else
	 {
	    SCIPdebugMessage("No variable newly generated for flow %d\n", k);
	    break;
	 }
      }

      /* free pricer MIP */
      SCIPfreeMemoryArray(subscip, &vars);
      
      if( addvar || SCIPgetStatus(subscip) == SCIP_STATUS_OPTIMAL )
	 (*result) = SCIP_SUCCESS;

      /* free sub SCIP */
      SCIP_CALL( SCIPfree(&subscip) );

   }

   SCIPfreeBufferArray(scip, &alpha);
   SCIPfreeBufferArray(scip, &beta);
   SCIPfreeBufferArray(scip, &gamma);

   return SCIP_OKAY;
}

/** farkas pricing method of variable pricer for infeasible LPs */
static
SCIP_DECL_PRICERFARKAS(pricerFarkasOAAR)
{  /*lint --e{715}*/

   /** @note In case of this binpacking example, the master LP should not get infeasible after branching, because of the
    *        way branching is performed. Therefore, the Farkas pricing is not implemented.
    *        1. In case of Ryan/Foster branching, the two items are selected in a way such that the sum of the LP values
    *           of all columns/packings containing both items is fractional. Hence, it exists at least one
    *           column/packing which contains both items and also at least one column/packing for each item containing
    *           this but not the other item. That means, branching in the "same" direction stays LP feasible since there
    *           exists at least one column/packing with both items and branching in the "differ" direction stays LP
    *           feasible since there exists at least one column/packing containing one item, but not the other.
    *        2. In case of variable branching, we only branch on fractional variables. If a variable is fixed to one,
    *           there is no issue.  If a variable is fixed to zero, then we know that for each item which is part of
    *           that column/packing, there exists at least one other column/packing containing this particular item due
    *           to the covering constraints.
    */
   SCIPwarningMessage(scip, "Current master LP is infeasible, but Farkas pricing was not implemented\n");
   SCIPABORT();

   return SCIP_OKAY;
}

/**@} */


/**@name Interface methods
 *
 * @{
 */

/** creates the OAAR variable pricer and includes it in SCIP */
SCIP_RETCODE SCIPincludePricerOAAR(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_PRICERDATA* pricerdata;
   SCIP_PRICER* pricer;

   SCIP_CALL( SCIPallocMemory(scip, &pricerdata) );

   pricerdata->conshdlr = SCIPfindConshdlr(scip, "zeroone");
   assert(pricerdata->conshdlr != NULL);

   pricerdata->conss = NULL;
   pricerdata->Nodes = NULL;
   pricerdata->Links = NULL;
   pricerdata->Flows = NULL;
   pricerdata->nNodes = 0;
   pricerdata->nOpticalNodes = 0;
   pricerdata->nLinks = 0;
   pricerdata->nOpticalLinks = 0;
   pricerdata->nFlows = 0;
   pricerdata->nCons = 0;
   //pricerdata->nFlowSol = NULL;

   /* include variable pricer */
   SCIP_CALL( SCIPincludePricerBasic(scip, &pricer, PRICER_NAME, PRICER_DESC, PRICER_PRIORITY, PRICER_DELAY,
         pricerRedcostOAAR, pricerFarkasOAAR, pricerdata) );

   SCIP_CALL( SCIPsetPricerFree(scip, pricer, pricerFreeOAAR) );
   SCIP_CALL( SCIPsetPricerInit(scip, pricer, pricerInitOAAR) );
   SCIP_CALL( SCIPsetPricerExitsol(scip, pricer, pricerExitsolOAAR) );

   return SCIP_OKAY;
}


/** added problem specific data to pricer and activates pricer */
SCIP_RETCODE SCIPpricerOAARActivate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           conss,              /**< set covering constraints for the items */
   OAARNode*             Nodes,
   OAARLink*             Links,
   OAARFlow*             Flows,
   int                   nNodes,
   int                   nOpticalNodes,
   int                   nLinks,
   int                   nOpticalLinks,
   int                   nFlows,
   int                   nCons
   //int*                  nFlowSol
   )
{
   SCIP_PRICER* pricer;
   SCIP_PRICERDATA* pricerdata;
   int c;

   assert(scip != NULL);
   assert(conss != NULL);

   pricer = SCIPfindPricer(scip, PRICER_NAME);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   /* copy arrays */
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &pricerdata->conss, conss, nCons) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &pricerdata->Nodes, Nodes, nNodes) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &pricerdata->Links, Links, nLinks) );
   SCIP_CALL( SCIPduplicateMemoryArray(scip, &pricerdata->Flows, Flows, nFlows) );
   //SCIP_CALL( SCIPduplicateMemoryArray(scip, &pricerdata->Flows, Flows, nFlows) );

   pricerdata->nNodes = nNodes;
   pricerdata->nOpticalNodes = nOpticalNodes;
   pricerdata->nLinks = nLinks;
   pricerdata->nOpticalLinks = nOpticalLinks;
   pricerdata->nFlows = nFlows;
   pricerdata->nCons = nCons;

   /* capture all constraints */
   for( c = 0; c < nCons; ++c )
   {
      SCIP_CALL( SCIPcaptureCons(scip, conss[c]) );
   }

   /* activate pricer */
   SCIP_CALL( SCIPactivatePricer(scip, pricer) );

   return SCIP_OKAY;
}

/**@} */
