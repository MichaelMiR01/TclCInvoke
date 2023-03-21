#!/usr/bin/tclsh
catch {console show}
proc pause {{message "Hit Enter to continue ==> "}} {
    #return
    puts -nonewline $message
    flush stdout
    gets stdin
    
}

proc timetest {code times} {
    puts "starting [string range $code 0 20]"
    puts [time {uplevel 0 $code} [expr $times*1]]
    puts ready
}

lappend auto_path .
package require tclcinvoke
if {[info exists dllloaded]==0} {
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}

CStruct deflen {
    int x
    int y
}

CStruct 3dxyz {
    deflen len
    int x
    int y
    int z
}
typedef struct 3dxyz
CType cs 3dxyz

timetest {
CStruct 3dmatrix {
    int i
    3dxyz vec01
    3dxyz vec02
    3dxyz vec03
}
typedef struct 3dmatrix

CType cs2 3dmatrix


#puts [cs info]
#puts [cs2 info]

cs2 set i 10
cs2 set vec01.x 11
cs2 set vec01.y 21
cs2 set vec01.z 31
cs2 set vec02.x 12
cs2 set vec02.y 22
cs2 set vec02.z 32
cs2 set vec03.x 13
cs2 set vec03.y 23
cs2 set vec03.z 33

#puts [cs2 get vec01.x]
#puts [cs2 get vec01.y]
#puts [cs2 get vec01.z]
} 1000

puts [cs info]
puts [cs2 info]

dump_struct cs2

puts [cs2 get vec01.x]
puts [cs2 get vec01.y]
puts [cs2 get vec01.z]
