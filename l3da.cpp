/*
    Android graphic buffer allocator for Xylon logi3D IP core

    Copyright (C) 2012 Xylon d.o.o.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/mman.h>
#include <sys/time.h>

#include <dlfcn.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#if HAVE_ANDROID_OS
#include <linux/fb.h>
#endif

#include "gralloc_priv.h"
#include "gr.h"
#include "logi3D_devtree.h"
#include "logi2D3D.h"

/*****************************************************************************/

#define LOGI3D_DRIVER_NAME  "/dev/logi2D3D"
#define LOGI3D_BPP          4
#define MAX_L3DA_BUFFERS    200
#define MMAP_CACHED         0

enum {
    PAGE_FLIP = 0x00000001,
    LOCKED = 0x00000002
};

struct fb_context_t {
    framebuffer_device_t  device;
};

/*****************************************************************************/

static int load_logi3d_setup(private_module_t* m)
{
    char root[MAXPATHLEN] = { 0 };

    /* Check is there a /proc/device-tree dir. */
    if(dt_get_path("/proc", "device-tree", root) != 0)
    {
        ALOGE( "/proc/device-tree not present, ipcore configuration unknown");
        return -EINVAL;
    }

    /* Check is there a /proc/device-tree/logi3d dir. */
    if(dt_get_path ("/proc/device-tree", "logi3d", root) != 0)
    {
        ALOGE( "logi3d config not found in the dts");
        return -EINVAL;
    }

    int base, size;
    if( dt_get_int_pair( root, "sw-video-mem", &base, &size  ) != 0 )
        { return -EINVAL; }
    if( dt_get_int( root, "xy-length", &m->l3da_stride ) != 0 )
        { return -EINVAL; }

	m->l3da_stride = 1 << m->l3da_stride;

    m->l3da_bufferTable = new l3da_buffer_t[MAX_L3DA_BUFFERS];
    m->l3da_bufferTable[0].end   = base;
    m->l3da_bufferTable[0].left  = 0;
	m->l3da_bufferTable[0].right = m->l3da_stride * LOGI3D_BPP;
    m->l3da_bufferTable[1].start = base + size;
    m->l3da_bufferTable[1].left  = 0;
	m->l3da_bufferTable[1].right = m->l3da_stride * LOGI3D_BPP;
    m->l3da_lastEntry = &m->l3da_bufferTable[1];

	return 0;
}

static int roundUpToPOT(int dim)
{
    int ret = 1;
    while (ret < dim) ret <<= 1;
    return ret;
}

static void insert_at(private_module_t* m, l3da_buffer_t* entry, uint32_t start, uint32_t size, int left, int right)
{
    ++ m->l3da_lastEntry;
    memmove(entry + 1, entry, (uint32_t)m->l3da_lastEntry - (uint32_t)entry);

    entry->start = start;
    entry->end   = start + size;
    entry->left  = left;
    entry->right = right;
}

static void remove_entry(private_module_t* m, uint32_t start)
{
    l3da_buffer_t * entry = &m->l3da_bufferTable[1];

    for(; entry < m->l3da_lastEntry; entry++)
    {
        if (entry->start == start)
        {
			memmove(entry, entry + 1, (uint32_t)m->l3da_lastEntry - (uint32_t)entry);
            -- m->l3da_lastEntry;
            return;
        }
    }
}

static l3da_buffer_t * allocate_space(private_module_t* m, int w, int h)
{
    uint32_t reqSize = (0==w)? h : h * m->l3da_stride * 4;
    uint32_t pageSizeMask = sysconf(_SC_PAGE_SIZE) - 1;
	l3da_buffer_t * entry;

	if (m->l3da_lastEntry - m->l3da_bufferTable == MAX_L3DA_BUFFERS - 1)
    {
        return NULL;
    }

    reqSize = (reqSize + pageSizeMask) & ~pageSizeMask;

	if (0 == w)
	{
		for (entry = m->l3da_lastEntry-1; entry >= m->l3da_bufferTable; entry--)
		{
			l3da_buffer_t * next  = entry+1;
			if (entry->end + reqSize <= next->start)
			{
				insert_at(m, next, next->start - reqSize, reqSize, 0, m->l3da_stride * 4);
				return next;
			}
		}
	}
	else
	{
		for (entry = m->l3da_bufferTable; entry < m->l3da_lastEntry; entry++)
		{
			l3da_buffer_t * next  = entry+1;
			if (entry->end + reqSize <= next->start)
			{
				insert_at(m, next, entry->end, reqSize, 0, w);
				return next;
			}
		}
	}

	return NULL;
}


void* map_fb_area(const private_handle_t* hnd)
{
#if (MMAP_CACHED == 1)
    ioctl(hnd->fd, L2D3D_IOCTL_L3D_MMAP_CACHED, 1);
#endif    
	void* ret = mmap(NULL, hnd->size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->fd, hnd->offset);
	if (ret == MAP_FAILED) {
		ALOGE("map_fb_area failed");
	}
	else {
		ALOGI("map_fb_area(0x%x, 0x%x, 0x%x) mapped to %p", hnd->fd, hnd->offset, hnd->size, ret);
	}
	return ret;
}

int l3da_alloc_buffer(alloc_device_t* dev, int w, int h, int bpp, int usage,
                      buffer_handle_t* pHandle, int* pStride)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);
    
	if (m->l3da_stride == 0) {
		if ( load_logi3d_setup(m) ) {
            ALOGE( "load_logi3d_setup failed" );
        }
    }

	int fd = open(LOGI3D_DRIVER_NAME, O_RDWR);

    if (fd < 0) {
        ALOGE( "l3da_alloc_buffer - open failed");
		return -ENODEV;
    }

  	l3da_buffer_t * bufEntry;
    if (usage & GRALLOC_USAGE_HW_RENDER) {
        *pStride = m->l3da_stride;
        if (usage & GRALLOC_USAGE_HW_TEXTURE) {
            h = roundUpToPOT(h);
        }
    	bufEntry  = allocate_space(m, w*4, h);
    }
    else {
        *pStride = roundUpToPOT(w);
    	bufEntry  = allocate_space(m, 0, *pStride * roundUpToPOT(h) * 4);
    }

	if (bufEntry == NULL)
		return -ENOMEM;

    private_handle_t* hnd = new private_handle_t(fd,
			bufEntry->end - bufEntry->start,
			private_handle_t::PRIV_FLAGS_L3DA);

    hnd->offset = bufEntry->start;
	hnd->base = (int)map_fb_area(hnd);

    *pHandle = hnd;

	return 0;
}

int l3da_free_buffer(alloc_device_t* dev, buffer_handle_t handle)
{
	int i;
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);

    remove_entry(m, hnd->offset);

	return 0;
}

int l3da_register_buffer(gralloc_module_t const* module,
				         buffer_handle_t handle)
{
    // handle checks and buffer ownership are already checked.
    private_handle_t* hnd = (private_handle_t*)handle;

	hnd->base = (int)map_fb_area(hnd);

    return 0;
}

int l3da_unregister_buffer(gralloc_module_t const* module,
						   buffer_handle_t handle)
{
	return 0;
}

float getTimeMs()
{
    struct timeval tp;
	gettimeofday(&tp, (struct timezone *)0);
	return (float)((tp.tv_sec*1000) % 10000) + (float)tp.tv_usec/1000;
}

