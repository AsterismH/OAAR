/**@file   cons_zeroone.c
 * @brief  Constraint handler stores the local branching decision data
 * @author He Xingqiu
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#define SCIP_DEBUG

#include <assert.h>
#include <string.h>

#include "cons_zeroone.h"
#include "probdata_OAAR.h"
#include "vardata_OAAR.h"


/**@name Constraint handler properties
 *
 * @{
 */

#define CONSHDLR_NAME          "zeroone"
#define CONSHDLR_DESC          "stores the local branching decisions"
#define CONSHDLR_SEPAPRIORITY         0 /**< priority of the constraint handler for separation */
#define CONSHDLR_ENFOPRIORITY         0 /**< priority of the constraint handler for constraint enforcing */
#define CONSHDLR_CHECKPRIORITY  9999999 /**< priority of the constraint handler for checking feasibility */
#define CONSHDLR_SEPAFREQ            -1 /**< frequency for separating cuts; zero means to separate only in the root node */
#define CONSHDLR_PROPFREQ             1 /**< frequency for propagating domains; zero means only preprocessing propagation */
#define CONSHDLR_EAGERFREQ            1 /**< frequency for using all instead of only the useful constraints in separation,
                                         *   propagation and enforcement, -1 for no eager evaluations, 0 for first only */
#define CONSHDLR_MAXPREROUNDS         0 /**< maximal number of presolving rounds the constraint handler participates in (-1: no limit) */
#define CONSHDLR_DELAYSEPA        FALSE /**< should separation method be delayed, if other separators found cuts? */
#define CONSHDLR_DELAYPROP        FALSE /**< should propagation method be delayed, if other propagators found reductions? */
#define CONSHDLR_DELAYPRESOL      FALSE /**< should presolving method be delayed, if other presolvers found reductions? */
#define CONSHDLR_NEEDSCONS         TRUE /**< should the constraint handler be skipped, if no constraints are available? */

#define CONSHDLR_PROP_TIMING       SCIP_PROPTIMING_BEFORELP

/**@} */

/*
 * Data structures
 */

/** @brief Constraint data  */
struct SCIP_ConsData
{
   int                   index1;            //index of flow 
   int                   index2;            //index of link
   CONSTYPE              type;              // zero or one 

   int                   npropagatedvars;    /**< number of variables that existed, the last time, the related node was
                                              *   propagated, used to determine whether the constraint should be
                                              *   repropagated*/
   int                   npropagations;      /**< stores the number propagations runs of this constraint */
   unsigned int          propagated:1;       /**< is constraint already propagated? */
   SCIP_NODE*            node;               /**< the node in the B&B-tree at which the cons is sticking */
};

/** @brief Constraint handler data for  */
struct SCIP_ConshdlrData
{
   int                   dummy;              /**< a dummy struct member to avoid compiling problem with an empty struct */
};




/**@name Local methods
 *
 * @{
 */

/** create constraint data */
static
SCIP_RETCODE consdataCreate(
   SCIP*                 scip,              
   SCIP_CONSDATA**       consdata,         
   int                   index1,
   int                   index2,
   CONSTYPE              type,   
   SCIP_NODE*            node               
   )
{
   assert( scip != NULL );
   assert( consdata != NULL );
   assert( index1 >= 0 );
   assert( index2 >= 0 );
   assert( type == ZERO || type == ONE );

   SCIP_CALL( SCIPallocBlockMemory(scip, consdata) );

   (*consdata)->index1 = index1;
   (*consdata)->index2 = index2;
   (*consdata)->type = type;
   (*consdata)->npropagatedvars = 0;
   (*consdata)->npropagations = 0;
   (*consdata)->propagated = FALSE;
   (*consdata)->node = node;

   return SCIP_OKAY;
}

/** display constraints */
static
void consdataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   FILE*                 file                /**< file stream */
   )
{
   SCIPinfoMessage(scip, file, "%s(%d,%d) at node %d\n",
      consdata->type == ZERO ? "zero" : "one",
      consdata->index1, consdata->index2, SCIPnodeGetNumber(consdata->node) );
}

