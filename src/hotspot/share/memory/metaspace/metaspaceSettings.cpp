/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020 SAP SE. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "logging/log.hpp"
#include "logging/logStream.hpp"
#include "memory/metaspace/metaspaceSettings.hpp"
#include "runtime/globals.hpp"
#include "runtime/java.hpp"
#include "runtime/os.hpp"
#include "utilities/debug.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/powerOfTwo.hpp"

namespace metaspace {

size_t Settings::_commit_granule_bytes = 0;
size_t Settings::_commit_granule_words = 0;

bool Settings::_new_chunks_are_fully_committed = false;
bool Settings::_uncommit_free_chunks = false;

DEBUG_ONLY(bool Settings::_use_allocation_guard = false;)

void Settings::ergo_initialize() {
  if (strcmp(MetaspaceReclaimPolicy, "none") == 0) {
    log_info(metaspace)("Initialized with strategy: no reclaim.");
    _commit_granule_bytes = MAX2(os::vm_page_size(), 64 * K);
    _commit_granule_words = _commit_granule_bytes / BytesPerWord;
    // In "none" reclamation mode, we do not uncommit, and we commit new chunks fully;
    // that very closely mimics the behaviour of old Metaspace.
    _new_chunks_are_fully_committed = true;
    _uncommit_free_chunks = false;
  } else if (strcmp(MetaspaceReclaimPolicy, "aggressive") == 0) {
    log_info(metaspace)("Initialized with strategy: aggressive reclaim.");
    // Set the granule size rather small; may increase
    // mapping fragmentation but also increase chance to uncommit.
    _commit_granule_bytes = MAX2(os::vm_page_size(), 16 * K);
    _commit_granule_words = _commit_granule_bytes / BytesPerWord;
    _new_chunks_are_fully_committed = false;
    _uncommit_free_chunks = true;
  } else if (strcmp(MetaspaceReclaimPolicy, "balanced") == 0) {
    log_info(metaspace)("Initialized with strategy: balanced reclaim.");
    _commit_granule_bytes = MAX2(os::vm_page_size(), 64 * K);
    _commit_granule_words = _commit_granule_bytes / BytesPerWord;
    _new_chunks_are_fully_committed = false;
    _uncommit_free_chunks = true;
  } else {
    vm_exit_during_initialization("Invalid value for MetaspaceReclaimPolicy: \"%s\".", MetaspaceReclaimPolicy);
  }

  // Sanity checks.
  assert(commit_granule_words() <= chunklevel::MAX_CHUNK_WORD_SIZE, "Too large granule size");
  assert(is_power_of_2(commit_granule_words()), "granule size must be a power of 2");

  // Off for release builds, off by default - but switchable - for debug builds.
  DEBUG_ONLY(_use_allocation_guard = MetaspaceGuardAllocations;)

  LogStream ls(Log(metaspace)::info());
  Settings::print_on(&ls);
}

void Settings::print_on(outputStream* st) {
  st->print_cr(" - commit_granule_bytes: " SIZE_FORMAT ".", commit_granule_bytes());
  st->print_cr(" - commit_granule_words: " SIZE_FORMAT ".", commit_granule_words());
  st->print_cr(" - virtual_space_node_default_size: " SIZE_FORMAT ".", virtual_space_node_default_word_size());
  st->print_cr(" - enlarge_chunks_in_place: %d.", (int)enlarge_chunks_in_place());
  st->print_cr(" - new_chunks_are_fully_committed: %d.", (int)new_chunks_are_fully_committed());
  st->print_cr(" - uncommit_free_chunks: %d.", (int)uncommit_free_chunks());
  st->print_cr(" - use_allocation_guard: %d.", (int)use_allocation_guard());
}

} // namespace metaspace

