/**@file   pricer_OAAR.h
 * @brief  OAAR variable pricer
 * @author He Xingqiu
 *
 * This file implements the variable pricer which check if variables exist with negative reduced cost.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __BIN_PRICER_BINPACKING__
#define __BIN_PRICER_BINPACKING__

#include "scip/scip.h"
#include "OAARdataStructure.h"


extern
SCIP_RETCODE SCIPincludePricerOAAR(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** added problem specific data to pricer and activates pricer */
extern
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
   );

#endif
