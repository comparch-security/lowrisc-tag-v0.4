import __future__
# import opensocdebug 
import sys
import os

# if len(sys.argv) < 2:
#     print "Usage: runelf.py <filename>"
#     exit(1)

# elffile = sys.argv[1]

# osd = opensocdebug.Session()

# osd.reset(halt=True)

# for m in osd.get_modules("STM"):
#     m.log("stm{:03x}.log".format(m.get_id()))

# for m in osd.get_modules("CTM"):
#     m.log("ctm{:03x}.log".format(m.get_id()), elffile)

# for m in osd.get_modules("MAM"):
#     m.loadelf(elffile)

# osd.start()
# osd.wait(10)


#define data structure to save dir number and input number.
# exactly same with dir and argstrset defined in bbl.c.
dir = [
    "/0:/400.perlbench",
    "/0:/403.gcc",
    "/0:/429.mcf",
    "/0:/445.gobmk",
    "/0:/456.hmmer",
    "/0:/462.libquantum",
    "/0:/464.h264ref",
    "/0:/471.omnetpp",
    "/0:/473.astar",
    "/0:/483.xalancbmk",
    "/0:/401.bzip2",
    "/0:/458.sjeng",
    "/0:/410.bwaves",
    "/0:/416.gamess",
    "/0:/433.milc",
    "/0:/434.zeusmp",
    "/0:/435.gromacs",
    "/0:/436.cactusADM",
    "/0:/437.leslie3d",
    "/0:/444.namd",
    "/0:/447.dealII",
    "/0:/450.soplex",
    "/0:/453.povray",
    "/0:/454.calculix",
    "/0:/459.GemsFDTD",
    "/0:/465.tonto",
    "/0:/470.lbm",
    "/0:/481.wrf",
    "/0:/482.sphinx3",
    "/0:/ehtest",
    "/0:/openftest",
    "/0:/"
    ]

input = [
      "libquantum 1397 8",
      "mcf inp.in",
      "sjeng ref.txt",
      "Xalan -v t5.xml xalanc.xsl",
      "astar BigLakes2048.cfg",
      "astar rivers.cfg",
      "omnetpp omnetpp.ini",
      "h264ref -d foreman_ref_encoder_baseline.cfg",
      "h264ref -d foreman_ref_encoder_main.cfg",
      "h264ref -d sss_encoder_main.cfg",
      "hmmer nph3.hmm swiss41",
      "hmmer --fixed 0 --mean 500 --num 500000 --sd 350 --seed 0 retro.hmm",
      "bzip2 input.source 280",
      "bzip2 chicken.jpg 30", 
      "bzip2 liberty.jpg 30",
      "bzip2 input.program 280",
      "bzip2 text.html 280",
      "bzip2 input.combined 200",
      "gcc 166.i -o 166.s",
      "gcc 200.i -o 200.s",
      "gcc c-typeck.i -o c-typeck.s",
      "gcc cp-decl.i -o cp-decl.s",
      "gcc expr.i -o expr.s",
      "gcc expr2.i -o expr2.s",
      "gcc g23.i -o g23.s",
      "gcc s04.i -o s04.s",
      "gcc scilab.i -o scilab.s",
      "perlbench -I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1",
      "perlbench -I./lib diffmail.pl 4 800 10 17 19 300",
      "perlbench -I./lib splitmail.pl 1600 12 26 16 4500",
      "gobmk --quiet --mode gtp < 13x13.tst",
      "gobmk --quiet --mode gtp < nngs.tst",
      "gobmk --quiet --mode gtp < score2.tst",
      "gobmk --quiet --mode gtp < trevorc.tst",
      "gobmk --quiet --mode gtp < trevord.tst",
# 35
      "bwaves",
      "gamess < cytosine.2.config",
      "gamess < h2ocu2+.gradient.config",
      "gamess < triazolium.config",
      "milc < su3imp.in",
      "zeusmp", #invalid
      "gromacs -silent -deffnm gromacs -nice 0", #invalid
      "cactusADM benchADM.par",
      "leslie3d < leslie3d.in",
      "namd --input namd.input --iterations 38 --output namd.out",
      "dealII 23",
      "soplex -s1 -e -m45000 pds-50.mps", #invalid
      "soplex -m3500 ref.mps", #invalid
      "povray SPEC-benchmark-ref.ini",
      "calculix -i  hyperviscoplastic",
      "GemsFDTD",
      "tonto", #invalid
      "lbm 3000 reference.dat 0 0 100_100_130_ldc.of",
      "wrf",
      "sphinx_livepretend ctlfile . args.an4" #invalid
    ]



# define main cycle : take an item in input, get its first token(program name), find the name in
# dir, get the ELFDIRNUM d and ELFINPNUM i;
# Then, make system calls to :
#   make clean
#   ELFDIRNUM=d ELFINPNUM=i make 
# After that, we reset, load the ELF object bbl and start run.
# WAIT for a specific period of time. (from early recorded data.)
# by now, the test program should be terminated, and the PFC data is written into a file named 
# "ref%d.log", where %d is the index of the array "input" of the test input.
# Finally, reset, and continue to the next loop.
elffile = "bbl"

# def init_osd() :
#     global osd
#     osd = opensocdebug.Session()
#     osd.reset(halt=True)


def test_route(i):
    token = input[i].split()[0]
    if (token == "sphinx_livepretend"):
        token = "sphinx"
    elif (token == "Xalan"):
        token = "xalancbmk"

    d = 0
    for id, folder in enumerate(dir) :
        if(folder.endswith(token)):
            d = id
            break

    print ("input #{}".format(i))
    print ("dir #{}".format(d))

    #system call for make
    commd = "make clean && ELFDIRNUM={} ELFINPNUM={} make && osd-cli -s startup.script"
    os.system(commd.format(d,i))
    commd2 = "mv xterm.log ref{}.output"
    os.system(commd2.format(i))

    # osd.wait(2)
    # osd.reset(halt=True)

    # osd.wait(2)

    # for m in osd.get_modules("MAM"):
    #     m.loadelf(elffile)
    #     print m

    # osd.start()
    # osd.wait(1800)


def main():
    # init_osd()

    rng = [0,2,3,4,5,6,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,39,43,44,45,48,49,50,52,53]
    sample = [3,39,45,13,19,6,28,31,49,50,53]
    for i in rng:
        test_route(i)

if __name__ == '__main__':
    main()
