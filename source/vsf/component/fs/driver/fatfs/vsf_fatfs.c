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

#include "../../vsf_fs_cfg.h"

#if VSF_USE_FS == ENABLED && VSF_USE_FATFS == ENABLED

#define VSF_FS_INHERIT
#define VSF_FATFS_IMPLEMENT
#define VSF_MALFS_INHERIT
// TODO: use dedicated include
#include "vsf.h"

/*============================ MACROS ========================================*/

#define FAT_ATTR_LFN                    0x0F
#define FAT_ATTR_READ_ONLY              0x01
#define FAT_ATTR_HIDDEN                 0x02
#define FAT_ATTR_SYSTEM                 0x04
#define FAT_ATTR_VOLUME_ID              0x08
#define FAT_ATTR_DIRECTORY              0x10
#define FAT_ATTR_ARCHIVE                0x20

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

struct fatfs_bpb_t {
    uint16_t BytsPerSec;
    uint8_t SecPerClus;
    uint16_t RsvdSecCnt;
    uint8_t NumFATs;
    uint16_t RootEntCnt;
    uint16_t TotSec16;
    uint8_t Media;
    uint16_t FATSz16;
    uint16_t SecPerTrk;
    uint16_t NumHeads;
    uint32_t HiddSec;
    uint32_t TotSec32;
} PACKED;
typedef struct fatfs_bpb_t fatfs_bpb_t;

struct fatfs_ebpb_t {
    uint8_t DrvNo;
    uint8_t Reserved;
    uint8_t BootSig;
    uint32_t VolID;
    uint8_t VolLab[11];
    uint8_t FilSysType[8];
} PACKED;
typedef struct fatfs_ebpb_t fatfs_ebpb_t;

struct fatfs_dbr_t {
    uint8_t jmp[3];
    uint8_t oem[8];
    // bpb All 0 for exFAT
    fatfs_bpb_t bpb;
    union {
        struct {
            struct {
                uint32_t FATSz32;
                uint16_t ExtFlags;
                uint16_t FSVer;
                uint32_t RootClus;
                uint16_t FSInfo;
                uint16_t BkBootSec;
                uint8_t Reserved[12];
            } PACKED bpb;
            fatfs_ebpb_t ebpb;
            uint8_t Bootstrap[420];
        } PACKED fat32;
        struct {
            fatfs_ebpb_t ebpb;
            uint8_t Bootstrap[448];
        } PACKED fat1216;
        struct {
            uint8_t Reserved_All0[28];
            struct {
                uint64_t SecStart;
                uint64_t SecCount;
                uint32_t FATSecStart;
                uint32_t FATSecCount;
                uint32_t ClusSecStart;
                uint32_t ClusSecCount;
                uint32_t RootClus;
                uint32_t VolSerial;
                struct {
                    uint8_t Minor;
                    uint8_t Major;
                } PACKED Ver;
                uint16_t VolState;
                uint8_t SecBits;
                uint8_t SPCBits;
                uint8_t NumFATs;
                uint8_t DrvNo;
                uint8_t AllocPercnet;
                uint8_t Reserved_All0[397];
            } PACKED bpb;
        } PACKED exfat;
    } PACKED;
    uint16_t Magic;
} PACKED;
typedef struct fatfs_dbr_t fatfs_dbr_t;

struct fatfs_dentry_t {
    union {
        struct {
            char Name[8];
            char Ext[3];
            uint8_t Attr;
            uint8_t LCase;
            uint8_t CrtTimeTenth;
            uint16_t CrtTime;
            uint16_t CrtData;
            uint16_t LstAccData;
            uint16_t FstClusHi;
            uint16_t WrtTime;
            uint16_t WrtData;
            uint16_t FstClusLo;
            uint32_t FileSize;
        } PACKED fat;
    } PACKED;
} PACKED;
typedef struct fatfs_dentry_t fatfs_dentry_t;

/*============================ PROTOTYPES ====================================*/

