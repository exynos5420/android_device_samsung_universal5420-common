/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*!
 * \file MemoryHeapIon.cpp
 * \brief source file for MemoryHeapIon
 * \author MinGu, Jeon(mingu85.jeon)
 * \date 2011/11/20
 *
 * <b>Revision History: </b>
 * - 2011/11/20 : MinGu, Jeon(mingu85.jeon)) \n
 * Initial version
 * - 2012/11/29 : MinGu, Jeon(mingu85.jeon)) \n
 * Change name
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <cutils/log.h>
#include <binder/MemoryHeapBase.h>
#include <binder/IMemory.h>
#include <binder/MemoryHeapIon.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <ion/ion.h>
#include <linux/ion.h>


#define HEAP_MASK_FILTER    ((1 << 16) - (2))
#define FLAG_MASK_FILTER    (~(HEAP_MASK_FILTER) - (1))

#define ION_HEAP_SYSTEM_MASK (1 << ION_HEAP_TYPE_SYSTEM)
#define ION_FLAG_PRESERVE_KMAP 4


enum { USE_ION_FD  = 0x00008000 };


namespace android {
/*
uint32_t ion_HeapMask_valid_check(uint32_t flags)
{
    uint32_t heap_mask, result;
    result = 0;

    heap_mask = flags & HEAP_MASK_FILTER;

    switch(heap_mask) {
        case MHB_ION_HEAP_SYSTEM_MASK:
            return ION_HEAP_SYSTEM_MASK;
        case MHB_ION_HEAP_SYSTEM_CONTIG_MASK:
            return ION_HEAP_SYSTEM_CONTIG_MASK;
        case MHB_ION_HEAP_EXYNOS_CONTIG_MASK:
            return ION_HEAP_EXYNOS_CONTIG_MASK;
        case MHB_ION_HEAP_EXYNOS_MASK:
            return ION_HEAP_EXYNOS_MASK;
        default:
            ALOGE("MemoryHeapIon : Heap Mask flag is default (flags:%x)", flags);
            return 0;
            break;
    }
    ALOGE("MemoryHeapIon : Heap Mask flag is wrong (flags:%x)", flags);
    return 0;
}

uint32_t ion_FlagMask_valid_check(uint32_t flags)
{
    uint32_t flag_mask, result;
    result = 0;

    flag_mask = flags & FLAG_MASK_FILTER;

    if (flag_mask & MHB_ION_FLAG_CACHED)
        result |= ION_FLAG_CACHED;
    if (flag_mask & MHB_ION_FLAG_CACHED_NEEDS_SYNC)
        result |= ION_FLAG_CACHED_NEEDS_SYNC;
    if (flag_mask & MHB_ION_FLAG_PRESERVE_KMAP)
        result |= ION_FLAG_PRESERVE_KMAP;
    if (flag_mask & MHB_ION_EXYNOS_VIDEO_MASK)
        result |= ION_EXYNOS_VIDEO_MASK;
    if (flag_mask & MHB_ION_EXYNOS_MFC_INPUT_MASK)
        result |= ION_EXYNOS_MFC_INPUT_MASK;
    if (flag_mask & MHB_ION_EXYNOS_MFC_OUTPUT_MASK)
        result |= ION_EXYNOS_MFC_OUTPUT_MASK;
    if (flag_mask & MHB_ION_EXYNOS_GSC_MASK)
        result |= ION_EXYNOS_GSC_MASK;
    if (flag_mask & MHB_ION_EXYNOS_FIMD_VIDEO_MASK)
        result |= ION_EXYNOS_FIMD_VIDEO_MASK;

    return result;
}
*/
struct ion_allocation_data {
    size_t len;
    size_t align;
    unsigned int heap_mask;
    unsigned int flags;
    int handle;
};

int ion_client_create(void)
{
    return open("/dev/ion", O_RDWR);
}

void ion_client_destroy(int client)
{
    close(client);
}

void *ion_map(int buffer, size_t len, off_t offset)
{
    return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,
                buffer, offset);
}

int ion_unmap(void *addr, size_t len)
{
    return munmap(addr, len);
}
void ion_free(int buffer)
{
    close(buffer);
}

