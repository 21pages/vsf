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

/*============================ INCLUDES ======================================*/

#include "./vsf_scsi_cfg.h"

#if VSF_USE_SCSI == ENABLED

#define VSF_SCSI_IMPLEMENT

// TODO: use dedicated include
#include "vsf.h"

/*============================ MACROS ========================================*/

#if VSF_KERNEL_CFG_EDA_SUPPORT_SUB_CALL != ENABLED
#   error VSF_KERNEL_CFG_EDA_SUPPORT_SUB_CALL is needed to use scsi
#endif

#if VSF_USE_KERNEL_SIMPLE_SHELL != ENABLED
#   error VSF_USE_KERNEL_SIMPLE_SHELL must be enabled
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

vsf_err_t vk_scsi_init(vk_scsi_t *pthis)
{
    VSF_SCSI_ASSERT((pthis != NULL) && (pthis->drv != NULL) && (pthis->drv->init != NULL));
    return __vsf_call_eda((uintptr_t)pthis->drv->init, (uintptr_t)pthis);
}

vsf_err_t vk_scsi_fini(vk_scsi_t *pthis)
{
    VSF_SCSI_ASSERT((pthis != NULL) && (pthis->drv != NULL) && (pthis->drv->fini != NULL));
    return __vsf_call_eda((uintptr_t)pthis->drv->fini, (uintptr_t)pthis);
}

bool vk_scsi_prepare_buffer(vk_scsi_t *pthis, uint8_t *cbd, vsf_mem_t *mem)
{
    VSF_SCSI_ASSERT((pthis != NULL) && (pthis->drv != NULL));
    if (pthis->drv->buffer != NULL) {
        return pthis->drv->buffer(pthis, cbd, mem);
    }
    return false;
}

vsf_err_t vk_scsi_execute(vk_scsi_t *pthis, uint8_t *cbd, vsf_mem_t *mem)
{
    VSF_SCSI_ASSERT((pthis != NULL) && (pthis->drv != NULL) && (pthis->drv->execute != NULL));
    pthis->args.cbd = cbd;
    pthis->args.mem = *mem;
#if VSF_USE_SERVICE_VSFSTREAM == ENABLED
    pthis->args.stream = NULL;
#endif
    return __vsf_call_eda((uintptr_t)pthis->drv->execute, (uintptr_t)pthis);
}

#if VSF_USE_SERVICE_VSFSTREAM == ENABLED
vsf_err_t vk_scsi_execute_stream(vk_scsi_t *pthis, uint8_t *cbd, vsf_stream_t *stream)
{
    VSF_SCSI_ASSERT((pthis != NULL) && (pthis->drv != NULL) && (pthis->drv->execute != NULL));
    pthis->args.cbd = cbd;
    pthis->args.stream = stream;
    return __vsf_call_eda((uintptr_t)pthis->drv->execute_stream, (uintptr_t)pthis);
}
#endif

vsf_err_t vk_scsi_get_errcode(vk_scsi_t *pthis, uint32_t *reply_len)
{
    VSF_SCSI_ASSERT(pthis != NULL);
    if (reply_len != NULL) {
        *reply_len = pthis->result.reply_len;
    }
    return pthis->result.errcode;
}

#endif
