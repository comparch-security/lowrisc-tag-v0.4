lowRISC chip v0.4 With an Improve Tag Cache
==============================================

### Improvements:
* Reduce lock granularity to a single bit in TM1 and Tm0.
* Dynamc search order selection.
* Added perfromance monitor.
* various bug fixes.

**Reference**: Wei Song, Da Xie, Zihan Xue, and Peng Liu. [A parallel tag cache for hardware managed tagged memory in multicore processors](https://doi.org/10.1109/TC.2024.3441835). IEEE Transactions on Computers, 2024, 73(11): 2488-2503.

Original README from the lowRISC project
===============================================
The root git repo for lowRISC development and FPGA
demos.

[master] status: [![master build status](https://travis-ci.org/lowRISC/lowrisc-chip.svg?branch=master)](https://travis-ci.org/lowRISC/lowrisc-chip)

[update] status: [![update build status](https://travis-ci.org/lowRISC/lowrisc-chip.svg?branch=update)](https://travis-ci.org/lowRISC/lowrisc-chip)

[dev] status: [![dev build status](https://travis-ci.org/lowRISC/lowrisc-chip.svg?branch=dev)](https://travis-ci.org/lowRISC/lowrisc-chip)

Current version: Release version 0.4 (05-2017) --- lowRISC with tagged memory and minion core

To download the repo:

~~~shell
git clone -b minion-v0.4 --recursive https://github.com/lowrisc/lowrisc-chip.git
~~~

For the previous release:

~~~shell
################
# Version 0.3: lowRISC with a trace debugger (07-2016)
################
git clone -b debug-v0.3 --recursive https://github.com/lowrisc/lowrisc-chip.git

################
# Version 0.2: untethered lowRISC (12-2015)
################
git clone -b untether-v0.2 --recursive https://github.com/lowrisc/lowrisc-chip.git

################
# Version 0.1: tagged memory (04-2015)
################
git clone -b tagged-memory-v0.1 --recursive https://github.com/lowrisc/lowrisc-chip.git
~~~

[traffic statistics](http://www.cl.cam.ac.uk/~ws327/lowrisc_stat/index.html)

[master]: https://github.com/lowrisc/lowrisc-chip/tree/master
[update]: https://github.com/lowrisc/lowrisc-chip/tree/update
[dev]: https://github.com/lowrisc/lowrisc-chip/tree/dev
