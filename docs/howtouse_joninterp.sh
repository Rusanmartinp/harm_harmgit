##################
# shows how to use interpolation routine to take fieldline data and get back interpolation for each quantity desired.

# choose which type of code/system setup
#system=1   # lustre and reduced code on orange
system=2   # ki-rh42

#whichmodel=0 # runlocal
#whichmodel=1 # sashaa9b100t0.6
whichmodel=2 # sasha99t1.5708

if [ $whichmodel -eq 0 ]
then
    modelname="runlocaldipole3fiducial"
fi
if [ $whichmodel -eq 1 ]
then
    modelname="sashaa9b100t0.6"
fi
if [ $whichmodel -eq 2 ]
then
    modelname="sashaa99t1.5708"
fi

#################################################
# setup dirs
if [ $system -eq 1 ]
then
    joninterpcodedir=/lustre/ki/pfs/jmckinne/harmgit_jon2interp/
    basedir=/lustre/ki/pfs/jmckinne/thickdisk7/
    iinterpprogname=~/bin/iinterp.orange.thickdisk7
    bin2txtprogname=~/bin/bin2txt.orange
fi

if [ $system -eq 2 ]
then
    joninterpcodedir=/data/jon/harmgit/
    basedir=`pwd`
    iinterpprogname=`pwd`/iinterp
    bin2txtprogname=`pwd`/bin2txt
fi



###################################################
# 1) make program


cd $joninterpcodedir

if [ $system -eq 1 ]
then
    # setup for reduced code set
    rm -rf init.c init.h
    touch init.h
fi


# 2) ensure PRINTHEADER and SCANHEADER in global.jon_interp.h are correct for older/newer simulations (i.e. THETAROT in new only)
# Do this by setting OLDERHEADER 1 if non-tilted runs.  Else set to 0.

# 3) make program itself (need Intel MKL -- modify makefile if path needs to be changed -- currently setup for ki-rh39)

make superclean ; make prepiinterp ; make iinterp &> make.log

# also make bin2txt program:

make superclean ; make prepbin2txt ; make bin2txt
# check makefile and setup for ki-rh39/orange/etc.

# ensure no errors during compile or link (need lapack!)


######################################################
# 4) copy programs to your path

cp iinterp $iinterpprogname
cp bin2txt $bin2txtprogname

#######################################################
# 5) do interpolation (directly read-in binary fieldline file and output full single file that contains interpolated data)

# CHOOSE correctly:
# 0=NEWER header with 32 entries (tilted sims)
# 1=OLDER header with 30 entries (thickdisk/sasha sims)
# 2=VERYOLD header with 21 entries (runlocaldipole3dfiducial)
OLDERHEADER=0

# REQUIRED FILES:
# 1) ensure dumps contains fieldline files
# 2) local $basedir contains coordparms.dat if different than default.
# 3) dumps contains gdump.bin file


# get 3 times so can compute temporal derivative for (e.g.) current density at same spatial/temporal location as dump

# CHOOSE:
if [ $whichmodel -eq 0 ]
    dumpnum=2000
then
fi
if [ $whichmodel -eq 1 ]
then
    dumpnum=4000
fi
if [ $whichmodel -eq 2 ]
then
    dumpnum=5736
fi



dumpnumm1=$(($dumpnum-1))
if [ -e dumps/fieldline$dumpnumm1.bin ]
then
    dumpnumm1=$(($dumpnum-1))
else
    dumpnumm1=$(($dumpnum))
fi
dumpnump1=$(($dumpnum+1))
if [ -e dumps/fieldline$dumpnump1.bin ]
then
    dumpnump1=$(($dumpnum+1))
else
    dumpnump1=$(($dumpnum))
fi

#
# get times of dumps
time0=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $1}'`
timem1=`head -1 dumps/fieldline$dumpnumm1.bin |awk '{print $1}'`
timep1=`head -1 dumps/fieldline$dumpnump1.bin |awk '{print $1}'`
#
# get original resolution
nt=1
nx=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $2}'`
ny=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $3}'`
nz=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $4}'`
if [ $OLDERHEADER -eq 0 ]
then
    numcolumns=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $32}'`
