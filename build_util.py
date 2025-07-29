import glob
import sys
import os
import fnmatch


class Dev:
    def __init__(self, env):
        self.env = env

        self.build_root = "#/build/" + env["mode"] + "-" + env["tools"]
        if env["arch"] != "x86":
            self.build_root += "-" + env["arch"]
        self.build_root += "/"

    def prepare(self):
        if not self.env["verbose"]:
            self.env["CCCOMSTR"] = "Compiling $TARGET (static)"
            self.env["SHCCCOMSTR"] = "Compiling $TARGET (shared)"
            self.env["CXXCOMSTR"] = "Compiling $TARGET (static)"
            self.env["SHCXXCOMSTR"] = "Compiling $TARGET (shared)"
            self.env["PCHCOMSTR"] = "Compiling $TARGET (precompiled header)"
            self.env["GCHCOMSTR"] = "Compiling $TARGET (static precompiled header)"
            self.env["GCHSHCOMSTR"] = "Compiling $TARGET (shared precompiled header)"
            self.env["SHLINKCOMSTR"] = "Linking $TARGET (shared)"
            self.env["LINKCOMSTR"] = "Linking $TARGET (static)"
            self.env["ARCOMSTR"] = "Archiving $TARGET"
            self.env["RCCOMSTR"] = "Resource $TARGET"

        self.env.SConsignFile()
        self.env.SetOption("implicit_cache", "1")
        self.env.SetOption("max_drift", 60 * 10)
        self.env.Decider("MD5-timestamp")

        if "gcc" in self.env["TOOLS"]:
            # when building with GCC, honor the "prefix" setting.
            prefix = self.env.get("prefix")

            if prefix is None:
                # no explicit prefix; set one for MinGW builds.
                if "mingw" not in self.env["TOOLS"]:
                    prefix = ""
                elif self.env["arch"] == "x86":
                    prefix = "i686-w64-mingw32-"
                elif self.env["arch"] == "x64":
                    prefix = "x86_64-w64-mingw32-"

            # prefix our build tools.
            BUILD_TOOLS = {
                "CC": "gcc",
                "CXX": "g++",
                "LINK": "g++",
                "AR": "ar",
                "RANLIB": "ranlib",
                "RC": "windres",
                "strip": "strip",
            }
            for tool_ref, tool_bin in BUILD_TOOLS.items():
                self.env[tool_ref] = prefix + tool_bin

            # "gcc" should always be present.
            gcc_path = self.env.WhereIs(self.env["CC"])
            if not gcc_path:
                raise Exception('GCC bin "%s" not found.' % self.env["CC"])

            # set explicit paths on missing build tools, as they are not always
            # all prefixed (though the "gcc" binary always is, so we use it to
            # fetch a base path).
            # <https://sourceforge.net/p/mingw-w64/mailman/message/33224826/>
            if "mingw" in self.env["TOOLS"]:
                for tool_ref, tool_bin in BUILD_TOOLS.items():
                    if not self.env.WhereIs(self.env[tool_ref]):
                        base_tool_path = os.path.dirname(gcc_path)
                        print(
                            'Using a non-prefixed version of "%s" from "%s".'
                            % (tool_bin, base_tool_path)
                        )
                        self.env[tool_ref] = os.path.join(
                            base_tool_path,
                            tool_bin,
                        )

            # when cross-compiling, be explicit about bin extensions.
            if sys.platform != "win32":
                self.env["PROGSUFFIX"] = ".exe"
                self.env["LIBPREFIX"] = "lib"
                self.env["LIBSUFFIX"] = ".a"
                self.env["SHLIBSUFFIX"] = ".dll"

            # some distros of windres fail when they receive Win paths as
            # input, so convert...
            if "RCCOM" in self.env:
                self.env["RCCOM"] = self.env["RCCOM"].replace(
                    "-i $SOURCE",
                    "-i ${SOURCE.posix}",
                    1,
                )

        if self.env["msvcproj"]:
            if "msvs" not in self.env["TOOLS"]:
                raise Exception("This is not an MSVC build; specify tools=default")
            msvcproj_arch = self.env["arch"]
            if msvcproj_arch == "x86":
                msvcproj_arch = "Win32"
            # TODO SCons doesn't seem to support multiple configs per project,
            # so each config goes to a separate directory. when this is fixed,
            # generate all projects in the root dir.
            self.msvcproj_path = (
                "#/msvc/" + self.env["mode"] + "-" + self.env["arch"] + "/"
            )
            self.msvcproj_variant = self.env["mode"] + "|" + msvcproj_arch
            self.msvcproj_projects = []

            # set up the command called when building from the VS IDE.
            self.env["MSVSSCONSFLAGS"] = (
                self.env["MSVSSCONSFLAGS"]
                + " tools="
                + self.env["tools"]
                + " mode="
                + self.env["mode"]
                + " arch="
                + self.env["arch"]
            )

            # work around a few bugs in MSVC project generation, see
            # msvcproj_workarounds for details.
            from SCons.Action import Action
            from SCons.Tool.msvs import GenerateProject

            self.env["MSVSPROJECTCOM"] = [
                Action(GenerateProject, None),
                Action(msvcproj_workarounds, None),
            ]

    def finalize(self):
        if self.env["msvcproj"]:
            path = self.msvcproj_path + "DCPlusPlus" + self.env["MSVSSOLUTIONSUFFIX"]
            self.env.Precious(path)
            self.env.MSVSSolution(
                target=path,
                variant=self.msvcproj_variant,
                projects=self.msvcproj_projects,
            )

    def is_win32(self):
        return sys.platform == "win32" or "mingw" in self.env["TOOLS"]

    def get_build_root(self):
        return self.build_root

    def get_build_path(self, source_path):
        return self.get_build_root() + source_path

    def get_target(self, source_path, name, in_bin=True):
        if in_bin:
            return self.get_build_root() + "bin/" + name
        else:
            return self.get_build_root() + source_path + name

    def get_sources(self, source_path, source_glob, recursive=False):
        matches = []
        for root, dirnames, filenames in os.walk("."):
            for filename in fnmatch.filter(filenames, source_glob):
                matches.append(root + "/" + filename)
            if not recursive:
                dirnames[:] = []
        return list(
            map(
                lambda x: (os.path.normpath(self.get_build_path(source_path) + x)),
                matches,
            )
        )

    # execute the SConscript file in the specified sub-directory.
    def build(self, source_path, local_env=None):
        if not local_env:
            local_env = self.env
        return local_env.SConscript(
            source_path + "SConscript",
            exports={"dev": self, "source_path": source_path},
        )

    # create a build environment and set up sources and targets.
    def prepare_build(
        self,
        source_path,
        name,
        source_glob="*.cpp",
        in_bin=True,
        precompiled_header=None,
        recursive=False,
    ):
        build_path = self.get_build_path(source_path)
        env = self.env.Clone()
        env.VariantDir(build_path, ".", duplicate=0)

        sources = self.get_sources(source_path, source_glob, recursive)

        if precompiled_header is not None and env["pch"] and not env["msvcproj"]:
            # TODO we work around the 2 problems described on
            # <http://scons.tigris.org/issues/show_bug.cgi?id=2680> - remove
            # once not needed

            for i, source in enumerate(sources):
                if source.find(precompiled_header + ".cpp") != -1:
                    # the PCH/GCH builder will take care of this one
                    del sources[i]

            if "msvc" in env["TOOLS"]:
                env["PCHSTOP"] = precompiled_header + ".h"
                pch = env.PCH(
                    build_path + precompiled_header + ".pch",
                    precompiled_header + ".cpp",
                )
                env["PCH"] = pch[0]
                env["ARFLAGS"] = env["ARFLAGS"] + " " + str(pch[1])
                env["LINKFLAGS"] = env["LINKFLAGS"] + " " + str(pch[1])

            elif "gcc" in env["TOOLS"]:
                env["Gch"] = env.Gch(
                    build_path + precompiled_header + ".h.gch",
                    precompiled_header + ".h",
                )[0]

                # little dance to add the pch object to include paths, while
                # overriding the current directory
                env["CXXCOM"] = (
                    env["CXXCOM"]
                    + " -include "
                    + env.Dir(build_path).abspath
                    + "/"
                    + precompiled_header
                    + ".h"
                )

        return (env, self.get_target(source_path, name, in_bin), sources)

    # helpers for the MSVC project builder (see build_lib)
    @staticmethod
    def simple_lib(inc_ext, src_ext):
        return lambda self, env: (env.Glob("*." + inc_ext), env.Glob("*." + src_ext))

    # TODO using __func__ sucks; if anyone has a better idea, chip in...
    c_lib = simple_lib.__func__("h", "c")
    cpp_lib = simple_lib.__func__("h", "cpp")

    def build_lib(
        self, env, target, sources, msvcproj_glob=None, msvcproj_name=None, shared=False
    ):
        if env["msvcproj"]:
            if msvcproj_glob is None:
                return
            if msvcproj_name is None:
                msvcproj_name = os.path.basename(os.path.dirname(sources[0]))

            glob_inc, glob_src = msvcproj_glob(env)
            # when there's only 1 file, SCons strips directories from the
            # path...
            if len(glob_inc) == 1:
                glob_inc.append(env.File("dummy"))
            if len(glob_src) == 1:
                glob_src.append(env.File("dummy"))

            path = self.msvcproj_path + msvcproj_name + env["MSVSPROJECTSUFFIX"]
            env.Precious(path)

            # we cheat a bit here: only the win32 project will be buildable.
            # this is to avoid simulatenous builds of all the projects at the
            # same time and general mayhem.
            if msvcproj_name == "win32":
                self.msvcproj_projects.insert(
                    0,
                    env.MSVSProject(
                        path,
                        variant=self.msvcproj_variant,
                        auto_build_solution=0,
                        incs=[f.abspath for f in glob_inc],
                        srcs=[f.abspath for f in glob_src],
                        buildtarget=target + env["PROGSUFFIX"],
                    ),
                )
            else:
                self.msvcproj_projects.append(
                    env.MSVSProject(
                        path,
                        variant=self.msvcproj_variant,
                        auto_build_solution=0,
                        incs=[f.abspath for f in glob_inc],
                        srcs=[f.abspath for f in glob_src],
                        MSVSSCONSCOM="",
                    )
                )
            return

        return (
            env.SharedLibrary(target, sources)
            if shared
            else env.StaticLibrary(target, sources)
        )

    def build_program(self, env, target, sources):
        return env.Program(
            target,
            [
                sources,
                self.client,
                self.dwarf,
                self.zlib,
                self.boost,
                self.bzip2,
                self.maxminddb,
                self.miniupnpc,
                self.natpmp,
                self.dwt,
                self.intl,
            ],
        )

    def i18n(self, source_path, buildenv, sources, name):
        if not self.env["i18n"]:
            return

        p_oze = glob.glob("po/*.po")

        potfile = "po/" + name + ".pot"
        buildenv["PACKAGE"] = name
        ret = buildenv.PotBuild(potfile, sources)

        for po_file in p_oze:
            buildenv.Precious(buildenv.PoBuild(po_file, [potfile]))

            lang = os.path.basename(po_file)[:-3]
            locale_path = self.get_target(source_path, "locale/" + lang + "/")

            buildenv.MoBuild(
                locale_path + "LC_MESSAGES/" + name + ".mo",
                po_file,
                NAME_FILE=buildenv.File(locale_path + "name.txt"),
            )

        return ret

    def add_boost(self, env):
        if self.is_win32():
            env.Append(CPPPATH=["#/boost"])

        else:
            boost_libs = ["atomic", "filesystem", "regex", "system", "thread"]
            boost_libs = ["boost_" + lib for lib in boost_libs]
            env.Append(LIBS=boost_libs)

    def add_crashlog(self, env):
        if "mingw" in env["TOOLS"]:
            env.Append(CPPPATH=["#/dwarf"])
            #env.Append(CPPDEFINES=["LIBDWARF_STATIC"]) # todo: msvc
            #env.Append(LIBPATH=["#/dwarf"])
            env.Append(LIBS=["imagehlp"]) # dwarf
        elif "msvc" in env["TOOLS"]:
            env.Append(LIBS=["dbghelp"])
        elif not self.is_win32():
            env.Append(CPPPATH=["#/dwarf"])

    def add_dcpp(self, env):
        if self.is_win32():
            env.Append(CPPPATH=["#/bzip2"])
        env.Append(CPPPATH=["#/maxminddb", "#/zlib"])

        if self.is_win32():
            env.Append(LIBS=["gdi32", "iphlpapi", "ole32", "ws2_32"])
        else:
            env.Append(LIBS=["bz2", "c", "iconv"])

    def add_intl(self, env):
        if self.is_win32():
            env.Append(CPPPATH=["#/intl"])
        else:
            env.Append(LIBS=["intl"])

    def add_openssl(self, env):
        if self.is_win32():
            env.Append(CPPPATH=["#/openssl/include"])
            openssl_lib = "#/openssl/lib/"
            if env["arch"] != "x86":
                openssl_lib += env["arch"] + "/"
            env.Append(LIBPATH=[openssl_lib])
            env.Append(LIBS=["crypt32"])
        if "msvc" in env["TOOLS"]:
            if env["mode"] == "debug":
                env.Prepend(LIBS=["libssld", "libcryptod"])
            else:
                env.Prepend(LIBS=["libssl", "libcrypto"])
        else:
            env.Prepend(LIBS=["ssl", "crypto"])

    def force_console(self, env):
        if "-mwindows" in env["CCFLAGS"]:
            env["CCFLAGS"].remove("-mwindows")
            env.Append(CCFLAGS=["-mconsole"])

        if "-mwindows" in env["LINKFLAGS"]:
            env["LINKFLAGS"].remove("-mwindows")
            env.Append(LINKFLAGS=["-mconsole"])

        if "/SUBSYSTEM:WINDOWS" in env["LINKFLAGS"]:
            env["LINKFLAGS"].remove("/SUBSYSTEM:WINDOWS")

    # support installs that only have an asciidoc.py file but no executable
    def get_asciidoc(self):
        if "PATHEXT" in self.env["ENV"]:
            pathext = self.env["ENV"]["PATHEXT"] + ";.py"
        else:
            pathext = ""
        asciidoc = self.env.WhereIs("asciidoc3", pathext=pathext)
        if asciidoc is None:
            return None
        if asciidoc[-3:] == ".py":
            if self.env.WhereIs("python") is None:
                return None
            asciidoc = "python " + asciidoc
        return asciidoc


