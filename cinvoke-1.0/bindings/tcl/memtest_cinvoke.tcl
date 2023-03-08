#!/usr/bin/tclsh

catch {console show}
proc pause {{message "Hit Enter to continue ==> "}} {
    return
    puts -nonewline $message
    flush stdout
    gets stdin
    # tk_messageBox -message "ok?"
}
proc fpause {{message "Hit Enter to continue ==> "}} {
    
    puts -nonewline $message
    flush stdout
    gets stdin
    #tk_messageBox -message "ok?"
}

proc timetest {code times} {
    puts "starting [string range $code 0 20]"
    update
    puts [time {uplevel 0 $code} [expr $times*10]]
    puts ready
    update
}

if {[info exists dllloaded]==0} {
    load ./cinvoke_tclcmd[info sharedlibextension]
    CInvoke "" ci
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}

ci function test1 test1 "" ""
ci function test2 test2 "int" "int"
ci function test2 tcltest2 "int" "int"
ci function test3 test3 "ptr" ""
ci function test4 test4 "float" ""
ci function test5 test5 "double" ""
ci function test6 test6 "longlong" "int longlong float double char" 11 22 33.500000 44.500000 [scan 5 %c]
ci function test9 test9 "" tclproc
ci function test12 test12 "int" "cs"
ci function test13 test13 "ptr.cs" {int int}
ci function test14 test14 "int" "ptr.int" 
ci function test15 test15 "int" "ptr.cs2" 
ci function test16 test16 string string 
ci function test17 test17 string ptr
ci function test19 test19 int withdata
ci function test20 test20 int {bytevar int}
ci function test_hellodata test_hellodata "" "string"


if {0} {
puts "Testing typedef"
typedef  char testchar
typedef  int testint
typedef  uchar testuchar

CType ext_test1 size_t 165
puts "getting [ext_test1 get]==165"
puts "Testing CTypes..."
pause

CType ext_test1 int 165
puts [ext_test1 get]
puts "int [sizeof int]"

CType ext_test1 size_t 165
puts [ext_test1 get]
puts "size_t [sizeof size_t]"

CType ext_test1 time_t 165
puts [ext_test1 get]
puts "time_t [sizeof time_t]"

CType ext_test1 uchar 165
puts [ext_test1 get]
puts "uchar [sizeof uchar]"

CType ext_test1 ushort 165
puts [ext_test1 get]
puts "ushort [sizeof ushort]"

CType ext_test1 uint 165
puts [ext_test1 get]
puts "uint [sizeof uint]"

CType ext_test1 ulong 165
puts [ext_test1 get]
puts "ulong [sizeof ulong]"

CType ext_test1 ulonglong 165
puts [ext_test1 get]
puts "ulonglong [sizeof ulonglong]"

CType ext_test1 ptr 165
puts [ext_test1 get]
puts "ptr [sizeof ptr]"

CType ext_test1 int8_t 165
puts [ext_test1 get]
puts "int8_t [sizeof int8_t]"
puts "overflow to -91==[ext_test1 get]"

CType ext_test1 uint8_t 165
puts [ext_test1 get]
puts "uint8_t [sizeof uint8_t]"

CType ext_test1 int16_t 165
puts [ext_test1 get]
puts "int16_t [sizeof int16_t]"

CType ext_test1 int32_t 165
puts [ext_test1 get]
puts "int32_t [sizeof int32_t]"

CType ext_test1 int64_t 165
puts [ext_test1 get]
puts "int64_t [sizeof int64_t]"


rename ext_test1 ""
pause
}



CStruct cs2_ {
    int i2
    int ooo(4)
    string ostr
    string strarr(5)
}

CStruct cs2 {
    int i2
    int ooo(4)
    string ostr
    string strarr(5)
    struct {
        string strarr1(25)
        string strarr2(25)
    }
}
typedef struct cs2


CType csi2 cs2
puts "length [string length [csi2 struct]]"

timetest {
CType csi2 cs2
csi2 set strarr {abc def ghi alp }
csi2 set strarr1 {jzt wer tz qwe tze}
csi2 set strarr2 {2jzt 2wer 2tz 2qwe 2tze}

} 10
fpause
#CCleanup
#return







puts "Testing normal function"
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

CType cs cs

pause
set ptr_cs [cs getptr]

timetest {test12 $::ptr_cs} 10
pause
timetest {test2 22} 10
pause


timetest {test1} 10
pause

timetest {test3} 10
pause

puts "test4 float [test4]"

timetest {test5} 10
pause

timetest {test6  11 22 33.500000 44.500000 [scan 5 %c]} 10
pause

timetest {test12 $::ptr_cs} 10
pause


if {1} {
proc tclproc {a b c} {
    #puts "$a"
    #puts "$b"
    #puts "$c"
    return 11.5
}
proc tclproc2 {a b c} {
    puts "2 $a"
    puts "2 $b"
    puts "2 $c"
    return 11.6
}
CCallback testtcl tclproc "float" {int char int}
CCallback testtcl tclproc "float" {int char int}
CCallback testtcl tclproc "float" {int char int}
puts [timetest {CCallback testtcl tclproc "float" {int char int}} 1]
puts "info on [testtcl info]"
puts "getptr on [testtcl getptr]"

test9 [testtcl getptr]
pause
puts "Testing callback multiple times"

puts [timetest {test9 [testtcl getptr]} 10]
pause
puts "New callback function"
CCallback testtcl2 tclproc2 "float" {int char int}
puts "info on [testtcl2 info]"
puts "getptr on [testtcl2 getptr]"
pause

test9 [testtcl2 getptr]


pause

}