fi
if [ $OLDERHEADER -eq 1 ]
then
    numcolumns=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $30}'`
fi
if [ $OLDERHEADER -eq 2 ]
then
    numcolumns=11
fi

bhspin=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $13}'`
R0=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $14}'`
Rin=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $15}'`
Rout=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $16}'`
hslope=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $17}'`
defcoord=`head -1 dumps/fieldline$dumpnum.bin |awk '{print $19}'`


######
# COPY correct coordparms.dat to local dir.
alias cp='cp'
cp coordparms.dat.$modelname coordparms.dat
alias cp='cp -i'
######

#
# Note that iinterp has x->xc y->zc z->yc since originally was doing 2D in x-z
# That is:
# So box(n)x refers to true x
# So box(n)y refers to true z
# So box(n)z refers to true y
#
# True x is V5D's x and iinterp's x
# True z is V5D's y and iinterp's y
# True y is V5D's z and iinterp's z
#
#
# Vectors are still in order as columns as vx, vy, vz for true x,y,z, respectively
#

#whichres=0
whichres=1
#whichres=2

# box grid count
boxnt=1
if [ $whichres -eq 0 ]
then
    boxnx=100
    boxny=100
    boxnz=100
fi
if [ $whichres -eq 1 ]
then
    boxnx=256
    boxny=256
    boxnz=256
fi
if [ $whichres -eq 2 ]
then
    boxnx=512
    boxny=512
    boxnz=512
fi

if [ 1 -eq 0 ]
then
    #
    # box size
    boxxl=-40
    boxyl=-40
    boxzl=-40
    boxxh=40
    boxyh=40
    boxzh=40
fi

if [ 1 -eq 1 ]
then
    #
    # box size
    boxxl=-1E3
    boxyl=-1E3
    boxzl=-1E3
    boxxh=1E3
    boxyh=1E3
    boxzh=1E3
fi


# set docurrent=0 if want quick result with no current density
# this will change number of output columns

# CHOOSE:
#docurrent=1
docurrent=0



cd $basedir
IDUMPDIR=$basedir/idumps/
# ensure coordparms.dat exists here -- required to read in harm internal grid parameters
mkdir $IDUMPDIR
#
#
#
if [ $whichmodel -eq 0 ]
then
    gdumpname=./dumps/gdump.bin.runlocaldipole3dfiducial
fi
if [ $whichmodel -eq 1 ]
then
    gdumpname=./dumps/gdump.bin.sashaa9b100t0.6
fi
if [ $whichmodel -eq 2 ]
then
    gdumpname=./dumps/gdump.bin.sashaa99t1.5708
fi


# CHOOSE:
#whichoutput=14
#whichoutput=15
#whichoutput=16
whichoutput=17

if [ $whichoutput -eq 16 ]
then
#    iinterpprogname=${iinterpprogname}16
    iinterpprogname=iinterp16new
fi
if [ $whichoutput -eq 17 ]
then
    iinterpprogname=iinterp17
fi

# non-vis5d
defaultvalue=0
# vis5d
#defaultvalue=4  # causes segfault after using vis5d after creating v5d file


outfilename=$IDUMPDIR/fieldline$dumpnum.cart.bin.boxzh${boxzh}.box${boxnx}x${boxny}x${boxnz}.out${whichoutput}.model${modelname}

$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -headtype $OLDERHEADER -oN $nt $nx $ny $nz -numcolumns $numcolumns -refine 1.0 -filter 0 -grids 1 0 -nN $boxnt $boxnx $boxny $boxnz -ibox $time0 $time0 $boxxl $boxxh $boxyl $boxyh $boxzl $boxzh -coord $Rin $Rout $R0 $hslope -defcoord $defcoord -dofull2pi 1 -docurrent $docurrent -tdata $timem1 $timep1 -extrap 1 -defaultvaluetype $defaultvalue -gdump $gdumpname -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double -infile dumps/fieldline$dumpnum.bin -infilem1 dumps/fieldline$dumpnumm1.bin -infilep1 dumps/fieldline$dumpnump1.bin -outfile $outfilename