/** fixes a variable to zero if the corresponding packings are not valid for this constraint/node (due to branching) */
static
SCIP_RETCODE checkVariable(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR*             var,                /**< variables to check  */
   int*                  nfixedvars,         /**< pointer to store the number of fixed variables */
   SCIP_Bool*            cutoff              /**< pointer to store if a cutoff was detected */
   )
{
   SCIP_VARDATA* vardata;
   //int* consids;
   //int nconsids;
   int* oriFlowVars;

   CONSTYPE type;

   SCIP_Bool fixed;
   SCIP_Bool infeasible;

   int nread;
   int tempI, tempJ;
   char* varname;

   assert(scip != NULL);
   assert(consdata != NULL);
   assert(var != NULL);
   assert(nfixedvars != NULL);
   assert(cutoff != NULL);

   /* if variables is locally fixed to zero continue */
   if( SCIPvarGetUbLocal(var) < 0.5 )
      return SCIP_OKAY;

   /* check if the packing which corresponds to the variable feasible for this constraint */
   vardata = SCIPvarGetData(var);

   //nconsids = SCIPvardataGetNConsids(vardata);
   //consids = SCIPvardataGetConsids(vardata);
   oriFlowVars = SCIPvardataGetOriFlowVars(vardata);
   type = consdata->type;

   varname = SCIPvarGetName(var);
   nread = sscanf(varname, "lambda_%d_%d", &tempI, &tempJ);
   if( nread == 0 )
      SCIPdebugMessage("error when parsing var name in cons_zeroone.c checkVariable");
   if(tempI == consdata->index1)
   {
      if(type == ZERO && oriFlowVars[consdata->index2] == 1)
      {
         SCIP_CALL( SCIPfixVar(scip, var, 0.0, &infeasible, &fixed) );
	 
	 if(infeasible)
	 {
	    assert(SCIPvarGetLbLocal(var) > 0.5);
	    SCIPdebugMessage("->cutoff\n");
	    (*cutoff) = TRUE;
	 }
	 else
	 {
	    assert(fixed);
	    (*nfixedvars)++;
	 }
      }

      if(type == ONE && oriFlowVars[consdata->index2] == 0)
      {
         SCIP_CALL( SCIPfixVar(scip, var, 0.0, &infeasible, &fixed) );

	 if(infeasible)
	 {
	    assert(SCIPvarGetLbLocal(var) > 0.5);
	    SCIPdebugMessage("->cutoff\n");
	    (*cutoff) = TRUE;
	 }
	 else
	 {
	    assert(fixed);
	    (*nfixedvars)++;
	 }
      }
   }
   
   return SCIP_OKAY;
}

/** fixes variables to zero if the corresponding packings are not valid for this sonstraint/node (due to branching) */
static
SCIP_RETCODE consdataFixVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR**            vars,               /**< generated variables */
   int                   nvars,              /**< number of generated variables */
   SCIP_RESULT*          result              /**< pointer to store the result of the fixing */
   )
{
   int nfixedvars;
   int v;
   SCIP_Bool cutoff;

   nfixedvars = 0;
   cutoff = FALSE;

   SCIPdebugMessage("check variables %d to %d\n", consdata->npropagatedvars, nvars);

   for( v = consdata->npropagatedvars; v < nvars && !cutoff; ++v )
   {
      SCIP_CALL( checkVariable(scip, consdata, vars[v], &nfixedvars, &cutoff) );
   }

   SCIPdebugMessage("fixed %d variables locally\n", nfixedvars);

   if( cutoff )
      *result = SCIP_CUTOFF;
   else if( nfixedvars > 0 )
      *result = SCIP_REDUCEDDOM;

   return SCIP_OKAY;
}


