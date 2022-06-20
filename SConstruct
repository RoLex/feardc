# vim: set filetype: py

EnsureSConsVersion(4, 0, 0)

import os
import sys

from build_util import Dev, gen_po_name

# TODO enable "-fdebug-types-section" when
# <https://sourceware.org/bugzilla/show_bug.cgi?id=20645> is resolved.

# TODO enable LTO once we move to a compiler based on gcc 8.2.1 or later
# where https://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg580583.html
# is fixed.

# Disabled GCC warnings:
#   -Wno-missing-field-initializers: Overzealous; makes sense to disable.
#   -Wno-unknown-pragmas: htmlhelp.h emits these.
#   -Wno-unused-[parameter / value]: We have a ton of these; maybe one day...
#   -Wno-unused-but-set-variable: dwarf/dwarf_arange.c:113:13
gcc_flags = {
    'common': [
        '-g', '-Wall', '-Wextra',
        '-Wno-missing-field-initializers',
        '-Wno-unknown-pragmas',
        '-Wno-unused-parameter', '-Wno-unused-value',
        '-Wno-unused-but-set-variable',
        '-fexceptions'
    ],
    'debug': [],
    'release': ['-O3']
}

gcc_xxflags = {
    'common': [],
    'debug': [],
    'release': ['-std=gnu++20']
}

msvc_flags = {
    # 4100: unreferenced formal parameter
    # 4121: alignment of member sensitive to packing
    # 4127: conditional expression is constant
    # 4189: var init'd, unused
    # 4244: possible loss of data on conversion
    # 4290: exception spec ignored
    # 4307: integral constant overflow (size_t -1 in boost lockfree)
    # 4324: structure padded due to __declspec(align())
    # 4355: "this" used in a constructor
    # 4510: no default constructor
    # 4512: assn not generated
    # 4610: no default constructor
    # 4706: assignment within conditional expression
    # 4800: converting from BOOL to bool
    # 4996: fn unsafe, use fn_s
    'common': [
        '/W4', '/EHsc', '/Zi', '/Zm200', '/GR', '/FC', '/wd4100', '/wd4121',
        '/wd4127', '/wd4189', '/wd4244', '/wd4290', '/wd4307', '/wd4324',
        '/wd4355', '/wd4510', '/wd4512', '/wd4610', '/wd4706', '/wd4800',
        '/wd4996'
    ],
    'debug': ['/MDd'],
    'release': ['/MD', '/O2']
}

msvc_xxflags = {
    'common': [],
    'debug': [],
    'release': []
}

gcc_link_flags = {
    'common': [
        '-g', '-Wl,--no-undefined,--nxcompat,--dynamicbase', '-time'
    ],
    'debug': [],
    'release': ['-O3', '-static']
}

msvc_link_flags = {
    'common': [
        '/DEBUG', '/FIXED:NO', '/INCREMENTAL:NO', '/SUBSYSTEM:WINDOWS',
        '/MANIFESTUAC:NO'
    ],
    'debug': [],
    'release': []
}

# BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE: Prevents conflicts in some cases where
# we do use a properly qualified std::forward / std::move but the boost version
# still matches - see <https://lists.boost.org/boost-users/2012/07/75176.php>.

msvc_defs = {
    'common': [
        '_REENTRANT', 'BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE',
        'snprintf=_snprintf',
    ],
    'debug': ['_DEBUG', '_HAS_ITERATOR_DEBUGGING=0', '_SECURE_SCL=0'],
    'release': ['NDEBUG']
}

gcc_defs = {
    'common': ['_REENTRANT', 'BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE', 'NO_VIZ'],
    'debug': ['_DEBUG'],
    'release': ['NDEBUG']
}

# defEnv will hold a temporary Environment used to get options that can't be
# set after the actual Environment has been created
defEnv = Environment(ENV=os.environ)
opts = Variables('custom.py', ARGUMENTS)

if sys.platform == 'win32':
    tooldef = 'mingw'
else:
    tooldef = 'default'

