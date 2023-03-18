#!/usr/bin/tclsh

catch {console show}
proc pause {{message "Hit Enter to continue ==> "}} {
    #return
    puts -nonewline $message
    flush stdout
    gets stdin
}

lappend auto_path .
package require tclcinvoke

if {[info exists dllloaded]==0} {
    CInvoke ./lib[info sharedlibextension] ci
    set dllloaded 1 
}
proc timetest {code times} {
    puts "starting [string range $code 0 20]"
    update
    puts [time {uplevel 0 $code} [expr $times*1]]
    puts ready
    update
}

proc photo'rect {im x0 y0 x1 y1 args} {
    array set "" {-fill black -outline black}
    array set "" $args
    $im put $(-fill) -to $x0 $y0 $x1 $y1
    if {$(-outline) ne $(-fill)} {
       $im put $(-outline) -to $x0 $y0 [expr {$x0+1}] $y1
       $im put $(-outline) -to [expr {$x1-1}] $y0 $x1 $y1
       $im put $(-outline) -to $x0 [expr {$y0+1}] $x1 $y0
       $im put $(-outline) -to $x0 [expr {$y1-1}] $x1 $y1
    }
} 

proc idata_get {img} {
    set w [$img cget -width]
    set h [$img cget -height]
    set data ""
    for {set y 0} {$y<$h} {incr y} {
        for {set x 0} {$x<$w} {incr x} {
            set raw [$img get $x $y]
            set bin [binary format c4 [list {*}$raw 255]]
            append data $bin
        }
    }
    return $data    
        
}
proc idata_set {img data} {
    #
    set w [$img cget -width]
    set h [$img cget -height]
    for {set y 0} {$y<$h} {incr y} {
        for {set x 0} {$x<$w} {incr x} {
            set p [expr {($y*$w+$x)*4}]
            set bin [string range $data $p $p+2]
            set val "#[binary encode hex $bin]"
            $img put $val -to $x $y
        }
    }
}

package require Tk

#toplevel .iv
#wm title .iv "Image"
# draw the histogram
set c .c
canvas $c -width 400 -height 400
pack $c

set img [image create photo -width 30 -height 30]
photo'rect $img 0 0 30 30 -width 5 -color blue -fill green
photo'rect $img 5 5 25 25 -width 5 -color blue -fill yellow

$c create image 1 1 -image $img
update

typedef ptr CDATA

set idata [idata_get $img]
#idata_set $img $idata

CDATA imgdata [expr 30*30*4]
imgdata set $idata
#return
set imgptr [imgdata getptr*]

ci function testimg testimg int "CDATA int int"

timetest {
 set i [testimg $::imgptr 30 30]
 idata_set $::img  [imgdata get]
 update
} 10

#$img put "#001122" -to 0 0

if {0} {
    # c part to test against
    # should be in lib.c --> lib.dll/.so
    # inverts image pixel
    DLLEXPORT int testimg(char* instring, int width, int height) {
        int pos;
        for (int x=0;x<width;x++) {
            for (int y=0;y<height;y++) {
                pos=(x+y*width)*4;
                instring[pos]=255-instring[pos];
                instring[pos+1]=255-instring[pos+1];
                instring[pos+2]=255-instring[pos+2];            
            }
        }
        return 0;
    }
}    

# define a type alias for Tk_PhotoHandle
CType Tk_PhotoHandle ptr

# define a structure for Tk_PhotoImageBlock
CStruct Tk_PhotoImageBlock { 
    CDATA  pixelPtr
    int width
    int height
    int pitch
    int pixelSize
    int red
    int green
    int blue
    int reserved
}
typedef struct Tk_PhotoImageBlock
typedef int TclOK

