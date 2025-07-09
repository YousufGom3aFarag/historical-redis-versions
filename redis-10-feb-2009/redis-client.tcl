#!/usr/bin/tclsh

proc str2repr o {
    format "s%d:%s" [string length $o] $o
}

proc list2repr o {
    tcllist2repr [split [string range $o 1 end-1] ,]
}

proc tcllist2repr l {
    set res {}
    append res "l[llength $l]:"
    foreach x $l {
        append res [obj2repr $x]
    }
    return $res
}

proc obj2repr o {
    if {[string index $o 0] eq "("} {
        list2repr $o
    } else {
        str2repr $o
    }
}

proc sendquery {s argv} {
    set query [tcllist2repr $argv]
    set query [string length $query]:$query
    puts -nonewline $s $query
    puts $query
    flush $s
}

proc readreply s {
    set len {}
    while 1 {
        set char [read $s 1]
        if {$char eq {:}} break
        append len $char
        continue
    }
    set reply [read $s $len]
    return $len:$reply
}

set s [socket 127.0.0.1 9665]
for {set i 0} {$i < 100000} {incr i} {
    sendquery $s [list set $i foobar]
#    puts [readreply $s]
}
puts "Deleting..."
#read stdin
for {set i 0} {$i < 100000} {incr i} {
    sendquery $s [list delete $i]
#    puts [readreply $s]
}
#sendquery $s $argv
#puts [readreply $s]
close $s
