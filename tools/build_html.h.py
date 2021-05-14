# Copyright (c) 2021 SQ9MDD Rysiek Labus
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT


def build_h(input_file,output_file,charname):
    with open(input_file, 'r') as f:
        input1 = f.read()
    output = "const char " + charname + "[] PROGMEM = R\"=====(\n"  + input1 + "\n)=====\";"
    with open(output_file, "w") as myfile:
        myfile.write(output)
    return()

build_h('html/index.html','include/html.h','HTTP_HTML')
build_h('html/settings_wifi.html','include/settings_wifi.h','HTTP_WIFI')
build_h('html/settings.html','include/settings.h','HTTP_SETTINGS')