static void __vk_fatfs_mount(uintptr_t, vsf_evt_t);
static void __vk_fatfs_lookup(uintptr_t, vsf_evt_t);
static void __vk_fatfs_close(uintptr_t, vsf_evt_t);
static void __vk_fatfs_read(uintptr_t, vsf_evt_t);
static void __vk_fatfs_write(uintptr_t, vsf_evt_t);

/*============================ GLOBAL VARIABLES ==============================*/

const vk_fs_op_t vk_fatfs_op = {
    .mount          = __vk_fatfs_mount,
    .unmount        = vk_dummyfs_succeed,
#if VSF_FS_CFG_USE_CACHE == ENABLED
    .sync           = vk_file_dummy,
#endif
    .fop            = {
        .read       = __vk_fatfs_read,
        .write      = __vk_fatfs_write,
        .close      = __vk_fatfs_close,
        .resize     = vk_dummyfs_not_support,
    },
    .dop            = {
        .lookup     = __vk_fatfs_lookup,
        .create     = vk_dummyfs_not_support,
        .unlink     = vk_dummyfs_not_support,
        .chmod      = vk_dummyfs_not_support,
        .rename     = vk_dummyfs_not_support,
    },
};

/*============================ LOCAL VARIABLES ===============================*/

static const uint8_t __vk_fatfs_fat_bitsize[] = {
    [VSF_FAT_NONE]  = 0,
    [VSF_FAT_12]    = 12,
    [VSF_FAT_16]    = 16,
    [VSF_FAT_32]    = 32,
    [VSF_FAT_EX]    = 32,
};

/*============================ IMPLEMENTATION ================================*/

bool vk_fatfs_is_lfn(char *name)
{
    char *ext = NULL;
    bool has_lower = false, has_upper = false;
    uint_fast32_t i, name_len = 0, ext_len = 0;

    if (name != NULL) {
        name_len = strlen(name);
        ext = vk_file_getfileext(name);
    }
    if (ext != NULL) {
        ext_len = strlen(ext);
        name_len -= ext_len + 1;    // 1 more byte for dot
    }
    if ((name_len > 8) || (ext_len > 3)) {
        return true;
    }

    for (i = 0; name[i] != '\0'; i++) {
        if (islower((int)name[i])) {
            has_lower = true;
        }
        if (isupper((int)name[i])) {
            has_upper = true;
        }
    }
    return has_lower && has_upper;
}

// entry_num is the number of entry remain in buffer,
//     and the entry_num of entry for current filename parsed
// lfn is unicode encoded, but we just support ascii
// if a filename parsed, parser->entry will point to the sfn
bool vk_fatfs_parse_dentry_fat(vk_fatfs_dentry_parser_t *parser)
{
    fatfs_dentry_t *entry = (fatfs_dentry_t *)parser->entry;
    bool parsed = false;

    while (parser->entry_num-- > 0) {
        if (!entry->fat.Name[0]) {
            break;
        } else if (entry->fat.Name[0] != (char)0xE5) {
            char *ptr;
            int i;

            if (entry->fat.Attr == FAT_ATTR_LFN) {
                uint_fast8_t index = entry->fat.Name[0];
                uint_fast8_t pos = ((index & 0x0F) - 1) * 13;
                uint8_t *buf;

                parser->lfn = index & 0x0F;
                ptr = parser->filename + pos;
                buf = (uint8_t *)entry + 1;
                for (i = 0; i < 5; i++, buf += 2) {
                    if ((buf[0] == '\0') && (buf[1] == '\0')) {
                        goto parsed;
                    }
                    *ptr++ = (char)buf[0];
                }

                buf = (uint8_t *)entry + 14;
                for (i = 0; i < 6; i++, buf += 2) {
                    if ((buf[0] == '\0') && (buf[1] == '\0')) {
                        goto parsed;
                    }
                    *ptr++ = (char)buf[0];
                }

                buf = (uint8_t *)entry + 28;
                for (i = 0; i < 2; i++, buf += 2) {
                    if ((buf[0] == '\0') && (buf[1] == '\0')) {
                        goto parsed;
                    }
                    *ptr++ = (char)buf[0];
                }

            parsed:
                if ((index & 0xF0) == 0x40) {
                    *ptr = '\0';
                }
            } else if (entry->fat.Attr != FAT_ATTR_VOLUME_ID) {
                bool lower;
                if (parser->lfn == 1) {
                    // previous lfn parsed, igure sfn and return
                    parser->lfn = 0;
                    parsed = true;
                    break;
                }

                parser->lfn = 0;
                ptr = parser->filename;
                lower = (entry->fat.LCase & 0x08) > 0;
                for (i = 0; (i < 8) && (entry->fat.Name[i] != ' '); i++) {
                    *ptr = entry->fat.Name[i];
                    if (lower) *ptr = tolower(*ptr);
                    ptr++;
                }
                if (entry->fat.Ext[0] != ' ') {
                    *ptr++ = '.';
                    lower = (entry->fat.LCase & 0x10) > 0;
                    for (i = 0; (i < 3) && (entry->fat.Ext[i] != ' '); i++) {
                        *ptr = entry->fat.Ext[i];
                        if (lower) *ptr = tolower(*ptr);
                        ptr++;
                    }
                }
                *ptr = '\0';
                parsed = true;
                break;
            }
        } else if (parser->lfn > 0) {
            // an erased entry with previous parsed lfn entry?
            parser->lfn = 0;
        }

        entry++;
    }
    parser->entry = (uint8_t *)entry;
    return parsed;
}