def set_lang_name(target, env):
    lang = os.path.basename(str(target))[:-3]
    file = open(str(target), "rb")
    data = file.read()
    file.close()
    import re

    data = re.sub(
        b'"Language: .*\\\\n"',
        bytes('"Language: ' + lang + '\\\\n"', "utf-8"),
        data,
        1
    )

    file = open(str(target), "wb")
    file.write(data)
    file.close()


def get_po_name(source):
    """ "Rely on the comments at the beginning of the po file to find the
    language name.
    @param source: *one* SCons file node (not a list!) designating the .po
    file.
    """
    import codecs
    import re

    match = re.compile("^# (.+) translation.*", re.I).search(
        codecs.open(str(source), "rb", "utf_8").readline()
    )
    if match:
        name = match.group(1)
        if name != "XXX":
            return name
    return None


def gen_po_name(source, env):
    """
    @param source: *one* SCons file node (not a list!) designating the .po
    file.
    @param env: must contain "NAME_FILE", which is a SCons file node to the
    target file.
    """
    import codecs

    name = get_po_name(source)
    if name:
        codecs.open(str(env["NAME_FILE"]), "wb", "utf_8").write(name)


def CheckPKGConfig(context, version):
    context.Message("Checking for pkg-config... ")
    ret = context.TryAction("pkg-config --atleast-pkgconfig-version=%s" % version)[0]
    context.Result(ret)
    return ret


