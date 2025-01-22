/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * Copyright (c) 2025 Proton Camera Innovations GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/* The ESP32 has three SRAM banks:
 *
 * SRAM0 (192kB): External RAM Cache + Instruction Memory
 * ------------------------------------------------------
 *
 * The first 64kB can be used as a cache for external memory (FLASH and SPIRAM). The
 * lower 32kB are used by the "Pro" CPU the upper 32kB by the "App" CPU. If the "App"
 * CPU is unused its cash can be used as IRAM by the "Pro" CPU (see
 * https://developer.espressif.com/blog/esp32-programmers-memory-model/#iram-organisation).
 *
 * The remaining part of SRAM0 is used as instruction memmory.
 *
 * If MCUboot (or any other second stage bootloader is used) it also needs space in the
 * instruction memory. It has to sections, "iram_seg" contains non critical code which
 * can be overwritten by the application, "iram_loader_seg" contains the code which is
 * copies the application code to IRAM. This means the "iram_loader_seg" must not be used
 * by the application, otherwise the second stage bootloader would overwrite itself.
 *
 * Since the bootloader does not use the "App" CPU its cache section can be used to place
 * the critical "iram_loader_seg" which makes that memory reclaimable (as cache) if the
 * "App" CPU is used by the application. If the "App" CPU is not used the memory cannot
 * be reclaimed by the application.
 *
 * SRAM1 (128kB): Instruction / Data Memory
 * ----------------------------------------
 *
 * SRAM1 can be used as either instruction or data memory.
 *
 * For this to work SRAM1 can be accessed both from the instruction bus and the data
 * bus at different addresses. SRAM1 is accessed via the instruction bus in REVERSE ORDER
 * compared to the data bus, this means that SRAM1 can extend both the instruction and
 * the data memory where the data memory uses the (phyiscally) lower bytes while the
 * instruction memory uses the higher bytes.
 *
 * Unfortuantely there are some reserved memory regions used by the ROM at the beginning
 * of SRAM1 (see table below) so there is a gap in the usable DRAM. To still make use
 * of SRAM1 as DRAM some parts of it are used as HEAP memory for the ESP heap allocator,
 * the rest is used as IRAM.
 *
 * SRAM2 (200kB): Data Memory
 * --------------------------
 *
 * The ESP32 ROM functions use the following DRAM regions:
 *
 * - 0x3ffae000 - 0x3ffb0000 (Reserved: data memory for ROM functions)
 * - 0x3ffb0000 - 0x3ffe0000 (RAM bank 1 for application usage)
 * - 0x3ffe0000 - 0x3ffe0440 (Reserved: data memory for ROM PRO CPU)
 * - 0x3ffe3f20 - 0x3ffe4350 (Reserved: data memory for ROM APP CPU)
 * - 0x3ffe4350 - 0x3ffe5230 (BT SHM buffers)
 * - 0x3ffe8000 - 0x3fffffff (RAM bank 2 for application usage)
 *
 * At the beginning of SRAM2 8kB are reserved for the ROM which it needs as data memory
 * for some of its functions. Additionally ~54kB (0xDB5C) have to be reserved at the
 * start of SRAM2 if the Bluetooth controller is used.
 *
 * There are some reserved regions at the beginning of SRAM1 (see
 * https://developer.espressif.com/blog/esp32-programmers-memory-model/#dram-organisation
 * for details). SRAM1 is also used for shared memory for inter processor communication
 * (see esp32_common.dtsi). In total 32kB at the start of SRAM1 are reserved.
 *
 *
 * Memory Map
 * ----------
 *
 * The above results in two memory maps, depending on which memory bus is viewed:
 *
 * Instruction Memory Bus (SRAM1 addresses are reversed):
 *
 * ********* ------------------------------------
 * | SRAM1 | Reserved for ROM functions
 * |       | ------------------------------------
 * |       | ESP HEAP
 * |       | ------------------------------------
 * |       | IRAM (high addresses)
 * *********
 * | SRAM0 | IRAM (low addresses)
 * |       | ------------------------------------
 * |       | 2nd Stage Bootloader IRAM (optional)
 * |       | External Memory Cache
 * ********* ------------------------------------
 *
 * Data Memory Bus (SRAM1 addresses are not reversed):
 *
 * *********
 * | SRAM1 | IRAM (high addreses)
 * |       | ------------------------------------
 * |       | ESP HEAP
 * |       | ------------------------------------
 * |       | Reserved for ROM functions
 * ********* ------------------------------------
 * | SRAM2 | DRAM
 * |       | ------------------------------------
 * |       | Reserved for Bluetooth (optional)
 * |       | Reserved for ROM functions
 * ********* ------------------------------------
 */

