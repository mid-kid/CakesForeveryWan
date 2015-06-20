CakesFW
=======

This is a homebrew for the 3ds that helps with patching the NATIVE\_FIRM, thus allowing for it to do things it wasn't supposed to do.

Building
--------

You need 3 things: Python 2.x, devkitARM, and armips.
Make sure the cross compilers (arm-none-eabi-\*), armips and python are in your $PATH.
On windows you might have to edit dir\_top in the Makefile to point to your current directory.
If everything's set up, just run make.


Thanks
------

* b1|1s for the POC based on a decompilation of rxTools, porting it to spider and a lot of help when creating this. He's the guy who reverse-engineered all the info I needed for this.
* Roxas75 for the font and firmlaunchax on MSET (rxTools)
* YifanLu and dukesrg for Spider3DSTools
* naehrwert for p3ds
* The KARL and OSKA guys for providing example code and firmware offsets (bootstrap)
* 3dbrew for info
* Yellows8 for firmlaunchax and memchunkhax
* smea and Yellows8 for gspwn
* Apache Thunder for being a fearless test-kanninchen.
* Gateway for some offsets