int ion_alloc(int client, size_t len, size_t align,
                     unsigned int heap_mask, unsigned int flags)
{
    int ret;
    struct ion_handle_data arg_free;
    struct ion_fd_data arg_share;
    struct ion_allocation_data arg_alloc;

    arg_alloc.len = len;
    arg_alloc.align = align;
    arg_alloc.heap_mask = heap_mask;
    arg_alloc.flags = flags;

    ret = ioctl(client, ION_IOC_ALLOC, &arg_alloc);
    if (ret < 0)
        return ret;

    arg_share.handle = arg_alloc.handle;
    ret = ioctl(client, ION_IOC_SHARE, &arg_share);

    if ((ret >= 0) && (!arg_share.fd)) {
        ret = ioctl(client, ION_IOC_SHARE, &arg_share);
        if (ret >= 0)
            close(0);
        else
            ret = 0;
    }

    arg_free.handle = arg_alloc.handle;
    ioctl(client, ION_IOC_FREE, &arg_free);

    if (ret < 0)
        return ret;

    return arg_share.fd;
}

MemoryHeapIon::MemoryHeapIon(size_t size, uint32_t flags, char const *name __attribute__((unused))):MemoryHeapBase()
{
    void* base = NULL;
    int fd = -1;
    uint32_t isReadOnly, heapMask, flagMask;

    mIonClient = ion_client_create();

    if (mIonClient < 0) {
        ALOGE("MemoryHeapIon : ION client creation failed : %s", strerror(errno));
        mIonClient = -1;
    } else {
        isReadOnly = 0; //flags & (IMemoryHeap::READ_ONLY);
        heapMask = 0;   // ion_HeapMask_valid_check(flags);
        flagMask = 0;   //ion_FlagMask_valid_check(flags);

        if (heapMask) {
            ALOGD("MemoryHeapIon : Allocated with size:%d, heap:0x%X , flag:0x%X", size, heapMask, flagMask);
            fd = ion_alloc(mIonClient, size, 0, heapMask, flagMask);
            if (fd < 0) {
                ALOGE("MemoryHeapIon : ION Reserve memory allocation failed(size[%u]) : %s", size, strerror(errno));
                if (errno == ENOMEM) { // Out of reserve memory. So re-try allocating in system heap
                    ALOGD("MemoryHeapIon : Re-try Allocating in default heap - SYSTEM heap");
                    fd = ion_alloc(mIonClient, size, 0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC | ION_FLAG_PRESERVE_KMAP);
                }
            }
        } else {
            fd = ion_alloc(mIonClient, size, 0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC | ION_FLAG_PRESERVE_KMAP);
            ALOGD("MemoryHeapIon : Allocated with default heap - SYSTEM heap");
        }

        flags = isReadOnly | heapMask | flagMask;

        if (fd < 0) {
            ALOGE("MemoryHeapIon : ION memory allocation failed(size[%u]) : %s", size, strerror(errno));
        } else {
            flags |= USE_ION_FD;
            base = ion_map(fd, size, 0);
            if (base != MAP_FAILED) {
                ALOGD("MemoryHeapIon init with fd=%d",fd);
                init(fd, base, size, flags, NULL);
            } else {
                ALOGE("MemoryHeapIon : ION mmap failed(size[%u], fd[%d]) : %s", size, fd, strerror(errno));
                ion_free(fd);
            }
        }
    }
}

MemoryHeapIon::MemoryHeapIon(int fd, size_t size, uint32_t flags, uint32_t offset __attribute__((unused))):MemoryHeapBase()
{
    void* base = NULL;
    int dup_fd = -1;

    mIonClient = ion_client_create();

    if (mIonClient < 0) {
        ALOGE("MemoryHeapIon : ION client creation failed : %s", strerror(errno));
        mIonClient = -1;
    } else {
        if (fd >= 0) {
            dup_fd = dup(fd);
            if (dup_fd == -1) {
                ALOGE("MemoryHeapIon : cannot dup fd (size[%u], fd[%d]) : %s", size, fd, strerror(errno));
            } else {
                flags |= USE_ION_FD;
                base = ion_map(dup_fd, size, 0);
                if (base != MAP_FAILED) {
                    ALOGD("MemoryHeapIon init with dup_fd=%d",dup_fd);
                    init(dup_fd, base, size, flags, NULL);
                } else {
                    ALOGE("MemoryHeapIon : ION mmap failed(size[%u], fd[%d]): %s", size, fd, strerror(errno));
                    ion_free(dup_fd);
                }
            }
        } else {
            ALOGE("MemoryHeapIon : fd parameter error(fd : %d)", fd);
        }
    }
}

MemoryHeapIon::~MemoryHeapIon()
{
    if (mIonClient != -1) {
        ion_unmap(getBase(), getSize());
        ion_client_destroy(mIonClient);
        mIonClient = -1;
    }
}

};