#/usr/local/bin/
/usr/bin/avrdude -F -P usb -p x32a4 -c stk600 -U flash:w:MachZehnder.hex
