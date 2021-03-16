#!/bin/python2

import re
import sys 
import os 
import __future__

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


PatInst=re.compile( r"instret: +(?P<Instret>\d+)")
PatL1I =re.compile( r"L1I_read,     L1I_readmiss\n(?P<L1IReadAcc>\d+) +(?P<L1IReadMiss>\d+)")
PatL1D =re.compile( r"L1D_read,     L1D_readmiss,     L1D_write,     L1D_writemiss,     L1D_writeback\n(?P<L1DReadAcc>\d+) +(?P<L1DReadMiss>\d+) +(?P<L1DWriteAcc>\d+) +(?P<L1DWriteMiss>\d+) +(?P<L1DWriteBack>\d+)")
PatL2  =re.compile( r"L2_read,      L2_readmiss,      L2_write,      L2_writeback\n(?P<L2ReadAcc>\d+) +(?P<L2ReadMiss>\d+) +(?P<L2WriteAcc>\d+) +(?P<L2WriteMiss>\d+)")
PatTT  =re.compile( r"TC_readTT,   TC_readTTmiss,    TC_writeTT,   TC_writeTTmiss,    TC_writeTTback\n(?P<TTReadAcc>\d+) +(?P<TTReadMiss>\d+) +(?P<TTWriteAcc>\d+) +(?P<TTWriteMiss>\d+) +(?P<TTWriteBack>\d+)")
PatTM0 =re.compile( r"TC_readTM0,  TC_readTM0miss,   TC_writeTM0,  TC_writeTM0miss,   TC_writeTM0back\n(?P<TM0ReadAcc>\d+) +(?P<TM0ReadMiss>\d+) +(?P<TM0WriteAcc>\d+) +(?P<TM0WriteMiss>\d+) +(?P<TM0WriteBack>\d+)")
PatTM1 =re.compile( r"TC_readTM1,  TC_readTM1miss,   TC_writeTM1,  TC_writeTM1miss,   TC_writeTM1back\n(?P<TM1ReadAcc>\d+) +(?P<TM1ReadMiss>\d+) +(?P<TM1WriteAcc>\d+) +(?P<TM1WriteMiss>\d+) +(?P<TM1WriteBack>\d+)")

PatFname = re.compile( r"ref(?P<InputNum>\d+).log")
PatFnCons= re.compile( r"ref\d+.log")
titlestr = "No,Bench,instret,L1I_read,L1I_readmiss,L1D_read,L1D_readmiss,L1D_write,L1D_writemiss,L1D_writeback,L2_read,L2_readmiss,L2_write,L2_writeback,TC_readTT,TC_readTTmiss,TC_writeTT,TC_writeTTmiss,TC_writeTTback,TC_readTM0,TC_readTM0miss,TC_writeTM0,TC_writeTM0miss,TC_writeTM0back,TC_readTM1,TC_readTM1miss,TC_writeTM1,TC_writeTM1miss,TC_writeTM1back,Command"
title = titlestr.split(',')

# global vars: g_stat output
g_stat = []
output = "stat.txt"

def readFile(filename):
  f_info = PatFname.match(filename).groupdict();
  # print(f_info)
  
  with open(filename) as file:
    content = file.read()
    f_Inst_stat = PatInst.findall(content)
    # print (f_Inst_stat)
    f_L1I_stat = PatL1I.findall(content)
    # print (f_L1I_stat)
    f_L1D_stat = PatL1D.findall(content)
    # print (f_L1D_stat)
    f_L2_stat = PatL2.findall(content)
    # print (f_L2_stat)
    f_TT_stat = PatTT.findall(content)
    # print (f_TT_stat)
    f_TM0_stat = PatTM0.findall(content)
    # print (f_TM0_stat)
    f_TM1_stat = PatTM1.findall(content)
    # print (f_TM1_stat)

    i = int(f_info['InputNum'])
    f_cmd = input[i]
    token = f_cmd.split()[0]
    if (token == "sphinx_livepretend"):
        token = "sphinx"
    elif (token == "Xalan"):
        token = "xalancbmk"

    d = 0
    f_benchname = ""
    for id, folder in enumerate(dir) :
        if(folder.endswith(token)):
            d = id
            f_benchname = folder.split('/')[-1]
            break

    # print (f_cmd )
    # print (d) 
    # print (f_benchname)
    retval = [str(i),f_benchname]
    retval.extend(f_Inst_stat)
    retval.extend(f_L1I_stat[0])
    retval.extend(f_L1D_stat[0])
    retval.extend(f_L2_stat[0])
    retval.extend(f_TT_stat[0])
    retval.extend(f_TM0_stat[0])
    retval.extend(f_TM1_stat[0])
    retval.append(f_cmd)

    # print(retval)
    return retval 

# define a function dealing with file lists.
def stat ():
    g_stat.append(title)
    files = [ f for f in os.listdir(os.getcwd()) if os.path.isfile(f) if PatFnCons.match(os.path.basename(f))]
    # print (files)
    for f in files :
        g_stat.append(readFile(f))


#Main statements
stat()
# print(g_stat)

if len(sys.argv) == 2 :
    output = sys.argv[1];

with open(output,"w+") as fout:
    for line in g_stat:
        print ("\t".join(line))
        fout.write("\t".join(line)+"\n")

print ("Done!")
