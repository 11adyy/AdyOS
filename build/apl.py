from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Environment import Environment


def _split(env: Environment, value):
    if value is None:
        return []
    return [env.subst(str(item)) for item in env.Split(value) if env.subst(str(item))]


def _append_option(args, option, value):
    if option and value:
        args.extend([option, value])


def _include_flags(env: Environment):
    prefix = env.subst('$APLINCPREFIX')
    suffix = env.subst('$APLINCSUFFIX')
    includes = []
    seen = set()

    for include_dir in _split(env, env.get('APLPATH', [])) + _split(env, env.get('CPPPATH', [])):
        if include_dir in seen:
            continue
        seen.add(include_dir)
        includes.append(f'{prefix}{include_dir}{suffix}')

    return includes


def _tool_flags(env: Environment):
    args = []

    _append_option(args, env.subst('$APLASOPTION'), env.subst('$AS'))
    _append_option(args, env.subst('$APLASFLAGSOPTION'), ' '.join(_split(env, env.get('ASFLAGS', []))))
    _append_option(args, env.subst('$APLLDOPTION'), env.subst('$LD'))
    _append_option(args, env.subst('$APLLINKFLAGSOPTION'), ' '.join(_split(env, env.get('LINKFLAGS', []))))

    return args


def _apl_command(env: Environment, mode_flag, target, source):
    args = [env.subst('$APL')]
    args.extend(_split(env, env.get('APLFLAGS', [])))
    args.extend(_include_flags(env))
    args.extend(_tool_flags(env))

    mode = env.subst(mode_flag)
    if mode:
        args.append(mode)

    output_flag = env.subst('$APLOUTPUTFLAG')
    if output_flag:
        args.extend([output_flag, str(target[0])])
    else:
        args.append(str(target[0]))

    args.extend(str(src) for src in source)
    return args


def _apl_asm_generator(target, source, env, for_signature):
    return Action(_apl_command(env, '$APLEMITASMFLAG', target, source), '$APLASMCOMSTR')


def _apl_object_generator(target, source, env, for_signature):
    return Action(_apl_command(env, '$APLCOMPILEFLAG', target, source), '$APLOBJCOMSTR')


def _apl_program_generator(target, source, env, for_signature):
    return Action(_apl_command(env, '$APLLINKFLAG', target, source), '$APLLINKCOMSTR')


def _apl_object(env: Environment, source):
    if env.subst('$APL_OBJECT_MODE') == 'object':
        return env.APLDirectObject(source)

    asm_sources = env.APLAsm(source)
    return env.Object(asm_sources)


def setup_apl_builders(env: Environment):
    env.SetDefault(
        APL='apl',
        APLFLAGS=[],
        APLPATH=[],
        APL_OBJECT_MODE='asm',
        APLEMITASMFLAG='--emit-asm',
        APLCOMPILEFLAG='--compile',
        APLLINKFLAG='--link',
        APLOUTPUTFLAG='-o',
        APLINCPREFIX='-I',
        APLINCSUFFIX='',
        APLASOPTION='--assembler',
        APLASFLAGSOPTION='--assembler-flags',
        APLLDOPTION='--linker',
        APLLINKFLAGSOPTION='--linker-flags',
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
