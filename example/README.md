# Demo: cambio de contexto preemptivo en Cortex-M0 (ARMv6-M)

Dos tareas (`task_a`, `task_b`) se alternan por `SysTick → PendSV`
sobre el simulador `armv6m-sim` (C). Incluye además un test de `SVC`.

## Cómo se hace un cambio de contexto en Cortex-M0

Arquitectura relevante (ARMv6-M, solo Thumb-16):

- Dos punteros de pila: `MSP` (handler/main) y `PSP` (tareas). Bit `SPSEL`
  en `CONTROL` selecciona cuál usa el modo Thread.
- Al entrar a una excepción, el **hardware** apila en la pila activa
  (de menor a mayor dirección): `R0,R1,R2,R3,R12,LR,PC,xPSR`
  (8 palabras, 32 bytes). Al retornar los restaura.
- El handler **solo** debe salvar/restaurar `R4-R11` (8 palabras).
- `LR` al entrar contiene `EXC_RETURN` (`0xFFFFFFF9` = volver a Thread
  con MSP, `0xFFFFFFFD` = volver a Thread con PSP). `bx lr` con ese
  valor dispara el retorno de excepción.
- `PendSV` debe tener la prioridad más baja para no adelantar a otras
  IRQ; `SysTick` solo pide PendSV (`SCB->ICSR |= PENDSVSET`) y sale.
- Los handlers de excepción que tocan la pila deben ser `naked`
  (sin prólogo C): un `push {r7,lr}` encima del frame hardware corrompe
  lo que `cpu_exc_return()` va a desapilar.

Flujo de este ejemplo (`example/main.c`, `example/startup.s`):

1. `main` corre en `MSP`. `task_create(id, entry)` reserva los 64 bytes
   superiores de cada pila: 8 palabras para `R4-R11` + 8 para el frame
   (`R0..R3=0, R12=0, LR=0, PC=entry, xPSR=0x01000000`), y guarda
   `task_table[id].sp = base`.
2. `SysTick` (cada `LOAD+1` ciclos) pone `PENDSTSET`. Su handler (naked)
   solo pone `PENDSVSET` y hace `bx lr`.
3. `PendSV_Handler` (`startup.s`):
   - si `sched_started==0`, salta el salvado (primer cambio);
   - si no, `mrs r0,psp / subs #32 / stmia R4-R7 / mov R8-R11→R4-R7 /
     stmia / task_table[cur].sp = base`;
   - `next = (cur+1) % NUM_TASKS`, restaura **primero R8-R11**
     (para no pisar R4-R7), luego R4-R7, `msr psp, base+32`;
   - `LR = 0xFFFFFFFD` (vía `movs r0,#0 / subs r0,#3`, sin tocar pila)
     y `bx lr`.
4. El simulador desapila el frame y salta a `PC` de la tarea con `PSP`.

## Compilar y ejecutar

```bash
cd example
make
../armv6m-sim -f build/example.elf -X 0x08000000 -c 20000000
```

Salida esperada (20M ciclos ≈ 8 iteraciones por tarea):

```
=== ARMv6-M Context Switch Demo ===
SVC test: svc 0,1,0...
SVC test PASS (count=3, last=0)
task_create done
Task A: 0
  Task B: 0
Task A: 1
  Task B: 1
...
```

Notas:

- `-O0` obligatorio en el ejemplo: con `-O1`/`-O2` el compilador
  reordena mal la aritmética de punteros de `task_create`.
- El retardo `for (volatile int d=0; d<200000; d++);` necesita ~1.4M
  instrucciones por tarea; con SysTick cada 48k ciclos, hacen falta
  varios millones de ciclos para ver `Task X: 1`.

## Parches aplicados (resumen)

1. **Frame de `task_create` al revés.** Estaba
   `exc[0]=xPSR, exc[1]=PC, exc[2..7]=0`; el emulador
   (`cpu_exception`/`cpu_exc_return` en `src/armv6m.c`) usa
   `R0(low)…xPSR(high)`, así que el `PC` retornado era 0
   (salto a `0x00000004`). Fix: `exc[0..4]=0, exc[5]=0,
   exc[6]=entry, exc[7]=0x01000000`.
2. **Pila al fondo sin sitio.** Con `&stacks[id][0]` la tarea
   (`push+sub #56` + `serial`) bajaba hasta `0x20000000`
   (corrompía `task_table`). Fix: reservar arriba,
   `&stacks[id][512-16]` (base R4-R11) y
   `&stacks[id][512-8]` (frame).
3. **LR apilado mal accedido.** `ldr r0,[sp,#20]` leía R2 en MSP
   en vez del LR en `PSP+20`, y `bx` saltaba a `4`. Fix: no tocar
   la pila; construir `EXC_RETURN` con `movs/subs`.
4. **`SysTick_Handler` en C corrompía el frame.**
   El prólogo `push {r7,lr}` quedaba encima del frame hardware y
   `cpu_exc_return()` desapilaba basura. Fix: `__attribute__((naked))`
   + asm (`ldr/str/bx lr`).
5. **Restore pisaba R4-R7 (incluido r7=frame pointer).**
   Dos `ldmia r0!,{r4-r7}` seguidos dejaban R8-R11 en R4-R7 y se
   perdían los R4-R7 reales → el contador del retardo nunca avanzaba
   (100M ciclos sin `Task A:1`). Fix: cargar primero R8-R11
   (`adds r0,#16`), mover a `r8-r11`, volver a base y cargar R4-R7.
6. **Test SVC.** `SVC_Handler` nuevo (naked): elige MSP/PSP según
   bit 2 de `EXC_RETURN`, lee `imm` de `[PC-2]`, cuenta en
   `svc_count`/`svc_last_num`. `main` llama `svc 0,1,0` y espera
   `count==3 && last==0`. Ojo: el inline asm necesita
   `.syntax unified` y formas válidas Thumb-16 (`subs r1,r1,#2`
   falla en sintaxis dividida).

## Test SVC

`svc #imm` genera la excepción 11 (`EXC_SVCALL`, ya soportada por
`src/armv6m_execute.c:320-321`). El vector `SVC_Handler` era débil
→ `Default_Handler` (bucle infinito). Ahora `main.c` lo redefine:

- lee el número SVC del byte en `PC_apilado - 2`;
- funciona desde MSP (main) y PSP (tareas) mirando `LR & 4`;
- no hace `push/pop`: solo usa `r0-r3` (scratch, el hardware los
  restaura) y vuelve con `bx lr`.

Se prueba en `main()` antes de arrancar el scheduler.
