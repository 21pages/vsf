/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

#ifndef __VSF_HAL_H__
#define __VSF_HAL_H__

/*============================ INCLUDES ======================================*/
#include "hal/vsf_hal_cfg.h"

#include "./arch/vsf_arch.h"
#include "./driver/driver.h"


/* \note: never include interface.h here, individual device drivers might 
 *        include it their own driver header files.  
 */
//#include "./interface.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

/*! \note initialize level 0/1 hardware abstract layer
 *  \param none
 *  \retval true initialization succeeded.
 *  \retval false initialization failed
 */  
extern bool vsf_hal_init(void);

/*! \note initialize level 2 hardware abstract layer
 *  \param none
 *  \retval true initialization succeeded.
 *  \retval false initialization failed
 */  
extern bool vsf_hal_advance_init(void);

#endif
/* EOF */
