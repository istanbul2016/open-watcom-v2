#pmake: build serv remote os_win cpu_i86 cpu_386 tcp x86
host_cpu = i86
host_os  = win

target_cpu = x86
target_os  = win

srv = tcp
which = SERVER
gui = 1
remote_link = 1

extra_cppflags = -DGUISERVER

extra_linker = lib winsock.lib

!include $(trap_dir)/master.mif
