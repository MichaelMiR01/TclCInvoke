# Package Index for cinvoke_tclcmd

proc cinvoke_loadlib {dir packagename} {
     if {[catch {load [file join $dir $packagename[info sharedlibextension]]} err]} {
         puts "Couldn't load $packagename: $err"
     }
     if {[catch {source [file join $dir tclcinvoke_helper.tcl]} err]} {
         puts "Couldn't load tclcinvoke_helper: $err"
     }
     
}

package ifneeded tclcinvoke 1.0 [list cinvoke_loadlib $dir {cinvoke_tclcmd}]
