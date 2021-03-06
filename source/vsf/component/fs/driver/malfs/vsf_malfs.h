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

#ifndef __VSF_MALFS_H__
#define __VSF_MALFS_H__

/*============================ INCLUDES ======================================*/

#include "../../vsf_fs_cfg.h"

#if VSF_USE_FS == ENABLED && VSF_USE_MALFS == ENABLED

#include "component/mal/vsf_mal.h"

#if     defined(VSF_MALFS_IMPLEMENT)
#   undef VSF_MALFS_IMPLEMENT
#   define __PLOOC_CLASS_IMPLEMENT
#elif   defined(VSF_MALFS_INHERIT)
#   undef VSF_MALFS_INHERIT
#   define __PLOOC_CLASS_INHERIT
#endif

#include "utilities/ooc_class.h"

/*============================ MACROS ========================================*/

#define __implement_malfs_cache(__size, __number)                               \
    __vk_malfs_cache_node_t __cache_nodes[__number];                            \
    uint8_t __buffer[__size * __number];

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

declare_simple_class(__vk_malfs_file_t)
declare_simple_class(__vk_malfs_info_t)
declare_simple_class(__vk_malfs_cache_t)

struct __vk_malfs_cache_node_t {
    uint64_t block_addr;
    uint32_t access_time_sec    : 30;
    uint32_t is_dirty           : 1;
    uint32_t is_alloced         : 1;
};
typedef struct __vk_malfs_cache_node_t __vk_malfs_cache_node_t;

def_simple_class(__vk_malfs_cache_t) {
    public_member(
        uint16_t number;
        __vk_malfs_cache_node_t *nodes;
    )

    private_member(
        __vk_malfs_info_t *info;
#if VSF_FS_CFG_SUPPORT_MULTI_THREAD == ENABLED
        vsf_crit_t crit;
#endif
        struct {
            uint64_t block_addr;
            __vk_malfs_cache_node_t *result;
        } ctx;
    )
};

def_simple_class(__vk_malfs_file_t) {
    implement(vk_file_t)
    public_member(
        void *info;
    )
};

// memory layout:
//  |-------------------------------|
//  |       __vk_malfs_info_t       |
//  |-------------------------------|
//  |  __vk_malfs_cache_node_t[num] |
//  |-------------------------------|
//  |       cache_buffer[num]       |
//  |-------------------------------|
def_simple_class(__vk_malfs_info_t) {
    public_member(
        vk_mal_t *mal;
        uint32_t block_size;
        __vk_malfs_cache_t cache;
    )

    protected_member(
        char *volume_name;
        struct {
            vsf_err_t err;
            uint64_t block_addr;
            uint32_t block_num;
            uint8_t *buff;
        } ctx_io;
    )

    private_member(
        struct {
            __vk_malfs_cache_node_t *node;
        } ctx_cache;
#if VSF_FS_CFG_SUPPORT_MULTI_THREAD == ENABLED
        vsf_crit_t crit;
#endif
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

extern void __vk_malfs_init(__vk_malfs_info_t *info);
extern void __vk_malfs_cache_init(__vk_malfs_info_t *info, __vk_malfs_cache_t *cache);
// read/write/get_cache will lock/unlock automatically
extern vsf_err_t __vk_malfs_alloc_cache(__vk_malfs_info_t *info, __vk_malfs_cache_t *cache, uint_fast64_t block_addr);
extern vsf_err_t __vk_malfs_read(__vk_malfs_info_t *info, uint_fast64_t block_addr, uint_fast32_t block_num, uint8_t *buff);
extern vsf_err_t __vk_malfs_write(__vk_malfs_info_t *info, uint_fast64_t block_addr, uint_fast32_t block_num, uint8_t *buff);

#endif      // VSF_USE_FS && VSF_USE_MALFS
#endif      // __VSF_MALFS_H__