def CheckPKG(context, name):
    context.Message("Checking for %s... " % name)
    ret = context.TryAction('pkg-config --exists "%s"' % name)[0]
    if ret:
        context.env.ParseConfig('pkg-config --cflags --libs "%s"' % name)

    context.Result(ret)
    return ret


def nixify(path):
    return path.replace("\\", "/")


def array_remove(array, to_remove):
    if to_remove in array:
        array.remove(to_remove)


def get_lcid(lang):
    from locale import windows_locale

    lang = lang.replace("-", "_")

    # look for an exact match
    for identifier, name in windows_locale.items():
        if name == lang:
            return identifier

    # ignore the "sub-language" part
    lang = lang.split("_")[0]
    for identifier, name in windows_locale.items():
        if name.split("_")[0] == lang:
            return identifier

    return 0x0409  # default: en-US


def get_win_cp(lcid):
    import ctypes

    LOCALE_IDEFAULTANSICODEPAGE = 0x1004
    LOCALE_RETURN_NUMBER = 0x20000000

    buf = ctypes.c_int()
    kern32 = None

    if sys.platform == "cygwin":
        kern32 = ctypes.CDLL("kernel32.dll", use_errno=True)
    else:
        kern32 = ctypes.windll.kernel32

    if not kern32:
        print("Error: build_util/get_win_cp failed to load kernel32")
        return "cp1252"

    kern32.GetLocaleInfoA(
        lcid, LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE, ctypes.byref(buf), 6
    )

    if buf.value <= 0:
        print("Error: build_util/get_win_cp failed to get locale info")
        return "cp1252"

    return "cp" + str(buf.value)


