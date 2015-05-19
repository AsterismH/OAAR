/**@file   vardata_OAAR.h
 * @brief  Variable data containing the ids of constraints in which the variable appears
 * @author He Xingqiu
 *
 * This file implements the handling of the variable data which is attached to each file. 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_VARDATA_BINPACKING__
#define __SCIP_VARDATA_BINPACKING__

#include "scip/scip.h"

/** create variable data */
extern
SCIP_RETCODE SCIPvardataCreateOAAR(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata,            /**< pointer to vardata */
   int*                  consids,            /**< array of constraints ids */
   int                   nconss,             /**< number of constraints */
   int*                  oriFlowVars,
   int                   nOriFlowVars
   );

/** get number of constraints */
extern
int SCIPvardataGetNConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   );

/** returns sorted constraint id array */
extern
int* SCIPvardataGetConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   );

extern
int* SCIPvardataGetOriFlowVars(
   SCIP_VARDATA*         vardata
   );

extern
int SCIPvardataGetNOriFlowVars(
   SCIP_VARDATA*         vardata
   );

/** creates variable */
extern
SCIP_RETCODE SCIPcreateVarOAAR(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR**            var,                /**< pointer to variable object */
   const char*           name,               /**< name of variable, or NULL for automatic name creation */
   SCIP_Real             obj,                /**< objective function value */
   SCIP_Bool             initial,            /**< should var's column be present in the initial root LP? */
   SCIP_Bool             removable,          /**< is var's column removable from the LP (due to aging or cleanup)? */
   SCIP_VARDATA*         vardata             /**< user data for this specific variable */
   );

/** prints vardata to file stream */
extern
void SCIPvardataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA*         vardata,            /**< variable data */
   FILE*                 file                /**< the text file to store the information into */
   );

#endif
