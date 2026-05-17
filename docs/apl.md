# APL Toolchain

APL is the custom language used by AdyOS experiments. It has an i386 backend and is being prepared as a first-class build input for kernel and toolchain testing.

## Compiler Interface

The current compiler is available in the development environment as:

```text
build/ccompiler
```

It reports itself as `capl 3.5`. The useful options for AdyOS are:

- Input files such as `*.apl`.
- Include directories with `-I <dir>`.
- Assembly output with `--emit-asm`.
- Stopping after assembly output with `--no-compile`.
- Assembly output naming with `--asm-output <file>`.
- i386 target selection with `--arch i386 --sys-type i386`.
- Output naming with `--output <file>` for compile/link outputs.
- Assembler selection with `--asm-compiler <tool>`.
- Assembler format selection with `--asm-format elf32`.
- Linker selection with `--linker <tool>`.

The SCons integration uses `--emit-asm --no-compile --asm-output <target>` for kernel APL sources.

## SCons Integration

APL support is enabled by default because several low-level i386 routines are now written in `.apl`.

The default build uses:

```bash
scons
```

Use assembly as the intermediate output:

```bash
scons enable_apl=1 apl=build/ccompiler apl_object_mode=asm
```

By default, the build asks APL for assembly, then lets the normal NASM path assemble it. This keeps APL integration close to the rest of the kernel build and avoids depending on APL's own assembler/linker stage for kernel objects.

The default APL flags are:

```bash
--arch i386 --sys-type i386
```

The build passes kernel include directories to APL, including:

```text
src/kernel
src/kernel/include
src/libs/include
```

## Why APL Belongs Here

An operating system is a useful compiler target because it exercises code generation in hostile conditions:

- No hosted standard library.
- Strict ABI expectations.
- Inline assembly and privileged instructions.
- Linker-script-controlled memory layout.
- Interrupts, syscalls, and user/kernel transitions.
- Real hardware and emulator differences.

AdyOS is small enough to understand, but real enough to punish vague compiler assumptions.
