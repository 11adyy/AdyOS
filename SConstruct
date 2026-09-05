# Thanks to nanobyte's series of OS building
from pathlib import Path

from SCons.Variables import *
from SCons.Environment import *
from SCons.Node import *

from build.apl import setup_apl_builders
from build.utility import remove_suffix

VARS = Variables('build_scripts/config.py', ARGUMENTS)
VARS.AddVariables(
    EnumVariable("config",
                 help="Build configuration",
                 default="debug",
                 allowed_values=("debug", "release")),
    EnumVariable("arch", 
                 help="Target architecture", 
                 default="i686",
                 allowed_values=("i686")),
    EnumVariable("image_type",
                 help="Type of image",
                 default="disk",
                 allowed_values=("floppy", "disk")),
    EnumVariable("image_file_system",
                 help="Type of image",
                 default="fat32",
                 allowed_values=("fat12", "fat16", "fat32", "ext2")),
    BoolVariable("enable_apl",
                 help="Build .apl sources with the APL compiler",
                 default=True),
    EnumVariable("apl_object_mode",
                 help="How APL sources are turned into objects",
                 default="asm",
                 allowed_values=("asm", "object")),
    )

VARS.Add("tool_chain", 
         help="Path to tool_chain directory.",
         default="../tool_chain")
VARS.Add("apl",
         help="Path to APL compiler binary.",
         default="build/aplc")
VARS.Add("apl_flags",
         help="Extra flags passed to the APL compiler.",
         default="--arch i386 --sys-type i386")
VARS.Add("apl_emit_asm_flag",
         help="APL compiler flag for producing assembly.",
         default="--emit-asm")
VARS.Add("apl_no_compile_flag",
         help="APL compiler flag used to stop after assembly generation.",
         default="-c")
VARS.Add("apl_asm_output_flag",
         help="APL compiler flag used to select assembly output path.",
         default="--asm-output")
VARS.Add("apl_compile_flag",
         help="Optional APL compiler flag for producing an object file.",
         default="")
VARS.Add("apl_link_flag",
         help="Optional APL compiler flag for compiling and linking.",
         default="")
VARS.Add("apl_output_flag",
         help="APL compiler output flag.",
         default="--output")
VARS.Add("apl_include_prefix",
         help="APL compiler include path prefix.",
         default="-I")
VARS.Add("apl_assembler_flag",
         help="APL compiler flag used to pass assembler executable.",
         default="--asm-compiler")
VARS.Add("apl_asm_format",
         help="Assembler object format requested from APL.",
         default="elf32")
VARS.Add("apl_asm_format_flag",
         help="APL compiler flag used to pass assembler object format.",
         default="--asm-format")
VARS.Add("apl_linker_flag",
         help="APL compiler flag used to pass linker executable.",
         default="--linker")
VARS.Add("apl_link_flags",
         help="Extra APL linker flags, for example --linker-mode raw.",
         default="")

DEPS = {
    'binutils': '2.37',
    'gcc': '11.2.0'
}

HOST_ENVIRONMENT = Environment(variables=VARS,
    ENV = os.environ,
    AS = 'nasm',
    CFLAGS = ['-std=c99'],
    CXXFLAGS = ['-std=c++17'],
    CCFLAGS = ['-g'],
    STRIP = 'strip',
)

HOST_ENVIRONMENT.Append(
    PROJECTDIR = HOST_ENVIRONMENT.Dir('.').srcnode()
)

if HOST_ENVIRONMENT['config'] == 'debug':
    HOST_ENVIRONMENT.Append(CCFLAGS = ['-O0'])
else:
    HOST_ENVIRONMENT.Append(CCFLAGS = ['-O3'])

if HOST_ENVIRONMENT['image_type'] == 'floppy':
    HOST_ENVIRONMENT['image_file_system'] = 'fat12'

HOST_ENVIRONMENT.Replace(ASCOMSTR        = "Assembling [$SOURCE]",
                         CCCOMSTR        = "Compiling  [$SOURCE]",
                         CXXCOMSTR       = "Compiling  [$SOURCE]",
                         FORTRANPPCOMSTR = "Compiling  [$SOURCE]",
                         FORTRANCOMSTR   = "Compiling  [$SOURCE]",
                         SHCCCOMSTR      = "Compiling  [$SOURCE]",
                         SHCXXCOMSTR     = "Compiling  [$SOURCE]",
                         LINKCOMSTR      = "Linking    [$TARGET]",
                         SHLINKCOMSTR    = "Linking    [$TARGET]",
                         INSTALLSTR      = "Installing [$TARGET]",
                         ARCOMSTR        = "Archiving  [$TARGET]",
                         RANLIBCOMSTR    = "Ranlib     [$TARGET]")

platform_prefix = ''
if HOST_ENVIRONMENT['arch'] == 'i686':
    platform_prefix = 'i686-elf-'

