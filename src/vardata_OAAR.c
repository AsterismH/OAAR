/**@file   vardata_OAAR.c
 * @brief  Variable data containing the ids of constraints in which the variable appears
 * @author He Xingqiu
 *
 * This file implements the handling of the variable data which is attached to each file. 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "probdata_OAAR.h"
#include "vardata_OAAR.h"

/** @brief Variable data which is attached to all variables.
 *
 *  This variables data is used to know in which constraints this variables appears. 
 *  Therefore, the variable data
 *  contains the ids of constraints in which the variable is part of. 
 *  Hence, that data give us a column view.
 */
struct SCIP_VarData
{
   int*                  consids;
   int                   nconsids;
   int*                  oriFlowVars;
   int                   nOriFlowVars;
};

/**@name Local methods
 *
 * @{
 */

/** create a vardata */
static
SCIP_RETCODE vardataCreate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata,            /**< pointer to vardata */
   int*                  consids,            /**< array of constraints ids */
   int                   nconsids,           /**< number of constraints */
   int*                  oriFlowVars,
   int                   nOriFlowVars
   )
{
   SCIP_CALL( SCIPallocBlockMemory(scip, vardata) );

   SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*vardata)->consids, consids, nconsids) );
   SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*vardata)->oriFlowVars, oriFlowVars, 
      nOriFlowVars) );
   SCIPsortInt((*vardata)->consids, nconsids);

   (*vardata)->nconsids = nconsids;
   (*vardata)->nOriFlowVars = nOriFlowVars;

   return SCIP_OKAY;
}

/** frees user data of variable */
static
SCIP_RETCODE vardataDelete(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata             /**< vardata to delete */
   )
{
   SCIPfreeBlockMemoryArray(scip, &(*vardata)->consids, (*vardata)->nconsids);
   SCIPfreeBlockMemoryArray(scip, &(*vardata)->oriFlowVars, (*vardata)->nOriFlowVars);
   SCIPfreeBlockMemory(scip, vardata);

   return SCIP_OKAY;
}

/**@} */


/**@name Callback methods
 *
 * @{
 */

/** frees user data of transformed variable (called when the transformed variable is freed) */
static
SCIP_DECL_VARDELTRANS(vardataDelTrans)
{
   SCIP_CALL( vardataDelete(scip, vardata) );

   return SCIP_OKAY;
}

/**@} */


/**@name Interface methods
 *
 * @{
 */

/** create variable data */
SCIP_RETCODE SCIPvardataCreateOAAR(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata,            /**< pointer to vardata */
   int*                  consids,            /**< array of constraints ids */
   int                   nconsids,           /**< number of constraints */
   int*                  oriFlowVars,
   int                   nOriFlowVars
   )
{
   SCIP_CALL( vardataCreate(scip, vardata, consids, nconsids, oriFlowVars, nOriFlowVars) );

   return SCIP_OKAY;
}

/** get number of constraints */
int SCIPvardataGetNConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   )
{
   return vardata->nconsids;
}
/** returns sorted constraint id array */
int* SCIPvardataGetConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   )
{
   /* check if the consids are sorted */
#ifndef NDEBUG
   {
      int i;

      for( i = 1; i < vardata->nconsids; ++i )
         assert( vardata->consids[i-1] < vardata->consids[i]);
   }
#endif

   return vardata->consids;
}
int SCIPvardataGetNOriFlowVars(
   SCIP_VARDATA*       vardata
   )
{
   return vardata->nOriFlowVars;
}
int* SCIPvardataGetOriFlowVars(
   SCIP_VARDATA*       vardata
   )
{
   return vardata->oriFlowVars;
}


/** creates variable */
SCIP_RETCODE SCIPcreateVarOAAR(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR**            var,                /**< pointer to variable object */
   const char*           name,               /**< name of variable, or NULL for automatic name creation */
   SCIP_Real             obj,                /**< objective function value */
   SCIP_Bool             initial,            /**< should var's column be present in the initial root LP? */
   SCIP_Bool             removable,          /**< is var's column removable from the LP (due to aging or cleanup)? */
   SCIP_VARDATA*         vardata             /**< user data for this specific variable */
   )
{
   assert(scip != NULL);
   assert(var != NULL);

   /* create a basic variable object */
   SCIP_CALL( SCIPcreateVarBasic(scip, var, name, 0.0, 1.0, obj, SCIP_VARTYPE_BINARY) );
   assert(*var != NULL);

   /* set callback functions */
   SCIPvarSetData(*var, vardata);
   SCIPvarSetDeltransData(*var, vardataDelTrans);

   /* set initial and removable flag */
   SCIP_CALL( SCIPvarSetInitial(*var, initial) );
   SCIP_CALL( SCIPvarSetRemovable(*var, removable) );

   SCIPvarMarkDeletable(*var);

   SCIPdebug( SCIPprintVar(scip, *var, NULL) );

   return SCIP_OKAY;
}


/** prints vardata to file stream */
void SCIPvardataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA*         vardata,            /**< variable data */
   FILE*                 file                /**< the text file to store the information into */
   )
{
   int i;

   SCIPinfoMessage(scip, file, "consids = {");

   for( i = 0; i < vardata->nconsids; ++i )
   {
      SCIPinfoMessage(scip, file, "%d", vardata->consids[i]);

      if( i < vardata->nconsids - 1 )
         SCIPinfoMessage(scip, file, ",");
   }

   SCIPinfoMessage(scip, file, "}\n");

   SCIPinfoMessage(scip, file, "oriFlowVars = {");
   for(i = 0; i < vardata->nOriFlowVars; i++)
   {
      SCIPinfoMessage(scip, file, "%d", vardata->oriFlowVars[i]);
      if(i < vardata->nOriFlowVars)
         SCIPinfoMessage(scip, file, ",");
   }
   SCIPinfoMessage(scip, file, "}\n");
}

/**@} */
