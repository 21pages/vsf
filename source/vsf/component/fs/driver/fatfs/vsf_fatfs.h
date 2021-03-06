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

#ifndef __VSF_FATFS_H__
#define __VSF_FATFS_H__

/*============================ INCLUDES ======================================*/

#include "../../vsf_fs_cfg.h"

#if VSF_USE_FS == ENABLED && VSF_USE_FATFS == ENABLED

#include "../malfs/vsf_malfs.h"

#if     defined(VSF_FATFS_IMPLEMENT)
#   undef VSF_FATFS_IMPLEMENT
#   define __PLOOC_CLASS_IMPLEMENT
#elif   defined(VSF_FATFS_INHERIT)
#   undef VSF_FATFS_INHERIT
#   define __PLOOC_CLASS_INHERIT
#endif

#include "utilities/ooc_class.h"

/*============================ MACROS ========================================*/

#define implement_fatfs_info(__block_size, __cache_num)                         \
    implement(__vk_fatfs_info_t)                                                \
    __implement_malfs_cache(__block_size, __cache_num)

#define init_fatfs_info(__info, __block_size, __cache_num)                      \
    .block_size = __block_size,                                                 \
    .cache      = {                                                             \
        .number = __cache_num,                                                  \
        .nodes  = __info.__cache_nodes,                                         \
    },

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

declare_simple_class(vk_fatfs_file_t)
declare_simple_class(__vk_fatfs_info_t)

enum vk_fat_type_t {
    VSF_FAT_NONE,
    VSF_FAT_12,
    VSF_FAT_16,
    VSF_FAT_32,
    VSF_FAT_EX,
};
typedef enum vk_fat_type_t vk_fat_type_t;

enum vk_fat_file_attr_t {
    VSF_FAT_FILE_ATTR_VOLUMID = VSF_FILE_ATTR_EXT,
    VSF_FAT_FILE_ATTR_SYSTEM  = VSF_FILE_ATTR_EXT << 1,
    VSF_FAT_FILE_ATTR_ARCHIVE = VSF_FILE_ATTR_EXT << 2,
};
typedef enum vk_fat_file_attr_t vk_fat_file_attr_t;

struct vk_fatfs_dentry_parser_t {
    uint8_t *entry;
    uint16_t entry_num;
    uint8_t lfn;
    char *filename;
};
typedef struct vk_fatfs_dentry_parser_t vk_fatfs_dentry_parser_t;

def_simple_class(vk_fatfs_file_t) {
    implement(__vk_malfs_file_t)

    private_member(
        uint32_t first_cluster;
        struct {
            uint32_t cluster;
            uint32_t fat_entry;
        } cur;
    )
};

// memory layout:
//  |-----------------------|
//  | __vk_fatfs_info_t:    |
//  |   |---------------|   |
//  |   |      ...      |   |
//  |   |---------------|   |
//  |   |vk_malfs_info_t|   |
//  |   |---------------|   |
//  |-----------------------|
//  | vk_malfs_cache_t[num] |
//  |-----------------------|
//  |   cache_buffer[num]   |
//  |-----------------------|
def_simple_class(__vk_fatfs_info_t) {
    private_member(
        vk_fatfs_file_t root;
        vk_fat_type_t type;
        char fat_volume_name[12];

        // fat parameters, will not change after mount
        uint32_t root_sector;
        uint32_t root_size;
        uint32_t fat_sector;
        uint32_t fat_size;
        uint32_t data_sector;
        uint32_t cluster_num;
        uint8_t sector_size_bits;
        uint8_t cluster_size_bits;
        uint8_t fat_num;

        // context
        uint32_t cur_fat_bit;
        uint32_t cur_cluster;
        uint32_t cur_cluster_tmp;
        uint32_t cur_sector;
        vsf_err_t err;
        vk_fatfs_dentry_parser_t dparser;
    )

    // vk_malfs_info_t must be the last in vk_fatfs_info_t
    public_member(
        implement(__vk_malfs_info_t)
    )
};

/*============================ GLOBAL VARIABLES ==============================*/

extern const vk_fs_op_t vk_fatfs_op;

/*============================ PROTOTYPES ====================================*/

extern bool vk_fatfs_is_lfn(char *name);
extern bool vk_fatfs_parse_dentry_fat(vk_fatfs_dentry_parser_t *parser);

#endif      // VSF_USE_FS && VSF_USE_FATFS
#endif      // __VSF_FATFS_H__