static vsf_err_t __vk_fatfs_parse_dbr(__vk_fatfs_info_t *info, uint8_t *buff)
{
    fatfs_dbr_t *dbr = (fatfs_dbr_t *)buff;
    uint_fast16_t sector_size;
    uint_fast32_t tmp32;

    if (dbr->Magic != cpu_to_be16(0x55AA)) {
        return VSF_ERR_FAIL;
    }

    for (tmp32 = 0; (tmp32 < 53) && *buff; tmp32++, buff++);
    if (tmp32 < 53) {
        // normal FAT12, FAT16, FAT32
        uint_fast32_t sector_num, cluster_num;
        uint_fast16_t reserved_size, root_entry;

        sector_size = le16_to_cpu(dbr->bpb.BytsPerSec);
        info->sector_size_bits = msb(sector_size);
        info->cluster_size_bits = msb(dbr->bpb.SecPerClus);
        reserved_size = le16_to_cpu(dbr->bpb.RsvdSecCnt);
        info->fat_num = dbr->bpb.NumFATs;
        sector_num = le16_to_cpu(dbr->bpb.TotSec16);
        if (!sector_num) {
            sector_num = le32_to_cpu(dbr->bpb.TotSec32);
        }
        info->fat_size = dbr->bpb.FATSz16 ?
            le16_to_cpu(dbr->bpb.FATSz16) : le32_to_cpu(dbr->fat32.bpb.FATSz32);

        // sector_size MUST the same as mal blocksize, and MUST following value:
        //         512, 1024, 2048, 4096
        // SecPerClus MUST be power of 2
        // RsvdSecCnt CANNOT be 0
        // NumFATs CANNOT be 0
        if (    (   (sector_size != info->block_size)
                ||  (   (sector_size != 512) && (sector_size != 1024)
                    &&  (sector_size != 2048) && (sector_size != 4096)))
            ||  (!dbr->bpb.SecPerClus || !info->cluster_size_bits)
            ||  !reserved_size
            ||  !info->fat_num
            ||  !sector_num
            ||  !info->fat_size) {
            return VSF_ERR_FAIL;
        }

        root_entry = le16_to_cpu(dbr->bpb.RootEntCnt);
        if (root_entry) {
            info->root_size = ((root_entry >> 5) + sector_size - 1) / sector_size;
        }
        // calculate base
        info->fat_sector = reserved_size;
        info->root_sector = info->fat_sector + info->fat_num * info->fat_size;
        info->data_sector = info->root_sector + info->root_size;
        // calculate cluster number: note that cluster starts from root_cluster
        cluster_num = (sector_num - reserved_size) >> info->cluster_size_bits;

        // for FAT32 RootEntCnt MUST be 0
        if (!root_entry) {
            // FAT32
            info->type = VSF_FAT_32;

            // for FAT32, TotSec16 and FATSz16 MUST be 0
            if (dbr->bpb.FATSz16 || dbr->bpb.TotSec16) {
                return VSF_ERR_FAIL;
            }

            // RootClus CANNOT be less than 2
            info->root.first_cluster = le32_to_cpu(dbr->fat32.bpb.RootClus);
            if (info->root.first_cluster < 2) {
                return VSF_ERR_FAIL;
            }

            info->cluster_num = info->root.first_cluster + cluster_num;
        } else {
            // FAT12 or FAT16
            info->type = (cluster_num < 4085) ? VSF_FAT_12 : VSF_FAT_16;

            // root has no cluster
            info->root.first_cluster = 0;
            info->cluster_num = cluster_num + 2;
        }
    } else {
        // bpb all 0, exFAT
        info->type = VSF_FAT_EX;

        info->sector_size_bits = dbr->exfat.bpb.SecBits;
        info->cluster_size_bits = dbr->exfat.bpb.SPCBits;
        info->fat_num = dbr->exfat.bpb.NumFATs;
        info->fat_size = le32_to_cpu(dbr->exfat.bpb.FATSecCount);
        info->root_size = 0;
        info->fat_sector = le32_to_cpu(dbr->exfat.bpb.FATSecStart);
        info->root_sector = le32_to_cpu(dbr->exfat.bpb.ClusSecStart);
        info->data_sector = info->root_sector;
        info->root.first_cluster = le32_to_cpu(dbr->exfat.bpb.RootClus);
        info->cluster_num = le32_to_cpu(dbr->exfat.bpb.ClusSecCount) + 2;

        // SecBits CANNOT be smaller than 9, which is 512 byte
        // RootClus CANNOT be less than 2
        if ((info->sector_size_bits < 9) || (info->root.first_cluster < 2)) {
            return VSF_ERR_FAIL;
        }
    }

    return VSF_ERR_NONE;
}

