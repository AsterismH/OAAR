/**@file   branch_originalvar.h
 * @brief  branching on the original var
 * @author He Xingqiu
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_BRANCH_RYANFOSTER_H__
#define __SCIP_BRANCH_RYANFOSTER_H__


#include "scip/scip.h"

extern
SCIP_RETCODE SCIPincludeBranchruleOriginalvar(
   SCIP*                 scip                /**< SCIP data structure */
   );

#endif
