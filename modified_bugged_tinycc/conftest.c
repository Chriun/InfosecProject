/* ----------------------------------------------------------------------- */
/* with -D C2STR: convert tccdefs.h to C-strings */

#if C2STR

#include <stdio.h>
#include <string.h>

/* replace native host macros by compile-time versions */
const char *platform_macros[] = {
    "__i386__",             "TCC_TARGET_I386",
    "__x86_64__",           "TCC_TARGET_X86_64",
    "_WIN32",               "TCC_TARGET_PE",
    "__arm__",              "TCC_TARGET_ARM",
    "__ARM_EABI__",         "TCC_ARM_EABI",
    "__aarch64__",          "TCC_TARGET_ARM64",
    "__riscv",              "TCC_TARGET_RISCV64",
    "__APPLE__",            "TCC_TARGET_MACHO",
    "__FreeBSD__",          "TARGETOS_FreeBSD",
    "__FreeBSD_kernel__",   "TARGETOS_FreeBSD_kernel",
    "__OpenBSD__",          "TARGETOS_OpenBSD",
    "__NetBSD__",           "TARGETOS_NetBSD",
    "__linux__",            "TARGETOS_Linux",
    "__ANDROID__",          "TARGETOS_ANDROID",

    "__SIZEOF_POINTER__",   "PTR_SIZE",
    "__SIZEOF_LONG__",      "LONG_SIZE",
    0
};

int isid(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_';
}

int isspc(int c)
{
    return (unsigned char)c <= ' ' && c != 0;
}

int main(int argc, char **argv)
{
    char l[1000], l2[1000], *p, *q, *p0;
    FILE *fp, *op;
    int c, e, f, s, cmt, cmt_n;
    const char *r;

    if (argc < 3)
        return 1;

    fp = fopen(argv[1], "rb");
    op = fopen(argv[2], "wb");
    if (!fp || !op) {
        fprintf(stderr, "c2str: file error\n");
        return 1;
    }

    cmt = cmt_n = 0;
    for (;;) {
        p = l;
    append:
        if (fgets(p, sizeof l - (p - l), fp)) {
            p = strchr(p, 0);
            while (p > l && isspc(p[-1]))
                --p;
            *p = 0;
        } else if (p == l)
            break;

        /* check for continuation */
        if (p > l && p[-1] == '\\') {
            p[-1] = ' ';
            goto append;
        }

        /* count & skip leading spaces */
        p = l, q = l2, f = 0;
        while (*p && isspc(*p))
            ++p, ++f;

        /* handle comments */
        if (p[0] == '/' && cmt == 0) {
            if (p[1] == '*')
                cmt = 2;
            if (p[1] == '/')
                cmt = 1;
        }
        if (cmt) {
            fprintf(op, "%s", l);
            if (++cmt_n == 1)
                fprintf(op, " (converted, do not edit this file)");
            fprintf(op, "\n");
            if (cmt == 1)
                cmt = 0;
            if (cmt == 2) {
                p = strchr(l, 0);
                if (p >= l + 2 && p[-1] == '/' && p[-2] == '*')
                    cmt = 0;
            }
            continue;
        }

        if (f < 4) {
            do {
                /* replace machine/os macros by compile-time counterparts */
                for (e = f = 0; (r = platform_macros[f]); f += 2) {
                    c = strlen(r);
                    /* remove 'defined' */
                    //e = memcmp(p, "defined ", 8) ? 0 : 8;
                    if (0 == memcmp(p + e, r, c)) {
                        p += e + c;
                        q = strchr(strcpy(q, platform_macros[f + 1]), 0);
                        break;
                    }

                }
                if (r)
                    continue;
            } while (!!(*q++ = *p++));
            /* output as is */
            fprintf(op, "%s\n", l2);
            continue;

        } else {
            s = e = f = 0, p0 = p;
            for (;;) {
                c = *p++;

                if (isspc(c)) {
                    s = 1;
                    continue;
                }
                if (c == '/' && (p[0] == '/' || p[0] == '*'))
                    c = 0; /* trailing comment detected */
                else if (s && q > l2
                    && ((isid(q[-1]) && isid(c))
                        // keep space after macro name
                        || (q >= l2 + 2
                            && l2[0] == '#'
                            && l2[1] == 'd'
                            && f < 2 && !e
                            )))
                    *q++ = ' ', ++f;
                s = 0;

                if (c == '(')
                    ++e;
                if (c == ')')
                    --e;
                if (c == '\\' || c == '\"')
                    *q++ = '\\';
                *q++ = c;
                if (c == 0)
                    break;
                p0 = p;
            }
            /* output with quotes */
            fprintf(op, "    \"%s\\n\"%s\n", l2, p0);
        }
    }

    fclose(fp);
    fclose(op);
    return 0;
}