# as a test, one can do just 1 variable (the density)
if [ 1 -eq 0 ]
then
    outfilename=$IDUMPDIR/fieldline$dumpnum.cart.bin.densityonly.$boxnx.$boxny.$boxnz
    $iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype 1 -itype 1 -head 1 1 -oN $nt $nx $ny $nz -refine 1.0 -filter 0 -grids 1 0 -nN $boxnt $boxnx $boxny $boxnz -ibox $time0 $time0 $boxxl $boxxh $boxyl $boxyh $boxzl $boxzh -coord $Rin $Rout $R0 $hslope -defcoord $defcoord -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -infile dumps/fieldline$dumpnum.bin -outfile $outfilename
fi


# The whichoutput==14 case results in a file with 1 line text header, line break, then data
# The binary data block of *floats* (4 bytes) is ordered as:
# fastest index: column or quantity list
# next fastest index: i (associated with true x)
# next fastest index: j (associated with true y)
# next fastest index: k (associated with true z)
# slowest index: h (time, but only single time here)

# Note that while array access of quantity is out of order in iinterp result, the 3D spatial grid is still written as a right-handed coordinate system.
# So, if you face the screen showing positive z-axis pointing up (increasing j) and positive x-axis pointing to the right (increasing i), then the positive y-axis points into the screen (increasing k).

# The columns or quantities in the list are ordered as:
# 14 things:
#
# rho0,ug,vx,vy,vz,Bx,By,Bz,FEMrad,Bphi,Jt,Jx,Jy,Jz  (J's only exist if -docurrent 1 was set)
# FEMrad is the radial energy flux
# Bphi is the poloidal enclosed current density
# J\\mu=J^\\mu is the current density.
#
#
# Derived quantities:
#
# 3-velocity magnitude: v^2=vx^2+vy^2+vz^2 (i.e. just spatials are squared)
# Lorentz factor: ut = 1/sqrt(1-v^2)
# 4-velocity: u={ut,ut*vx,ut*vy,ut*vz)
# Lab current squared: J^2=J.J = -Jt*Jt + Jx*Jx + Jy*Jy + Jz*Jz (full space-time square)
# Comoving current density: j^\\nu = J^\\mu h_\\mu^\\nu = J^\\mu (\\delta_\\mu^\\nu + u_\\mu u^\\nu) = J^\\nu + (J^\\mu u_\\mu)u^\\nu
# Comoving square current density: j^2 = j^\\nu j_\\nu = J^2 + (J.u)^2 = J^2 + ut*(Jt + Jx*vx + Jy*vy + Jz*vz)
# field along flow: u.B = ux*Bx + uy*By + uz*Bz (i.e. Bt=0)
# comoving magnetic field: b^\mu = (B^\mu + (u.B)u^\mu)/ut
# comoving mag energy: b^2/2 = 0.5*(B^2 + (u.B)^2)/ut^2



############################################

file=$outfilename
if [ $OLDERHEADER -eq 0 ]
then
    numoutputcols=`head -1 $file |awk '{print $32}'`
fi
if [ $OLDERHEADER -eq 1 ]
then
    numoutputcols=`head -1 $file  |awk '{print $30}'`
fi
if [ $OLDERHEADER -eq 2 ]
then
    numoutputcols=11 # or 14 or 4 or whatever
fi
newnx=`head -1 $file |awk '{print $2}'`
newny=`head -1 $file |awk '{print $3}'`
newnz=`head -1 $file |awk '{print $4}'`




# can check how looks in text by doing:
if [ 1 -eq 0 ]
then
    $bin2txtprogname 1 2 0 -1 3 $newnx $newny $newnz 1 $file $file.txt f $numoutputcols
    less -S $file.txt
    # should look reasonable.
fi




############################################
# can look in vis5d and see how looks

[jon@ki-rh42 v5dfield]$ cat  head.v5d
density 1E-4 1
ug 1E-4 1
negudt 1E-4 1
mu 1E-4 1
uut 1E-4 1
vr 1E-4 1
vh 1E-4 1
vph 1E-4 1
Br 1E-4 1
Bh 1E-4 1
Bp 1E-4 1
0 1 0 1 0 1


# original SPC data cube
# ./bin2txt 1 5 0 1 3 288 128 128 1 head.v5d fieldline4000.bin fieldline4000.v5d f 11