opts.AddVariables(
    EnumVariable(
        'tools',
        'Toolset to compile with, default = platform default (msvc under '
        'windows)',
        tooldef, ['mingw', 'default'],
    ),
    EnumVariable('mode', 'Compile mode', 'debug', ['debug', 'release']),
    BoolVariable('pch', 'Use precompiled headers', 'no'),
    BoolVariable('verbose', 'Show verbose command lines', 'no'),
    BoolVariable(
        'savetemps',
        'Save intermediate compilation files (assembly output)',
        'no',
    ),
    BoolVariable('unicode', 'Build a Unicode version on Windows', 'yes'),
    BoolVariable(
        'stdatomic',
        'Use a standard implementation of <atomic> (turn off to switch to '
        'Boost.Atomic)',
        'no',
    ),
    BoolVariable('i18n', 'Rebuild i18n files', 'no'),
    BoolVariable('help', 'Build help files (requires i18n=1)', 'yes'),
    BoolVariable(
        'webhelp',
        'Build help files for the web (requires help=1)',
        'no',
    ),
    ('prefix',
     'Prefix to use when calling GCC build tools; defaults to:\n'
     '- nothing for Linux builds.\n'
     '- "i686-w64-mingw32-" for 32-bit MinGW builds.\n'
     '- "x86_64-w64-mingw32-" for 64-bit MinGW builds.\n'
     'May be forced to nothing via "prefix=".'),
    EnumVariable('arch', 'Target architecture', 'x86', ['x86', 'x64']),
    BoolVariable('msvcproj', 'Build MSVC project files', 'no'),
    BoolVariable(
        'distro',
        'Produce the official distro (forces tools=mingw, mode=release, '
        'unicode=1, i18n=1, help=1, webhelp=1, arch=x86)',
        'no',
    )
)

opts.Update(defEnv)
Help(opts.GenerateHelpText(defEnv))

if defEnv['distro']:
    defEnv['tools'] = 'mingw'
    defEnv['mode'] = 'release'
    defEnv['unicode'] = 1
    defEnv['i18n'] = 1
    defEnv['help'] = 1
    defEnv['webhelp'] = 1
    defEnv['arch'] = 'x86'

# workaround for SCons 1.2 which hard-codes possible archs (only allows 'x86'
# and 'amd64'...) TODO remove when SCons knows about all available archs
TARGET_ARCH = defEnv['arch']
if TARGET_ARCH == 'x64':
    TARGET_ARCH = 'amd64'

# TODO MSVC_USE_SCRIPT disables SCons' automatic env setup as it doesn't know
# about VC 12 yet.
env = Environment(
    ENV=os.environ, tools=[defEnv['tools']], options=opts,
    TARGET_ARCH=TARGET_ARCH, MSVS_ARCH=TARGET_ARCH, MSVC_USE_SCRIPT=False
)

if env['distro']:
    env['tools'] = 'mingw'
    env['mode'] = 'release'
    env['unicode'] = 1
    env['i18n'] = 1
    env['help'] = 1
    env['webhelp'] = 1
    env['arch'] = 'x86'


def filterBoost(x):
    """filter out boost from dependencies to get a speedier rebuild scan.
    this means that if boost changes, scons -c needs to be run. delete
    .sconsign.dblite to see the effects of this if you're upgrading.
    """
    return [y for y in x if str(y).find('boost') == -1]

SourceFileScanner.function['.c'].recurse_nodes = filterBoost
SourceFileScanner.function['.cpp'].recurse_nodes = filterBoost
SourceFileScanner.function['.h'].recurse_nodes = filterBoost
SourceFileScanner.function['.hpp'].recurse_nodes = filterBoost

dev = Dev(env)
dev.prepare()

env.SConsignFile()

env.Append(CPPPATH=['#/'])

if dev.is_win32():
    # Windows header defines
    # <https://msdn.microsoft.com/en-us/library/aa383745(VS.85).aspx>
    env.Append(CPPDEFINES=[
        '_WIN32_WINNT=0x601',  # Windows 7
        'WINVER=0x601',  # Windows 7
        '_WIN32_IE=0x600',  # Common Controls 6

        # other defs that influence Windows headers
        'NOMINMAX', 'STRICT', 'WIN32_LEAN_AND_MEAN'])

    if env['unicode']:
        env.Append(CPPDEFINES=['UNICODE', '_UNICODE'])

    # boost defines
    env.Append(CPPDEFINES=['BOOST_ALL_NO_LIB', 'BOOST_USE_WINDOWS_H'])

    # zlib defines
    env.Append(CPPDEFINES=['CASESENSITIVITYDEFAULT_YES', 'ZLIB_WINAPI'])

