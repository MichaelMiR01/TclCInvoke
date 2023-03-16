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
lappend auto_path .
package require tclcinvoke

if {[info exists dllloaded]==0} {
    CInvoke msvcrt[info sharedlibextension] CINV_CC_CDECL ci
    set dllloaded 1 
}

ci function atol tcl_atol long {string}
ci function  strtol tcl_strtol long {ptr.char* ptr.char* int} long 
ci function  strtoul tcl_strtoul ulong {ptr.char* ptr.char* int} 

puts "Atol 12abcde [tcl_atol 12abcde]"

CType str_end char* ""
CType strin char* -12abcde
puts "strtol [strin getptr*] -12abcde --> [tcl_strtol [strin getptr*] [str_end getptr] 10] and  [str_end get]"
CType str_new char*
puts "str_end ptr to [str_end getptr*] value [str_end get]"

puts "Funny enough, strtoul (unsigned long!!!) read a negative value from the string"
puts "as a result and since TCL cannot easily create an unsignd long obj... the resulting value in TCL is a negativ as it can be"
puts "so, no way to unsign int, long and longlong types here... sigh"

puts "strtoul [strin getptr*] -12abcde --> [tcl_strtoul [strin getptr*] [str_end getptr] 10] and  [str_end get]"


#
