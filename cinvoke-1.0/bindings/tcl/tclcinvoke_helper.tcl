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

set ::valid_types {
    char* char* string char* 
    char char short short int int long long longlong "long long" 
    uchar "unsigned char" ushort "unsigned short" uint "unsigned int" ulong "unsigned long" ulonglong "unsigned long long"
}

proc validatetype {typename} {
    if {[dict exists $::valid_types $typename]} {
        return [dict get $::valid_types $typename]
    }
    return $typename;
}

proc CStruct2struct {struct {pad "   "} {output ""} {sname ""}} {
    # convert a CStruct to a compilable struct
    
    if {$struct==""} {
        return $output;
    }
    foreach {key name} $struct {
        set key [string trim $key]
        set name [string trim $name]
        
        if {[lsearch "struct union" $key]>-1} {
            append output "$pad$key {" "\n"
            set output [CStruct2struct $name "$pad   " $output] 
            append output "$pad} $sname ;" "\n"
        } else {
            set lname [split $name (] 
            set name [lindex $lname 0]
            set arrsize 0
            set name [string map ". _" $name]
            set key [validatetype $key]
            if {[llength $lname]>1} {
                set arrsize [lindex [split [lindex $lname 1] )] 0]
                append output "${pad}$key $name\[$arrsize\];" "\n"
            } else {
                append output "${pad}$key $name;" "\n"
            }
        }
    }
    return $output;
}

proc CreateRandStruct {{num 10} {pad ""} {name ""}} {
    # creates a random struct for testing purposes
    set mlen [expr round(rand()*$num)]
    if {$mlen==0} {
        set mlen 1
    }
    set type "struct"
    if {$pad!=""} {
        if {[expr round(rand()*10)]>5} {
            set type "union"
        }
    }
    set struct "${pad}${type} \{\n"
    for {set i 0} {$i<$mlen} {incr i} {
        set substr [expr round(rand()*10)]
        if {$substr>7} {
            # substruct
            append struct  [CreateRandStruct [expr $num/3] "$pad   " "sb${i}_${name}"] "\n"
        }
        set rmem [expr round(rand()*([llength $::valid_types]-1)/2)*2]
        set membertype [lindex $::valid_types $rmem]
        set membername mem_${name}$i
        append struct "   $pad$membertype $membername\n"
    }
    return "$struct\n$pad\}"
}
