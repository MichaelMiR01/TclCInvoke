# some helpers for TclCInvoke
proc dump_struct {str {struct ""} {pad ""}} {
    if {$struct==""} {
        set struct [$str struct]
    }
    foreach {key name} $struct {
        set key [string trim $key]
        set name [string trim $name]
        set name [lindex [split $name (] 0]
        if {[lsearch "struct union" $key]>-1} {
            puts "$pad$key {"
            dump_struct $str $name "$pad\t"
            puts "$pad}"
        } else {
            puts "${pad}$key\t\t$name: \t\t[$str get $name]"
        }
    }
}

proc PointerUnwrap {optr} {
    if {$optr eq ""} {
        return 0;
    }
    set hasTag [string first ^ $optr]
    set tag ""
    set ptr 0
    if {$hasTag>0} {
        set tag [string range $optr $hasTag+1 end]
        scan [string range $optr 0 $hasTag-1] %x decimalptr
        set ptr $decimalptr
    }
    return [list $ptr $tag]
}
proc PointerPtr {optr} {
    return [lindex [PointerUnwrap $optr] 0]
}
proc PointerTag {optr} {
    return [lindex [PointerUnwrap $optr] 1]
}
proc PointerWrap {ptr tag} {
    set pad [expr [sizeof size_t]*2]
    set ptr [format "0x%0${pad}x" $ptr]
    return "$ptr^$tag"
}
