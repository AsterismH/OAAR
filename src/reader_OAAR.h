/**@file   reader_OAAR.h
 * @brief  OAAR file reader
 * @author He Xingqiu
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_READER_OAAR_H__
#define __SCIP_READER_OAAR_H__


#include "scip/scip.h"
#include "OAARdataStructure.h"


extern
SCIP_RETCODE SCIPincludeReaderOAAR(
   SCIP*                 scip                /**< SCIP data structure */
   );

#endif
