# rping.tcl
#
# Extended ping with record route option
# 
# Don't forget to create alias for command:
# (config)# alias exec rping tclsh rping.tcl
#
# Usage: rping dst [src]
#
# johnny ^_^ <johnny@netvor.sk>
# kofolaware (http://netvor.sk/~johnny/kofola-ware)
#

terminal no history

set dst [lindex $argv 0]
set src [lindex $argv 1]

typeahead "$dst\n\n\n\ny\n$src\n\n\n\n\nrecord\n\n\n\n"
puts [ping ip]

terminal history

