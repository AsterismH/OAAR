/**@file   probdata_OAAR.h
 * @brief  Problem data for OAAR problem
 * @author He Xingqiu
 *
 * This file handles the main problem data used in that project. 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_PROBDATA_BINPACKING__
#define __SCIP_PROBDATA_BINPACKING__

#include "scip/scip.h"
#include "OAARdataStructure.h"

/** sets up the problem data */
extern
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
   );

/** returns array of Nodes */
extern
OAARNode* SCIPprobdataGetNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns array of Links */
extern
OAARLink* SCIPprobdataGetLinks(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns array of Flows */
extern
OAARFlow* SCIPprobdataGetFlows(
   SCIP_PROBDATA*       probdata
   );

/** returns number of Nodes */
extern
int SCIPprobdataGetNNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns number of Optical Nodes */
extern
int SCIPprobdataGetNOpticalNodes(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns number of Links */
extern
int SCIPprobdataGetNLinks(
   SCIP_PROBDATA*        probdata
   );

/** reuturns number of Optical Links */
extern
int SCIPprobdataGetNOpticalLinks(
   SCIP_PROBDATA*        probdata
   );

/** returns number of Flows */
extern
int SCIPprobdataGetNFlows(
   SCIP_PROBDATA*        probdata
   );

/** returns array of all variables itemed in the way they got generated */
extern
SCIP_VAR** SCIPprobdataGetVars(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns number of variables */
extern
int SCIPprobdataGetNVars(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns array of constraints */
extern
SCIP_CONS** SCIPprobdataGetConss(
   SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns number of constraints */
extern
int SCIPprobdataGetNCons(
   SCIP_PROBDATA*        probdata
   );

extern
int* SCIPprobdataGetNFlowSol(
   SCIP_PROBDATA*       probdata
   );

/** adds given variable to the problem data */
extern
SCIP_RETCODE SCIPprobdataAddVar(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata,           /**< problem data */
   SCIP_VAR*             var                 /**< variables to add */
   );

#endif