/** check if all variables are valid for the given consdata */
#ifndef NDEBUG
static
SCIP_Bool consdataCheck(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata,           /**< problem data */
   SCIP_CONSDATA*        consdata            /**< constraint data */
   )
{
   SCIP_VAR** vars;
   int nvars;

   SCIP_VARDATA* vardata;
   SCIP_VAR* var;

   int* consids;
   int nconsids;
   int* oriFlowVars;
   CONSTYPE type;

   int v;
   char* varname;

   vars = SCIPprobdataGetVars(probdata);
   nvars = SCIPprobdataGetNVars(probdata);
   
   int nread, tempI, tempJ;

   for( v = 0; v < nvars; ++v )
   {
      var = vars[v];

      /* if variables is locally fixed to zero continue */
      if( SCIPvarGetLbLocal(var) < 0.5 )
         continue;

      /* check if the packing which corresponds to the variable is feasible for this constraint */
      vardata = SCIPvarGetData(var);

      nconsids = SCIPvardataGetNConsids(vardata);
      consids = SCIPvardataGetConsids(vardata);
      oriFlowVars = SCIPvardataGetOriFlowVars(vardata);

      type = consdata->type;

      varname = SCIPvarGetName(var);
      nread = sscanf(varname, "lambda_%d_%d", &tempI, &tempJ);
      if(tempI == consdata->index1)
      {
	 if(type == ZERO && oriFlowVars[consdata->index2] == 1)
	 {
	    SCIPdebug( SCIPvardataPrint(scip, vardata, NULL) );
	    SCIPdebug( consdataPrint(scip, consdata, NULL) );
	    SCIPdebug( SCIPprintVar(scip, var, NULL) );
	    return FALSE;
	 }
      }
   }

   return TRUE;
}
#endif

/** frees a logic or constraint data */
static
SCIP_RETCODE consdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA**       consdata            /**< pointer to the constraint data */
   )
{
   assert(consdata != NULL);
   assert(*consdata != NULL);

   SCIPfreeBlockMemory(scip, consdata);

   return SCIP_OKAY;
}

/**@} */


/**@name Callback methods
 *
 * @{
 */

/** frees specific constraint data */
static
SCIP_DECL_CONSDELETE(consDeleteZeroone)
{  /*lint --e{715}*/
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(consdata != NULL);
   assert(*consdata != NULL);

   /* free LP row and logic or constraint */
   SCIP_CALL( consdataFree(scip, consdata) );

   return SCIP_OKAY;
}

/** transforms constraint data into data belonging to the transformed problem */
static
SCIP_DECL_CONSTRANS(consTransZeroone)
{  /*lint --e{715}*/
   SCIP_CONSDATA* sourcedata;
   SCIP_CONSDATA* targetdata;

   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(SCIPgetStage(scip) == SCIP_STAGE_TRANSFORMING);
   assert(sourcecons != NULL);
   assert(targetcons != NULL);

   sourcedata = SCIPconsGetData(sourcecons);
   assert(sourcedata != NULL);

   /* create constraint data for target constraint */
   SCIP_CALL( consdataCreate(scip, &targetdata,
         sourcedata->index1, sourcedata->index2, sourcedata->type, sourcedata->node) );

   /* create target constraint */
   SCIP_CALL( SCIPcreateCons(scip, targetcons, SCIPconsGetName(sourcecons), conshdlr, targetdata,
         SCIPconsIsInitial(sourcecons), SCIPconsIsSeparated(sourcecons), SCIPconsIsEnforced(sourcecons),
         SCIPconsIsChecked(sourcecons), SCIPconsIsPropagated(sourcecons),
         SCIPconsIsLocal(sourcecons), SCIPconsIsModifiable(sourcecons),
         SCIPconsIsDynamic(sourcecons), SCIPconsIsRemovable(sourcecons), SCIPconsIsStickingAtNode(sourcecons)) );

   return SCIP_OKAY;
}

/** constraint enforcing method of constraint handler for LP solutions */
#define consEnfolpZeroone NULL

