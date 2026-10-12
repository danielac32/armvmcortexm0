#include "armv6m.h"
#include "elf_load.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Minimal ELF32 definitions (no system headers needed) */
#define EI_NIDENT 16
#define PT_LOAD   1
#define ET_EXEC   2
#define EM_ARM    40

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

static uint16_t read16(uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read32(uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

int elf_load(const char *filename, armv6m_t *cpu, uint32_t *start_addr)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return 0;
    }

    /* Read ELF header */
    Elf32_Ehdr ehdr;
    uint8_t buf[sizeof(Elf32_Ehdr)];
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        return 0;
    }

    memcpy(ehdr.e_ident, buf, EI_NIDENT);
    ehdr.e_type      = read16(buf + 16);
    ehdr.e_machine   = read16(buf + 18);
    ehdr.e_version   = read32(buf + 20);
    ehdr.e_entry     = read32(buf + 24);
    ehdr.e_phoff     = read32(buf + 28);
    ehdr.e_shoff     = read32(buf + 32);
    ehdr.e_flags     = read32(buf + 36);
    ehdr.e_ehsize    = read16(buf + 40);
    ehdr.e_phentsize = read16(buf + 42);
    ehdr.e_phnum     = read16(buf + 44);

    /* Validate */
    /* Validate ELF magic */
    static const char elf_magic[] = { 0x7f, 'E', 'L', 'F' };
    if (memcmp(ehdr.e_ident, elf_magic, 4) != 0) {
        fprintf(stderr, "Not an ELF file\n");
        close(fd);
        return 0;
    }
    if (ehdr.e_ident[4] != 1) { /* ELFCLASS32 */
        fprintf(stderr, "Not a 32-bit ELF\n");
        close(fd);
        return 0;
    }
    if (ehdr.e_type != ET_EXEC) {
        fprintf(stderr, "Not an executable ELF (type=%d)\n", ehdr.e_type);
        close(fd);
        return 0;
    }

    /* Load program headers (PT_LOAD segments) */
    for (int i = 0; i < ehdr.e_phnum; i++) {
        uint8_t phdr_buf[32]; /* sizeof(Elf32_Phdr) = 32 */
        lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
        if (read(fd, phdr_buf, 32) != 32)
            continue;

        Elf32_Phdr phdr;
        phdr.p_type   = read32(phdr_buf + 0);
        phdr.p_offset = read32(phdr_buf + 4);
        phdr.p_vaddr  = read32(phdr_buf + 8);
        phdr.p_paddr  = read32(phdr_buf + 12);
        phdr.p_filesz = read32(phdr_buf + 16);
        phdr.p_memsz  = read32(phdr_buf + 20);
        phdr.p_flags  = read32(phdr_buf + 24);
        phdr.p_align  = read32(phdr_buf + 28);

        if (phdr.p_type != PT_LOAD)
            continue;

        if (phdr.p_memsz == 0)
            continue;

        /* Create memory region */
        uint32_t region_size = phdr.p_memsz;
        /* Align up to 4 bytes */
        region_size = (region_size + 3) & ~3u;

        mem_type_t mtype = (phdr.p_flags & 0x1) ? MEM_FLASH : MEM_RAM;
        if (!mem_create_region(cpu, phdr.p_vaddr, region_size, mtype)) {
            fprintf(stderr, "Cannot create region at 0x%08x (%d bytes)\n",
                    phdr.p_vaddr, region_size);
            close(fd);
            return 0;
        }

        /* Find the region we just created */
        mem_region_t *r = &cpu->regions[cpu->n_regions - 1];

        /* Load from file */
        if (phdr.p_filesz > 0) {
            lseek(fd, phdr.p_offset, SEEK_SET);
            uint32_t to_read = phdr.p_filesz;
            if (to_read > region_size) to_read = region_size;
            if (read(fd, r->data, to_read) != (ssize_t)to_read) {
                fprintf(stderr, "Error reading segment\n");
                close(fd);
                return 0;
            }
        }

        printf("  LOAD: 0x%08x - 0x%08x (%d bytes) [%s]\n",
               phdr.p_vaddr, phdr.p_vaddr + region_size - 1,
               region_size,
               (phdr.p_flags & 0x1) ? "code" : "data");
    }

    if (start_addr)
        *start_addr = ehdr.e_entry;

    close(fd);
    return 1;
}
