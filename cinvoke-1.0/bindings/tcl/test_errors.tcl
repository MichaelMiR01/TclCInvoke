#!/usr/bin/tclsh

catch {console show}
proc pause {{message "Hit Enter to continue ==> "}} {
    return
    puts -nonewline $message
    flush stdout
    gets stdin
    
}
proc fpause {{message "Hit Enter to continue ==> "}} {
    puts -nonewline $message
    flush stdout
    gets stdin
    
}
proc timetest {code times} {
    puts "starting [string range $code 0 20]"
    update
    puts [time {uplevel 1 $code} [expr $times*1]]
    puts ready
    update
}

proc shield {code} {
    if {[catch {uplevel 1 $code} e] } {
        puts "[string trim $code]\nresulted in Error: $e"
    }
}

lappend auto_path .
package require tclcinvoke

puts "Testing error handling..."
#CType
shield {
    CType
}
shield {
    CType a
}
shield {
    CType a int()
}
shield {
    CType a 123
}
#CStruct
shield {
    CStruct
}
shield {
    CStruct sdfsfsd
}
shield {
    CStruct sdfsfsd ewrwerwe
    typedef struct sdfsfsd
    CType test_sdfsfsd sdfsfsd
}
shield {
    CStruct sdfsfsd {int a struct {}}
    typedef struct sdfsfsd
    CType test_sdfsfsd sdfsfsd
}
shield {
    CStruct sdfsfsd {int a struct {int}}
    typedef struct sdfsfsd
    CType test_sdfsfsd sdfsfsd
}
shield {
    CStruct sdfsfsd {int a struct {intu ab}}
    typedef struct sdfsfsd
    CType test_sdfsfsd sdfsfsd
}
shield {
    CStruct sdfsfsd {int a struct {int a}}
    typedef struct sdfsfsd
    CType test_sdfsfsd sdfsfsd
}
shield {
    typedef struct sdfsfsdwerw
    CType test_sdfsfsd sdfsfsdwerw
}
#CData
shield {
 CDATA
}
shield {
 CDATA a
}
shield {
 CDATA a -2
}