# interpolated Cartesian data cube
# ./bin2txt 1 5 0 1 3 $boxnx $boxny $boxnz 1 head.v5d $outfilename $outfilename.v5d f 14


[jon@ki-rh42 v5dfield]$ cat  head14.v5d
density 1E-4 1
ug 1E-4 1
vx 1E-4 1
vy 1E-4 1
vz 1E-4 1
Bx 1E-4 1
By 1E-4 1
Bz 1E-4 1
FEMrad 1E-4 1
Bphi 1E-4 1
Jt 1E-4 1
Jx 1E-4 1
Jy 1E-4 1
Jz 1E-4 1
0 1 0 1 0 1

[jon@ki-rh42 v5dfield]$ cat  head4.v5d
density 1E-4 1
ug 1E-4 1
uu0 1E-4 1
bsq 1E-4 1
0 1 0 1 0 1

[jon@ki-rh42 v5dfield]$ cat head8.v5d
density 1E-4 1
ug 1E-4 1
uu0 1E-4 1
bsq 1E-4 1
lrho 1E-4 1
neglrho 1E-4 1
lbsq 1E-4 1
Rcyl 1E-4 1
0 1 0 1 0 1

[jon@ki-rh42 v5dfield]$ cat headout16.v5d
density 1E-4 1
ug 1E-4 1
uu0 1E-4 1
bsq 1E-4 1
lrho 1E-4 1
Rcyl 1E-4 1
0 1 0 1 0 1

# using iinterp16
[jon@ki-rh42 v5dfield]$ cat headout16old.v5d
density 1E-4 1
ug 1E-4 1
uu0 1E-4 1
bsq 1E-4 1
lrho 1E-4 1
lbsq 1E-4 1
0 1 0 1 0 1


[jon@ki-rh42 v5dfield]$ cat headout17.v5d
density 1E-4 1
ug 1E-4 1
uu0 1E-4 1
bsqorho 1E-4 1
lrho 1E-4 1
neglrho 1E-4 1
lbsq 1E-4 1
Rcyl 1E-4 1
W 1E-4 1
V 1E-4 1
U 1E-4 1
W2 1E-4 1
V2 1E-4 1
U2 1E-4 1
0 1 0 1 0 1

# Old iinterp + vis5d: v1,v2,v3 -> V,W,U because v1=vx, v2=vz, v3=vy

# Now, after becoming orthonormal and using lambdatrans, v1=vx, v2=vy, v3=vz
# With V,W,U assigned by head.v5d above, then V=vx W=vy, U=vz
# So in vis5d:
# East/West :  oldV2  =  Bx   = newU2
# North/South: oldW2  =  By   = newV2
# Vertical:    oldU2  =  Bz   = newW2

# To get natural order of U=vx, V=vy, W=vz, then use header with order in head.v5d as just:
# U,V,W   and U2,V2,W2

# NO!  Apparently it's:
#  East/West: W2   = Bz
#  North/South: V2 = By
#  Vertical: U2    = Bx
# So set order in head.v5d as:
# W,V,U = Bx,By,Bz



# rho0,ug,vx,vy,vz,Bx,By,Bz,FEMrad,Bphi,Jt,Jx,Jy,Jz  (J's only exist if -docurrent 1 was set)
# ./bin2txt 1 5 0 1 3 100 100 100 1 head14.v5d $outfilename $outfilename.v5d f 14
# ./bin2txt 1 5 0 1 3 256 256 256 1 head14.v5d $outfilename $outfilename.v5d f 14

# rho0,ug,uu0,bsq
# ./bin2txt 1 5 0 1 3 $boxnx $boxny $boxnz 1 head4.v5d $outfilename $outfilename.v5d f 4


# rho0,ug,uu0,bsq,lrho,neglrho,lbsq,Rcyl
# ./bin2txt 1 5 0 1 3 $boxnx $boxny $boxnz 1 head8.v5d $outfilename $outfilename.v5d f 8


############################


./bin2txt 1 5 0 1 3 $boxnx $boxny $boxnz 1 headout17.v5d $outfilename $outfilename.v5d f $numoutputcols