if 'gcc' in env['TOOLS']:
    if env['savetemps']:
        env.Append(CCFLAGS=['-save-temps', '-fverbose-asm'])
    else:
        env.Append(CCFLAGS=['-pipe'])

    # require i686 instructions for atomic<int64_t>, used by boost::lockfree
    # (otherwise lockfree lists won't actually be lock-free).
    # Require SSE2 for e.g., significantly faster Tiger hashing
    # https://www.cryptopp.com/benchmarks.html
    # Require SSE3 for FISTTP
    if env['arch'] == 'x86':
        env.Append(CCFLAGS=['-march=nocona', '-mtune=generic']) # Through SSE3
    elif env['arch'] == 'x64': # native - for self only
        env.Append(CCFLAGS=['-march=core2', '-mtune=generic', '-msse4.1']) # Through SSSE3

if 'msvc' in env['TOOLS']:
    env['pch'] = True
if env['pch']:
    env.Append(CPPDEFINES=['HAS_PCH'])

if 'mingw' in env['TOOLS']:
    env.Append(CCFLAGS=['-mthreads'])

    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=40838
    # ("gcc shouldn't assume that the stack is aligned")
    # https://eigen.tuxfamily.org/dox/group__TopicWrongStackAlignment.html
    # Not necessary on either x86 or amd64 Linux or 64-bit Windows.
    # https://msdn.microsoft.com/en-us/library/ms235286.aspx on
    # "Overview of x64 Calling Conventions" says 64-bit Windows stacks are
    # already 16-byte aligned.
    if env['arch'] == 'x86':
        env.Append(CCFLAGS=['-mincoming-stack-boundary=2', '-mstackrealign'])

    env.Append(LINKFLAGS=[
        '-static-libgcc', '-static-libstdc++', '-mthreads',
        '-Wl,--enable-runtime-pseudo-reloc'
    ])

    if env['mode'] == 'release' or sys.platform == 'win32':
        env.Append(CCFLAGS=['-mwindows'])
        env.Append(LINKFLAGS=['-mwindows'])
    else:
        env.Append(CPPDEFINES=['CONSOLE'])

    env.Append(CPPPATH=['#/mingw/preload/', '#/mingw/include/'])
    mingw_lib = '#/mingw/lib/'
    if env['arch'] != 'x86':
        mingw_lib = mingw_lib + env['arch'] + '/'
    env.Append(LIBPATH=[mingw_lib])

if 'msvc' in env['TOOLS']:
    flags = msvc_flags
    xxflags = msvc_xxflags
    link_flags = msvc_link_flags
    defs = msvc_defs

    env.Append(LIBS=['User32', 'shell32', 'Advapi32'])

else:
    flags = gcc_flags
    xxflags = gcc_xxflags
    link_flags = gcc_link_flags
    defs = gcc_defs

    env.Tool("gch", toolpath=".")

env.Append(CPPDEFINES=defs[env['mode']])
env.Append(CPPDEFINES=defs['common'])

env.Append(CCFLAGS=flags[env['mode']])
env.Append(CCFLAGS=flags['common'])

env.Append(CXXFLAGS=xxflags[env['mode']])
env.Append(CXXFLAGS=xxflags['common'])

env.Append(LINKFLAGS=link_flags[env['mode']])
env.Append(LINKFLAGS=link_flags['common'])

import SCons.Scanner
SWIGScanner = SCons.Scanner.ClassicCPP(
    "SWIGScan",
    ".i",
    "CPPPATH",
    '^[ \t]*[%,#][ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")'
)
env.Append(SCANNERS=[SWIGScanner])

#
# internationalization (ardour.org provided the initial idea)
#

