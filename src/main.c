#include "armv6m.h"
#include "elf_load.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options] <file.elf>\n", prog);
    fprintf(stderr, "  -f <file>     ELF binary to load\n");
    fprintf(stderr, "  -X <addr>     Override start address\n");
    fprintf(stderr, "  -c <count>    Max instructions to execute\n");
    fprintf(stderr, "  -t <0|1>      Enable instruction trace\n");
    fprintf(stderr, "  -r <addr>     Stop at PC address\n");
}

int main(int argc, char *argv[])
{
    armv6m_t cpu;
    const char *filename = NULL;
    int max_cycles = -1;
    int trace = 0;
    uint32_t stop_pc = 0xFFFFFFFF;
    int explicit_start = 0;
    uint32_t explicit_start_addr = 0;
    int c;

    while ((c = getopt(argc, argv, "f:X:c:t:r:h")) != -1) {
        switch (c) {
        case 'f': filename = optarg; break;
        case 'X':
            explicit_start = 1;
            explicit_start_addr = strtoul(optarg, NULL, 0);
            break;
        case 'c': max_cycles = atoi(optarg); break;
        case 't': trace = atoi(optarg); break;
        case 'r': stop_pc = strtoul(optarg, NULL, 0); break;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!filename) {
        print_usage(argv[0]);
        return 1;
    }

    /* Initialize CPU */
    cpu_init(&cpu);
    mem_init(&cpu);

    /* Create memory map (STM32-like) */
    printf("=== Memory Map ===\n");

    /* Peripheral regions (MMIO) - created before ELF load */
    mem_create_region(&cpu, 0xE000E010, 0x10, MEM_MMIO_SYPICK);
    mem_create_region(&cpu, 0xE000E100, 0x300, MEM_MMIO_NVIC);
    mem_create_region(&cpu, 0xE000ED00, 0x10, MEM_MMIO_SCB);
    mem_create_region(&cpu, 0xE000ED04, 0x04, MEM_MMIO_PENDSV);
    mem_create_region(&cpu, 0xE0000000, 0x04, MEM_MMIO_UART);
    mem_create_region(&cpu, 0xE0001000, 0x04, MEM_MMIO_REG);

    /* FLASH and SRAM regions are created by the ELF loader */

    /* Load ELF */
    printf("\nLoading: %s\n", filename);
    uint32_t start_addr = 0;

    if (!elf_load(filename, &cpu, &start_addr)) {
        fprintf(stderr, "Failed to load ELF\n");
        return 1;
    }

    if (explicit_start)
        start_addr = explicit_start_addr;

    printf("Entry point: 0x%08x\n", start_addr);

    /* Reset CPU */
    cpu_reset(&cpu, start_addr);
    cpu.trace = trace ? TRACE_INST : TRACE_NONE;

    printf("\n=== Starting execution ===\n");

    /* Execution loop */
    int cycles = 0;
    while (!cpu.fault && cpu.r[REG_PC] != stop_pc) {
        cpu_step(&cpu);
        cycles++;

        if (max_cycles != -1 && cycles >= max_cycles)
            break;
    }

    printf("\n=== Execution stopped ===\n");
    printf("  Cycles: %d\n", cycles);
    printf("  PC:     0x%08x\n", cpu.r[REG_PC]);
    printf("  SP:     0x%08x\n", cpu.r[REG_SP]);
    printf("  Fault:  %d\n", cpu.fault);

    return cpu.fault ? 1 : 0;
}