/** constraint enforcing method of constraint handler for pseudo solutions */
#define consEnfopsZeroone NULL

/** feasibility check method of constraint handler for integral solutions */
#define consCheckZeroone NULL

/** domain propagation method of constraint handler */
static
SCIP_DECL_CONSPROP(consPropZeroone)
{  /*lint --e{715}*/
   SCIP_PROBDATA* probdata;
   SCIP_CONSDATA* consdata;

   SCIP_VAR** vars;
   int nvars;
   int c;

   assert(scip != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);

   SCIPdebugMessage("propagation constraints of constraint handler <"CONSHDLR_NAME">\n");

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);

   vars = SCIPprobdataGetVars(probdata);
   nvars = SCIPprobdataGetNVars(probdata);

   *result = SCIP_DIDNOTFIND;

   for( c = 0; c < nconss; ++c )
   {
      consdata = SCIPconsGetData(conss[c]);

#ifndef NDEBUG
      {
         /* check if there are no equal consdatas */
         SCIP_CONSDATA* consdata2;
         int i;

         for( i = c+1; i < nconss; ++i )
         {
            consdata2 = SCIPconsGetData(conss[i]);
            assert( !(consdata->index1 == consdata2->index1
                  && consdata->index2 == consdata2->index2
                  && consdata->type == consdata2->type) );
            assert( !(consdata->index1 == consdata2->index2
                  && consdata->index2 == consdata2->index1
                  && consdata->type == consdata2->type) );
         }
      }
#endif

      if( !consdata->propagated )
      {
         SCIPdebugMessage("propagate constraint <%s> ", SCIPconsGetName(conss[c]));
         SCIPdebug( consdataPrint(scip, consdata, NULL) );

         SCIP_CALL( consdataFixVariables(scip, consdata, vars, nvars, result) );
         consdata->npropagations++;

         if( *result != SCIP_CUTOFF )
         {
            consdata->propagated = TRUE;
            consdata->npropagatedvars = nvars;
         }
         else
            break;
      }

      /* check if constraint is completely propagated */
      assert( consdataCheck(scip, probdata, consdata) );
   }

   return SCIP_OKAY;
}

/** variable rounding lock method of constraint handler */
#define consLockZeroone NULL

/** constraint activation notification method of constraint handler */
static
SCIP_DECL_CONSACTIVE(consActiveZeroone)
{  /*lint --e{715}*/
   SCIP_CONSDATA* consdata;

   assert(scip != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   assert(consdata->npropagatedvars <= SCIPprobdataGetNVars(SCIPgetProbData(scip)));

   SCIPdebugMessage("activate constraint <%s> at node <%"SCIP_LONGINT_FORMAT"> in depth <%d>: ",
      SCIPconsGetName(cons), SCIPnodeGetNumber(consdata->node), SCIPnodeGetDepth(consdata->node));
   SCIPdebug( consdataPrint(scip, consdata, NULL) );

   if( consdata->npropagatedvars != SCIPprobdataGetNVars(SCIPgetProbData(scip)) )
   {
      SCIPdebugMessage("-> mark constraint to be repropagated\n");
      consdata->propagated = FALSE;
      SCIP_CALL( SCIPrepropagateNode(scip, consdata->node) );
   }

   return SCIP_OKAY;
}

/** constraint deactivation notification method of constraint handler */
static
SCIP_DECL_CONSDEACTIVE(consDeactiveZeroone)
{  /*lint --e{715}*/
   SCIP_CONSDATA* consdata;
   SCIP_PROBDATA* probdata;

   assert(scip != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);
   assert(consdata->propagated || SCIPgetNChildren(scip) == 0);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);

   /* check if all variables which are not fixed locally to zero are valid for this constraint/node */
   assert( consdataCheck(scip, probdata, consdata) );

   SCIPdebugMessage("deactivate constraint <%s> at node <%"SCIP_LONGINT_FORMAT"> in depth <%d>: ",
      SCIPconsGetName(cons), SCIPnodeGetNumber(consdata->node), SCIPnodeGetDepth(consdata->node));
   SCIPdebug( consdataPrint(scip, consdata, NULL) );

   /* set the number of propagated variables to current number of variables is SCIP */
   consdata->npropagatedvars = SCIPprobdataGetNVars(probdata);

   /* check if all variables are valid for this constraint */
   assert( consdataCheck(scip, probdata, consdata) );

   return SCIP_OKAY;
}

