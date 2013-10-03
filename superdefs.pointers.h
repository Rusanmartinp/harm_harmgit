////////////////////////////////////////////////////////////////
//
//
// (possibly shifted) pointers: PTRDEFGLOBALMACP?A?(simplename,?,?,?,...) to memory  real space: BASEMACP?A?(simplename,?,?,?,...)
//
//
/////////////////////////////////////////////////////////////////




PFTYPE PTRDEFGLOBALMACP0A1(pflag,N1M,N2M,N3M,NUMPFLAGS);
PFTYPE PTRDEFGLOBALMACP0A1(pflagfailorig,N1M,N2M,N3M,NUMFAILPFLAGS);

CTYPE PTRDEFGLOBALMACP0A4(enodebugarray,N1M,N2M,N3M,3-1,NUMENOINTERPTYPES,NPR-4,NUMENODEBUGS);
CTYPE PTRDEFGLOBALMACP0A3(failfloorcount,N1M,N2M,N3M,2,NUMTSCALES,NUMFAILFLOORFLAGS);

FTYPE PTRDEFGLOBALMACP0A1(failfloordu,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(dissmeasurearray,N1M,N2M,N3M,NSPECIAL+1+3*2);

FTYPE PTRDEFGLOBALMACP0A1(dissfunpos,N1M,N2M,N3M,NUMDISSFUNPOS);


FTYPE PTRDEFGLOBALMACP0A1(pglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP1A0(pother,FILL,N1M,N2M,N3M);
FTYPE PTRDEFGLOBALMACP0A1(panalytic,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(pstaganalytic,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(omegafanalytic,N1M,N2M,N3M,NPR);


// just pointers that duplicate other points but for dumping
FTYPE PTRDEFGLOBALMACP0A1(pdump,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(pstagdump,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(Bhatdump,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP1A0(vpotarraydump,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMACP1A1(pl_ctdump,FILL,N1M,N2M,N3M,NPR2INTERP);
FTYPE PTRDEFGLOBALMACP1A1(pr_ctdump,FILL,N1M,N2M,N3M,NPR2INTERP);


struct of_state PTRDEFGLOBALMACP1A1(fluxstate,FILL,N1M,N2M,N3M,NUMLEFTRIGHT);
struct of_state PTRDEFGLOBALMACP0A0(fluxstatecent,N1M,N2M,N3M);

FTYPE PTRDEFGLOBALMACP1A0(emf,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMACP0A1(vconemf,N1M,N2M,N3M,NDIM-1);

FTYPE PTRDEFGLOBALMACP1A0(vpotarrayglobal,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMACP1A0(vpotanalytic,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);

FTYPE PTRDEFGLOBALMACP0A1(dtijk,N1M,N2M,N3M,COMPDIM);


FTYPE PTRDEFGLOBALMACP1A1(wspeedtemp,NUMEOMSETS,N1M,N2M,N3M,NUMCS); // temporary wave speeds
FTYPE PTRDEFGLOBALMACP3A0(wspeed,FILL,COMPDIM,NUMCS,N1M,N2M,N3M); // wave speeds

FTYPE PTRDEFGLOBALMACP1A0(shockindicatorarray,NUMSHOCKPLS,N1M,N2M,N3M);

//FTYPE PTRDEFGLOBALMACP0A1(ubound,N1M,N2M,N3M,NPR); // doesn't appear to be needed
FTYPE PTRDEFGLOBALMACP0A1(udump,N1M,N2M,N3M,NPR);

FTYPE PTRDEFGLOBALMACP0A1(unewglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(ulastglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(uinitialglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(upointglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(upointglobaluf,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(oldufstore,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(dUgeomarray,N1M,N2M,N3M,NPR);



FTYPE PTRDEFGLOBALMACP1A1(gp_l,FILL,N1M,N2M,N3M,NPR2INTERP);
FTYPE PTRDEFGLOBALMACP1A1(gp_r,FILL,N1M,N2M,N3M,NPR2INTERP);

FTYPE PTRDEFGLOBALMACP0A1(pleft,N1M,N2M,N3M,NPR2INTERP);
FTYPE PTRDEFGLOBALMACP0A1(pright,N1M,N2M,N3M,NPR2INTERP);

//FTYPE PTRDEFGLOBALMACP2A0(wspeedcorn,FILL,NUMCS,N1M,N2M,N3M); // wave speeds
//FTYPE PTRDEFGLOBALMACP0A1(utoinvert,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(pstagglobal,N1M,N2M,N3M,NPR);
//FTYPE PTRDEFGLOBALMACP3A0(pbcorninterp,FILL,COMPDIM,NUMCS,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMACP1A3(pvbcorninterp,COMPDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,COMPDIM,NUMCS+1,NUMCS); // NUMCS+1 has +1 to store old pbcorninterp
FTYPE PTRDEFGLOBALMACP1A0(geomcornglobal,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);

FTYPE PTRDEFGLOBALMACP0A1(Bhatglobal,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(Bhatanalytic,N1M,N2M,N3M,NPR);




FTYPE PTRDEFGLOBALMACP0A1(dq1,N1M,N2M,N3M,NPR2INTERP);
FTYPE PTRDEFGLOBALMACP0A1(dq2,N1M,N2M,N3M,NPR2INTERP);
FTYPE PTRDEFGLOBALMACP0A1(dq3,N1M,N2M,N3M,NPR2INTERP);

FTYPE PTRDEFGLOBALMACP0A1(prc,N1M,N2M,N3M,NPR2INTERP);

FTYPE PTRDEFGLOBALMACP0A1(ptemparray,N1M,N2M,N3M,NPR);
FTYPE PTRDEFGLOBALMACP0A1(utemparray,N1M,N2M,N3M,NPR);

FTYPE PTRDEFGLOBALMACP0A1(ucumformetric,N1M,N2M,N3M,NPR);

// Not with SHIFT1/2/3 like BASE pointer
FTYPE PTRDEFGLOBALMACP0A1(F1,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(F2,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(F3,N1M,N2M,N3M,NPR+NSPECIAL);

FTYPE PTRDEFGLOBALMACP0A1(F1EM,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(F2EM,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(F3EM,N1M,N2M,N3M,NPR+NSPECIAL);

FTYPE PTRDEFGLOBALMACP0A1(fluxvectemp,N1M,N2M,N3M,NPR+NSPECIAL);

FTYPE PTRDEFGLOBALMACP0A1(Fa,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(Fb,N1M,N2M,N3M,NPR+NSPECIAL);

// storage for variable used to compute stencil
FTYPE PTRDEFGLOBALMACP0A1(stencilvartemp,N1M,N2M,N3M,NPR);


FTYPE PTRDEFGLOBALMACP0A1(a2cin,N1M,N2M,N3M,NPR+NSPECIAL);
FTYPE PTRDEFGLOBALMACP0A1(a2cout,N1M,N2M,N3M,NPR+NSPECIAL);

FTYPE PTRDEFGLOBALMACP1A1(pk,FILL,N1M,N2M,N3M,NPR);





///////////////////////////////
//
// grid functions (+1 size larger so can have geometry at upper corners -- say for vector potential or whatever)
//
///////////////////////////////
#if(NEWMETRICSTORAGE)
struct of_gdetgeom PTRDEFGLOBALMETMACP0A1(gdetgeom,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NPG);

struct of_gdetgeom PTRDEFGLOBALMETMACP1A0(gdetgeomnormal,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);

struct of_compgeom PTRDEFGLOBALMETMACP1A0(compgeom,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
struct of_compgeom PTRDEFGLOBALMETMACP1A0(compgeomlast,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#else
FTYPE PTRDEFGLOBALMETMACP1A1(gcon,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE PTRDEFGLOBALMETMACP1A1(gcov,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE PTRDEFGLOBALMETMACP1A1(gcovpert,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
FTYPE PTRDEFGLOBALMETMACP1A0(gdet,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMETMACP1A1(eomfunc,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NPR);
FTYPE PTRDEFGLOBALMETMACP1A0(gdetvol,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMETMACP1A0(alphalapse,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMETMACP1A0(betasqoalphasq,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE PTRDEFGLOBALMETMACP1A1(beta,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);

FTYPE PTRDEFGLOBALMETMACP1A1(gcovlast,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE PTRDEFGLOBALMETMACP1A1(gcovpertlast,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
FTYPE PTRDEFGLOBALMETMACP1A0(alphalapselast,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#endif


FTYPE PTRDEFGLOBALMETMACP1A2(dxdxpstore,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM,NDIM);
FTYPE PTRDEFGLOBALMETMACP1A2(idxdxpstore,FILL,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM,NDIM);

FTYPE PTRDEFGLOBALMACP1A1(Xstore,FILL,N1M+SHIFT1*3,N2M+SHIFT2*3,N3M+SHIFT3*3,NDIM); // Xstore varies with space always
FTYPE PTRDEFGLOBALMACP1A1(Vstore,FILL,N1M+SHIFT1*3,N2M+SHIFT2*3,N3M+SHIFT3*3,NDIM); // Vstore varies with space always

FTYPE PTRDEFGLOBALMETMACP0A3(conn,N1M,N2M,N3M,NDIM,NDIM,NDIM);
FTYPE PTRDEFGLOBALMETMACP0A1(conn2,N1M,N2M,N3M,NDIM);
FTYPE PTRDEFGLOBALMETMACP0A1(idxvol,N1M,N2M,N3M,NDIM);





// current density stuff
FTYPE PTRDEFGLOBALMACP0A2(cfaraday,N1M,N2M,N3M,NUMCURRENTSLOTS,3);
FTYPE PTRDEFGLOBALMACP0A1(fcon,N1M,N2M,N3M,NUMFARADAY);
FTYPE PTRDEFGLOBALMACP0A1(jcon,N1M,N2M,N3M,NDIM);

// time average stuff
FTYPE PTRDEFGLOBALMACP0A1(normalvarstavg,N1M,N2M,N3M,NUMNORMDUMP);
FTYPE PTRDEFGLOBALMACP0A1(anormalvarstavg,N1M,N2M,N3M,NUMNORMDUMP);

FTYPE PTRDEFGLOBALMACP0A1(jcontavg,N1M,N2M,N3M,NDIM);
FTYPE PTRDEFGLOBALMACP0A1(jcovtavg,N1M,N2M,N3M,NDIM);
FTYPE PTRDEFGLOBALMACP0A1(ajcontavg,N1M,N2M,N3M,NDIM);
FTYPE PTRDEFGLOBALMACP0A1(ajcovtavg,N1M,N2M,N3M,NDIM);

FTYPE PTRDEFGLOBALMACP0A1(massfluxtavg,N1M,N2M,N3M,NDIM);
FTYPE PTRDEFGLOBALMACP0A1(amassfluxtavg,N1M,N2M,N3M,NDIM);

FTYPE PTRDEFGLOBALMACP0A1(othertavg,N1M,N2M,N3M,NUMOTHER);
FTYPE PTRDEFGLOBALMACP0A1(aothertavg,N1M,N2M,N3M,NUMOTHER);

FTYPE PTRDEFGLOBALMACP0A1(fcontavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE PTRDEFGLOBALMACP0A1(fcovtavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE PTRDEFGLOBALMACP0A1(afcontavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE PTRDEFGLOBALMACP0A1(afcovtavg,N1M,N2M,N3M,NUMFARADAY);

FTYPE PTRDEFGLOBALMACP0A1(tudtavg,N1M,N2M,N3M,NUMSTRESSTERMS);
FTYPE PTRDEFGLOBALMACP0A1(atudtavg,N1M,N2M,N3M,NUMSTRESSTERMS);

// for image, see image.c
FTYPE PTRDEFGLOBALMACP0A1(pimage,N1M,N2M,N3M,NPR);


FTYPE PTRDEFGLOBALMACP0A1(fluxdump,N1M,N2M,N3M,NUMFLUXDUMP);


#include "superdefs.pointers.rad.h" // KORAL