static uint_fast32_t __vk_fatfs_clus2sec(__vk_fatfs_info_t *fsinfo, uint_fast32_t cluster)
{
    cluster -= fsinfo->root.first_cluster ? fsinfo->root.first_cluster : 2;
    return fsinfo->data_sector + (cluster << fsinfo->cluster_size_bits);
}

static bool __vk_fatfs_fat_entry_is_valid(__vk_fatfs_info_t *fsinfo, uint_fast32_t cluster)
{
    uint_fast8_t fat_bit = __vk_fatfs_fat_bitsize[fsinfo->type];
    uint_fast32_t mask = ((1ULL << fat_bit) - 1) & 0x0FFFFFFF;

    cluster &= (32 == fat_bit) ? 0x0FFFFFFF : mask;
    return (cluster >= 2) && (cluster <= mask);
}

static uint_fast8_t __vk_fatfs_parse_file_attr(uint_fast8_t fat_attr)
{
    uint_fast8_t attr = VSF_FILE_ATTR_READ | VSF_FILE_ATTR_WRITE;
    if (fat_attr & FAT_ATTR_READ_ONLY) {
        attr &= ~VSF_FILE_ATTR_WRITE;
    }
    if (fat_attr & FAT_ATTR_HIDDEN) {
        attr |= VSF_FILE_ATTR_HIDDEN;
    }
    if (fat_attr & FAT_ATTR_DIRECTORY) {
        attr |= VSF_FILE_ATTR_DIRECTORY;
    }
    return attr;
}

static bool __vk_fatfs_fat_entry_is_eof(__vk_fatfs_info_t *fsinfo, uint_fast32_t cluster)
{
    uint_fast8_t fat_bit = __vk_fatfs_fat_bitsize[fsinfo->type];
    uint_fast32_t mask = ((1ULL << fat_bit) - 1) & 0x0FFFFFFF;

    cluster &= (32 == fat_bit) ? 0x0FFFFFFF : mask;
    return (cluster >= (mask - 8)) && (cluster <= mask);
}

