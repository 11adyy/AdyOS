import os
import subprocess

from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Environment import Environment


def _split(env: Environment, value):
    if value is None:
        return []
    result = []
    for item in env.Split(value):
        item = env.subst(str(item))
        if item:
            result.append(item)
    return result


def _flatten(value):
    if value is None:
        return []
    if isinstance(value, (list, tuple)):
        result = []
        for item in value:
            result.extend(_flatten(item))
        return result
    return [value]


def _append_option(args, option, value):
    if option and value:
        args.extend([option, value])


def _append_mode_flags(env: Environment, args, *flags):
    for flag in _flatten(flags):
        mode = env.subst(flag)
        if mode:
            args.extend(_split(env, mode))


def _include_flags(env: Environment):
    prefix = env.subst('$APLINCPREFIX')
    suffix = env.subst('$APLINCSUFFIX')
    includes = []
    seen = set()

    for include_dir in _split(env, env.get('APLPATH', [])) + _split(env, env.get('CPPPATH', [])):
        if include_dir in seen:
            continue
        seen.add(include_dir)
        if prefix and not suffix:
            includes.extend([prefix, include_dir])
        else:
            includes.append(f'{prefix}{include_dir}{suffix}')

    return includes


def _tool_flags(env: Environment):
    args = []

    _append_option(args, env.subst('$APLASOPTION'), env.subst('$AS'))
    _append_option(args, env.subst('$APLASMFORMATOPTION'), env.subst('$APLASMFORMAT'))
    _append_option(args, env.subst('$APLLDOPTION'), env.subst('$LD'))
    _append_option(args, env.subst('$APLCODESECTIONOPTION'), env.subst('$APLCODESECTION'))
    _append_option(args, env.subst('$APLROSECTIONOPTION'), env.subst('$APLROSECTION'))
    _append_option(args, env.subst('$APLGLOBSECTIONOPTION'), env.subst('$APLGLOBSECTION'))
    args.extend(_split(env, env.get('APLLINKFLAGS', [])))

    return args


def _apl_command(
    env: Environment,
    mode_flag,
    target,
    source,
    output_flag='$APLOUTPUTFLAG',
    use_include_flags=True,
    use_tool_flags=True,
    extra_mode_flags=None,
):
    apl = env.subst('$APL')
    if not os.path.isabs(apl) and os.path.exists(apl):
        apl = os.path.abspath(apl)

    args = [apl]
    args.extend(_split(env, env.get('APLFLAGS', [])))
    if use_include_flags:
        args.extend(_include_flags(env))
    if use_tool_flags:
        args.extend(_tool_flags(env))

    _append_mode_flags(env, args, mode_flag, extra_mode_flags)

    output = env.subst(output_flag)
    if output:
        args.extend([output, target[0].abspath])
    else:
        args.append(target[0].abspath)

    args.extend(src.srcnode().abspath for src in source)
    return args


def _apl_emit_asm_action(target, source, env):
    target_path = target[0].abspath
    target_dir = os.path.dirname(target_path)
    if target_dir:
        os.makedirs(target_dir, exist_ok=True)

    command = _apl_command(
        env,
        '$APLEMITASMFLAGS',
        target,
        source,
        output_flag='$APLASMOUTPUTFLAG',
        use_include_flags=True,
        use_tool_flags=True,
        extra_mode_flags='$APLCOMPILEFLAG',
    )
    subprocess.check_call(command)
    if not os.path.exists(target_path):
        raise RuntimeError(f'APL compiler did not produce {target_path}')

    return 0


def _apl_asm_generator(target, source, env, for_signature):
    return Action(_apl_emit_asm_action, '$APLASMCOMSTR')


def _apl_object_generator(target, source, env, for_signature):
    return Action(_apl_command(env, '$APLCOMPILEFLAG', target, source), '$APLOBJCOMSTR')


def _apl_program_generator(target, source, env, for_signature):
    return Action(_apl_command(env, '$APLLINKFLAG', target, source), '$APLLINKCOMSTR')


def _apl_target_name(env: Environment, src, suffix):
    src_node = env.File(src).srcnode()
    src_root = env.Dir('.').srcnode().abspath
    src_path = os.path.relpath(src_node.abspath, src_root)
    base, _ = os.path.splitext(src_path)
    return f'{base}_apl{suffix}'


def _apl_object(env: Environment, source):
    sources = _flatten(source)
    objects = []

    if env.subst('$APL_OBJECT_MODE') == 'object':
        for src in sources:
            objects.extend(_flatten(env.APLDirectObject(_apl_target_name(env, src, '.o'), src)))
        return objects

    for src in sources:
        asm_sources = env.APLAsm(_apl_target_name(env, src, '.asm'), src)
        objects.extend(_flatten(env.Object(_apl_target_name(env, src, '.o'), asm_sources)))
    return objects


def setup_apl_builders(env: Environment):
    env.SetDefault(
        APL='apl',
        APLFLAGS=[],
        APLPATH=[],
        APLLINKFLAGS=[],
        APL_OBJECT_MODE='asm',
        APLEMITASMFLAGS=['--emit-asm', '-no'],
        APLCOMPILEFLAG='-O3',
        APLLINKFLAG='',
        APLOUTPUTFLAG='--output',
        APLASMOUTPUTFLAG='--asm-output',
        APLINCPREFIX='-I',
        APLINCSUFFIX='',
        APLASOPTION='--asm-compiler',
        APLASMFORMAT='elf32',
        APLASMFORMATOPTION='--asm-format',
        APLLDOPTION='--linker',
        APLCODESECTION='.text',
        APLCODESECTIONOPTION='--code-section',
        APLROSECTION='.rodata',
        APLROSECTIONOPTION='--ro-section',
        APLGLOBSECTION='.data',
        APLGLOBSECTIONOPTION='--glob-section',
        APLASMCOMSTR='APL -> asm [$SOURCE]',
        APLOBJCOMSTR='APL -> obj [$SOURCE]',
        APLLINKCOMSTR='APL linking [$TARGET]',
    )

    env.Append(
        BUILDERS={
            'APLAsm': Builder(
                generator=_apl_asm_generator,
                suffix='.asm',
                src_suffix='.apl',
            ),
            'APLDirectObject': Builder(
                generator=_apl_object_generator,
                suffix='.o',
                src_suffix='.apl',
            ),
            'APLProgram': Builder(
                generator=_apl_program_generator,
                suffix='.elf',
                src_suffix='.apl',
            ),
        }
    )

    env.AddMethod(_apl_object, 'APLObject')
