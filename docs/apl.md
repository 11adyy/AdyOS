# APL Toolchain

APL is the custom language used by AdyOS experiments. It has an i386 backend and is being prepared as a first-class build input for kernel and toolchain testing.

## Intended Compiler Interface

The compiler binary should accept:

- Input files such as `*.apl`.
- Include directories for source headers and imported files.
- A mode that emits assembly.
- A mode that compiles to object code.
- A mode that compiles and links.
- Explicit assembler and linker paths.
- Explicit assembler and linker flags.

## SCons Integration

APL support is off by default so the normal C and assembly build continues to work without a APL compiler installed.

Enable APL sources with:

```bash
scons enable_apl=1 apl=/path/to/apl
```

Use assembly as the intermediate output:

```bash
scons enable_apl=1 apl=/path/to/apl apl_object_mode=asm
```

Ask APL to produce object files directly:

```bash
scons enable_apl=1 apl=/path/to/apl apl_object_mode=object
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