static void __vk_fatfs_mount(uintptr_t target, vsf_evt_t evt)
{
    enum {
        MOUNT_STATE_PARSE_DBR,
        MOUNT_STATE_PARSE_ROOT,
    };
    vk_vfs_file_t *dir = (vk_vfs_file_t *)target;
    __vk_fatfs_info_t *fsinfo = dir->subfs.data;
    __vk_malfs_info_t *malfs_info = &fsinfo->use_as____vk_malfs_info_t;
    VSF_FS_ASSERT(fsinfo != NULL);

    switch (evt) {
    case VSF_EVT_INIT:
        __vk_malfs_init(malfs_info);
        vsf_eda_frame_user_value_set(MOUNT_STATE_PARSE_DBR);
        __vk_malfs_read(malfs_info, 0, 1, NULL);
        break;
    case VSF_EVT_RETURN: {
            uint_fast8_t state;

            if (malfs_info->ctx_io.err != VSF_ERR_NONE) {
                VSF_FS_ASSERT(false);
                vk_file_return(&dir->use_as__vk_file_t, malfs_info->ctx_io.err);
                return;
            }

            vsf_eda_frame_user_value_get((uint8_t *)&state);
            switch (state) {
            case MOUNT_STATE_PARSE_DBR:
                if (VSF_ERR_NONE != __vk_fatfs_parse_dbr(fsinfo, malfs_info->ctx_io.buff)) {
                return_fail:
                    VSF_FS_ASSERT(false);
                    vk_file_return(&dir->use_as__vk_file_t, VSF_ERR_FAIL);
                    return;
                }
                vsf_eda_frame_user_value_set(MOUNT_STATE_PARSE_ROOT);
                __vk_malfs_read(malfs_info, fsinfo->root_sector, 1, NULL);
                break;
            case MOUNT_STATE_PARSE_ROOT: {
                    fatfs_dentry_t *dentry = (fatfs_dentry_t *)malfs_info->ctx_io.buff;
                    malfs_info->volume_name = NULL;
                    if (VSF_FAT_EX == fsinfo->type) {
                        // TODO: parse VolID for exfat
                        goto return_fail;
                    } else if (FAT_ATTR_VOLUME_ID == dentry->fat.Attr) {
                        fsinfo->fat_volume_name[11] = '\0';
                        memcpy(fsinfo->fat_volume_name, dentry->fat.Name, 11);
                        for (int_fast8_t i = 10; i >= 0; i--) {
                            if (fsinfo->fat_volume_name[i] != ' ') {
                                break;
                            }
                            fsinfo->fat_volume_name[i] = '\0';
                        }
                        malfs_info->volume_name = fsinfo->fat_volume_name;
                    }
                    fsinfo->root.info = fsinfo;
                    dir->subfs.root = &fsinfo->root.use_as__vk_file_t;
                    vk_file_return(&dir->use_as__vk_file_t, VSF_ERR_NONE);
                }
                break;
            }
        }
        break;
    }
}

