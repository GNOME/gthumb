dnl
dnl check for working fnmatch(3)
dnl
AC_DEFUN(FUNC_FNMATCH,
[AC_MSG_CHECKING(for working fnmatch with FNM_CASEFOLD)
AC_CACHE_VAL(fr_cv_func_fnmatch,
[rm -f conftestdata; > conftestdata
AC_TRY_RUN([#include <fnmatch.h>
main() { exit(fnmatch("/*/bin/echo *", "/usr/bin/echo just a test", FNM_CASEFOLD)); }
], fr_cv_func_fnmatch=yes, fr_cv_func_fnmatch=no,
  fr_cv_func_fnmatch=no)
rm -f core core.* *.core])dnl
AC_MSG_RESULT($fr_cv_func_fnmatch)
if test $fr_cv_func_fnmatch = yes; then
  [$1]
else
  [$2]
fi
])
