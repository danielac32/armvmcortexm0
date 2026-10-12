#ifndef ELF_LOAD_H
#define ELF_LOAD_H

#include "armv6m.h"

int elf_load(const char *filename, armv6m_t *cpu, uint32_t *start_addr);

#endif