static void __vk_fatfs_get_fat_entry(uintptr_t target, vsf_evt_t evt)
{
    enum {
        LOOKUP_FAT_STATE_START,
        LOOKUP_FAT_STATE_PARSE,
    };
    __vk_fatfs_info_t *fsinfo = (__vk_fatfs_info_t *)target;
    __vk_malfs_info_t *malfs_info = &fsinfo->use_as____vk_malfs_info_t;
    uint_fast8_t fat_bit = __vk_fatfs_fat_bitsize[fsinfo->type];
    uint_fast32_t start_bit = fsinfo->cur_cluster * fat_bit;
    uint_fast32_t sector_bit = 1 << (fsinfo->sector_size_bits + 3);
    uint_fast32_t start_bit_sec = start_bit & (sector_bit - 1);

    switch (evt) {
    case VSF_EVT_INIT:
        fsinfo->cur_fat_bit = 0;
        vsf_eda_frame_user_value_set(LOOKUP_FAT_STATE_START);
        // fall through
    case VSF_EVT_RETURN: {
            uint_fast8_t state;
            vsf_eda_frame_user_value_get((uint8_t *)&state);
            switch (state) {
            case LOOKUP_FAT_STATE_START:
            read_fat_sector:
                if (fsinfo->cur_fat_bit < fat_bit) {
                    start_bit = fsinfo->fat_sector + (start_bit >> (fsinfo->sector_size_bits + 3));
                    start_bit += fsinfo->cur_fat_bit ? 1 : 0;

                    vsf_eda_frame_user_value_set(LOOKUP_FAT_STATE_PARSE);
                    __vk_malfs_read(malfs_info, start_bit, 1, NULL);
                }
                break;
            case LOOKUP_FAT_STATE_PARSE:
                if (VSF_ERR_NONE != malfs_info->ctx_io.err) {
                    VSF_FS_ASSERT(false);
                    fsinfo->err = VSF_ERR_FAIL;
                    vsf_eda_return();
                    break;
                }

                if (fsinfo->cur_fat_bit) {
                    fsinfo->cur_cluster_tmp |= get_unaligned_le32(malfs_info->ctx_io.buff) << fsinfo->cur_fat_bit;
                    fsinfo->cur_cluster_tmp &= (1 << fat_bit) - 1;
                    fsinfo->cur_cluster = fsinfo->cur_cluster_tmp;

                    fsinfo->err = VSF_ERR_NONE;
                    vsf_eda_return();
                    break;
                }

                fsinfo->cur_fat_bit += min(fat_bit, sector_bit - start_bit_sec);
                fsinfo->cur_cluster_tmp = get_unaligned_le32(&malfs_info->ctx_io.buff[start_bit_sec >> 3]);
                fsinfo->cur_cluster_tmp = (fsinfo->cur_cluster_tmp >> (start_bit & 7)) & ((1 << fsinfo->cur_fat_bit) - 1);
                goto read_fat_sector;
            }
        }
    }
}