set adr [ci getadr test1]
puts "Testing CTypes..."

puts [timetest {CType hellodata string "Test for Ctype string"} 10]
puts [timetest {hellodata set "Test for Ctype string [clock microseconds]"} 10]
puts [hellodata get]

puts [timetest {
    CType intarr int() 4 {1 2 3 4}
} 10]
puts [intarr get]

puts [timetest {
    CType strarr string() 4 {abc def ghi jkl}
} 10]
puts [strarr get]

#testing for handover into c
# this should work like a single char**
puts "Calling test17 with strarr"
CType strarr char*() 4 {abc def ghi jkl}
puts [test17 [strarr getptr]] 

puts [strarr get]

pause

test_hellodata "3 werwer"
test_hellodata [hellodata get]

set tclhellodata "2 another new text"
timetest {test_hellodata "3 werwer"} 10
timetest {test_hellodata "4 succesfully tested"} 10
timetest {test_hellodata "6 succesfully tested"} 10

CType hellodata string "Test for Ctype string"

puts "hellodata string is [hellodata get]"

puts "Testing struct"
pause
timetest {
CStruct cs {
    char cc
    int i 
    long il
    longlong ill
    float tf
    double td
    ptr pp
} 
} 10
puts "Info on cs: \n[cs info] \n[string length [cs info]]"

puts "Testing set struct member"
pause
timetest {cs set cc 21} 10
pause

puts "Testing struct with array"
pause

timetest {
CStruct cs2 {
    int i2
    int ooo(4)
    string ostr
    string strarr(5)
    union { struct {
        string strarr1(5)
        string strarr2(5)
    } }
}
typedef struct cs2
CType cs2 cs2
cs2 set strarr {abc def ghi alp }
cs2 set strarr1 {jzt wer tz qwe tze}
cs2 set strarr2 {2jzt 2wer 2tz 2qwe 2tze}

} 10

puts "Testing struct with substruct"
pause

timetest {
CStruct cs3 {
    int i2
    struct {
        int s.a
        int s.b
        union {
            char u.c
            int u.i
        }
    }
    int a
    int b
    union {
        struct {
            int us.i1
            int us.i2
        }
        struct {
            int us.i3
            char us.c1
        }
    }
}
} 10
typedef struct cs3
CType cs3 cs3

pause
puts [cs info]
puts [cs2 info]

set ptr_cs [cs getptr]

puts "ptr to cs: $ptr_cs"
puts [test12 $::ptr_cs]

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
puts "Setting int array ok"

timetest {
    cs2 set strarr {abc def ghi alp }
} 10

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
puts "[cs getptr] == $ptr_cs"

pause

timetest {test12 $::ptr_cs} 10
pause
timetest {test2  22} 10
timetest {test1 } 10
timetest {test3 } 10

timetest {test4} 10
timetest {test5} 10
timetest {test6 11 22 33.500000 44.500000 [scan 5 %c]} 10
pause
timetest {test12 $::ptr_cs} 10

pause
set p_newcs [test13 12 13]
puts "ptr is $p_newcs"
puts [test12 $::p_newcs]

cs setptr $p_newcs mem_none

puts "cs->cc [cs get cc]"
puts "cs->i [cs get i]"
puts "cs->il [cs get il]"
puts "cs->ill [cs get ill]"

puts "cs->float [cs get tf]"
puts "cs->double [cs get td]"
pause
timetest {CType ::mytype int 123} 10
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
pause
# this string get's a char* since we want to export it to dll as pointer
# so the dll can modify it
timetest {
    CType stringtype char* "Dies ist ein String!"
} 10
puts "Getting String [stringtype get]"

CType arrtype bytearray "123456789012345678901234567890"
puts "Getting String [arrtype get]"

puts [test16 "Dies ist ein Test"]

# char* test17(char** instring)
puts [test17 [stringtype getptr]] 

puts [stringtype get]


puts "Testing CType with large data"
pause

puts [timetest {
    CType strarr string() 4 {abc def ghi jkl}
} 10]
puts [strarr get]

CDATA arrtype 1024
    
puts "before: [arrtype get]"


timetest {
    CDATA arrtype 1024
} 10
puts "before: [arrtype get]"

if {[catch {arrtype poke 0 0} e ] } {puts $e}

puts "Timing poke 1024" 
for {set i 0} {$i<1024} {incr i} {
    arrtype poke $i [expr $i%255]
}

puts "Timing peek 1024" 
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
CType withdata withdata

set arrptr [arrtype getptr*]
withdata set data $arrptr
withdata set size 1024

set sptr [withdata getptr]

timetest {test19 $::sptr} 10
puts "after2: [arrtype get]"

fpause
CCleanup
# if MEMDEBUG is  compiled, CCleanup will do mem_check and report leaks
fpause
return