# bind to tk
#ffidl::callout ffidl-find-photo {pointer pointer-utf8} Tk_PhotoHandle 
set Tk_FindPhoto [cinv::stubsymbol tk stubs 64]; #Tk_FindPhoto
ci function $Tk_FindPhoto Tk_FindPhoto ptr.pblockh {interp string}
#ffidl::callout ffidl-photo-put-block {Tk_PhotoHandle pointer-byte int int int int} void 
set Tk_PhotoPutBlock [cinv::stubsymbol tk stubs 266]; #Tk_PhotoPutBlock
ci function $Tk_PhotoPutBlock Tk_PhotoPutBlock "" {interp ptr.pblockh Tk_PhotoImageBlock int int int int}
#ffidl::callout ffidl-photo-put-zoomed-block {Tk_PhotoHandle pointer-byte int int int int int int int int} void 
set Tk_PhotoPutZoomedBlock [cinv::stubsymbol tk stubs 267]; #Tk_PhotoPutZoomedBlock
ci function $Tk_PhotoPutZoomedBlock Tk_PhotoPutZoomedBlock "" {interp ptr.pblockh Tk_PhotoImageBlock int int int int int int int int}
#ffidl::callout ffidl-photo-get-image {Tk_PhotoHandle pointer-var} int 
set Tk_PhotoGetImage    [cinv::stubsymbol tk stubs 146]; #Tk_PhotoGetImage
ci function $Tk_PhotoGetImage Tk_PhotoGetImage TclOK {ptr.pblockh Tk_PhotoImageBlock}
#ffidl::callout ffidl-photo-blank {Tk_PhotoHandle} void 
set Tk_PhotoBlank    [cinv::stubsymbol tk stubs 147]; #Tk_PhotoBlank
ci function $Tk_PhotoBlank Tk_PhotoBlank "" {ptr.pblockh}
#ffidl::callout ffidl-photo-expand {Tk_PhotoHandle int int} void 
set Tk_PhotoExpand    [cinv::stubsymbol tk stubs 148]; #Tk_PhotoExpand
ci function $Tk_PhotoExpand Tk_PhotoExpand "" {interp ptr.pblockh int int}
#ffidl::callout ffidl-photo-get-size {Tk_PhotoHandle pointer-var pointer-var} void 
set Tk_PhotoGetSize    [cinv::stubsymbol tk stubs 149]; #Tk_PhotoGetSize
ci function $Tk_PhotoGetSize Tk_PhotoGetSize "" {ptr.pblockh ptr.int ptr.int}
#ffidl::callout ffidl-photo-set-size {Tk_PhotoHandle int int} void 
set Tk_PhotoSetSize    [cinv::stubsymbol tk stubs 150]; #Tk_PhotoSetSize
ci function $Tk_PhotoSetSize Tk_PhotoSetSize "" {ptr.pblockh int int}

proc ffidl-photo-get-block-bytes {block} {
    #puts "pixelPtr [$block get pixelPtr]"
    set nbytes [expr {[$block get height]*[$block get pitch]}]
    CDATA bytes $nbytes
    bytes memcpy << [$block get pixelPtr]
    CDATA bytes2 $nbytes
    bytes2 setptr [$block get pixelPtr] mem_none
    if {[bytes get] ne [bytes2 get]} {
        puts "Error, got different data blocks"
    }
    return [bytes get]
}

puts "$Tk_FindPhoto --> $img"
pause
set PBlockH [Tk_FindPhoto . $img]

puts "ok lets see what we got"
puts $PBlockH
puts "# calling Tk_PhotoGetSize "
# calling Tk_PhotoGetSize
CType pwidth int 0
CType pheight int 0
Tk_PhotoGetSize $PBlockH [pwidth getptr] [pheight getptr]
puts "Size: [pwidth get] [pheight get]"

puts "# calling Tk_PhotoGetImage returning a strutct Tk_PhotoImageBlock "
# calling Tk_PhotoGetSize

CType bl1 Tk_PhotoImageBlock
Tk_PhotoGetImage $PBlockH [bl1 getptr]

dump_struct bl1
pause


timetest {
    ::ffidl-photo-get-block-bytes bl1
} 10


CDATA imgraw [expr 30*30*4]
imgraw set [idata_get $img]

set idata [idata_get $img]
set idata2 [imgraw get]

if {($idata2 ne [bytes get])||($idata ne $idata2)} {
    puts "Error: bytearrays not identical"
    puts $idata2
    puts "----------------------"
    puts [bytes get]
} else {
    puts "Checking imgdata successful"
    #idata_set $::img  [imgdata get]
    #idata_set $::img  [bytes get]
}


#puts [ffidl-photo-get-block-bytes bl1]

#set i [testimg $::imgptr 30 30]
#idata_set $::img  [imgdata get]

CType bl2 Tk_PhotoImageBlock
bl2 set width 30
bl2 set height 30
bl2 set pitch  120
bl2 set pixelSize 4
bl2 set red 0
bl2 set green 1
bl2 set blue 2
bl2 set reserved 3
bl2 set pixelPtr [bytes getptr*]
Tk_PhotoBlank $PBlockH
update
pause
testimg [bytes getptr*] 30 30
Tk_PhotoPutBlock . $PBlockH [bl2 getptr] 0 0 30 30 1
update
pause
timetest {
testimg [bl2 get pixelPtr] 30 30
Tk_PhotoPutBlock . $::PBlockH [bl2 getptr] 0 0 30 30 1
update 
} 10
pause
CCleanup
return


#exit