def html_to_rtf(string):
    # escape chars: \, {, }
    # <br/> -> \line + remove surrounding spaces + append a space
    # remove double new lines + remove new lines at beginning and at end
    # <b>...</b> -> {\b ...}
    # <i>...</i> -> {\i ...}
    # <u>...</u> -> {\ul ...}
    import re

    line = r"\\line "
    return re.sub(  # noqa
        "<([bi])>",
        r"{\\\1 ",
        re.sub(
            "</[biu]>",
            "}",
            re.sub(
                "^(" + line + ")",
                "",
                re.sub(
                    "(" + line + ")$",
                    "",
                    re.sub(
                        "(" + line + ")+",
                        line,
                        re.sub(
                            r"\s*<br ?/?>\s*",
                            line,
                            string.replace("\\", "\\\\")
                            .replace("{", "\\{")
                            .replace("}", "\\}"),
                        ),
                    ),
                ),
            ),
        ),
    ).replace("<u>", "{\\ul ")


def msvcproj_workarounds(target, source, env):
    f = open(str(target[0]), "rb")
    contents = f.read()
    f.close()

    import re

    # clean up empty commands for non-built projects to avoid build failures.
    contents = re.sub(
        br"echo Starting SCons &amp;&amp;\s*(-c)?\s*&quot;&quot;", b"", contents
    )

    # remove SConstruct from included files since it points nowhere anyway.
    contents = re.sub(
        br'<ItemGroup>\s*<None Include="SConstruct" />\s*</ItemGroup>', b"", contents
    )

    # update the platform toolset to the VS 2022 one.
    # TODO remove when SCons adds this.
    contents = contents.replace(
        b"<UseOfMfc>false</UseOfMfc>",
        b"<UseOfMfc>false</UseOfMfc>\r\n\t\t"
        b"<PlatformToolset>v143</PlatformToolset>",
    )

    f = open(str(target[0]), "wb")
    f.write(contents)
    f.close()
