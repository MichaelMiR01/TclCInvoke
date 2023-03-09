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

if {[info exists dllloaded]==0} {
    load ./cinvoke_tclcmd[info sharedlibextension]
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}

typedef int TclOk
typedef ptr Tcl_Obj*
typedef ptr Tcl_Interp*

set Tcl_ListObjGetElements  [ffidl::stubsymbol tcl stubs 45]; #Tcl_ListObjGetElements
ci function $Tcl_ListObjGetElements Tcl_ListObjGetElements TclOk {interp tclobj ptr.int tclobj}
#int (*tcl_ListObjGetElements) (Tcl_Interp *interp, Tcl_Obj *listPtr, int *objcPtr, Tcl_Obj ***objvPtr); /* 45 */
set Tcl_CreateObjCommand    [ffidl::stubsymbol tcl stubs 96]; #Tcl_CreateObjCommand
ci function $Tcl_CreateObjCommand Tcl_CreateObjCommand ptr {interp string  tclproc clientdata ptr}
#Tcl_Command (*tcl_CreateObjCommand) (Tcl_Interp *interp, const char *cmdName, Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *deleteProc); /* 96 */
set Tcl_GetString    [ffidl::stubsymbol tcl stubs 340]; #Tcl_GetString
ci function $Tcl_GetString Tcl_GetString string ptr
#    char * (*tcl_GetString) (Tcl_Obj *objPtr); /* 340 */
ci function [ffidl::stubsymbol tcl stubs 129] Tcl_Eval TclOk {Tcl_Interp* string}
#/* 129 */ EXTERN int		Tcl_Eval(Tcl_Interp *interp, const char *script);

ci function test_hellodata test_hellodata "" "string"
ci function test1 test1 "" ""
ci function test2 test2 "int" "int"
ci function test2 tcltest2 "int" "int"
ci function test3 test3 "ptr" ""
ci function test4 test4 "float" ""
ci function test5 test5 "double" ""
ci function test6 test6 "longlong" "int longlong float double char" 11 22 33.500000 44.500000 [scan 5 %c]
ci function test12 test12 "int" "cs"
ci function test13 test13 "ptr.cs" {int int}
ci function test14 test14 "int" "ptr.int" 
ci function test15 test15 "int" "ptr.cs2" 
ci function test16 test16 string string 
ci function test17 test17 string ptr
ci function test19 test19 int withdata
ci function test20 test20 int {bytevar int}

puts [tcltest2 22]
#create a struct to pass as clientdata
CStruct clientdata {
    int a
    int b
}
typedef struct clientdata 

# make a new one for use as global _cdata ptr
    CType _ctype0 int 123
    puts [_ctype0 get]