tool_chainDir = Path(HOST_ENVIRONMENT['tool_chain'], remove_suffix(platform_prefix, '-')).resolve()
tool_chainBin = Path(tool_chainDir, 'bin')
tool_chainGccLibs = Path(tool_chainDir, 'lib', 'gcc', remove_suffix(platform_prefix, '-'), DEPS['gcc'])

TARGET_ENVIRONMENT = HOST_ENVIRONMENT.Clone(
    AR      = f'{platform_prefix}ar',
    CC      = f'{platform_prefix}gcc',
    CXX     = f'{platform_prefix}g++',
    LD      = f'{platform_prefix}g++',
    RANLIB  = f'{platform_prefix}ranlib',
    STRIP   = f'{platform_prefix}strip',

    # toolchain
    TOOLCHAIN_PREFIX = str(tool_chainDir),
    TOOLCHAIN_LIBGCC = str(tool_chainGccLibs),

    BINUTILS_URL    = f'https://ftp.gnu.org/gnu/binutils/binutils-{DEPS["binutils"]}.tar.xz',
    GCC_URL         = f'https://ftp.gnu.org/gnu/gcc/gcc-{DEPS["gcc"]}/gcc-{DEPS["gcc"]}.tar.xz',
)

TARGET_ENVIRONMENT.Append(
    ASFLAGS = [
        '-f', 'elf',
        '-g'
    ],

    CCFLAGS = [
        '-ffreestanding',
        '-nostdlib',
        '-Wall', '-Wno-comment', '-Wno-unknown-pragmas'
    ],

    CXXFLAGS = [
        '-fno-exceptions',
        '-fno-rtti',
    ],

    LINKFLAGS = [
        '-nostdlib'
    ],
    
    LIBS    = ['gcc'],
    LIBPATH = [ str(tool_chainGccLibs) ],
)

TARGET_ENVIRONMENT.Replace(
    APL                 = TARGET_ENVIRONMENT['apl'],
    APLFLAGS            = TARGET_ENVIRONMENT.Split(TARGET_ENVIRONMENT['apl_flags']),
    APL_OBJECT_MODE     = TARGET_ENVIRONMENT['apl_object_mode'],
    APLEMITASMFLAGS     = TARGET_ENVIRONMENT.Split(TARGET_ENVIRONMENT['apl_emit_asm_flag']) + TARGET_ENVIRONMENT.Split(TARGET_ENVIRONMENT['apl_no_compile_flag']),
    APLCOMPILEFLAG      = TARGET_ENVIRONMENT['apl_compile_flag'],
    APLLINKFLAG         = TARGET_ENVIRONMENT['apl_link_flag'],
    APLOUTPUTFLAG       = TARGET_ENVIRONMENT['apl_output_flag'],
    APLASMOUTPUTFLAG    = TARGET_ENVIRONMENT['apl_asm_output_flag'],
    APLINCPREFIX        = TARGET_ENVIRONMENT['apl_include_prefix'],
    APLASOPTION         = TARGET_ENVIRONMENT['apl_assembler_flag'],
    APLASMFORMAT        = TARGET_ENVIRONMENT['apl_asm_format'],
    APLASMFORMATOPTION  = TARGET_ENVIRONMENT['apl_asm_format_flag'],
    APLLDOPTION         = TARGET_ENVIRONMENT['apl_linker_flag'],
    APLLINKFLAGS        = TARGET_ENVIRONMENT.Split(TARGET_ENVIRONMENT['apl_link_flags']),
)

setup_apl_builders(TARGET_ENVIRONMENT)

TARGET_ENVIRONMENT['ENV']['PATH'] += os.pathsep + str(tool_chainBin)

Help(VARS.GenerateHelpText(HOST_ENVIRONMENT))
Export('HOST_ENVIRONMENT')
Export('TARGET_ENVIRONMENT')

bootDir = 'build/AdyOS/boot'
homeDir = 'build/AdyOS/home'

# Static libs
SConscript('src/libs/SConscript', variant_dir=bootDir + '/libs', duplicate=0)

# User land
# SConscript('src/userl/SConscript', variant_dir=variantDir + '/userl', duplicate=0)

# Apps
SConscript('apps/shell/SConscript', variant_dir=homeDir + '/apps/shell', duplicate=0)
SConscript('apps/games/doom/SConscript', variant_dir=homeDir + '/apps/games/doom', duplicate=0)
SConscript('apps/std/calc/SConscript', variant_dir=homeDir + '/apps/std/calc', duplicate=0)
# SConscript('apps/std/editor/SConscript', variant_dir=homeDir + '/apps/std/editor', duplicate=0)

# Kernel
SConscript('src/kernel/SConscript', variant_dir=bootDir + '/kernel', duplicate=0)

Import('kernel')
Default(kernel)