/* SRAM0 (addresses in the device tree are in the instruction memory bus) */
#define SRAM0_START	DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM0_SIZE	DT_REG_SIZE(DT_NODELABEL(sram0))
/* SRAM1 (addresses in the devie tree are in the data memory bus) */
#define SRAM1_START	DT_REG_ADDR(DT_NODELABEL(sram1))
#define SRAM1_SIZE	DT_REG_SIZE(DT_NODELABEL(sram1))
/* SRAM2 (addresses in the device tree are in the data memory bus) */
#define SRAM2_START	DT_REG_ADDR(DT_NODELABEL(sram2))
#define SRAM2_SIZE	DT_REG_SIZE(DT_NODELABEL(sram2))

/*
 * External Memory Cache
 */

/* SRAM0: External Memory Cache (32kB per active CPU) */
#define CACHE_START		SRAM0_START
#define CACHE_SIZE_SINGLE_CPU	0x8000
#define CACHE_SIZE_DUAL_CPU	0x10000
#if defined(CONFIG_SOC_ENABLE_APPCPU) || defined(CONFIG_SOC_ESP32_APPCPU)
#define CACHE_SIZE		CACHE_SIZE_SINGLE_CPU
#else
#define CACHE_SIZE		CACHE_SIZE_DUAL_CPU
#endif

/*
 * 1st Stage bootloader
 */

#if defined(CONFIG_MCUBOOT) || defined(CONFIG_BOOTLOADER_MCUBOOT)
/*
 * 2nd Stage Bootloader
 */

/* Place critical bootloader segment at start of SRAM0 in the cache segment, this memory cannot
 * be reclaimed by the application but since it is "hidden" in the cache segment this does not
 * matter. Use the rest of SRAM0 for the uncritical (reclaimable) IRAM section.
 */

/* SRAM0: Critical IRAM for 2nd Stage Bootloader */
#define BOOTLOADER_IRAM_LOADER_START	(SRAM0_START + CACHE_SIZE_SINGLE_CPU)
#define BOOTLOADER_IRAM_LOADER_SIZE	CONFIG_ESP32_MCUBOOT_IRAM
#define BOOTLOADER_IRAM_RESERVED	(CACHE_SIZE_SINGLE_CPU + BOOTLOADER_IRAM_LOADER_SIZE)

/* SRAM0: Reclaimable IRAM for 2nd Stage Bootloader (remainder of SRAM0)*/
#define BOOTLOADER_IRAM_START		(BOOTLOADER_IRAM_LOADER_START + BOOTLOADER_IRAM_LOADER_SIZE)
#define BOOTLOADER_IRAM_SIZE		(SRAM0_SIZE - BOOTLOADER_IRAM_RESERVED)

/* Place DRAM of second stage bootloader in "RAM bank 2 for application usage" (see ROM DRAM table
 * above), the DRAM can later be reused by the application.
 */
/* SRAM1: Bootloader DRAM (96kB) */
#define BOOTLOADER_DRAM_START		0x3ffe8000
#define BOOTLOADER_DRAM_SIZE		0x18000
#else
#define BOOTLOADER_IRAM_RESERVED	0
#endif

/*
 * Application
 */

/* Some sections at the start of SRAM1 are reserved for ROM functions and shared
 * memory, in total 32kB are blocked.
 * Depending on the use case a part of SRAM1 can be used as HEAP for the ESP heap allocator.
 * The remaining part of SRAM1 is used for IRAM.
 */
#define SRAM1_RESERVED	0x8000
#define SRAM1_DRAM_SIZE	CONFIG_ESP32_SRAM1_HEAP_SIZE
#define SRAM1_IRAM_SIZE (SRAM1_SIZE - SRAM1_RESERVED - CONFIG_ESP32_SRAM1_HEAP_SIZE)

