#!/bin/sh
TCLCONFIGPATH=/dev/null/null

for try_tclsh in "$TCLSH_NATIVE" "$TCLCONFIGPATH/../bin/tclsh" \
                 "$TCLCONFIGPATH/../bin/tclsh8.6" \
                 "$TCLCONFIGPATH/../bin/tclsh8.5" \
                 "$TCLCONFIGPATH/../bin/tclsh8.4" \
                 `which tclsh 2>/dev/null` \
                 `which tclsh8.6 2>/dev/null` \
                 `which tclsh8.5 2>/dev/null` \
                 `which tclsh8.4 2>/dev/null` \
                 tclsh; do
    if test -z "$try_tclsh"; then
        continue
    fi
    if test -x "$try_tclsh"; then
        if echo 'exit 0' | "$try_tclsh" 2>/dev/null >/dev/null; then
            tcl_cv_tclsh_native_path="$try_tclsh"

            break
        fi
    fi
done

if test "$TCLCONFIGPATH" = '/dev/null/null'; then
    unset TCLCONFIGPATH
fi
echo "found tclsh in "
echo $tcl_cv_tclsh_native_path

TCLSH_PROG="${tcl_cv_tclsh_native_path}"


tclConfigCheckDir0="`echo 'puts [tcl::pkgconfig get libdir,runtime]' | "$TCLSH_PROG" 2>/dev/null`"
tclConfigCheckDir1="`echo 'puts [tcl::pkgconfig get scriptdir,runtime]' | "$TCLSH_PROG" 2>/dev/null`"

dirs="/usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64 /usr/lib/tcl8.6/"

for dir in "$tclConfigCheckDir0" "$tclConfigCheckDir1" $dirs; do
    echo "check TCLCONFIGPATH in $dir" 
    if test -f "$dir/tclConfig.sh"; then
        TCLCONFIGPATH="$dir"
        echo "found TCLCONFIGPATH in $dir" 
        break
    fi
done

if test -f "$TCLCONFIGPATH/tclConfig.sh"; then
    . "$TCLCONFIGPATH/tclConfig.sh"
else
    echo "unable to load tclConfig.sh"
fi
TCL_STUB_LIB_SPEC="`eval echo "${TCL_STUB_LIB_SPEC}"`"
LIBS="${LIBS} ${TCL_STUB_LIB_SPEC}"
echo $LIBS


TCL_INCLUDE_SPEC="`eval echo "${TCL_INCLUDE_SPEC}"`"
TCL_EXT1_INLCUDE_SPEC=${TCL_INCLUDE_SPEC}/tcl-private
TCL_EXT2_INLCUDE_SPEC=${TCL_INCLUDE_SPEC}/tcl-private/generic
TCL_EXT3_INLCUDE_SPEC=${TCL_INCLUDE_SPEC}/tcl-private/unix
CPPFLAGS="${CFLAGS} -O2 -fPIC -shared -s -D_GNU_SOURCE -DHAVE_TCL_H -DUSE_TCL_STUBS ${TCL_EXT1_INLCUDE_SPEC} ${TCL_EXT2_INLCUDE_SPEC} ${TCL_EXT3_INLCUDE_SPEC}"

export TCC4TCL_CFLAGS="${CPPFLAGS}"
export TCC4TCL_LIBS="${LIBS}"
export TCC4TCL_CONF=OK
echo $TCC4TCL_CFLAGS