po_args = ['msgmerge', '-q', '--update', '--backup=none', '$TARGET', '$SOURCE']
po_bld = Builder(action=Action(
    [po_args],
    'Updating translation $TARGET from $SOURCES'
))
env.Append(BUILDERS={'PoBuild': po_bld})

mo_args = ['msgfmt', '-c', '-o', '$TARGET', '$SOURCE']
mo_bld = Builder(action=[
    Action(
        [mo_args],
        'Compiling message catalog $TARGET from $SOURCES'
    ),
    Action(
        lambda target, source, env: gen_po_name(source[0], env),
        'Generating $NAME_FILE')
    ]
)
env.Append(BUILDERS={'MoBuild': mo_bld})

pot_args = [
    'xgettext', '--from-code=UTF-8', '--foreign-user',
    '--package-name=$PACKAGE', '--copyright-holder=Jacek Sieka',
    '--msgid-bugs-address=dcplusplus-devel@lists.sourceforge.net',
    '--no-wrap', '--keyword=_', '--keyword=T_', '--keyword=TF_',
    '--keyword=TFN_:1,2', '--keyword=F_', '--keyword=gettext_noop',
    '--keyword=N_', '--keyword=CT_', '--boost', '-s', '--output=$TARGET',
    '$SOURCES'
]
pot_bld = Builder(action=Action(
    [pot_args],
    'Extracting messages to $TARGET from $SOURCES'
))
env.Append(BUILDERS={'PotBuild': pot_bld})


def CheckFlag(context, flag):
    context.Message('Checking support for the ' + flag + ' flag... ')
    prevFlags = context.env['CCFLAGS']
    context.env.Append(CCFLAGS=[flag])
    ret = context.TryCompile('int main() {}', '.cpp')
    context.env.Replace(CCFLAGS=prevFlags)
    context.Result(ret)
    return ret

conf = Configure(
    env,
    custom_tests={'CheckFlag': CheckFlag},
    conf_dir=dev.get_build_path('.sconf_temp'),
    log_file=dev.get_build_path('config.log'),
    clean=False,
    help=False
)

if dev.is_win32():
    # some mingw distros have one but not the other, breaking linking (note:
    # CheckLib functions do not support explicit include_quotes)
    if (
        conf.CheckCXXHeader(['windows.h', 'htmlhelp.h'], '<>') and
        conf.CheckLib('htmlhelp', 'main', None, 'C++')
    ):
        conf.env.Append(CPPDEFINES='HAVE_HTMLHELP_H')

if 'mingw' in env['TOOLS']:
    # see whether we're compiling with MinGW or MinGW-w64 (2 different projects
    # that can both build a 32-bit program). the only differentiator is
    # __MINGW64_VERSION_MAJOR.
    if not conf.CheckDeclaration(
        '__MINGW64_VERSION_MAJOR',
        '#include <windows.h>',
        'C++'
    ):
        conf.env.Append(CPPDEFINES='HAVE_OLD_MINGW')

if 'gcc' in env['TOOLS'] and env['mode'] == 'debug':
    if conf.CheckFlag('-Og'):
        conf.env.Append(CCFLAGS=['-Og'])
        conf.env.Append(LINKFLAGS=['-Og'])

env = conf.Finish()

# TODO run config tests to determine which libs to build

dev.boost = dev.build('boost/libs/') if dev.is_win32() else []
dev.dwarf = dev.build('dwarf/')
dev.zlib = dev.build('zlib/')
dev.build('zlib/test/')
dev.bzip2 = dev.build('bzip2/') if dev.is_win32() else []
dev.geoip = dev.build('geoip/')
dev.intl = dev.build('intl/') if dev.is_win32() else []
dev.miniupnpc = dev.build('miniupnpc/')
dev.natpmp = dev.build('natpmp/')
dev.client = dev.build('dcpp/')

dev.dwt = dev.build('dwt/src/') if dev.is_win32() else []
if dev.is_win32():
    dev.build('dwt/test/')

if dev.is_win32():
    dev.build('help/')

dev.build('test/')
dev.build('utils/')

if dev.is_win32():
    dev.build('win32/')

    dev.build('installer/')

dev.finalize()
