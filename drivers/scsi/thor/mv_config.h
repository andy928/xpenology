#if !defined(MV_CONFIGURATION_H)
#define MV_CONFIGURATION_H

#ifdef PRODUCTNAME_ODIN
#include "mv_odin.h"

#elif defined(SIMULATOR)

#include "mv_simu.h"

#elif defined(PRODUCTNAME_THOR)

#include "mv_thor.h"

#endif /*PRODUCTNAME_ODIN , PRODUCTNAME_THOR*/

#endif /* MV_CONFIGURATION_H */