/* If the external RAM cache is used, the ciritcal section of the bootloader is placed
 * inside the cache area and we do not have to care about it. If no cache is used the
 * critical section must be excluded from the applications' IRAM.
 */
#define IRAM_RESERVED	(CACHE_SIZE > BOOTLOADER_IRAM_RESERVED ? \
			 CACHE_SIZE : BOOTLOADER_IRAM_RESERVED)

/* SRAM0 + SRAM1: Instruction Memory */
#define IRAM_START	(SRAM0_START + IRAM_RESERVED)
#define IRAM_SIZE	(SRAM0_SIZE - IRAM_RESERVED + SRAM1_IRAM_SIZE)

/* SRAM2: Reserved ROM + Bluetooth Data Memory (8kB + 0xdb5c Byte (~54kB) if BT is used) */
#define DRAM_RESERVED	(0x2000 + CONFIG_ESP32_BT_RESERVE_DRAM)

/* SRAM2: Data Memory */
#define DRAM_START	(SRAM2_START + DRAM_RESERVED)
#define DRAM_SIZE	(SRAM2_SIZE - DRAM_RESERVED)

/* SRAM1: ESP Heap Memory */
#define HEAP_START	SRAM1_START + SRAM1_RESERVED
#define HEAP_SIZE	SRAM1_DRAM_SIZE

/* "Pro" and "App" CPU Memory
 *
 * The second processor core (the "App" core) can be used for asymetric multiprocessing.
 * In that case it needs its own IRAM and DRAM sections which are placed at the end of
 * the IRAM and DRAM sections of the primary processor (the "Pro" core).
 */

/* Secondary / App CPU */
#if defined(CONFIG_SOC_ENABLE_APPCPU) || defined(CONFIG_SOC_ESP32_APPCPU)
#define APPCPU_IRAM_SIZE	CONFIG_ESP_APPCPU_IRAM_SIZE
#define APPCPU_DRAM_SIZE	CONFIG_ESP_APPCPU_DRAM_SIZE
#else
#define APPCPU_IRAM_SIZE	0
#define APPCPU_DRAM_SIZE	0
#endif
#define APPCPU_IRAM_START	(IRAM_START + IRAM_SIZE - APPCPU_IRAM_SIZE)
#define APPCPU_DRAM_START	(DRAM_START + DRAM_SIZE - APPCPU_DRAM_SIZE)

/* Primary / Pro CPU */
#define PROCPU_IRAM_START	IRAM_START
#define PROCPU_IRAM_SIZE	(IRAM_SIZE - APPCPU_IRAM_SIZE)

#define PROCPU_DRAM_START	DRAM_START
#define PROCPU_DRAM_SIZE	(DRAM_SIZE - APPCPU_DRAM_SIZE)

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE		CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE		0x400000
#endif
/* Get size of the FLASH code partition from the device tree */
#define FLASH_CODE_START	DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#define FLASH_CODE_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_code_partition))

/* Cached memories */
#define CACHE_ALIGN		CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG		0x400d0000
#define IROM_SEG_LEN		(FLASH_SIZE - 0x1000)
#define DROM_SEG_ORG		0x3f400000
#define DROM_SEG_LEN		(FLASH_SIZE - 0x1000)

/*
 * IRAM <-> DRAM Address Conversion (used by the linker scripts)
 */

/* Get the start addresses of SRAM1 in IRAM and DRAM address space.
 *
 * The IRAM start address must be calculated from SRAM0 as the addresses of SRAM1 which are read
 * from the device tree are on the data memory bus.
 *
 * The DRAM start address can be read directly from the devie tree.
 */
#define SRAM1_IRAM_START		(SRAM0_START + SRAM0_SIZE)
#define SRAM1_DRAM_START		SRAM1_START

/* Convert IRAM address to its DRAM counterpart in SRAM1 memory */
#define SRAM1_IRAM_DRAM_CALC(addr_iram)	(SRAM1_SIZE - (addr_iram - SRAM1_IRAM_START) + \
					 SRAM1_DRAM_START)
/* Convert DRAM address to its IRAM counterpart in SRAM1 memory */
#define SRAM1_DRAM_IRAM_CALC(addr_dram) (SRAM1_SIZE - (addr_dram - SRAM1_DRAM_START) + \
					 SRAM1_IRAM_START)