/* ----------------------------------------------------------------------- */
/* get some information from the host compiler for configure */

#elif 1

#include <stdio.h>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
int _CRT_glob = 0;
#endif

/* Define architecture */
#if defined(__i386__) || defined _M_IX86
# define TRIPLET_ARCH "i386"
#elif defined(__x86_64__) || defined _M_AMD64
# define TRIPLET_ARCH "x86_64"
#elif defined(__arm__)
# define TRIPLET_ARCH "arm"
#elif defined(__aarch64__)
# define TRIPLET_ARCH "aarch64"
#elif defined(__riscv) && defined(__LP64__)
# define TRIPLET_ARCH "riscv64"
#else
# define TRIPLET_ARCH "unknown"
#endif

/* Define OS */
#if defined (__linux__)
# define TRIPLET_OS "linux"
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
# define TRIPLET_OS "kfreebsd"
#elif defined(__NetBSD__)
# define TRIPLET_OS "netbsd"
#elif defined(__OpenBSD__)
# define TRIPLET_OS "openbsd"
#elif defined(_WIN32)
# define TRIPLET_OS "win32"
#elif defined(__APPLE__)
# define TRIPLET_OS "darwin"
#elif !defined (__GNU__)
# define TRIPLET_OS "unknown"
#endif

#if defined __ANDROID__
# define ABI_PREFIX "android"
#else
# define ABI_PREFIX "gnu"
#endif

/* Define calling convention and ABI */
#if defined (__ARM_EABI__)
# if defined (__ARM_PCS_VFP)
#  define TRIPLET_ABI ABI_PREFIX"eabihf"
# else
#  define TRIPLET_ABI ABI_PREFIX"eabi"
# endif
#else
# define TRIPLET_ABI ABI_PREFIX
#endif

#if defined _WIN32
# define TRIPLET TRIPLET_ARCH "-" TRIPLET_OS
#elif defined __GNU__
# define TRIPLET TRIPLET_ARCH "-" TRIPLET_ABI
#else
# define TRIPLET TRIPLET_ARCH "-" TRIPLET_OS "-" TRIPLET_ABI
#endif

int main(int argc, char *argv[])
{
#if defined(_WIN32)
    _setmode(_fileno(stdout), _O_BINARY);  /* don't translate \n to \r\n */
#endif
    switch(argc == 2 ? argv[1][0] : 0) {
        case 'b'://igendian
        {
            volatile unsigned foo = 0x01234567;
            puts(*(unsigned char*)&foo == 0x67 ? "no" : "yes");
            break;
        }
#if defined(__clang__)
        case 'm'://inor
            printf("%d\n", __clang_minor__);
            break;
        case 'v'://ersion
            printf("%d\n", __clang_major__);
            break;
#elif defined(__TINYC__)
        case 'v'://ersion
            puts("0");
            break;
        case 'm'://inor
            printf("%d\n", __TINYC__);
            break;
#elif defined(_MSC_VER)
        case 'v'://ersion
            puts("0");
            break;
        case 'm'://inor
            printf("%d\n", _MSC_VER);
            break;
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
        /* GNU comes last as other compilers may add 'GNU' compatibility */
        case 'm'://inor
            printf("%d\n", __GNUC_MINOR__);
            break;
        case 'v'://ersion
            printf("%d\n", __GNUC__);
            break;
#else
        case 'm'://inor
        case 'v'://ersion
            puts("0");
            break;
#endif
        case 't'://riplet
            puts(TRIPLET);
            break;
        case 'c'://ompiler
#if defined(__clang__)
            puts("clang");
#elif defined(__TINYC__)
            puts("tcc");
#elif defined(_MSC_VER)
            puts("msvc");
#elif defined(__GNUC__)
            puts("gcc");
#else
            puts("unknown");
#endif
            break;
        default:
            break;
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
#endif
