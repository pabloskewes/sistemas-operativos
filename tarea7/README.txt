Este ejemplo es una adaptacion del tutorial incluido
(archivo "device drivers tutorial.pdf") y bajado de:
http://www.freesoftwaremagazine.com/articles/drivers_linux

---

Guia rapida:

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
$ make
...
$ ls
... cdown.ko ...

+ Instalacion (en modo root)

# mknod /dev/cdown c 61 0
# chmod a+rw /dev/cdown
# insmod cdown.ko
# dmesg | tail
...
[...........] Inserting cdown module
#

+ Testing (en modo usuario preferentemente)

Ud. necesitara crear varios shells independientes.  Luego
siga las instrucciones del enunciado de la tarea.

+ Desinstalar el modulo

# rmmod cdown.ko
#