static void __vk_fatfs_lookup(uintptr_t target, vsf_evt_t evt)
{
    enum {
        LOOKUP_STATE_READ_SECTOR,
        LOOKUP_STATE_PARSE_SECTOR,
        LOOKUP_STATE_READ_FAT,
    };
    vk_fatfs_file_t *dir = (vk_fatfs_file_t *)target;
    __vk_fatfs_info_t *fsinfo = (__vk_fatfs_info_t *)dir->info;
    __vk_malfs_info_t *malfs_info = &fsinfo->use_as____vk_malfs_info_t;
    const char *name = dir->ctx.lookup.name;
    vsf_err_t err = VSF_ERR_NONE;

    switch (evt) {
    case VSF_EVT_INIT:
        fsinfo->cur_cluster = dir->first_cluster;
        if (!dir->first_cluster) {
            if ((fsinfo->type != VSF_FAT_12) && (fsinfo->type != VSF_FAT_16)) {
                VSF_FS_ASSERT(false);
                err = VSF_ERR_FAIL;
                goto exit;
            }
            fsinfo->cur_sector = fsinfo->root_sector;
        } else {
            fsinfo->cur_sector = __vk_fatfs_clus2sec(fsinfo, fsinfo->cur_cluster);
        }
        vsf_eda_frame_user_value_set(LOOKUP_STATE_READ_SECTOR);

        // fall through
    case VSF_EVT_RETURN: {
            uint_fast8_t state;
            vsf_eda_frame_user_value_get((uint8_t *)&state);
            switch (state) {
            case LOOKUP_STATE_READ_SECTOR:
            read_sector:
                vsf_eda_frame_user_value_set(LOOKUP_STATE_PARSE_SECTOR);
                __vk_malfs_read(malfs_info, fsinfo->cur_sector, 1, NULL);
                break;
            case LOOKUP_STATE_PARSE_SECTOR: {
                    fatfs_dentry_t *dentry = (fatfs_dentry_t *)malfs_info->ctx_io.buff;
                    vk_fatfs_dentry_parser_t *dparser = &fsinfo->dparser;

                    err = malfs_info->ctx_io.err;
                    if (err != VSF_ERR_NONE) {
                        goto exit;
                    }

                    if (fsinfo->type == VSF_FAT_EX) {
                        err = VSF_ERR_NOT_SUPPORT;
                        goto exit;
                    }

                    dparser->entry = (uint8_t *)dentry;
                    dparser->entry_num = 1 << (fsinfo->sector_size_bits - 5);
                    dparser->lfn = 0;
                    dparser->filename = dentry->fat.Name;
                    while (dparser->entry_num) {
                        if (vk_fatfs_parse_dentry_fat(dparser)) {
                            if (    (name && vk_file_is_match((char *)name, dparser->filename))
                                ||  (!name && !dir->ctx.lookup.idx--)) {

                                // matched
                                vk_fatfs_file_t *fatfs_file = (vk_fatfs_file_t *)vk_file_alloc(sizeof(vk_fatfs_file_t));
                                if (NULL == fatfs_file) {
                                fail_mem:
                                    err = VSF_ERR_NOT_ENOUGH_RESOURCES;
                                    goto exit;
                                }

                                fatfs_file->name = vsf_heap_malloc(strlen(dparser->filename) + 1);
                                if (NULL == fatfs_file->name) {
                                    vk_file_free(&fatfs_file->use_as__vk_file_t);
                                    goto fail_mem;
                                }

                                dentry = (fatfs_dentry_t *)dparser->entry;
                                strcpy(fatfs_file->name, dparser->filename);
                                fatfs_file->attr = (vk_file_attr_t)__vk_fatfs_parse_file_attr(dentry->fat.Attr);
                                fatfs_file->fsop = &vk_fatfs_op;
                                fatfs_file->size = dentry->fat.FileSize;
                                fatfs_file->info = fsinfo;
                                fatfs_file->first_cluster = dentry->fat.FstClusLo + (dentry->fat.FstClusHi << 16);

                                *dir->ctx.lookup.result = &fatfs_file->use_as__vk_file_t;
                                vk_file_return(&dir->use_as__vk_file_t, VSF_ERR_NONE);
                                return;
                            }
                            dparser->entry += 32;
                            dparser->filename = (char *)dentry;
                        } else if (dparser->entry_num > 0) {
                            err = VSF_ERR_NOT_AVAILABLE;
                            goto exit;
                        }
                    }

                    if (    (!fsinfo->cur_cluster && (fsinfo->cur_sector < fsinfo->root_size))
                        ||  (fsinfo->cur_sector & ((1 << fsinfo->cluster_size_bits) - 1))) {
                        // not found in current sector, find next sector
                        fsinfo->cur_sector++;
                        goto read_sector;
                    } else {
                        // not found in current cluster, find next cluster if exists
                        vsf_eda_frame_user_value_set(LOOKUP_STATE_READ_FAT);
                        vsf_eda_call_param_eda((uintptr_t)__vk_fatfs_get_fat_entry, (uintptr_t)fsinfo);
                    }
                }
                break;
            case LOOKUP_STATE_READ_FAT:
                if (    (fsinfo->err != VSF_ERR_NONE)
                    ||  !__vk_fatfs_fat_entry_is_valid(fsinfo, fsinfo->cur_cluster)
                    ||  __vk_fatfs_fat_entry_is_eof(fsinfo, fsinfo->cur_cluster)) {
                    err = VSF_ERR_NOT_AVAILABLE;
                    goto exit;
                }

                // remove MSB 4-bit for 32-bit FAT entry
                fsinfo->cur_cluster &= 0x0FFFFFFF;
                fsinfo->cur_sector = __vk_fatfs_clus2sec(fsinfo, fsinfo->cur_cluster);
                goto read_sector;
            }
        }
        break;
    }
    return;
exit:
    vk_file_return(&dir->use_as__vk_file_t, err);
}

static void __vk_fatfs_close(uintptr_t target, vsf_evt_t evt)
{
    vk_fatfs_file_t *fatfs_file = (vk_fatfs_file_t *)target;

    // TODO: flush file buffer if enabled
    if (fatfs_file->name != NULL) {
        vsf_heap_free(fatfs_file->name);
    }
    vk_file_free(&fatfs_file->use_as__vk_file_t);
    vk_file_return(NULL, VSF_ERR_NONE);
}

static void __vk_fatfs_read(uintptr_t target, vsf_evt_t evt)
{
}

static void __vk_fatfs_write(uintptr_t target, vsf_evt_t evt)
{
}

#endif
