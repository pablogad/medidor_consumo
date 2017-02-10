# medidor_consumo
A program to read the electricity used @ home getting data from a Envi-R meter plugged into an USB input

The readings are stored to a sqlite3 database contained in a single file, and they are also presented in a VFD display from an old DVD which is connected to the GPIO pins of a Raspberry PI computer.

Dependencies of the project:

  sqlite3
  libpcrecpp
  libbcm2835
  libvfd (at github.com/pablogad/libvfd)
  
