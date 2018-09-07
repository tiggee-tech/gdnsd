/* Copyright © 2012 Brandon L Black <blblack@gmail.com>
 *
 * This file is part of gdnsd.
 *
 * gdnsd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gdnsd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gdnsd.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GDNSD_ZTREE_H
#define GDNSD_ZTREE_H

#include "ltarena.h"

// [zl]tree.h have a mutual dependency due to type definitions:
struct _zone_struct;
typedef struct _zone_struct zone_t;
#include "ltree.h"

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

// re: zone_t mtime/serial fields:
// These initialize to zero during zone_new(), and the zone data loader can
// optionally set them at any time between zone_new() and zone_finalize().
// At the time of zone_finalize(), the following logic is applied:
// 1) If the zone's SOA record has a non-zero serial value, the mtime is
//    ignored and the explicit SOA serial is copied to the zone-level serial.
// 2) If the zone's SOA record has a zero value and the mtime field has a nonzero value,
//    an automatic serial is calculated from the mtime and copied to both the
//    zone-level serial and the SOA record serial.
// 3) If both the SOA record and the serial field here are zero, they're left at zero.
// 4) The zone-level serial is what's output in log messages about zone loading.
struct _zone_struct {
    unsigned hash;        // hash of dname
    time_t mtime;         // effective mtime of the zone data
    unsigned serial;      // SOA serial
    char* src;            // string description of src, e.g. "rfc1035:example.com"
    const uint8_t* dname; // zone name as a dname (stored in ->arena)
    ltarena_t* arena;     // arena for dname/label storage
    ltree_node_t* root;   // the zone root
};

// The tree data structure that will hold the zone_t's
struct _ztree_struct;
typedef struct _ztree_struct ztree_t;

// Initialize once at startup
void ztree_init(void);

F_NONNULL
bool ztree_insert_zone(ztree_t* tree, zone_t* new_zone);
void* ztree_zones_reloader_thread(void* init_asvoid);

// --- zsrc_* interfaces ---

F_NONNULL F_WUNUSED
zone_t* zone_new(const char* zname, const char* source);
F_NONNULL
bool zone_finalize(zone_t* zone);
F_NONNULL
void zone_delete(zone_t* zone);

// --- dnsio/dnspacket reader interfaces ---

// primary interface for zone data runtime lookups from dnsio threads
// Argument is any legal fully-qualified dname
// Output is the zone_t structure for the known containing zone,
//   or NULL if no current zone contains the name.
// auth_depth_out is mostly useful for dnspacket.c, it tells you
//   how many bytes into the dname the authoritative zone name
//   starts at.
// PRCU: executing thread must be registered, online and have reader-lock
F_HOT F_NONNULL
zone_t* ztree_find_zone_for(const uint8_t* dname, unsigned* auth_depth_out);

#endif // GDNSD_ZTREE_H
