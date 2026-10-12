# ARMv6-M Simulator (C)

Simulador del conjunto de instrucciones ARMv6-M (Cortex-M0) escrito en C puro, sin dependencias externas.

Fork de [armv6m-sim](https://github.com/ultraembedded/armv6m-sim) original en C++.

## Compilación del simulador

```bash
make
```

No necesita libelf ni libbfd. Solo libc estándar.

## Compilar ejemplo bare-metal

Necesita `arm-none-eabi-gcc`:

```bash
cd example
make
```

## Uso

```bash
# Ejecutar un ELF
./armv6m-sim -f example/build/example.elf

# Con traza de instrucciones
./armv6m-sim -f example/build/example.elf -t 1

# Limitar ciclos
./armv6m-sim -f example/build/example.elf -c 100000

# Dirección de inicio explícita
./armv6m-sim -f example/build/example.elf -X 0x08000000
```

## Memoria (layout STM32-like)

| Dirección    | Tamaño  | Descripción         |
|-------------|---------|---------------------|
| 0x08000000  | 512KB   | FLASH (código)      |
| 0x20000000  | 32KB    | SRAM (datos+stack)  |
| 0xE000E010  | 16B     | SysTick             |
| 0xE000E100  | 768B    | NVIC                |
| 0xE000ED00  | 16B     | SCB                 |
| 0xE000ED04  | 4B      | PendSV trigger      |
| 0xE0000000  | 4B      | UART (console out)  |
| 0xE0001000  | 4B      | Registro MMIO test  |

## Periféricos simulados

- **SysTick**: Timer decremental, genera interrupción cuando llega a 0
- **NVIC**: Controlador de interrupciones simplificado (ISER/ICER/ISPR/ICPR)
- **SCB**: System Control Block (ICSR para PendSV)
- **PendSV**: Escritura a 0xE000ED04 o 0xE0000000 dispara excepción 14
- **UART**: Escritura a 0xE0000000 imprime carácter en consola

## Cambio de contexto preemptivo

El ejemplo `example/` demuestra context switching entre 2 tareas usando:

1. **SysTick** genera interrupción periódica
2. **SysTick_Handler** dispara PendSV
3. **PendSV_Handler** salva contexto R4-R11 de la tarea actual
4. Restaura R4-R11 de la siguiente tarea
5. El frame de excepción (xPSR,PC,LR,R12,R0-R3) se maneja automáticamente

```
Task A: "Hola desde tarea A"
Task B: "Hola desde tarea B"
--> Se alternan gracias al context switch preemptivo
```

Detalle del mecanismo Cortex-M0 y lista de parches aplicados:
ver [`example/README.md`](example/README.md).

## Test SVC

`svc #imm` (excepción 11) verificado con `SVC_Handler` naked en
`example/main.c`: `main()` llama `svc 0,1,0` y comprueba
`count==3 && last==0` → `SVC test PASS`.

## Estructura del código

```
armv6m-sim-c/
├── src/
│   ├── armv6m.h          # Structs CPU, NVIC, SysTick, SCB, opcodes
│   ├── armv6m.c          # Init, reset, step, memoria, excepciones
│   ├── armv6m_decode.c   # Decodificación de instrucciones
│   ├── armv6m_execute.c  # Ejecución de instrucciones
│   ├── elf_load.h/.c     # Loader ELF32 mínimo (sin libelf)
│   └── main.c            # CLI del simulador
├── example/
│   ├── makefile
│   ├── linker.ld         # Script de enlace STM32-like
│   ├── startup.s         # Vector de interrupciones + reset handler
│   ├── serial.c/h        # UART simple
│   └── main.c            # Demo context switching
├── Makefile
└── README.md
```

## Etapas de desarrollo

- [x] Etapa 1: CPU + memoria + UART (funcional)
- [x] Etapa 2: SysTick + PendSV corregidos (funcional)
- [ ] Etapa 3: NVIC completo (prioridades, enable/disable)
- [x] Etapa 4: SVC handler (test svc 0,1,0 → PASS)
- [ ] Etapa 5: MPU simplificado, HardFault, NMI handlers
