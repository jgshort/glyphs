m4_define([glyphs_major_version],[0])
m4_define([glyphs_minor_version],[1])
m4_define([glyphs_micro_version],[0]) # Keep this in sync with glyphs_crate/Cargo.toml
m4_define([glyphs_version],[glyphs_major_version.glyphs_minor_version.glyphs_micro_version])

AC_INIT([glyphs], m4_esyscmd_s(echo $(git describe)$(git status --porcelain | awk '{if ($1 == "M") {print "-wip";exit}}')), [53501+jgshort@users.noreply.github.com])

AC_CANONICAL_HOST

WINDOWS_FLAGS=""
OSX_FRAMEWORKS=""
case "${host_os}" in
	cygwin*|mingw*)
		WINDOWS_FLAGS="-lmingw32 -lSDL2main"
		;;
  darwin*)
    # Required by SDL2 Rust library
    OSX_FRAMEWORKS="-framework Security"
    ;;
esac

AC_CONFIG_MACRO_DIR([autoconf-archive/m4])
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CC_STDC

AM_PROG_AR
LT_INIT

AC_SUBST([AM_CFLAGS], ["-std=c17 -v -g -O -Wextra -Wfloat-equal -Wunused -Wall -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wunreachable-code -Wno-unused-function -Wno-cpp -Werror -pedantic -pedantic-errors -Waggregate-return -Wcast-qual -Wchar-subscripts -Wcomment -Wconversion -Wdisabled-optimization -Wformat -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k -Wimplicit -Wimport -Winit-self -Winline -Winvalid-pch -Wmissing-braces -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wpacked -Wpadded -Wparentheses -Wredundant-decls -Wreturn-type -Wsequence-point -Wsign-compare -Wstack-protector -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch -Wswitch-default -Wswitch-enum -Wtrigraphs -Wuninitialized -Wunknown-pragmas -Wunreachable-code -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wvariadic-macros -Wvolatile-register-var -Wwrite-strings ${WINDOWS_FLAGS}"])

glyphs_API_VERSION=0.1.0
glyphs_API_MAJOR_VERSION=0
glyphs_API_MINOR_VERSION=1

AC_SUBST([glyphs_API_VERSION])
AC_SUBST([glyphs_API_MAJOR_VERSION])
AC_SUBST([glyphs_API_MINOR_VERSION])
AC_SUBST([glyphs_API_VERSION_U],[AS_TR_SH([$glyphs_API_VERSION])])

# ===========================================================================

AC_SUBST([glyphs_LT_VERSION_INFO],[glyphs_lt_version_info])
AC_SUBST([LIBglyphs_MAJOR_VERSION],[glyphs_major_version])
AC_SUBST([LIBglyphs_MINOR_VERSION],[glyphs_minor_version])
AC_SUBST([LIBglyphs_MICRO_VERSION],[glyphs_micro_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
                 Makefile
                 src/Makefile
                 ])

AC_CHECK_TYPES(long long)

AC_SEARCH_LIBS([SDL_Init], [SDL2], [], [
                AC_MSG_ERROR([unable to find the SDL_Init() function in libSDL2])
                ])
AC_SEARCH_LIBS([IMG_Init], [SDL2_image], [], [
                AC_MSG_ERROR([unable to find the IMG_Init() function in libSDL2_image])
                ])
AC_SEARCH_LIBS([TTF_Init], [SDL2_ttf], [], [
                AC_MSG_ERROR([unable to find the TTF_Init() function in libSDL2_ttf])
                ])
AC_SEARCH_LIBS([fmaxf], [m], [], [
                AC_MSG_ERROR([unable to find the fmaxf() function in libm])
                ])

AC_SUBST(OSX_FRAMEWORKS)
AC_OUTPUT

