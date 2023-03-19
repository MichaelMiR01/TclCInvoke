#!/usr/bin/tclsh
catch {console show}
proc pause {{message "Hit Enter to continue ==> "}} {
    return
    puts -nonewline $message
    flush stdout
    gets stdin
    
}

proc timetest {code times} {
    puts "starting [string range $code 0 20]"
    puts [time {uplevel 0 $code} [expr $times*10]]
    puts ready
}

lappend auto_path .
package require tclcinvoke
if {[info exists dllloaded]==0} {
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}

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
CStruct cs2 {
    int i2
    int ooo(5)
    string strarr(5)
    string ostr
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
typedef struct cs2

CType cs cs
CType cs2 cs2

puts [cs info]
puts [cs2 info]
puts "size: [cs size]"
puts "size: [cs2 size]"

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

cs2 set s.a 99
cs2 set s.b 999
cs2 set us.i3 13

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

puts [cs2 get s.a ]
puts [cs2 get s.b ]
puts [cs2 set u.i 55 ]
cs2 set a 123

set ptr_cs [cs2 getptr]

puts "ptr to cs2: $ptr_cs"

puts [ci __call test18 "int" "ptr" $::ptr_cs]



puts "Get s.a [cs2 get s.a ]"
puts "Get s.b [cs2 get s.b ]"

puts [cs get cc]
puts [cs get i]
puts [cs get il]
puts [cs get ill]

puts [cs get tf]
puts [cs get td]

set adr [ci getadr test1]
puts [cs set pp $adr]
puts "[cs get pp] == $adr"

set ptr_cs [cs getptr]

puts "ptr to cs: $ptr_cs"

ci __call test12 "int" "ptr" $::ptr_cs

puts "[cs getptr] == $ptr_cs"

#timetest {ci __call test12 "int" "ptr" $::ptr_cs} 1000

puts "cs->cc [cs get cc]"
puts "cs->i [cs get i]"
puts "cs->il [cs get il]"
puts "cs->ill [cs get ill]"

puts "cs->float [cs get tf]"
puts "cs->double [cs get td]"
puts "testing memcpy of struct "

CType cs2b cs2
cs2b memcpy << [cs2 getptr]

set ptr_cs [cs2 getptr]
puts "ptr to cs2: $ptr_cs"

puts [ci __call test18 "int" "ptr" $::ptr_cs]
dump_struct cs2
puts "Get s.a [cs2 get s.a ]"
puts "Get s.b [cs2 get s.b ]"

set ptr_cs [cs2b getptr]
puts "ptr to cs2b: $ptr_cs"

puts [ci __call test18 "int" "ptr" $::ptr_cs]
puts "Get s.a [cs2b get s.a ]"
puts "Get s.b [cs2b get s.b ]"

puts "setting 55 "
cs2b set s.a 55
cs2b set s.b 555

set ptr_cs [cs2b getptr]
puts "ptr to cs2b: $ptr_cs"

puts "Get s.a [cs2b get s.a ]"
puts "Get s.b [cs2b get s.b ]"

puts "and copy back to [cs2 getptr]"
cs2b memcpy >> [cs2 getptr]

puts "Get s.a [cs2 get s.a ]"
puts "Get s.b [cs2 get s.b ]"
dump_struct cs2