namespace eval test {
    puts [CType _ctype int 234]
    puts [_ctype get]

proc tcl_command {cdata interp objc objv} {
    puts "Getting arguments from $cdata $objc $objv"

    # get the clientdata 
    CType _cdata clientdata
    _cdata setptr $cdata mem_none
    puts "got [_cdata get a] and [_cdata get b] from clientdata"
    # takeover the objv arguments
    CType objarr tclobj 
    CType testest int
    objarr setptr $objv mem_none
    objarr length $objc
    puts "len: [objarr length]"
    # iterate over the arguments   
    for {set i 0} {$i<$objc} {incr i} {
        puts "Got arg $i [objarr get $i]"
    }
    incr i -1
        myparray [objarr get $i]
        
    _cdata set a [expr [_cdata get a]+1]
    _cdata set b [expr [_cdata get b]-1]
    # cleanup
    return 0
}

proc myparray {ea} { 
      upvar $ea a 
      puts "-------"
      parray a; # or any other manipulation on $a(..)
      puts "-------"
 }


proc tcl_command_raw {cdata interp objc objv} {
    puts "Getting arguments from $cdata $interp $objc $objv"

    # get the clientdata 
    CType _cdata clientdata
    _cdata setptr $cdata mem_none
    puts "got [_cdata get a] and [_cdata get b] from clientdata"
    # takeover the objv arguments
    CType objarr Tcl_Obj*
    objarr setptr $objv mem_none
    objarr length $objc
    puts "len: [objarr length]"
    # iterate over the arguments   
    for {set i 0} {$i<$objc} {incr i} {
        puts "Got arg $i [objarr get $i]"
        puts "Has text [Tcl_GetString [objarr get $i]]"
    }
    Tcl_Eval $interp [list puts "ready with all objc [objarr get 0]"]
    _cdata set a [expr [_cdata get a]+1]
    _cdata set b [expr [_cdata get b]-1]
    # cleanup
    return 0
}

 
CType cd1 clientdata
puts "Setting values"
cd1 set a 10
cd1 set b 20
#create a callback from tclproc with the classic tcl_objcommand signature
puts "Creating Callback"
CCallback tclcommand tcl_command int {clientdata ptr int ptr}
CCallback tclcommandraw tcl_command_raw int {clientdata Tcl_Interp* int Tcl_Obj*}
puts "Calling callback"
# create tcl obj command with tclproc as callback and a cstruct as clientdata
Tcl_CreateObjCommand  "." "test_tclcommand" [tclcommand getptr] [cd1 getptr] NULL
Tcl_CreateObjCommand  "." "test_tclcommand_raw" [tclcommandraw getptr] [cd1 getptr] NULL


# and test ist
set i 0
set ::a(0) 1
set ::a(1) 2

timetest {
    test_tclcommand $i "*2=" [expr $i*2] {list: l1 l2 l3} ::a
    incr i
} 1
timetest {
    test_tclcommand_raw $i "*2=" [expr $i*2] {list: l1 l2 l3} ::a
    incr i
} 1
fpause
puts "Clientdata:"
puts "[cd1 get a] [cd1 get b]"

rename cd1 ""
rename tclcommand ""

#rename ci ""
#return


pause

CType strarr string() 4 {abc def ghi jkl}
puts [strarr get]

CDATA arrtype 1024
for {set i 0} {$i<1024} {incr i} {
    arrtype poke $i [expr $i%255]
}

set mybyteptr [arrtype get]
puts "before var: $mybyteptr"
puts [test20 mybyteptr 1024]
puts "after var: $mybyteptr"

if {[catch {arrtype poke 0 0} e ] } {puts $e}
#puts [arrtype peek 0]

for {set i 0} {$i<1024} {incr i} {
    arrtype poke $i [expr $i%255]
}

for {set i 0} {$i<1024} {incr i} {
    set s [arrtype peek $i]
    if {$s!= [expr $i%255]} {
        puts "failed getting value from $i $s!= [expr $i%255]"
        update
    }
}

puts "after: [arrtype get]"

set adr [ci getadr test1]

for {set i 0} {$i<1000} {incr i} {
    CType intarr int() 4 {1 2 3 4}
} 
puts [intarr get]

CType strarr string() 4 {abc def ghi jkl} 
puts [strarr get]



CType realint int 45
puts "realint: [realint get]"
puts "realint ptr [realint getptr]"
puts "realint ptr* [realint getptr*]"

CType realstring string "fünfundvierzig"

CType realstring char* "fünfundvierzig"
puts "realstring: [realstring get]"
puts "realstring ptr [realstring getptr]"
puts "realstring ptr* [realstring getptr*]"

test17 [realstring getptr]
puts "realstring: [realstring get]"
puts "realstring ptr [realstring getptr]"
puts "realstring ptr* [realstring getptr*]"

CType realstring char* "fünfundvierzig2"
puts "realstring: [realstring get]"
puts "realstring ptr [realstring getptr]"
puts "realstring ptr* [realstring getptr*]"

pause


#ci linkvar hello_data tclhellodata TCL_LINK_STRING
CType hellodata string "Test for Ctype string"
set tclhellodata "another new text"
puts [test_hellodata "test"]
puts "1hello_data is $tclhellodata"
puts [test_hellodata "succesfully tested"]
puts [test_hellodata "succesfully tested"]
puts "2hello_data is $tclhellodata"
set tclhellodata "another new text"
puts "3hello_data is $tclhellodata"
puts [test_hellodata "succesfully tested"]
puts "4ready.."
pause
CStruct cs {
    char cc
    int i 
    long il
    longlong ill
    float tf
    double td
    ptr pp
}
typedef struct cs
pause
CStruct cs2 {
    int i2
    int ooo(4)
    string ostr
    string strarr(5)
}
typedef struct cs2

CType cs cs
CType csxy cs
CType cs2 cs2
puts [cs info]
puts [cs2 info]


set ptr_cs [csxy getptr]

puts "ptr to csxy: $ptr_cs"

cs set cc 21
cs set i 12

cs set il 123
cs set ill 1234

cs set tf 1.1
cs set td 1.11

cs2 set i2 65
cs2 set ooo 0 10
cs2 set ooo 1 11
cs2 set ooo 2 12
cs2 set ooo 3 13

cs2 set ooo {123 124 125}

cs2 set strarr {abc def ghi alp }

puts "geting array"
puts "get Array ooo [cs2 get ooo]"
puts "get Array 0 [cs2 get ooo 0]"
puts "get Array 1 [cs2 get ooo 1]"
puts "get Array 2 [cs2 get ooo 2]"
puts "get Array 3 [cs2 get ooo 3]"

puts "get Array strarr [cs2 get strarr]" 

cs2 set ostr "This is a string"
puts "get string back [cs2 get ostr]"

puts [cs get cc]
puts [cs get i]
puts [cs get il]
puts [cs get ill]

puts [cs get tf]
puts [cs get td]

puts [cs set pp $adr]
puts "[cs get pp] == $adr"

puts [test12 $ptr_cs]

puts [test2 22]
puts [test1]
puts [test3]
puts [test5]

puts [test4]

puts [test6  11 22 33.500000 44.500000 [scan 5 %c]]

puts [test12 $ptr_cs]

set p_newcs [test13  12 13]
puts "is this a taged pointer (x0^cs) $p_newcs"
puts [test12 $p_newcs]
cs setptr $p_newcs

puts "cs->cc [cs get cc]"
puts "cs->i [cs get i]"
puts "cs->il [cs get il]"
puts "cs->ill [cs get ill]"

puts "cs->float [cs get tf]"
puts "cs->double [cs get td]"

CType mytype int 123
puts "Ptr of mytype: [mytype getptr]"
mytype set 22
puts [mytype get]
puts [test14 [mytype getptr]]
puts [mytype get]

puts [test15 [cs2 getptr]]

puts "get Array [cs2 get ooo]"
puts "get Array 0 [cs2 get ooo 0]"
puts "get Array 1 [cs2 get ooo 1]"
puts "get Array 2 [cs2 get ooo 2]"
puts "get Array 3 [cs2 get ooo 3]"


CType stringtype char* "Dies ist ein String!"
puts "Getting String [stringtype get]"

CType arrtype bytearray "123456789012345678901234567890"
puts "Getting String [arrtype get]"

puts [test16 "Dies ist ein Test"]
puts [test17 [stringtype getptr]] 

puts "from param: [stringtype get]"

CDATA arrtype 1024
puts "before: [arrtype get]"

if {[catch {arrtype poke 0 0} e ] } {puts $e}
#puts [arrtype peek 0]

for {set i 0} {$i<1024} {incr i} {
    arrtype poke $i [expr $i%255]
}

for {set i 0} {$i<1024} {incr i} {
    set s [arrtype peek $i]
    if {$s!= [expr $i%255]} {
        puts "failed getting value from $i $s!= [expr $i%255]"
        update
    }
}


puts "after: [arrtype get]"

CStruct withdata {
    ptr data
    int size
}
typedef struct withdata
fpause
CType withdata withdata
set arrptr [arrtype getptr*]
withdata set data $arrptr
withdata set size 1024

set sptr [withdata getptr]
puts [test19 $sptr]
fpause
puts "after2: [arrtype get]"

puts "Testing Aditional type"
puts "...size_t is [typedef size_t] [sizeof size_t]"
puts "...WPARAM is [typedef WPARAM] [sizeof WPARAM]"

if {[typedef WPARAM] eq ""} {
    puts "No WPARAM"
} else {
    CType WPARAM WPARAM 1234
    puts [WPARAM get]
    CType CHAR CHAR 165
    puts [CHAR get]
    puts "sizeof CHAR [sizeof CHAR]"
}
CType char char 165
puts "char of 165 is -91==[char get]"
CType uchar uchar 165
puts "uchar of 165 is 165==[uchar get]"

puts "Testing typedef"
typedef char testchar
typedef int testint
typedef uchar testuchar
typedef testuchar testuchar2

CType char testchar 165
puts "char of 165 is -91==[char get]"
CType uchar testuchar2 165
puts "uchar of 165 is 165==[uchar get]"


set formatStr {%15s%15s}
set tdefs [lsort [typedef]]
foreach typ $tdefs {
    puts [format $formatStr $typ [sizeof $typ]]
}

}

puts "Exiting"
#CCleanup
puts "OK"



