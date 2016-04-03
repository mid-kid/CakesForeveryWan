CakesFW
=======

This is homebrew software for the Nintendo 3DS™ that assists with patching the system's NATIVE\_FIRM, thereby allowing it to perform unsanctioned operations.

Building
--------

A total of three (3) components are required: Python 3.x with PyYAML, devkitARM, and armips.  
Ensure that the required cross compilers (arm-none-eabi-\*), armips, and python3 directories are included in your $PATH system variable.  
Once the environment is configured, run the `make` command.  


Thanks
------

* b1|1s for the POC based on a decompilation of rxTools, porting it to spider and a lot of help when creating this. He's the guy who reverse-engineered all the info I needed for this;
* Normmatt for the emuNAND patch;
* Roxas75 for the font and firmlaunchax on MSET (rxTools);
* 3dbrew for info;
* Apache Thunder for being a fearless test-kanninchen;
* Gateway for some offsets.
