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
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}
set Tcl_ListObjGetElements  [ffidl::stubsymbol tcl stubs 45]; #Tcl_ListObjGetElements
#int (*tcl_ListObjGetElements) (Tcl_Interp *interp, Tcl_Obj *listPtr, int *objcPtr, Tcl_Obj ***objvPtr); /* 45 */
set Tcl_CreateObjCommand    [ffidl::stubsymbol tcl stubs 96]; #Tcl_CreateObjCommand
#Tcl_Command (*tcl_CreateObjCommand) (Tcl_Interp *interp, const char *cmdName, Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *deleteProc); /* 96 */
set Tcl_GetString    [ffidl::stubsymbol tcl stubs 340]; #Tcl_GetString
#    char * (*tcl_GetString) (Tcl_Obj *objPtr); /* 340 */
ci function test14 test14 "int" "ptr.int"
set hellodata [ci getadr hello_data]

puts "Testing getadr"
puts "hellodata $hellodata"
puts "Casting to string  [PointerCast $hellodata string]"

CType test string
test setptr [PointerCast $hellodata string] mem_none
puts "Getting [test get]"
rename test ""

puts "Testing with pointer match (int -- int)"
CType mytype int 123
puts "Ptr of mytype: [mytype getptr]"
mytype set 22
puts [mytype get]
puts [ci __call test14 "int" "ptr.int" [mytype getptr]]
puts [mytype get]
#rename mytype ""

puts "Testing with pointer match implicit conversion(int -- ptr)"
CType mytype int 123
puts "Ptr of mytype: [mytype getptr]"
mytype set 22
puts [mytype get]
puts [ci __call test14 "int" "ptr" [mytype getptr]]
puts [mytype get]
#rename mytype ""

puts "Testing error handling with pointer mismatch"
CType wrongtype long 123
puts "Ptr of wrongtype: [wrongtype getptr]"
wrongtype set 22
puts [wrongtype get]
if {[catch {
    puts [ci __call test14 "int" "ptr.int" [wrongtype getptr]]
} e ]} {
    puts "Error: $e"
}
puts [wrongtype get]


if {[catch {
    puts [test14 [wrongtype getptr]]
} e ]} {
    puts "Error: $e"
}


CCleanup