/** constraint display method of constraint handler */
static
SCIP_DECL_CONSPRINT(consPrintZeroone)
{  /*lint --e{715}*/
   SCIP_CONSDATA*  consdata;

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   consdataPrint(scip, consdata, file);

   return SCIP_OKAY;
}

/**@} */

/**@name Interface methods
 *
 * @{
 */

/** creates the handler for zeroone constraints and includes it in SCIP */
SCIP_RETCODE SCIPincludeConshdlrZeroone(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_CONSHDLRDATA* conshdlrdata;
   SCIP_CONSHDLR* conshdlr;

   conshdlrdata = NULL;

   conshdlr = NULL;
   /* include constraint handler */
   SCIP_CALL( SCIPincludeConshdlrBasic(scip, &conshdlr, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY, CONSHDLR_EAGERFREQ, CONSHDLR_NEEDSCONS,
         consEnfolpZeroone, consEnfopsZeroone, consCheckZeroone, consLockZeroone,
         conshdlrdata) );
   assert(conshdlr != NULL);

   SCIP_CALL( SCIPsetConshdlrDelete(scip, conshdlr, consDeleteZeroone) );
   SCIP_CALL( SCIPsetConshdlrTrans(scip, conshdlr, consTransZeroone) );
   SCIP_CALL( SCIPsetConshdlrProp(scip, conshdlr, consPropZeroone, CONSHDLR_PROPFREQ, CONSHDLR_DELAYPROP,
         CONSHDLR_PROP_TIMING) );
   SCIP_CALL( SCIPsetConshdlrActive(scip, conshdlr, consActiveZeroone) );
   SCIP_CALL( SCIPsetConshdlrDeactive(scip, conshdlr, consDeactiveZeroone) );
   SCIP_CALL( SCIPsetConshdlrPrint(scip, conshdlr, consPrintZeroone) );

   return SCIP_OKAY;
}

/** creates and captures a samediff constraint */
SCIP_RETCODE SCIPcreateConsZeroone(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   index1,           /**< item id one */
   int                   index2,           /**< item id two */
   CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   )
{
   SCIP_CONSHDLR* conshdlr;
   SCIP_CONSDATA* consdata;

   /* find the samediff constraint handler */
   conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
   if( conshdlr == NULL )
   {
      SCIPerrorMessage("zeroone constraint handler not found\n");
      return SCIP_PLUGINNOTFOUND;
   }

   /* create the constraint specific data */
   SCIP_CALL( consdataCreate(scip, &consdata, index1, index2, type, node) );

   /* create constraint */
   SCIP_CALL( SCIPcreateCons(scip, cons, name, conshdlr, consdata, FALSE, FALSE, FALSE, FALSE, TRUE,
         local, FALSE, FALSE, FALSE, TRUE) );

   SCIPdebugMessage("created constraint: ");
   SCIPdebug( consdataPrint(scip, consdata, NULL) );

   return SCIP_OKAY;
}

/** returns item id one */
int SCIPgetIndex1Zeroone(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
   SCIP_CONSDATA* consdata;

   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->index1;
}

/** returns item id two */
int SCIPgetIndex2Zeroone(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
   SCIP_CONSDATA* consdata;

   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->index2;
}

/** return constraint type SAME or DIFFER */
CONSTYPE SCIPgetTypeZeroone(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
   SCIP_CONSDATA* consdata;

   assert(cons != NULL);

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   return consdata->type;
}

/**@} */