# to just check (no script):
#
#  vis5d $outfilename.v5d -mbs 1000

#  vis5d $outfilename.v5d -mbs 3000



ixmin=$boxxl
iymin=$boxyl
izmin=$boxzl
ixmax=$boxxh
iymax=$boxyh
izmax=$boxzh


# don't pass path to vis5d
imagefilename=`basename ${outfilename}.v5d.3dmovie.ppm`

echo "set outputfilename" \"${imagefilename}\" > $outfilename.headscript.tcl
#
echo "set ixmin [expr $ixmin]" >> $outfilename.headscript.tcl
echo "set ixmax [expr $ixmax]" >> $outfilename.headscript.tcl
#
echo "set iymin [expr $iymin]" >> $outfilename.headscript.tcl
echo "set iymax [expr $iymax]" >> $outfilename.headscript.tcl
#
echo "set izmin [expr $izmin]" >> $outfilename.headscript.tcl
echo "set izmax [expr $izmax]" >> $outfilename.headscript.tcl
#


# create final v5d script
cat $outfilename.headscript.tcl fulltiltmovie.tcl > $outfilename.v5d.3dmovie.tcl

resx=800
resy=800

# -offscreen
vis5d $outfilename.v5d -mbs 1000 -geometry ${resx}x${resy} -script $outfilename.v5d.3dmovie.tcl







#
############################################
############################################
################################### DONE!


















############
# bad way to create 3-time file (too slow for read-in to seek all over the place)


# another beginning of slow way:
# setup new compact 3-time file read-in as 1-time file
$bin2txtprogname 1 2 0 -1 3 $nx $ny $nz 1 dumps/fieldline$dumpnum.bin $IDUMPDIR/fieldline$dumpnum.txt f $numcolumns
$bin2txtprogname 1 2 0 -1 3 $nx $ny $nz 1 dumps/fieldline$dumpnumm1.bin $IDUMPDIR/fieldline$dumpnumm1.txt f $numcolumns
$bin2txtprogname 1 2 0 -1 3 $nx $ny $nz 1 dumps/fieldline$dumpnump1.bin $IDUMPDIR/fieldline$dumpnump1.txt f $numcolumns
#... using "join" or "pr" or similar -- but too slow to create text files and pointless.




# setup new 3-time file:
if [ 1 -eq 0 ]
then
    head -1 ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.bin.head
    tail -n +2 ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.bin.data
    tail -n +2 ./dumps/fieldline$dumpnumm1.bin > $IDUMPDIR/fieldline$dumpnumm1.bin.data
    tail -n +2 ./dumps/fieldline$dumpnump1.bin > $IDUMPDIR/fieldline$dumpnump1.bin.data
    cat $IDUMPDIR/fieldline$dumpnum.bin.head $IDUMPDIR/fieldline$dumpnumm1.bin.data $IDUMPDIR/fieldline$dumpnum.bin.data $IDUMPDIR/fieldline$dumpnump1.bin.data > $IDUMPDIR/fieldline$dumpnum.bin.3time
fi



###########
# NOTES:

# note that files needed by iinterp and bin2txt are only:

CFILES1="jon_interp.c jon_interp_computepreprocess.c jon_interp_filter.c jon_interp_interpolationitself.c jon_interp_mnewt.c jon_interp_newt.c nrutil2.c coord.c nrutil.c ludcmp.c lubksb.c lnsrch.c fdjac.c fmin.c broydn.c qrdcmp.c rsolv.c qrupdt.c rotate.c ranc.c bcucof.c bcuint.c metric.tools.c mpi_fprintfs.c gaussj.c tetrad.c"
CFILES2="tensor.c smcalc.c generatenprs.c bin2txt.c initbase.defaultnprlists.c"

CFILES="$CFILES1 $CFILES2"

# create grep -v command:
GREPCOMMAND=""
for fil in $CFILES ; do GREPCOMMAND=$GREPCOMMAND"|grep -v $fil " ; done


ls -art *.c <PASTE echo $GREPCOMMAND here>




#######################################
# OLDER THINGS -- don't use -- just use above if reading in field lines

