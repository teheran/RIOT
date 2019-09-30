#ifdef PARTICLE_MONOFIRMWARE
#include <stdint.h>
#include "vectors_cortexm.h"

extern const void *_isr_vectors;
extern uint32_t _etext, _srelocate, _erelocate;

// As described in Particle's dynalib/inc/module_info.h (there under LGPL-3
// terms; detailed license compatibility discussion skipped for not crossing
// any threshold of originality)
typedef struct module_info_t {
    const void* module_start_address;
    const void* module_end_address;
    uint8_t reserved;
    uint8_t flags;
    uint16_t module_version;
    uint16_t platform_id;
    uint8_t  module_function;
    uint8_t  module_index;
    uint32_t dependency; // was module_dependency_t; kept in here to ensure it's blank
    uint32_t dependency2; // was module_dependency_t
} module_info_t;

// 66 words is the distance between the known ISR table and the fixed-position module_info at 0x200.
ISR_VECTOR(50) const uint32_t particle_monofirmware_padding[66] = {0, };

ISR_VECTOR(51) const struct module_info_t particle_monofirmware_module_info = {
    .module_start_address = &_isr_vectors,
    // .module_end_address = &_etext + (&_erelocate - &_srelocate), // Not computable at compile time, letting the Makefile patch that
    .module_version = 0x138, // module version that micropython uses
    .platform_id = 14, // Xenon; see Particle's build/platform-id.mk
    .module_index = 3, // MOD_FUNC_MONO_FIRMWARE from dynalib/inc/module_info.h
};

#endif