# interpolate to get rest-mass density
whichoutput=1000
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.rho.bin
#
# interpolate to get thermal gas internal energy density
whichoutput=1001
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.ug.bin
#
#
# interpolate to get orthonormal v^x
whichoutput=1003
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.vx.bin
#
# interpolate to get orthonormal v^y
whichoutput=1004
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.vy.bin
#
# interpolate to get orthonormal v^z
whichoutput=1005
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.vz.bin
#
# interpolate to get orthonormal B^x
whichoutput=1007
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.Bx.bin
#
# interpolate to get orthonormal B^y
whichoutput=1008
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.By.bin
#
# interpolate to get orthonormal B^z
whichoutput=1009
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.Bz.bin
#
# interpolate to get radial energy flux per unit sin(\theta)
whichoutput=1011
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.FE.bin
#
# interpolate to get poloidal current density (B_\phi)
whichoutput=1012
$iinterpprogname -binaryinput 1 -binaryoutput 1 -inFTYPE float -outFTYPE float -dtype $whichoutput -itype 1 -head 1 1 -oN 1 272 128 256 -refine 1.0 -filter 0 -grids 1 0 -nN 1 $boxnx $boxny $boxnz -ibox 0 0 -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvaluetype 0 -gdump ./dumps/gdump.bin -gdumphead 1 1 -binaryinputgdump 1 -inFTYPEgdump double < ./dumps/fieldline$dumpnum.bin > $IDUMPDIR/fieldline$dumpnum.cart.Bphi.bin

# merge interpolation results




#####################################
# NOTES, OLDER THINGS -- DONT USE!




# MTB12 simulations have the following headers in fieldline and gdump files:
tsteppartf, realtotalsize[1], realtotalsize[2], realtotalsize[3], realstartx[1], realstartx[2], realstartx[3], dx[1], dx[2], dx[3], localrealnstep,gam,a,R0,Rin,Rout,hslope,localdt,defcoord,MBH,QBH,EP3,is,ie,js,je,ks,ke,whichdump,whichdumpversion,numcolumns


# new tilted simulation header has extra THETAROT
tsteppartf, realtotalsize[1], realtotalsize[2], realtotalsize[3], realstartx[1], realstartx[2], realstartx[3], dx[1], dx[2], dx[3], localrealnstep,gam,a,R0,Rin,Rout,hslope,localdt,defcoord,MBH,QBH,EP3,THETAROT,is,ie,js,je,ks,ke,whichdump,whichdumpversion,numcolumns);



############ SKIP THIS SECTION AND GO TO IINTERP SECTION
# convert fieldline????.bin into text format.
cd $joninterpcodedir
make superclean ; make prepbin2txt ; make bin2txt

$bin2txtprogname  1 2 0 -1 3 272 128 256 1 fieldline5437.bin fieldline5437.txt f 11

# now break into separate columns
head -1 fieldline5437.txt > fieldline5437.txt.head
tail -n +2 fieldline5437.txt > fieldline5437.txt.data
awk '{print $1}' fieldline5437.txt.data > fieldline5437.txt.data.rho0
awk '{print $2}' fieldline5437.txt.data > fieldline5437.txt.data.ug

# get u^i from u^t and v^i = u^i/u^t
awk '{print $4}' fieldline5437.txt.data > fieldline5437.txt.data.uux0
awk '{print $5*$4}' fieldline5437.txt.data > fieldline5437.txt.data.uux1
awk '{print $6*$4}' fieldline5437.txt.data > fieldline5437.txt.data.uux2
awk '{print $7*$4}' fieldline5437.txt.data > fieldline5437.txt.data.uux3

################

# -coord <Rin> <Rout> <R0> <hslope>
#$iinterpprogname -binaryinput 0 -binaryoutput 0 -inFTYPE float -outFTYPE float -dtype 1 -itype 1 -head 1 1 -oN 272 128 256 -refine 1.0 -filter 0 0 -grids 1 0 -nN $boxnx $boxny $boxnz -ibox -$boxx $boxx -$boxy $boxy -$boxz $boxz -coord 1.15256306940633 26000 0 1.04 -defcoord 1401 -dofull2pi 1 -extrap 1 -defaultvalue 0 -gdump ./gdump.txt < fieldline5437.rho.txt > fieldline5437.cart.rho.txt
