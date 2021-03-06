
#include "decs.h"



// Uses globals inside init() because sets memory and sets physics
int init(int *argc, char **argv[])
{
  extern int prepre_init(void);
  extern int pre_init(int *argc, char **argv[]);
  extern int init_defdefcoord(void);
  extern int init_defgrid(void);
  extern int init_defglobal(void);
  extern int init_defconsts(void);
  extern int init_arrays_before_set_pu(void);
  extern void filterffde(int i, int j, int k, FTYPE *pr);
  extern void filter_coldgrmhd(int i, int j, int k, FTYPE *pr);
  void set_grid_all(FTYPE thetarot, int restartmode);
  //
  void  init_all_conservatives(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*utemp)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR]);
  int i,j,k;
  int jj;
  int pl,pliter;
  int selfgraviter;
  int gotnan;
  int faketimeorder,fakenumtimeorders;





  stderrfprintf("Start init\n"); fflush(stderr);

  //////////////
  //
  // Both normal and restart mode need all "pre_init" functions called
  //
  //////////////


  if(RESTARTMODE==0 || RESTARTMODE==1){
    //////////////
    //
    // prepre_init type functions should not assume anything done yet
    //
    //////////////
    prepre_init();
    prepre_init_specific_init(); // user func

    //////////////
    //
    // pre_init type functions initialize MPI and other things
    //
    //////////////
    pre_init(argc,argv);
    pre_init_specific_init(); // user func
  }



  ///////////////////
  //
  // Always set parameters and coordinates as default in case restart file doesn't have this information.  Properly, the restart file will overwrite if file contains those variables.
  //
  //////////////////
  if(RESTARTMODE==0 || RESTARTMODE==1){
    // define parameters
    init_defglobal(); // init default global parameters
    init_defconsts(); // init default physical constants
    init_consts(); // init user constants
    init_global(); // request choices for global parameters

    init_defdefcoord(); // choose defcoord
    init_defcoord(); // user choose defcoord

    set_coord_parms_nodeps(defcoord); // requires only defcoord, no other grid parameters

    init_defgrid(); // init default grid
    init_grid(); // request choices for grid/coordinate/metric parameters


    set_coord_parms_deps(defcoord); // requires correct defcoord at least

    // requires special3dspc, periodicx?, and dofull2pi and other things defining grid type already be set
    // after the below call, can then call boundary functions and MPI decomposition will be ready
    init_placeongrid_griddecomposition();
    

    // set certain arrays to zero for diagnostic purposes (must come after all basic init stuff that sets up grid parms and all other parms)
    init_arrays_before_set_pu();
  


  }


  ////////////////
  //
  // RESTART MODE
  //
  // Loads primitives and conserved quantities.
  // Always assume conserved quantities are in the file, regardless of whether used.  Only used if doing higher-order method
  //
  ///////////////
  if(RESTARTMODE==1){
    trifprintf("start restart_init: proc=%04d\n",myid);
    if (restart_init(WHICHFILE) >= 1) {
      dualfprintf(fail_file, "main:restart_init: failure\n");
      return(1);
    }
    trifprintf("end restart_init: proc=%04d\n",myid);

    // don't bound read-in data yet since need grid and other things

  }




  ///////////////////
  //
  // Always write new coordparms file.  Fresh start needs it, but also do in case user updated restart file but not coord file
  //
  ////////////////////
  if(RESTARTMODE==0 || RESTARTMODE==1){
    write_coord_parms(defcoord); // output coordinate parameters to file
  }



  ///////////////////
  //
  // Both normal and restart mode need to setup grid
  //
  //////////////////
  if(RESTARTMODE==0 || RESTARTMODE==1){
    set_grid_all(THETAROTPRIMITIVES,RESTARTMODE); // use THETAROT=THETAROTPRIMITIVES
  }







  ///////////////////
  //
  // Normal fresh start need to get primitive and conserved quantities
  //
  //////////////////
  if(RESTARTMODE==0){


#if(DOSELFGRAVVSR)
#define NUMSELFGRAVITER 3 // MINIMUM of 2 iterations should be done for metric to computed once
    // further iterations will determine PRIMECOORD velocity and magnetic field using newly updated metric
    // 3 iterations allows this new metric to be used once, and I suggest this as good minimum # of iterations
#else
#define NUMSELFGRAVITER 1 // NO CHOICE
#endif



    // iterations to get consistent/converged initial conditions
    // assume user chooses primitive in final-to-be-metric so first primitive is correct final primitive
    // conservatives adjusted with adjusting metric
    for(selfgraviter=1;selfgraviter<=NUMSELFGRAVITER;selfgraviter++){


      trifprintf("begin iteration over metric: selfgraviter=%d\n",selfgraviter);


      if(selfgraviter>1){
        if(DOSELFGRAVVSR){
          trifprintf("new metric with self-gravity: selfgraviter=%d\n",selfgraviter);
          // if box_grid needs to change, is done inside below function
          compute_new_metric_and_prims(0,MBH, a, QBH, EP3, THETAROT, GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal),GLOBALPOINT(vpotarrayglobal),GLOBALPOINT(Bhatglobal),GLOBALPOINT(gp_l),GLOBALPOINT(gp_r),GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3),GLOBALPOINT(emf),GLOBALPOINT(ulastglobal));
          trifprintf("done with computing new metric with self-gravity dt=%21.15g selfgraviter=%d\n",dt,selfgraviter);
        }
      }


      
    

      // user function that should fill p with primitives
      // assumes everyting computed by:  compute_EOS_parms() is known by init_primitives without immediate reference to what's computed by compute_EOS_parms since this function depends upon primitives themselves
      MYFUN(init_primitives(GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal),GLOBALPOINT(vpotarrayglobal),GLOBALPOINT(Bhatglobal),GLOBALPOINT(panalytic),GLOBALPOINT(pstaganalytic),GLOBALPOINT(vpotanalytic),GLOBALPOINT(Bhatanalytic),GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3),GLOBALPOINT(emf)),"initbase.c:init()", "init_primitives()", 0);


      
      // dump analytic solution
      //GLOBALPOINT(pdump)=GLOBALPOINT(panalytic);
      //if (dump(-9999) >= 1){
      //  dualfprintf(fail_file,"unable to print dump file\n");
      //  return (1);
      //}


      // assign memory for dumping primitives
      GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
      
      
      
      //    COMPLOOPF{
      // MACP0A1(p,i,j,k,UU)=0.0;
      // }
      
      // mandatory filters on user supplied primitives
      
      /////////////////////////////
      //
      // Filter to get correct degenerate FFDE solution
      //
      /////////////////////////////// 
      
      if(EOMTYPE==EOMFFDE){
        trifprintf("System filtered to FFDE\n");
        // filter to get force-free
        COMPFULLLOOP{
          filterffde(i,j,k,GLOBALMAC(pglobal,i,j,k));
        }
      }
      
      if(EOMTYPE==EOMCOLDGRMHD){
        trifprintf("System filtered to cold GRMHD\n");
        // filter to get cold GRMHD
        COMPFULLLOOP{
          filter_coldgrmhd(i,j,k,GLOBALMAC(pglobal,i,j,k));
        }
      }
      
      
      /////////////////////////////
      //
      // Fixup and Bound variables since field may have changed
      // Also setup pre_fixup() type quantities
      //
      /////////////////////////////// 
      
      trifprintf("System Fixup and Bound\n");
      
#if(FIXUPAFTERINIT)
      if(fixup(STAGEM1,GLOBALPOINT(pglobal),GLOBALPOINT(unewglobal),0)>=1)
        FAILSTATEMENT("initbase.c:init()", "fixup()", 1);
#endif

      
      {
        int finalstep=1; // assume user wants to know if initial conserved quants changed
        if (bound_allprim(STAGEM1,finalstep,t,GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal), USEMPI) >= 1)
          FAILSTATEMENT("initbase.c:init()", "bound_allprim()", 1);
      }


      // now fully bounded, initialize interpolations in case interpolate using prim/pstag data
      pre_interpolate_and_advance(GLOBALPOINT(pglobal));

      
      if(pre_fixup(STAGEM1,GLOBALPOINT(pglobal))>=1)
        FAILSTATEMENT("initbase.c:init()", "pre_fixup()", 1);


      if(DODIAGS && PRODUCTION==0){
        ///////////////////////////////
        // BEGIN DEBUG
        // dump solution so far
        if(selfgraviter==1){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9000) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        else if(selfgraviter==2){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9001) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        else if(selfgraviter==3){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9002) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        // END DEBUG
        ///////////////////////////////
      }// end if doing diagnostics and production==0

      
      // after all parameters and primitives are set, then can set these items
      post_init(GLOBALPOINT(pglobal),GLOBALPOINT(cfaraday));
      // user post_init function
      post_init_specific_init();

      init_all_conservatives(GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(ulastglobal),GLOBALPOINT(unewglobal));


      if(DODIAGS && PRODUCTION==0){
        ///////////////////////////////
        // BEGIN DEBUG
        // dump solution so far
        if(selfgraviter==1){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9100) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        else if(selfgraviter==2){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9101) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        else if(selfgraviter==3){
          GLOBALPOINT(pdump)=GLOBALPOINT(pglobal);
          if (dump(9102) >= 1){
            dualfprintf(fail_file,"unable to print dump file\n");
            return (1);
          }
        }
        // END DEBUG
        ///////////////////////////////
      }

      trifprintf("end iteration over metric: selfgraviter=%d\n",selfgraviter);
    
    }// end loop to get metric and primitive setting consistent





  }// end if RESTARTMODE==0
  ///////////////////
  //
  // Restart and get primitive and conserved quantities
  //
  //////////////////
  else if(RESTARTMODE==1){


    restart_init_simple_checks(2);


#if(FIXUPAFTERINIT)
    trifprintf("before fixup() during restart: proc=%04d\n",myid);
    if(fixup(STAGEM1,GLOBALPOINT(pglobal),GLOBALPOINT(unewglobal),0)>=1)  FAILSTATEMENT("initbase.c:init()", "fixup()", 1);
    trifprintf("after fixup() during restart: proc=%04d\n",myid);
#endif

    trifprintf("before bound_prim() during restart: proc=%04d\n",myid);

    {
      // pstag not computed from unewglobal yet, so don't bound it.  It'll get self-consistently bounded when ucons2upointppoint() is called below
      int finalstep=1; // assume user wants to know if initial conserved quants changed
      if (bound_prim(STAGEM1,finalstep,t,GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal), USEMPI) >= 1)      FAILSTATEMENT("initbase.c:init()", "bound_allprim()", 1);
    }

    trifprintf("after bound_prim() during restart: proc=%04d\n",myid);


      
    trifprintf("before pre_fixup() during restart: proc=%04d\n",myid);
    if(pre_fixup(STAGEM1,GLOBALPOINT(pglobal))>=1) FAILSTATEMENT("initbase.c:init()", "pre_fixup()", 1);
    trifprintf("after pre_fixup() during restart: proc=%04d\n",myid);


    restart_init_simple_checks(3);


    // before can interpolate inside ucons2upointppoint(), need to compute things needed by interpolation, including for STORESHOCKINDICATOR==1
    if(STORESHOCKINDICATOR==1){
      pre_interpolate_and_advance(GLOBALPOINT(pglobal));
    }


    ////////////////
    //
    // Note there is no need to convert average or quasi-deaveraged field to staggered field (see comments in ucons2upointppoint())
    // However, divb diagnostics at first won't be right at t=0 since set to use primitive for lower-order method, but just assume diagnostic won't likely be immediately after restart
    // For RESTARTMODE==0 the pstag quantity is set by user or during vector potential conversion to u and p, but during restart we only read-in p and unew while we need also pstagscratch
    // so after ucons2upointppoint() call, pcent as in boundaries will not be updated but not required.  And it will not be Nan since did bound above.
    //
    /////////////////
    trifprintf("before ucons2upointppoint during restart: proc=%04d\n",myid);
    ucons2upointppoint(t, GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal),GLOBALPOINT(ulastglobal),GLOBALPOINT(pglobal)); // this regenerates pcentered for B1,B2,B3
    trifprintf("after ucons2upointppoint during restart: proc=%04d\n",myid);

    
    // now bound unewglobal and vpot's
    if(FLUXB==FLUXCTSTAG){
      int boundvartype=BOUNDPRIMTYPE;
      int finalstep=1; // assume user wants to know if initial conserved quants changed
      int doboundmpi=1;
      bound_anypstag(STAGEM1, finalstep, t, boundvartype, GLOBALPOINT(unewglobal), GLOBALPOINT(unewglobal), GLOBALPOINT(unewglobal), doboundmpi);
    }
    if(EVOLVEWITHVPOT||TRACKVPOT){
      int boundvartype=BOUNDVPOTTYPE;
      int finalstep=1; // assume user wants to know if initial conserved quants changed
      int doboundmpi=1;
      bound_vpot(STAGEM1, finalstep, t, boundvartype, GLOBALPOINT(vpotarrayglobal),doboundmpi);
    }


    restart_init_simple_checks(4);


    // after all parameters and primitives are set, then can set these items
    trifprintf("before post_init and post_init_specific_init during restart: proc=%04d\n",myid);
    post_init(GLOBALPOINT(pglobal),GLOBALPOINT(cfaraday));
    // user post_init function
    post_init_specific_init();
    trifprintf("after post_init and post_init_specific_init during restart: proc=%04d\n",myid);


    restart_init_simple_checks(5);


    // don't want conservatives or primitives to change, just compute metric
    if(DOSELFGRAVVSR){
      trifprintf("new metric with self-gravity\n");
      compute_new_metric_and_prims(0,MBH, a, QBH, EP3, THETAROT, GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal),GLOBALPOINT(vpotarrayglobal),GLOBALPOINT(Bhatglobal),GLOBALPOINT(gp_l),GLOBALPOINT(gp_r),GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3),GLOBALPOINT(emf),GLOBALPOINT(ulastglobal));
      trifprintf("done with computing new metric with self-gravity dt=%21.15g\n",dt);
    }


    trifprintf("before restart_init_checks() during restart: proc=%04d\n",myid);
    if (restart_init_checks(WHICHFILE, GLOBALPOINT(pglobal), GLOBALPOINT(pstagglobal), GLOBALPOINT(unewglobal)) >= 1) {
      dualfprintf(fail_file, "main:restart_init_checks: failure\n");
      return(1);
    }
    trifprintf("after restart_init_checks() during restart: proc=%04d\n",myid);


    trifprintf( "proc: %d restart completed: failed=%d\n", myid, failed);
  


  }// done if RESTARTMODE==1



  if(THETAROTMETRIC!=THETAROTPRIMITIVES){ // Deal with initial tilt
    // SUPERGODMARK: After changing metric, should really also do after below:
    // 1) A_i or U -> Bstag for stag method *or* A_i -> B for Toth
    // 2) Bstag -> B for stag method
    // 3) Bstag,U -> Bhat for stag higher order method
    // etc.
    //
    // Otherwise, gdet changes and t=0 Bstag and B won't be consistent with A_i and U for the new metric.
    
    
    ///////////////////
    //
    // Both normal and restart mode need to setup grid
    //
    //////////////////
    if(RESTARTMODE==0 || RESTARTMODE==1){
      set_grid_all(THETAROTMETRIC,RESTARTMODE); // now use THETAROT=THETAROTMETRIC
    }

  }


  ///////////////////
  //
  // Final things done whether fresh run or restart run
  //
  //////////////////
#if(ANALYTICMEMORY)
  // copy over initial solution as analytic solution
  // NEEDED FOR BOUND in case uses panalytic
  // Should be done before any changes in grid due to grid sectioning so that analytic variables set over all grid rather than limited range.  Then expansion of grid uses those analytical values.
  copy_prim2panalytic(GLOBALPOINT(pglobal),GLOBALPOINT(panalytic),GLOBALPOINT(pstagglobal),GLOBALPOINT(pstaganalytic),GLOBALPOINT(vpotarrayglobal),GLOBALPOINT(vpotanalytic),GLOBALPOINT(Bhatglobal),GLOBALPOINT(Bhatanalytic));
#endif


  // redo grid positions using actual gridding (not full grid just because was doing inits before)
  faketimeorder=1;
  fakenumtimeorders=2;
  recompute_fluxpositions(3,faketimeorder,fakenumtimeorders,nstep,t);


  // must set dt after DOSELFGRAVVSR so have acceleration in case v=0 and c_s=0 initially
  // set initial dt correctly rather than random choice
  // actually compute_new_metric_and_prims() computes dt from set_dt()
  MYFUN(set_dt(GLOBALPOINT(pglobal),&dt),"initbase.c:init()","set_dt()",1);
  trifprintf("dt=%21.15g at t=%21.15g at nstep=%ld at realnstep=%ld\n",dt,t,nstep,realnstep);




  ///////////////////
  //
  // Some final diagnostics
  //
  //////////////////


#if(PRODUCTION==0)

  // final checks
  restart_init_simple_checks(6);

#endif




  trifprintf("end init.c\n");
  return (0);

}



// wrapper to do repeated set_grid when involving tilt
void set_grid_all(FTYPE thetarot, int restartmode)
{
  int set_box_grid_parameters(void);
  extern int post_par_set(void);


  // sets global THETAROT
  THETAROT = thetarot; // No, if restarting, then no need since not actually calling init_primitives // set so init rho,u,v,B are as if no rotation

  // once all interpolation parameters are set, now can set dependent items that may be used to set primitives or conservatives
  // doesn't use metric parameters, so doesn't need to be in SELFGRAV loop
  post_par_set();

  if(restartmode==1) trifprintf("proc: %d post_par_set completed: failed=%d\n", myid,failed);

  // get grid
  // 0 tells set_grid that it's first call to set_grid() and so have to assume stationarity of the metric since have no time information yet
  set_grid(0,0,0);

  if(restartmode==1) trifprintf("proc: %d grid restart completed: failed=%d\n", myid,failed);

  // set grid boundary parameters that uses metric parameters to determine loop ranges using enerregions and enerpos
  set_box_grid_parameters();

  if(restartmode==1) trifprintf("proc: %d set_box_grid_parameters completed: failed=%d\n", myid,failed);

  // user post_set_grid function
  init_grid_post_set_grid(GLOBALPOINT(pglobal),GLOBALPOINT(pstagglobal),GLOBALPOINT(unewglobal),GLOBALPOINT(vpotarrayglobal),GLOBALPOINT(Bhatglobal),GLOBALPOINT(panalytic),GLOBALPOINT(pstaganalytic),GLOBALPOINT(vpotanalytic),GLOBALPOINT(Bhatanalytic),GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3),GLOBALPOINT(emf));

  if(restartmode==1) trifprintf("proc: %d init_grid_post_set_grid completed: failed=%d\n", myid,failed);
  

  trifprintf("MCOORD=%d\n",MCOORD);
  trifprintf("COMPDIM=%d\n",COMPDIM);
  trifprintf("MAXBND=%d\n",MAXBND);
  trifprintf("FLUXB=%d\n",FLUXB);

}



// copy over initial solution as analytic solution
int copy_prim2panalytic(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*panalytic)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*pstaganalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*vpotanalytic)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhatanalytic)[NSTORE2][NSTORE3][NPR])
{

#if(ANALYTICMEMORY==0)
  dualfprintf(fail_file,"Shouldn't be setting analytic with ANALYTICMEMORY==0\n");
  myexit(ERRORCODEBELOWCLEANFINISH+35968346);
#endif


#pragma omp parallel
  {
    int i,j,k;
    int pl,pliter;
    int jj;

    OPENMP3DLOOPVARSDEFINE;

    OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // next vpot assignment does not depend upon this loop completing
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);
      ////////////  COMPFULLLOOP{

      PLOOP(pliter,pl) MACP0A1(panalytic,i,j,k,pl)=MACP0A1(prim,i,j,k,pl);
      if(FIELDSTAGMEM){
        PLOOP(pliter,pl) MACP0A1(pstaganalytic,i,j,k,pl)=MACP0A1(pstag,i,j,k,pl);
      }
      if(HIGHERORDERMEM){
        PLOOP(pliter,pl) MACP0A1(Bhatanalytic,i,j,k,pl)=MACP0A1(Bhat,i,j,k,pl);
      }

    }

    if(TRACKVPOT){
      /////////      COMPFULLLOOPP1{
      OPENMP3DLOOPSETUPFULLP1;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // next vpot assignment is independent
      OPENMP3DLOOPBLOCK{
        OPENMP3DLOOPBLOCK2IJK(i,j,k);
        
        DLOOPA(jj) MACP1A0(vpotanalytic,jj,i,j,k)=MACP1A0(vpot,jj,i,j,k);
      }
    }

  }// end parallel region (and implied barrier)


  return(0);
}


// initialize conserved quantities
//  if(RESTARTMODE==0)
void init_all_conservatives(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*utemp)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR])
{
  int pl,pliter;
  // below is user function that usually uses system function
  extern int init_conservatives(FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*Utemp)[NSTORE2][NSTORE3][NPR], FTYPE (*U)[NSTORE2][NSTORE3][NPR]);

  
  init_conservatives(prim, pstag, utemp, ucons);

  //  bound_uavg(STAGEM1,ucons); DEBUG DEBUG

}



#include "initbase.defaultnprlists.c"

// Called before pre_init() : i.e. before MPI init
int prepre_init(void)
{
  int pl,pliter;


  SQRTMINNUMREPRESENT=sqrt(MINNUMREPRESENT);

  NUMEPSILONPOW23=pow(NUMEPSILON,2.0/3.0);

  rancvaln=0; // for ranc.c

  // things initialized whether restarting or init fresh

  ranc(1,0);  // power up random number generator in case used without init


  // set default performance parameters
  set_defaults_performance_checks_prepreinit();


  // set file version numbers
  set_file_versionnumbers();


  ////////////////////
  //
  // Setup Loop ranges for primitive/conserved/interpolated variables
  //
  ////////////////////

  set_default_nprlists();




  advancepassnumber=-1; // by default assume all things done (should only matter if SPLITNPR==1 and debugging it)

  // still need to avoid conserved+flux calculations for other PL's in phys.c


  // below 2 now determined at command line.  See init_MPI_GRMHD() in init_mpi.c (myargs and init_MPI).
  //  RESTARTMODE=0;// whether restarting from rdump or not (0=no, 1=yes)
  //WHICHFILE=0; // see diag.c for dump_cnt and image_cnt starts
  // user defined parameter
  restartonfail=0; // whether we are restarting on failure or not and want special diagnostics
  specialstep=0; // normal step assumed
  didstorepositiondata=0; // assume haven't stored position data (yet)
  horizoni=-200;
  horizoncpupos1=-1;


  if(WHICHVEL==VEL3){
    jonchecks=1; // whether to include jon's checks to make sure u^t real and some rho/u limits throughout code
    jonchecks=0;
  }
  else jonchecks=0; // not relevant

  // choice// GODMARK: not convenient location, but needed for init_MPI_GRMHD()
  periodicx1=0;
  periodicx2=0;
  periodicx3=0;// GODMARK: periodic in \phi for 3D spherical polar
  dofull2pi=1;

  if(USEMPI&&USEROMIO){
    binaryoutput=MIXEDOUTPUT; // choice: mixed or binary
    sortedoutput=SORTED; // no choice
  }
  else{
    // choice
    //    binaryoutput=TEXTOUTPUT;
    binaryoutput=MIXEDOUTPUT; // choice: mixed or binary
    sortedoutput=SORTED;
  }


  /////////////////////
  //
  // Initialize global EOS pointers and initial repeated array values (originally static)
  //
  /////////////////////
  initeos_eomtype();


  return(0);
}



void set_defaults_performance_checks_prepreinit(void)
{

  // whether to log steps
  // log the time step as per NDTCCHECK
  DOLOGSTEP=1 ;
  // log the performance as per NZCCHECK
  DOLOGPERF=1;

  CHECKCONT=1; // 1: check if user wants to continue run 0: don't

  // initial guess for how often to check time per timestep
  NTIMECHECK=1000;

  // how often in steps to output step/dt/t data (controlled by above if above are nonzero, else use below numbers)
  // MARK 100 100 20 500
  // MARK 10 10 1 100 for 1024x1024 vortex
  // MARK 1D bondi: 10000 10000 1000 20000
  // MARK 2D MHD Tori128128: 500 500 10 1000
  NDTCCHECK=100;
  // how often in steps to check speed in zonecycles/sec
  NZCCHECK=100;
  NDTDOTCCHECK=10;
  NGOCHECK=1000; // how often in steps to check the go.go file to see if to continue running
  NDTPERFDUMPCHECK=1000;


}



void set_defaults_performance_checks_preinit(void)
{
  // must come after MPI is initialized
  FTYPE secsperdump;
  
  // how often in REAL *seconds* to dump 0_logstep.out file (unless 0, which then uses below)
  DTstep=10.0;
  DTstepdot=1.0;
  DTperf=DTstep;
  DTgocheck=30.0;
  DTtimecheck=60.0;
  // below assumes shared disk space at 20MB/sec access
  secsperdump=(100.0*8.0*(FTYPE)(totalsize[1]*totalsize[2]*totalsize[3])/(20.0*1024.0*1024.0*1024.0));
  DTperfdump=3.0*secsperdump; // factor of 3 longer assumed so dumps don't hurt performance


  // number of wallseconds per perf run(see: main.c)
  PERFWALLTIME=30.0;
  // 1 linux cluster cpu
  // ZCPSESTIMATE (50000)
  // 25 linux cluster cpu(550Mhz Xeon's connected via Myrinet)
  // ZCPSESTIMATE (1250000)
  // 36 linux cluster cpu(550Mhz Xeon's connected via Myrinet)
  // ZCPSESTIMATE (1800000)
  // 64 linux cluster cpu(550Mhz Xeon's connected via Myrinet)
  // ZCPSESTIMATE (3200000)
  // 121 linux cluster cpu(550Mhz Xeon's connected via Myrinet)
  // ZCPSESTIMATE (6050000)
  // photon MHD for one cpu alone
  // ZCPSESTIMATE (265000)
  // photon HD 1 cpu
  // ZCPSESTIMATE (400000)
  // rainman MHD for one cpu alone
  // ZCPSESTIMATE (220000)
  // ZCPSESTIMATE (100000)
  // sgi r10000 for one cpu alone(195sMhz)
  // ZCPSESTIMATE (80000)
  // 4cpu mpigm
  // ZCPSESTIMATE (800000)
  // 4cpu r10000
  // ZCPSESTIMATE (343000)
  // 9cpu r10000
  // ZCPSESTIMATE (745000)
  // 16cpu r10000
  // ZCPSESTIMATE (1325000)
  // 25cpu r10000
  // ZCPSESTIMATE (1200000)
  // 36cpu r10000
  // ZCPSESTIMATE (1700000)
  // 49cpu r10000
  // ZCPSESTIMATE (4021000)
  // 64 r10000's 64x64 tile
  // ZCPSESTIMATE (4309000)
  // 121 r10000's
  // ZCPSESTIMATE (10943000)
  // 256 r10000's
  // ZCPSESTIMATE (20000000)
  // kerr 2DMHD
  // ZCPSESTIMATE (50000)
  // rainman 2DMHD
  //  ZCPSESTIMATE=(200000);

  // Latest HARM w/ lim=WENO5BND and FLUXCTSTAG on ki-rh42
  //  ZCPSESTIMATE=(5000);

  // Latest HARM w/ lim=PARA and FLUXCTSTAG on ki-rh42
  //  ZCPSESTIMATE=(10000);
  // Latest HARM w/ lim=PARA and FLUXCTSTAG on 1 Orange CPU
  //  ZCPSESTIMATE=(7000);

  // Latest HARM w/ lim=WENO5BND and FLUXCTTOTH on 1 Orange CPU
  ZCPSESTIMATE=(3200);


}




// not used quite yet
void set_file_versionnumbers(void)
{

  // file versions numbers(use sm for backwards compat)
  PVER= 11;
  GRIDVER= 3; // 3 is without cotangent
  DVER= 1 ;   // dumps same as for pdumps, adumps
  FLVER= 2;
  NPVER= 2;
  AVG1DVER= 2;
  AVG2DVER= 2;
  ENERVER= 7; // 6 is without c/s mode amp in ener, 7 has new ang mom
  MODEVER= 2; // 2 is all vars for 9 modes
  LOSSVER= 7; // 6 has x3 losses, 5 doesn't, 7 has new ang mom losses
  SPVER=   1;
  TSVER=   1;
  LOGDTVER= 1;
  STEPVER= 1;
  PERFVER= 3;
  ADVER= DVER;
  PDVER= DVER;
  CALCVER= 1;
  FLINEVER= 1;
  // type designations for sm automagical read in correct format for similar things
  PTYPE=     1; // global par file
  GRIDTYPE=  2;
  DTYPE=     3 ;// dump
  FLTYPE=    4; // floor
  NPTYPE=    5; // np
  AVG2DTYPE= 6;
  AVG1DTYPE= 7;
  ENERTYPE=  8;
  LOSSTYPE=  9;
  SPTYPE=    10;
  TSTYPE=    11;
  LOGDTTYPE= 12 ;
  STEPTYPE=  13;
  PERFTYPE=  14;
  ADTYPE=    15 ;// same as dump except filename
  PDTYPE=    16; // same as dump except filename
  CALCTYPE=  17; // arbitrary calcs during pp
  FLINETYPE=  18; // field line during pp
  MODETYPE=  19;
  EXPANDTYPE= 50 ;// used to signify doing pp expansion
  NPCOMPUTETYPE= 33; // used to signify want to compute np before output


}






// used to setup local versions of lists
// currently used in interpline.c for NUMPRIMSLOOP()
void  setup_nprlocalist(int whichprimtype, int *nprlocalstart, int *nprlocalend,int *nprlocallist, int *numprims)
{
  int pl,pliter;

  // setup primitive loops
  if(whichprimtype==ENOPRIMITIVE){
    *nprlocalstart=npr2interpstart;
    *nprlocalend=npr2interpend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=npr2interplist[pl];
    *numprims=NPR2INTERP;
  }
  else{ // NPR type
    *nprlocalstart=nprstart;
    *nprlocalend=nprend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=nprlist[pl];
    *numprims=NPR;
  }

}




// initialize MPI and other things
// NO computations should be performed here
int pre_init(int *argc, char **argv[])
{
  int ii;
  int dir,pl,pliter,sc,fl,floor,enerregion;
  int jj;
  int tscale;
  int dissloop;
  int i,j,k;
  extern void set_arrays(void);
  int checki;
  int faketimeorder,fakenumtimeorders;




  // must do in MPI mode or not MPI mode  
  init_MPI_GRMHD(argc, argv);

  // set default performance parameters
  set_defaults_performance_checks_preinit();


#if(USEMPI)
  mpi_set_arrays();
#endif


  // check starting go files (must be after init_mpi so all files know)
  gocheck(STARTTIME);


  report_systeminfo(stderr);
  if(log_file) report_systeminfo(log_file);
  if(myid==0&&logfull_file) report_systeminfo(logfull_file);

  /////////////////
  //
  // setup files for writing and reading (must come after init_MPI_GRMHD())
  //
  makedirs();


  // init arrays
  set_arrays();


  // set default variable to dump (must come before init() where if failed or other reasons can dump output)
  GLOBALPOINT(udump) = GLOBALPOINT(unewglobal);
  //  GLOBALPOINT(ubound) = GLOBALPOINT(unewglobal);
  GLOBALPOINT(pdump) = GLOBALPOINT(pglobal);
  GLOBALPOINT(pstagdump) = GLOBALPOINT(pstagglobal);
  GLOBALPOINT(Bhatdump) = GLOBALPOINT(Bhatglobal);
  GLOBALPOINT(vpotarraydump) = GLOBALPOINT(vpotarrayglobal);
  GLOBALPOINT(pl_ctdump) = GLOBALPOINT(gp_l);
  GLOBALPOINT(pr_ctdump) = GLOBALPOINT(gp_r);





  init_dumps();
  
  // compute default enerregion/section information and initialize ENERREGIONLOOP()
  // initial call is reason why (1)
  faketimeorder=-1;
  fakenumtimeorders=-1;
  recompute_fluxpositions(1,faketimeorder,fakenumtimeorders,nstep,t);



  // must go here b4 restart if restarting
  ENERREGIONLOOP(enerregion){
    // used for each region, related to global quantities
    // no need to initialize _tot quantities, they are overwritten during MPI sum in diag.c
    fladd=fladdreg[enerregion];
    fladdterms=fladdtermsreg[enerregion];
    U_init=Ureg_init[enerregion];
    pcum=pcumreg[enerregion];
    pdot=pdotreg[enerregion];
    pdotterms=pdottermsreg[enerregion];
    sourceaddterms=sourceaddtermsreg[enerregion];
    sourceadd=sourceaddreg[enerregion];
    diss=dissreg[enerregion];

    PLOOP(pliter,pl){
      fladd[pl] = 0;
      FLOORLOOP(floor) fladdterms[floor][pl]=0;
      U_init[pl] = 0;
      DIRLOOP(dir){
        pcum[dir][pl]=0;
        pdot[dir][pl]=0;
        FLLOOP(fl) pdotterms[dir][fl][pl]=0;
        if(enerregion==0) FLLOOP(fl) pdottermsjet2[dir][fl][pl]=0; // needed for other not-flux cpus!
      }
      sourceadd[pl] = 0;
      SCLOOP(sc) sourceaddterms[sc][pl] = 0;
    }
    for(dissloop=0;dissloop<NUMDISSVERSIONS;dissloop++)  diss[dissloop] = 0;

    if(DOLUMVSR) if(enerregion==0) for(ii=0;ii<ncpux1*N1;ii++) lumvsr[ii]=0;
    if(DODISSVSR) if(enerregion==0) for(ii=0;ii<ncpux1*N1;ii++) for(dissloop=0;dissloop<NUMDISSVERSIONS;dissloop++) dissvsr[dissloop][ii]=0;
    if(DOSELFGRAVVSR) if(enerregion==0) for(ii=0;ii<ncpux1*N1;ii++) dMvsr[ii]=0;

    // below quantities have been subsumed into own full enerregion
    //    if(DOEVOLVEMETRIC) if(enerregion==0) PLOOP(pliter,pl){
    //  horizonflux[pl]=0;
    //  horizoncum[pl]=0;
    // }
  }


  
  // start counter
  // full loop since user may choose to count something in boundary zones
  int indexfinalstep;
  if(DODEBUG) FULLLOOP FAILFLOORLOOP(indexfinalstep,tscale,floor) GLOBALMACP0A3(failfloorcount,i,j,k,indexfinalstep,tscale,floor)=0;

  // start du diag_fixup() sum
  // full loop since user may choose to count something in boundary zones
  if(DOFLOORDIAG) FULLLOOP PALLLOOP(pl) GLOBALMACP0A1(failfloordu,i,j,k,pl)=0.0;

#if(CALCFARADAYANDCURRENTS)
  // zero out jcon since outer boundaries not set ever since j^\mu involves spatial derivatives that don't exist outside a certain point
  for(pl=0;pl<NDIM;pl++) FULLLOOP GLOBALMACP0A1(jcon,i,j,k,pl)=0.0;
#endif


  

  return(0);
}

int init_defdefcoord(void)
{
  // set coordinates
  defcoord=LOGRSINTH;

  return(0);
}

int init_defgrid(void)
{
  // sets metric
  //  a=0.0;
  // sets parameters of coordinates, default changes
  R0 = 0.0;
  Rin = 0.98 * Rhor;
  Rout = 40.;
  hslope = 0.3;

  // default black hole parameters, and so length is in GMBH/c^2 and time in GMBH/c^3
  a=a0;
  MBH=MBH0;
  QBH=QBH0;
  EP3=EP30;
  THETAROTMETRIC=THETAROT0;
  THETAROTPRIMITIVES=0.0; // assume by default is zero


  return(0);
}


int init_defglobal(void)
{
  int i;
  int pl,pliter;
  int dtloop;

#if(PRODUCTION==0)
  debugfail=2; // CHANGINGMARK
#else
  debugfail=0; // no messages in production -- assumes all utoprim-like failures need not be debugged
#endif
  // whether to show debug into on failures.  Desirable to turn off if don't care and just want code to burn away using given hacks/kludges
  // 0: no messages
  // 1: critical messages
  // 2: all failure messages

  // included in rdump
  defcon = 1.0;
  /* maximum increase in timestep */
  SAFE=1.3;
  nstep = realnstep = 0;
  whichrestart = 0;
  restartsteps[0] = 0;
  restartsteps[1] = 0;
  whichfake = whichrestart;
  fakesteps[0] = restartsteps[0];
  fakesteps[1] = restartsteps[1];
  failed = 0;
  cour = 0.5;  //atch: modified the courant factor from 0.9
  doevolvemetricsubsteps=0; // default is to evolve on long steps (only applicable if DOEVOLVEMETRIC==1 && EVOLVEMETRICSUBSTEP==2)
  gravityskipstep=0; // default is no skipping
  gravitydtglobal = BIG;
  sourcedtglobal  = BIG;
  wavedtglobal    = BIG;



  //  avgscheme=WENO5BND;
  avgscheme[1]=avgscheme[2]=avgscheme[3]=DONOR;
  
  PALLLOOP(pl) do_transverse_flux_integration[pl] = 1;
  PALLLOOP(pl) do_source_integration[pl] = 1;
  PALLLOOP(pl) do_conserved_integration[pl] = 1;

  INVERTFROMAVERAGEIFFAILED = 1;
  LIMIT_AC_PRIM_FRAC_CHANGE = 1;
  MAX_AC_PRIM_FRAC_CHANGE = 0.1;

  LIMIT_AC_FRAC_CHANGE = 1;
  MAX_AC_FRAC_CHANGE = 0.1;

  PARAMODWENO=0;

  dofluxreconevolvepointfield=1;


  if(DOEVOLVERHO){
    //lim = WENO5FLAT;
    //lim = WENO5BND;
    //  lim = WENO3;
    //lim = DONOR;
    //lim = MINM;
    //  lim = PARA;
    //lim = MC;
    lim[1]=lim[2]=lim[3]=MC;
    //lim = PARA;
    //  lim = PARAFLAT;
    //lim = MC;
    TIMEORDER=4;
    // whether/which ENO used to interpolate fluxes
    DOENOFLUX = ENOFINITEVOLUME;
    //DOENOFLUX= NOENOFLUX;
    //DOENOFLUX=ENOFLUXRECON;
    //  fluxmethod=MUSTAFLUX;
    //fluxmethod=FORCEFLUX;
    fluxmethod=HLLFLUX;
    //fluxmethod=HLLLAXF1FLUX;
    //fluxmethod=LAXFFLUX;
    FLUXB = FLUXCTTOTH;
    UTOPRIMVERSION=UTOPRIM5D1;  //UTOPRIM2DFINAL;
    //UTOPRIMVERSION=UTOPRIM5D2;
    //  UTOPRIMVERSION=UTOPRIM2DFINAL;
  }


  if(EOMTYPE==EOMFFDE){
    // PARA and TO=4 and HLL not trustable in FFDE so far
    lim[1] = lim[2] = lim[3] = MC;
    TIMEORDER=2;
    fluxmethod=LAXFFLUX;
    FLUXB = FLUXCTTOTH;
    UTOPRIMVERSION=UTOPRIM2DFINAL;
    // whether/which ENO used to interpolate fluxes
    //DOENOFLUX = ENOFINITEVOLUME;
    DOENOFLUX= NOENOFLUX;
    //DOENOFLUX=ENOFLUXRECON;
  }


  t = 0.;
  tstepparti = t;
  tsteppartf = t;

  tf = 1.0;
  
  for(dtloop=0;dtloop<NUMDUMPTYPES;dtloop++) DTdumpgen[dtloop]=1.0;
  //  DTd = DTavg = DTdebug = 1.0;
  //  DTener=1.0;
  //  DTi=1.0;
  DTr=1;
  DTfake=MAX(1,DTr/10);



  GAMMIEDUMP=0;// whether in Gammie output types (sets filename with 3 numbers and doesn't output i,j)
  GAMMIEIMAGE=0; // Gammie filename and single density output
  GAMMIEENER=0; // currently doing gener as well as ener, but this would also output some messages in gammie form

  // DOCOLSPLIT
  //
  // 0: don't ..
  // 1: split dump files into 1 column per file with ending number being column number
  // works in MPI-mode as well.  ROMIO+DOCOLSPLIT is useful for tungsten with low memory and small files to avoid diskspace and memory limits.

  // default
  for(i=0;i<NUMDUMPTYPES;i++){
    DOCOLSPLIT[i]=0;
  }
  // otherwise specify for each dump type


  DODIAGEVERYSUBSTEP = 0;
  DOENODEBUGEVERYSUBSTEP = 0;
 

  DODIAGS=1; // whether to do diagnostics
  // specify individual diagnostics to be done

  if(PRODUCTION>=2){ // then disable things not really using
    DOENERDIAG=0;
    // still kinda use images to check if looks reasonable, and already limit images to 1 image file if PRODUCTION==1
  }
  else{
    DOENERDIAG=1;
  }
  DOGDUMPDIAG=1;
  DORDUMPDIAG=1;
  DODUMPDIAG=1;
  if(DOAVG){
    DOAVGDIAG=1; // choice
  }
  else DOAVGDIAG=0; // no choice
  DOIMAGEDIAG=1;
  DOAREAMAPDIAG=1;

  POSDEFMETRIC=0; // see metric.c, bounds.c, and coord.c

  rescaletype=1;
  // 0: normal
  // 1: extended b^2/rho in funnel
  // 2: conserve E and L along field lines
  //   1 or 2 required to conserve E and L along field line

  /** FIXUP PARAMETERS **/
  RHOMIN=1.e-4;
  UUMIN =1.e-6;
  RHOMINLIMIT=SMALL;
  UUMINLIMIT =SMALL;

  // limit of B^2/rho if using that flag
  BSQORHOLIMIT=1E2;
  BSQOULIMIT=1E3;
  UORHOLIMIT=1E3;
  GAMMADAMP=5.0;

  if(DOEVOLVERHO){
    // GODMARK -- unstable beyond about 25, but can sometimes get away with 1000
    GAMMAMAX=25.0; // when we think gamma is just too high and may cause unstable flow, but solution is probably accurate.
  }
  else{
    GAMMAMAX=2000.0;
  }

  GAMMAFAIL=100.0*GAMMAMAX; // when we think gamma is rediculous as to mean failure and solution is not accurate.
  prMAX[RHO]=20.0;
  prMAX[UU]=20.0;
  prMAX[U1]=100.0;
  prMAX[U2]=100.0;
  prMAX[U3]=100.0;
  prMAX[B1]=100.0;
  prMAX[B2]=100.0;
  prMAX[B3]=100.0;

  // AKMARK: parameter: gam (adiabatic index)
  // some physics
  gam=4./3.; // ultrarelativistic gas, assumes pgas<prad and radiation
  gamideal=gam;
  zerouuperbaryon=0.0; // definition of zero-point energy per baryon (i.e. such that internal energy cannot be lower than this times baryon density)

  // doesn't escape
  // gam=5/3 for non-relativistic gas, such as neucleons in collapsar model
  cooling=NOCOOLING;
  // cooling: 0: no cooling 1: adhoc thin disk cooling 2: neutrino cooling for collapsar model

  // boundary conditions (default is for 3D spherical polar grid -- full r,pi,2pi)
  BCtype[X1UP]=OUTFLOW;
  BCtype[X1DN]=OUTFLOW;
  BCtype[X2UP]=POLARAXIS;
  BCtype[X2DN]=POLARAXIS;
  BCtype[X3UP]=PERIODIC;
  BCtype[X3DN]=PERIODIC;

  AVOIDADVANCESHIFTX1DN= 1;
  AVOIDADVANCESHIFTX1UP= 1;
  AVOIDADVANCESHIFTX2DN= 1;
  AVOIDADVANCESHIFTX2UP= 1;
  AVOIDADVANCESHIFTX3DN= 1;
  AVOIDADVANCESHIFTX3UP= 1;
  GLOBALBCMOVEDWITHACTIVESECTION= 0;



  // special3dspc assignment must come after dofull2pi and periodicx? are set in prepre_init()
  // must come before any use of special3dspc such as by init_placeongrid_griddecomposition()
  special3dspc=dofull2pi && N3>1 && IF3DSPCTHENMPITRANSFERATPOLE&&periodicx3&&ISSPCMCOORDNATIVE(MCOORD);
  trifprintf("Using %s 3D polar boundary conditions\n", (special3dspc)?("full"):("limited") );


  return(0);
}


int init_defconsts(void)
{


  // some constants that define neutrino cooling and spin evolution
  // cgs
  msun=1.989E33;
  lsun=3.89E33;
  rsun=695500.0*1E3*1E2;
  C=2.99792458E10;
  G=6.672E-8;

  // erg K^{-1} g^{-1}
  kb=1.3807E-16;
  // defs:
  C=2.99792458E10;
  amu=1.660538782E-24;
  qe=4.8032068E-10;
  Na=1/amu;
  // http://physics.nist.gov/cgi-bin/cuu/Convert?exp=0&num=1&From=ev&To=kg&Action=Convert+value+and+show+factor
  ergPmev = 1.782661758E-30*1.0E3*C*C;
  //
  // http://en.wikipedia.org/wiki/Proton
  // http://physics.nist.gov/cgi-bin/cuu/Value?mp
  mp=1.672621637E-24;
  // NIST:
  //me=9.10938188E-28
  me=9.10938215E-28;
  // http://physics.nist.gov/cgi-bin/cuu/Value?mn|search_for=atomnuc!
  mn=1.674927211E-24;
  //
  malpha=6.64465598E-24;
  // some constants
  //hpl=6.6262E-27
  //
  hpl=4.13566733E-15*1E-6*ergPmev;
  //

  //  mn=1.67492716E-24;
  //  mp=1.67262158E-24;
  //  me=9.10938188E-28;
  kb=1.380658E-16; // erg K^{-1} g^{-1}
  Q=(mn-mp)*C*C;
  R=kb/mp;
  Re=kb/me;
  //  hpl=6.6260755E-27;
  H=hpl;
  hbar=hpl/(2.0*M_PI);
  K=1.24E15;
  K2=9.9E12;
  //
  // compare below
  arad=8.*pow(M_PI,5.0)*pow(kb,4.0)/(15*C*C*C*H*H*H);
  //  arad=5.6704E-5 * 4.0 / C;
  sigmasb=arad*C/4.0;
  sigmamat=6.652E-29*100*100;
  mevocsq=1.783E-27;


  // default for units is GM=C=1
  // GRB
  //  mb=mn;
  // definition of baryon mass uses the below
  mb = amu;

  Mcgs=3.*msun;
  Mdot=0.1*msun;
  Mdotc=0.35; // approx code units mass accretion rate
  
  // units
  Lunit=G*Mcgs/(C*C);
  Tunit=G*Mcgs/(C*C*C);
  Vunit=Lunit/Tunit;
  Ccode=C/Vunit;
  rho0unittype=0; // mass is mass
  rhounit=Mdot/(Mdotc*Lunit*Lunit*Vunit);
  mbwithrhounit=mb;
  rhomassunit=rhounit;
  Munit=rhounit*pow(Lunit,3.0);
  mdotunit=Munit/Tunit;
  energyunit=Munit*Vunit*Vunit;
  edotunit=energyunit/Tunit;
  Pressureunit=rhounit*Vunit*Vunit;
  Tempunit=Pressureunit*mb/(rhounit*kb);
  Bunit=Vunit*sqrt(rhounit);
  massunitPmsun=Munit/msun;

  
  // physics stuff
  ledd=4.*M_PI*C*G*Mcgs*mb/sigmamat;
  leddcode=ledd/edotunit;

  // normalizations and settings for metric changes 
  a0=0.0;
  MBH0=1.0;
  QBH0=0.0;
  EP30=0.0;
  THETAROT0=0.0;
  Mfactor=1.0;
  Jfactor=1.0;
  rhofactor=1.0;

  dabh=0.0;
  dE=0.0;
  dJ=0.0;


  return(0);
}







int post_init(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*faraday)[NSTORE2][NSTORE3][NUMCURRENTSLOTS][3])
{
  int compute_currents_t0(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*faraday)[NSTORE2][NSTORE3][NUMCURRENTSLOTS][3]);


  trifprintf("begin: post_init\n");


  // need to compute various things for EOS
  // also done per timestep in step_ch.c
  // GODMARK: Check that no EOS call is before this point 
  compute_EOS_parms_full(WHICHEOS, GLOBALPOINT(EOSextraglobal),prim);



  // in synch always here
  if (error_check(3)) {
    dualfprintf(fail_file, "error_check detected failure at main:1\n");
    dualfprintf(fail_file, "Bad initial conditions\n");
    myexit(1);
  }


  // all calculations that do not change for any initial conditions or problem setup or restart conditions

 
  // don't recompute these things if doing metric evolution on substeps
  if(RESTARTMODE!=0){
    setfailresponse(restartonfail);
  }
  
  
  ////////////////
  // current
  compute_currents_t0(prim,faraday);



  
  trifprintf("end: post_init\n");


  return(0);

}


// bounds, etc. use enerpos type stuff that is set by recompute_fluxpositions()
int set_box_grid_parameters(void)
{
  int faketimeorder,fakenumtimeorders;

  // need to recompute horizon-related quantities if horizon is growing due to accretion
  if(ISBLACKHOLEMCOORD(MCOORD)){
    find_horizon(0);
    trifprintf("Rhor=%21.15g Risco=%21.15g MBH=%21.15g a=%21.15g QBH=%21.15g EP3=%21.15g THETAROT=%21.15g\n",Rhor,Risco,MBH,a,QBH,EP3,THETAROT);
  }
  else{
    horizoni=horizoncpupos1=0;
  }

  // not the initial call -- uses horizon information
  // By this point the code expects user to be able to specify ACTIVEREGION domain
  // however, if doing grid sectioning still need to have full COMP loops (hence 2 and not 1 below)
  faketimeorder=-1;
  fakenumtimeorders=-1;
  recompute_fluxpositions(2,faketimeorder,fakenumtimeorders,nstep,t);



  return(0);

}


int compute_currents_t0(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*faraday)[NSTORE2][NSTORE3][NUMCURRENTSLOTS][3])
{

#if(CALCFARADAYANDCURRENTS)
    
#if(CURRENTST0)
  // setup faraday so t=0 dump has diagnostics
  // may want currents for dump=0 (time-derivative terms will be wrong)
  current_doprecalc(CURTYPET,prim);
  current_doprecalc(CURTYPEX,prim);
  current_doprecalc(CURTYPEY,prim);
  current_doprecalc(CURTYPEZ,prim);
  // compute current
  current_calc(faraday);
  
#else
  
  // need to at least compute t=0 faraday so currents can start to be computed before reach step_ch.c (just an ordering issue with how step_ch.c does this case)
  if(WHICHCURRENTCALC==1){
    // compute faraday components needed for time centering of J
    current_doprecalc(CURTYPET,prim);
  }
#endif
  
#if(FARADAYT0)
  current_doprecalc(CURTYPEFARADAY,prim);
#endif
  
#endif


  return(0);

}



// set certain arrays to zero for diagnostic purposes (must come after all basic init stuff that sets up grid parms and all other parms)
int init_arrays_before_set_pu(void)
{
  int i,j,k,pliter,pl;
  int jj;

  ///////////////
  // 0 out these things so dump files are readable by SM under any cases
  ///////////////
  FULLLOOP{

    if(FLUXB==FLUXCTSTAG){
      // then pl=B1,B2,B3 actually should be correct (i.e. not NaN), so don't reset those so nan-check works
      PLOOP(pliter,pl){
        if(! (pl==B1 || pl==B2 || pl==B3) ){
          GLOBALMACP0A1(udump,i,j,k,pl)=0.0;
        }
      }
    }
    else if(DOENOFLUX != NOENOFLUX){
      // then all pl actually should be correct (i.e. not NaN), so don't reset those so nan-check works
    }
    else{
      PLOOP(pliter,pl) GLOBALMACP0A1(udump,i,j,k,pl)=0.0;
    }


#if(CALCFARADAYANDCURRENTS)
    DLOOPA(jj) GLOBALMACP0A1(jcon,i,j,k,jj)=0.0;
    for(pl=0;pl<NUMFARADAY;pl++) GLOBALMACP0A1(fcon,i,j,k,pl)=0.0;
#endif

  }


  return(0);

}





// after all parameters are set, can call this
int post_par_set(void)
{
  int interp_loop_set(void);
  int orders_set(void);
  void check_bnd_num(void);





  // if higher order, then set useghostplusactive to 1 since need that region in general
  if(HIGHERORDERMEM && (DOENOFLUX==ENOFINITEVOLUME || DOENOFLUX==ENOFLUXRECON) ){

    useghostplusactive=1;
  }
  else{
    useghostplusactive=0;
  }


  if(useghostplusactive && DOENOFLUX == ENOFLUXRECON && FLUXB==FLUXCTSTAG && dofluxreconevolvepointfield==0){
    // no matter how this parameter is set, we reset it to 
    extrazones4emf=1;
  }
  else extrazones4emf=0;

  if(SPLITMAEMMEM && HIGHERORDERMEM && DOENOFLUX == ENOFLUXRECON){
    splitmaem=1;
  }
  else splitmaem=0;

  if(splitmaem==0 && dofluxreconevolvepointfield==1 && DOENOFLUX == ENOFLUXRECON && FLUXB == FLUXCTSTAG && SPLITPRESSURETERMINFLUXMA==0 && SPLITPRESSURETERMINFLUXEM==0){
    // when not splitting MA/EM terms and doing FLUXRECON point field method, then need to still fix stencil for EMF terms
    emffixedstencil=1;
  }

  
  if(useghostplusactive && ((DOENOFLUX == ENOFLUXRECON && FLUXB==FLUXCTSTAG && dofluxreconevolvepointfield==0) || DOENOFLUX==ENOFINITEVOLUME)){
    // no matter how this parameter is set, we reset it to 
    unewisavg=1;
  }
  else unewisavg=0;


  trifprintf("useghostplusactive=%d extrazones4emf=%d\n",useghostplusactive,extrazones4emf);


  trifprintf("Setting orders\n");
  orders_set();

  trifprintf("Boundary number checks\n");
  check_bnd_num();

  trifprintf("Setting interp loop\n");
  interp_loop_set();

  trifprintf("Reporting bound loop\n");
  report_bound_loop();


  return(0);
}





// check that there are enough boundary zones for interpolation order used
void check_bnd_num(void)
{
  int totalo;
  int get_num_bnd_zones_used(int dir);
  int dimen;
  int bndnum[NDIM];
  int doingdimen[NDIM];
  int pl,pliter;
  int doingenough;


  doingdimen[1]=N1NOT1;
  doingdimen[2]=N2NOT1;
  doingdimen[3]=N3NOT1;


  bndnum[1]=N1BND;
  bndnum[2]=N2BND;
  bndnum[3]=N3BND;


  DIMENLOOP(dimen){

    // first fix avgscheme and lim for case when not doing dimension
    if(!doingdimen[dimen]){
      avgscheme[dimen]=0;
      lim[dimen]=0;
    } // otherwise assume as wanted

    // get number of boundary zones needed
    totalo=get_num_bnd_zones_used(dimen);

  
    // bndnum[dimen] (MAXBND is maximum) is the number of boundary zones to be set by boundary routines and to be passed by MPI routines
    if(totalo==bndnum[dimen] || !doingdimen[dimen]){
      // then good
    }
    else if(totalo>bndnum[dimen]){
      dualfprintf(fail_file,"Not enough: dimen=%d totalo=%d MAXBND=%d bndnum=%d for avgscheme interporder[%d]=%d or lim interporder[%d]=%d extrazones4emf=%d\n",dimen,totalo,MAXBND,bndnum[dimen],avgscheme[dimen],interporder[avgscheme[dimen]],lim[dimen],interporder[lim[dimen]],extrazones4emf);
      failed=1; // so don't try to compute things in dump
      myexit(1);
    }
    else{
      // then MAXBND too large
      dualfprintf(fail_file,"WARNING: MAXBND excessive\n");
      dualfprintf(fail_file,"WARNING: dimen=%d totalo=%d MAXBND=%d and bndnum=%d for avgscheme interporder[%d]=%d or lim interporder[%d]=%d extrazones4emf=%d\n",dimen,totalo,MAXBND,bndnum[dimen],avgscheme[dimen],interporder[avgscheme[dimen]],lim[dimen],interporder[lim[dimen]],extrazones4emf);
      // not a failure, but user should be aware especially when doing MPI
    }
    
  }


  // see if need to be here at all                                                                                         
  if(interporder[avgscheme[1]]<3 && interporder[avgscheme[2]]<3 && interporder[avgscheme[3]]<3){
    PALLLOOP(pl) do_transverse_flux_integration[pl] = 0;
    PALLLOOP(pl) do_source_integration[pl] = 0;
    PALLLOOP(pl) do_conserved_integration[pl] = 0;
    
    trifprintf("Changed do_{conserved/source/transverse_flux}_integration to 0 since avgscheme[1]=%d avgscheme[2]=%d avgscheme[3]=%d\n",avgscheme[1],avgscheme[2],avgscheme[3]);
  }


  
  // checks on parameters so user doesn't do something stupid
  if(FULLOUTPUT&&USEMPI&&(numprocs>1 || mpicombine==1)){
    dualfprintf(fail_file,"Cannot use FULLOUTPUT!=0 when USEMPI=1 and (numprocs>1 || mpicombine==1)\n");
    myexit(ERRORCODEBELOWCLEANFINISH+200);
  }

  if(DOEVOLVEMETRIC&& (ANALYTICCONNECTION||ANALYTICSOURCE)){
    dualfprintf(fail_file,"Unlikely you have metric in time analytically\n");
    myexit(ERRORCODEBELOWCLEANFINISH+201);
  }


  if(DOSELFGRAVVSR && (ANALYTICCONNECTION||ANALYTICSOURCE||ANALYTICGCON)){
    dualfprintf(fail_file,"Unlikely you have metric analytically with self gravity\n");
    myexit(ERRORCODEBELOWCLEANFINISH+202);
  }

  if(GDETVOLDIFF){
    if(ISSPCMCOORDNATIVE(MCOORD)){
      // then fine
    }
    else{
      dualfprintf(fail_file,"GDETVOLDIFF==1 not setup for non-SPC metrics\n");
      myexit(ERRORCODEBELOWCLEANFINISH+203);
    }
  }

  if(DOSELFGRAVVSR==0 && (MCOORD==KS_BH_TOV_COORDS || MCOORD==KS_TOV_COORDS || MCOORD==BL_TOV_COORDS) ){
    dualfprintf(fail_file,"DOSELFGRAVVSR==0 with MCOORD=%d is not likely what you want\n",MCOORD);
    dualfprintf(fail_file,"Continuing....\n");
  }


  

  if(HIGHERORDERMEM==0 && DOENOFLUX != NOENOFLUX){
    dualfprintf(fail_file,"Need to turn on HIGHERORDERMEM when doing higher order methods (i.e. DOENOFLUX(=%d)!=NOENOFLUX)\n",DOENOFLUX);
    myexit(ERRORCODEBELOWCLEANFINISH+204);
  }

  if(COMPDIM!=3){
    dualfprintf(fail_file,"Code not setup for anything but COMPDIM==3\n");
    myexit(ERRORCODEBELOWCLEANFINISH+205);
  }


  if(FIELDSTAGMEM==0 && FLUXB==FLUXCTSTAG){
    dualfprintf(fail_file,"FIELDSTAGMEM should be 1 if FLUXB==FLUXCTSTAG\n");
    myexit(ERRORCODEBELOWCLEANFINISH+206);
  }

  if(FIELDSTAGMEM==1 && FLUXB!=FLUXCTSTAG){
    dualfprintf(fail_file,"WARNING: FIELDSTAGMEM==1 will be slower with non-stag method\n");
  }

  if(FIELDTOTHMEM==0 && FLUXB==FLUXCTTOTH){
    dualfprintf(fail_file,"FIELDTOTHMEM should be 1 if FLUXB==FLUXCTTOTH\n");
    myexit(ERRORCODEBELOWCLEANFINISH+207);
  }


  //  if( (N1%2>0 && N1>1) || (N2%2>0 && N2>1) || (N3%2>0 && N3>1) ){
  if( (N2%2>0 && N2>1) || (N3%2>0 && N3>1) ){
    dualfprintf(fail_file,"N1, N2, N3 should be even since some parts of code assume so\n");
    myexit(ERRORCODEBELOWCLEANFINISH+208);
  }


  if(SUPERLONGDOUBLE){
    dualfprintf(fail_file,"If doing SUPERLONGDOUBLE, then should have compiled as such\n");
  }
  
  if(ROEAVERAGEDWAVESPEED || ATHENAROE){
    dualfprintf(fail_file,"ATHENA stuff only for non-rel setup and while was tested hasn't been used or kept up to date\n");
    myexit(ERRORCODEBELOWCLEANFINISH+209);
  }

  if(STOREWAVESPEEDS==0 && FLUXB==FLUXCTSTAG){
    dualfprintf(fail_file,"Must set STOREWAVESPEEDS==1 when doing FLUXCTSTAG\n");
    myexit(ERRORCODEBELOWCLEANFINISH+210);
  }

  if(LIMADJUST!=LIMITERFIXED){
    dualfprintf(fail_file,"LIMADJUST old code\n");
    myexit(ERRORCODEBELOWCLEANFINISH+211);
  }


  if(FLUXADJUST!=FLUXFIXED){
    dualfprintf(fail_file,"FLUXADJUST old code\n");
    myexit(ERRORCODEBELOWCLEANFINISH+212);
  }

  if(DOEVOLVEMETRIC&& (WHICHEOM!=WITHGDET )){
    dualfprintf(fail_file,"conn2 not computed for time-dependent metric yet\n");
    myexit(ERRORCODEBELOWCLEANFINISH+213);
  }

  if(CONNMACHINEBODY){

    if(WHICHEOM!=WITHGDET){
      dualfprintf(fail_file,"Not setup for body correction when f is not detg\n");
      myexit(ERRORCODEBELOWCLEANFINISH+215);
    }
  }

  if(CONNMACHINEBODY&&(DOENOFLUX!=NOENOFLUX || MERGEDC2EA2CMETHOD)){
    dualfprintf(fail_file,"WARNING: MACHINEBODY with higher order scheme makes no sense\n");
  }



  if(CONTACTINDICATOR!=0){
    dualfprintf(fail_file,"Contact not recommended\n");
  }

  if(FLUXDUMP!=0){
    dualfprintf(fail_file,"FLUXDUMP ACTIVE -- lots of extra output\n");
  }

  if(NUMPOTHER!=0){
    dualfprintf(fail_file,"NUMPOTHER ACTIVE -- lots of extra memory used\n");
  }

  if(LIMIT_FLUXC2A_PRIM_CHANGE){
    dualfprintf(fail_file,"LIMIT_FLUXC2A_PRIM_CHANGE doesn't work according to Sasha, so don't use\n");
    myexit(ERRORCODEBELOWCLEANFINISH+216);
  }

  // check if 
  doingenough=1;
  DIMENLOOP(dimen){
    doingenough *= (interporder[avgscheme[dimen]]>=interporder[WENO5BNDPLUSMIN] || doingdimen[dimen]==0);
  }

  if(WENO_EXTRA_A2C_MINIMIZATION==1 && doingenough==0){
    dualfprintf(fail_file,"WENO_EXTRA_A2C_MINIMIZATION==1 and interporder=%d %d %d invalid\n",interporder[avgscheme[1]],interporder[avgscheme[2]],interporder[avgscheme[3]]);
    myexit(ERRORCODEBELOWCLEANFINISH+217);
  }



  DIMENLOOP(dimen){
    if(avgscheme[dimen]!=DONOR){ // using DONOR just turns off and assume standard way to turn off so no need to message user
      if(avgscheme[dimen]<FIRSTWENO || avgscheme[dimen]>LASTWENO){
        dualfprintf(fail_file,"Choice of avgscheme[%d]=%d has no effect\n",dimen,avgscheme[dimen]);
      }
    }
  }

  
  //  if( (FLUXB==FLUXCTHLL || FLUXB==FLUXCTTOTH || (FLUXB==FLUXCTSTAG && extrazones4emf==1 && )) && EVOLVEWITHVPOT){ // avoid complicated conditional
  if(EVOLVEWITHVPOT && !(FLUXB==FLUXCTSTAG && extrazones4emf==0) ){
    // Even with FV or FLUXRECON method that relies on updating non-point fields, can't evolve with A_i since truncation error different rather than just machine error different.  This leads to large errors -- especially at boundaries? GODMARK -- not 100% sure this is the problem for test=1102 and EVOLVEWITHVPOT
    dualfprintf(fail_file,"Cannot evolve field using A_i for FLUXB==FLUXCTHLL or FLUXB==FLUXCTHLL since no single A_i evolved forward in time.  And cannot use with non-point field method since while single A_i updated, the update diverges at truncation error between updating A_i and updating the non-point-field -- this is especially bad for periodic boundary conditions where one must have machine error correct behavior at boundaries\n");
    myexit(ERRORCODEBELOWCLEANFINISH+218);
  }


  if(splitmaem==0 && (SPLITPRESSURETERMINFLUXMA || SPLITPRESSURETERMINFLUXEM)){
    dualfprintf(fail_file,"To use SPLITPRESSURE must turn on splitmaem, which requires FLUXRECON\n");
    myexit(ERRORCODEBELOWCLEANFINISH+219);
  }

  if(splitmaem==1){
    dualfprintf(fail_file,"Noticed some tests are more accurate if don't split (splitmaem==0), like test=1102 in init.sasha.c when doing non-point field FLUXRECON method the density error is smaller by factor of 2 with splitmaem==0\n");
  }

  // when using FLUXRECON, WENO5BND lim/avg:
  // 32x16: splitmaem==1 (pressure split or not!): 5.164e-05   0.0005637    0.001195    0.001326    0.003499   0.0001439   0.0001438    0.000415
  // 32x16: splitmaem==0 (higherordersmooth or rough!): 8.369e-06   0.0005615    0.001208    0.001224    0.002977   0.0001433   0.0001431   0.0003529


  if(ISSPCMCOORD(MCOORD) && ACCURATESINCOS==0){
    dualfprintf(fail_file,"Warning: if polar axis or r=0 singularity don't have \\detg=0 you should force it -- normally ACCURATESINCOS==1 does a good job of this, but still should have code to check\n");
  }


  if(FULLOUTPUT!=0){
    // then ensure that boundary zones are off if necessary

    if(DOLUMVSR){
      dualfprintf(fail_file,"lumvsr requires no boundary zones outputted so dump and lumvsr file can be used together\n");
      myexit(ERRORCODEBELOWCLEANFINISH+195815983);
    }
    if(DODISSVSR){
      dualfprintf(fail_file,"dissvsr requires no boundary zones outputted so dump and dissvsr file can be used together\n");
      myexit(ERRORCODEBELOWCLEANFINISH+195815984);
    }

  }



  if(2*N1<N1BND || N1BND!=0 && N1==1 || periodicx1==1 && ncpux1>1 && N1<N1BND){
    dualfprintf(fail_file,"Code not setup to handle boundary cells N1BND=%d with only N1=%d\n",N1BND,N1);
    myexit(ERRORCODEBELOWCLEANFINISH+246872462);
  }
  if(2*N2<N2BND || N2BND!=0 && N2==1 || periodicx2==1 && ncpux2>1 && N2<N2BND){
    dualfprintf(fail_file,"Code not setup to handle boundary cells N2BND=%d with only N2=%d\n",N2BND,N2);
    myexit(ERRORCODEBELOWCLEANFINISH+246872463);
  }
  if(2*N3<N3BND || N3BND!=0 && N3==1 || periodicx3==1 && ncpux3>1 && N3<N3BND){
    dualfprintf(fail_file,"Code not setup to handle boundary cells N3BND=%d with only N3=%d\n",N3BND,N3);
    myexit(ERRORCODEBELOWCLEANFINISH+246872464);
  }

  //  if(N1%2 && N1>1 || N2%2 && N2>1 || N3%2 && N3>1){
  if(N2%2 && N2>1 || N3%2 && N3>1){
    dualfprintf(fail_file,"Need even N1,N2,N3 AFAIK N1=%d N2=%d N3=%d\n",N1,N2,N3);
    myexit(ERRORCODEBELOWCLEANFINISH+19846286);
  }


  if(SENSITIVE!=LONGDOUBLETYPE){
    dualfprintf(fail_file,"WARNING: With SENSITIVE!=LONGDOUBLETYPE you may have problems for some integral or counting quantities (e.g. DTd too small or many zones to integrate over)\n");
  }



#if(MERGEDC2EA2CMETHOD==1)
  //  if(splitmaem){
  //    dualfprintf(fail_file,"MERGEDC2EA2CMETHOD==1 is not setup for splitmaem since it was assumed splitmaem only needed with old a2c method\n");
  //    myexit(ERRORCODEBELOWCLEANFINISH+346897346);
  //  }
#endif

#if(MERGEDC2EA2CMETHOD==1 && STOREFLUXSTATE==0)
  dualfprintf(fail_file,"Must store flux state (STOREFLUXSTATE 1) if doing merged method\n");
  myexit(9842511);
#endif



  if(PRODUCTION>0){
    if(DOENOFLUX != NOENOFLUX && FLUXB==FLUXCTSTAG){
      dualfprintf(fail_file,"NOTE: With PRODUCTION>0, higher-order staggered field method won't compute ener file value of divB correctly because turned off diagnostic bounding to avoid excessive MPI calls to bound unew.  dump file will still be correct for MPI boundaries but not for real boundaries since unew not defined to be bounded by user\n");
    }
  }


  if(PRODUCTION==0){
    dualfprintf(fail_file,"WARNING: PRODUCTION set to 0, code may be slower\n");
  }

  if(LIMITDTWITHSOURCETERM){
    dualfprintf(fail_file,"WARNING: LIMITDTWITHSOURCETERM set to 1, code may be slower\n");
  }

  //  if(LIMITDTWITHSOURCETERM){
  //    dualfprintf(fail_file,"LIMITDTWITHSOURCETERM==1 doesn't work right now\n");
  //    myexit(ERRORCODEBELOWCLEANFINISH+54986456);
  //  }

  if(DODISS || DOLUMVSR || DODISSVSR || DOENTROPY!=DONOENTROPY){
    dualfprintf(fail_file,"WARNING: DODISS/DOLUMVSR/DODISSVSR/DOENTROPY!=DONOENTROPY set to 1, code may be slower\n");
  }

  if(CHECKONINVERSION){
    dualfprintf(fail_file,"WARNING: CHECKONINVERSION set to 1, code may be slower\n");
  }


  if(CHECKSOLUTION){
    dualfprintf(fail_file,"WARNING: CHECKSOLUTION set to 1, code may be slower\n");
  }


  DIMENLOOP(dimen){
    if(interporder[lim[dimen]]-1 > TIMEORDER){
      dualfprintf(fail_file,"WARNING: interporder[dimen=%d lim=%d]=%d -1 > TIMEORDER=%d is unstable in region where Courant condition setting dt\n",dimen,lim[dimen],interporder[lim[dimen]],TIMEORDER);
    }
  }

  if(DOINGLIAISON && USEMPI==0){
    dualfprintf(fail_file,"WARNING: DOINGLIAISON==1 but USEMPI==0\n");
  }


  // complain if b^2/rho b^2/u or u/rho too large for given resolution given experience with GRMHD torus problem
  DIMENLOOP(dimen){
    if(BSQORHOLIMIT/30.0>((FTYPE)totalsize[dimen])/64.0){
      dualfprintf(fail_file,"WARNING: BSQORHOLIMIT=%21.15g for totalsize[%d]=%d\n",BSQORHOLIMIT,dimen,totalsize[dimen]);
    }
    if(BSQOULIMIT/100.0>((FTYPE)totalsize[dimen])/64.0){
      dualfprintf(fail_file,"WARNING: BSQOULIMIT=%21.15g for totalsize[%d]=%d\n",BSQOULIMIT,dimen,totalsize[dimen]);
    }
    if(UORHOLIMIT/100.0>((FTYPE)totalsize[dimen])/64.0){
      dualfprintf(fail_file,"WARNING: UORHOLIMIT=%21.15g for totalsize[%d]=%d\n",UORHOLIMIT,dimen,totalsize[dimen]);
    }
  }

  if(MERGEDC2EA2CMETHOD && DOENOFLUX != NOENOFLUX){
    dualfprintf(fail_file,"Cannot do merged method with older weno-type c2a/a2c methods\n");
    myexit(ERRORCODEBELOWCLEANFINISH+289754897);
  }

  if((MERGEDC2EA2CMETHOD==0) && (MERGEDC2EA2CMETHODMA==1 || MERGEDC2EA2CMETHODEM==1) ){
    dualfprintf(fail_file,"MERGEDC2EA2CMETHOD,MA,EM set inconsistently: %d %d %d\n",MERGEDC2EA2CMETHOD,MERGEDC2EA2CMETHODMA,MERGEDC2EA2CMETHODEM);
    myexit(ERRORCODEBELOWCLEANFINISH+289754898);
  }



  if(special3dspc && ncpux3==1 && (N3%2)){
    dualfprintf(fail_file,"Must have even N3 (N3=%d) if special3dspc==1 && ncpux3=1\n",N3);
    myexit(ERRORCODEBELOWCLEANFINISH+83746837);
  }


  if(DOENOFLUX == ENOFINITEVOLUME && (DODISS || DODISSVSR)){
    dualfprintf(fail_file,"WARNING: FV method does not allow accurate tracking of dissipation as code is currently written.  Overestimates compared to actual dissipation.\n");
  }


  if(FLUXB==FLUXCTSTAG && special3dspc && (COORDSINGFIX==0 || SINGSMALL<=0.0)){
    dualfprintf(fail_file,"IF3DSPCTHENMPITRANSFERATPOLE==1 with N3>1 requires COORDSINGFIX>0 and SINGSMALL>0.0 in order for B2 to be evolved properly at the pole\n");
    myexit(ERRORCODEBELOWCLEANFINISH+978343943);
  }

  if(FLUXB!=FLUXCTSTAG && COORDSINGFIX==1 ){
    dualfprintf(fail_file,"WARNING: No need to have COORDSINGFIX==1 with non-staggered field\n");
  }

  if(COORDSINGFIX==1 && (N3==1) && ISSPCMCOORDNATIVE(MCOORD)){
    dualfprintf(fail_file,"WARNING: No need to have COORDSINGFIX==1 with 2D\n");
  }

  if(N1==1 && ncpux1>1 ||N2==1 && ncpux2>1 ||N3==1 && ncpux3>1){
    dualfprintf(fail_file,"Must have N?>1 if ncpux?>1 for code to recognize that dimension\n");
    myexit(ERRORCODEBELOWCLEANFINISH+28347525);
  }


  DIMENLOOP(dimen){
    if(WENOMEMORY==0 && ( WENOINTERPTYPE(lim[dimen])||WENOINTERPTYPE(avgscheme[dimen]))){
      dualfprintf(fail_file,"Need to turn on WENOMEMORY when lim or avgscheme  = eno or weno\n");
      myexit(ERRORCODEBELOWCLEANFINISH+83763463);
    }
  }


  if(USEOPENMP){
    dualfprintf(fail_file,"WARNING: MYFUN function tracing for failures turned off because not allowed inside OpenMP constructs\n");
  }

  // GODMARK: No obvious way to check for ANALYTICMEMORY==0 since depends on user bounds.  Depend upon user to check for this.


  if(ALLOWKAZEOS && USEOPENMP){
    dualfprintf(fail_file,"WARNING: If not using Kaz EOS, then setting ALLOWKAZEOS==1 will introduce more OpenMP overhead\n");
  }

  if(numopenmpthreads==1 && USEOPENMP==1){
    dualfprintf(fail_file,"WARNING: Using only 1 OpenMP thread: Excessive overhead due to pragma's: Recommend turning off USEOPENMP in makehead.inc.\n");
  }

  if(DOGRIDSECTIONING && FLUXB==FLUXCTTOTH){
    dualfprintf(fail_file,"******SUPERWARNING******: If reveal region with Toth method, then divb can't be preserved due to how fluxes are averaged.\n");
  }

  if(DOGRIDSECTIONING && FLUXB==FLUXCTSTAG){
    dualfprintf(fail_file,"******SUPERWARNING******: divb won't be zero for absorbed regions when using AVOIDADVANCESHIFTX???==1 as required since need those regions to inject solution with arbitrary velocity profile.\n");
  }

  if(ANALYTICGCON==0){
    dualfprintf(fail_file,"******SUPERWARNING******: If far from black hole, even with apparently g^{t\\phi}\\sim 0 at the 10^{-34} level, still leads to exponential spurious growth in u^3 u_3 and B^3 to catastrophic levels!!  For KS metric with a\\neq 0, best to use analytical gcon that leads to exactly g^{t\\phi}=0 and so \\beta[\\phi]=0.\n");
  }

  if(CONNDERTYPE!=DIFFNUMREC){
    dualfprintf(fail_file,"WARNING: Using inaccurate CONNDERTYPE can lead to problems, such as force errors near poles can lead to secular errors growing.\n");
  }
  else{
    dualfprintf(fail_file,"WARNING: Using DIFFNUMREC can be very slow, but more accurate than DIFFGAMMIE.\n");
  }

  if(WHICHEOM==WITHGDET){
    dualfprintf(fail_file,"WARNING: WHICHEOM==WITHGDET is inferior to WHICHEOM==WITHNOGDET and setting NOGDETU1=NOGDETU2=1 near the poles where force balance is difficult to ensure.  If pressure constant, then NOGDET method ensures force balance between flux and source terms.\n");
  }
  
  if(STOREWAVESPEEDS==1){
    dualfprintf(fail_file,"WARNING: STOREWAVESPEEDS==1 has been found to be unstable due to how Riemann solver uses max-averaged wavespeeds.  You should use STOREWAVESPEEDS==2 or 0 .\n");
  }
  

  if(DOENTROPY==DONOENTROPY && EOMTYPE==EOMENTROPYGRMHD){
    dualfprintf(fail_file,"ERROR: Must have DOENTROPY enabled to use EOMENTROPYGRMHD\n");
    myexit(34897562);
  }

  if(DOENTROPY==DONOENTROPY && HOT2ENTROPY && EOMTYPE!=EOMCOLDGRMHD){
    dualfprintf(fail_file,"ERROR: Must have DOENTROPY enabled to use HOT2ENTROPY\n");
    myexit(13892345);
  }


  if(UTOPRIMVERSION == UTOPRIM5D1 && EOMTYPE==EOMENTROPYGRMHD){
    dualfprintf(fail_file,"SUPERWARNING: Old 5D method often fails to find solution where solution to inversion does exist.  This can readily lead to completely wrong solutions due to failure fixups.\n");
    myexit(3987634);
  }


  if(WHICHEOS==KAZFULL && EOMTYPE==EOMCOLDGRMHD){
    dualfprintf(fail_file,"For COLD GRMHD, turn off Kaz EOS\n");
    myexit(9782362);
  }

  if(EOMTYPE==EOMCOLDGRMHD && DOENTROPY == DOEVOLVEENTROPY && WHICHEOS!=COLDEOS){
    dualfprintf(fail_file,"For COLD GRMHD with fake entropy tracking, ensure WHICHEOS==COLDEOS\n");
    myexit(872762211);
  }


  if(TRACKVPOT==0 && EVOLVEWITHVPOT==1){
    dualfprintf(fail_file,"Must turn on TRACKVPOT if EVOLVEWITHVPOT==1\n");
    myexit(38974632);
  }


  if(ALLOWMETRICROT==1 && (CONNAXISYMM==1 || DOMIXTHETAPHI==0)){
    dualfprintf(fail_file,"Generally must set CONNAXISYMM==0 and DOMIXTHETAPHI==1 if ALLOWMETRICROT==1 -- only special cases would override this.\n");
    myexit(2467346463);
  }







  // external checks
  parainitchecks();



}


int get_num_bnd_zones_used(int dimen)
{
  int avgo;
  int interpo;
  int totalo;
  int doingdimen[NDIM];
  int extraavgo;



  doingdimen[1]=N1NOT1;
  doingdimen[2]=N2NOT1;
  doingdimen[3]=N3NOT1;


  if(useghostplusactive){

    if(doingdimen[dimen]){
      // number of zones one way for finite volume scheme to convert Uavg -> Upoint
      avgo=(interporder[avgscheme[dimen]]-1)/2;
    }
    else avgo=0; // nothing done for this dimension, so no avg zones
  }
  else avgo=0; // no need for extra zones

  if(extrazones4emf){
    if(doingdimen[dimen]){
      // number of zones one way for finite volume scheme to convert Uavg -> Upoint
      extraavgo=(interporder[avgscheme[dimen]]-1)/2;
    }
    else extraavgo=0; // nothing done for this dimension, so no avg zones
  }
  else extraavgo=0; // no need for extra zones


  // number of zones one way to have for interpolation to get fluxes
  // need to get flux at i,j,k=-1 in any case, and need boundary zones from there, so 1 extra effective boundary zone for interpolation
  if(doingdimen[dimen]){
    interpo=(interporder[lim[dimen]]-1)/2+1;
  }
  else interpo=0; // then not doing this dimension

  totalo=avgo+interpo+extraavgo;

  return(totalo);
}










// define range over which various loops go
// all these should be as if no grid sectioning SECTIONMARK since used in loops that have SHIFTS inside
int interp_loop_set(void)
{
  int avgo[NDIM];
  int doingdimen[NDIM];
  int dimen;
  int dir;
  int jj;
  int avgoperdir[NDIM][NDIM];
  int odimen1,odimen2;




  doingdimen[1]=N1NOT1;
  doingdimen[2]=N2NOT1;
  doingdimen[3]=N3NOT1;


  // the fluxloop[dir][FIJKDEL]'s can be used for general purpose to get idel, jdel, kdel


  DIMENLOOP(dimen){

    if(doingdimen[dimen]){
      // number of zones one way for finite volume scheme to convert Uavg -> Upoint
      avgo[dimen]=(interporder[avgscheme[dimen]]-1)/2;
    }
    else avgo[dimen]=0; // nothing done for this dimension, so no avg zones
    trifprintf("dimen=%d lim=%d avgo=%d\n",dimen,lim[dimen],avgo[dimen]);
  }



  // always need fluxes in ghost+active if doing higher order methods
  if(useghostplusactive){

    // (interporder[avgscheme]-1)/2 is the number of points to the left and the number of ponits to the right that are needed for the finite volume scheme


    // scheme used to convert Uavg -> Upoint requires extra zones
    //    avgo=(interporder[avgscheme]-1)/2;
    // i.e. if avgscheme=WENO5, then interporder[WENO5]=5 and the Uconsloop goes from -2 .. N+1 inclusive  and fluxes go from -2 to N+2 inclusive
    // same for WENO4
    // this is -avgo .. N-1+avgo for Uconsloop and -avgo .. N+avgo for fluxes

    
    // note that if doing FLUXCTSTAG (FIELDSTAGMEM) then cross directions already expanded


    // merged method needs to store/compute state for full cell.  This is excessively computing final dissipative flux, but not major cost and this keeps loops simple


    // +SHIFT1/2/3 for fluxloop[dimen][F?E] is so flux computed on both lower and upper faces, which happens to only be required for IF3DSPCTHENMPITRANSFERATPOLE for upper pole
    // OUTM? already goes to upper face

    dimen=1;
    fluxloop[dimen][FIDEL]=SHIFT1;
    fluxloop[dimen][FJDEL]=0;
    fluxloop[dimen][FKDEL]=0;
    fluxloop[dimen][FFACE]=FACE1;
    fluxloop[dimen][FIS]=(-avgo[dimen]-MERGEDC2EA2CMETHOD)*N1NOT1;
    fluxloop[dimen][FIE]=N1-1 +SHIFT1 +(avgo[dimen]+1+MERGEDC2EA2CMETHOD)*N1NOT1;
    fluxloop[dimen][FJS]=INFULL2;
    fluxloop[dimen][FJE]=OUTFULL2;
    fluxloop[dimen][FKS]=INFULL3;
    fluxloop[dimen][FKE]=OUTFULL3;

    dimen=2;
    fluxloop[dimen][FIDEL]=0;
    fluxloop[dimen][FJDEL]=SHIFT2;
    fluxloop[dimen][FKDEL]=0;
    fluxloop[dimen][FFACE]=FACE2;
    fluxloop[dimen][FIS]=INFULL1;
    fluxloop[dimen][FIE]=OUTFULL1;
    fluxloop[dimen][FJS]=(-avgo[dimen]-MERGEDC2EA2CMETHOD)*N2NOT1;
    fluxloop[dimen][FJE]=N2-1 +SHIFT2 +(avgo[dimen]+1+MERGEDC2EA2CMETHOD)*N2NOT1;
    fluxloop[dimen][FKS]=INFULL3;
    fluxloop[dimen][FKE]=OUTFULL3;

    dimen=3;
    fluxloop[dimen][FIDEL]=0;
    fluxloop[dimen][FJDEL]=0;
    fluxloop[dimen][FKDEL]=SHIFT3;
    fluxloop[dimen][FFACE]=FACE3;
    fluxloop[dimen][FIS]=INFULL1;
    fluxloop[dimen][FIE]=OUTFULL1;
    fluxloop[dimen][FJS]=INFULL2;
    fluxloop[dimen][FJE]=OUTFULL2;
    fluxloop[dimen][FKS]=(-avgo[dimen]-MERGEDC2EA2CMETHOD)*N3NOT1;
    fluxloop[dimen][FKE]=N3-1 +SHIFT3 +(avgo[dimen]+1+MERGEDC2EA2CMETHOD)*N3NOT1;

  }
  else{

    // Uconsloop for these methods just involve normal CZLOOP
    // inversion for this method just involves CZLOOP
    dimen=1;
    fluxloop[dimen][FIDEL]=SHIFT1;
    fluxloop[dimen][FJDEL]=0;
    fluxloop[dimen][FKDEL]=0;
    fluxloop[dimen][FFACE]=FACE1;
    fluxloop[dimen][FIS]=      -MERGEDC2EA2CMETHOD*N1NOT1;
    fluxloop[dimen][FIE]=OUTM1 +MERGEDC2EA2CMETHOD*N1NOT1;
    if(FLUXB==FLUXCTSTAG){
      fluxloop[dimen][FJS]=INFULL2;
      fluxloop[dimen][FJE]=OUTFULL2;
      fluxloop[dimen][FKS]=INFULL3;
      fluxloop[dimen][FKE]=OUTFULL3;
    }
    else{ // then only averaging for FLUXCT so only need 1 additional off-direction point
      fluxloop[dimen][FJS]=-SHIFT2;  //atch: loop over additional row to provide enough fluxes for FLUXCT, etc. to operate near the boundary
      fluxloop[dimen][FJE]=N2-1+SHIFT2; // " " 
      fluxloop[dimen][FKS]=-SHIFT3;     // " "
      fluxloop[dimen][FKE]=N3-1+SHIFT3; // " "
    }

    dimen=2;
    fluxloop[dimen][FIDEL]=0;
    fluxloop[dimen][FJDEL]=SHIFT2;
    fluxloop[dimen][FKDEL]=0;
    fluxloop[dimen][FFACE]=FACE2;
    fluxloop[dimen][FJS]=      -MERGEDC2EA2CMETHOD*N2NOT1;
    fluxloop[dimen][FJE]=OUTM2 +MERGEDC2EA2CMETHOD*N2NOT1;
    if(FLUXB==FLUXCTSTAG){
      fluxloop[dimen][FIS]=INFULL1;
      fluxloop[dimen][FIE]=OUTFULL1;
      fluxloop[dimen][FKS]=INFULL3;
      fluxloop[dimen][FKE]=OUTFULL3;
    }
    else{
      fluxloop[dimen][FIS]=-SHIFT1;   //atch: loop over additional row to provide enough fluxes for FLUXCT, etc. to operate near the boundary
      fluxloop[dimen][FIE]=N1-1+SHIFT1; // " "
      fluxloop[dimen][FKS]=-SHIFT3;    // " "
      fluxloop[dimen][FKE]=N3-1+SHIFT3;// " "
    }


    dimen=3;
    fluxloop[dimen][FIDEL]=0;
    fluxloop[dimen][FJDEL]=0;
    fluxloop[dimen][FKDEL]=SHIFT3;
    fluxloop[dimen][FFACE]=FACE3;
    fluxloop[dimen][FKS]=      -MERGEDC2EA2CMETHOD*N3NOT1;
    fluxloop[dimen][FKE]=OUTM3 +MERGEDC2EA2CMETHOD*N3NOT1;
    if(FLUXB==FLUXCTSTAG){
      fluxloop[dimen][FIS]=INFULL1;
      fluxloop[dimen][FIE]=OUTFULL1;
      fluxloop[dimen][FJS]=INFULL2;
      fluxloop[dimen][FJE]=OUTFULL2;
    }
    else{
      fluxloop[dimen][FIS]=-SHIFT1;   //atch: loop over additional row to provide enough fluxes for FLUXCT, etc. to operate near the boundary
      fluxloop[dimen][FIE]=N1-1+SHIFT1;  // " "
      fluxloop[dimen][FJS]=-SHIFT2;      // " "
      fluxloop[dimen][FJE]=N2-1+SHIFT2;  // " "
    }

  }



  DIMENLOOP(dimen) DIMENLOOP(dir){
    avgoperdir[dir][dimen]=avgo[dir]*(dimen==dir);
  }


  // fluxloop for staggered field's EMF when doing old dofluxreconevolvepointfield==0 method
  DIMENLOOP(dimen){
    odimen1=dimen%3+1;
    odimen2=(dimen+1)%3+1;

    emffluxloop[dimen][FIDEL]=MAX(fluxloop[odimen1][FIDEL],fluxloop[odimen2][FIDEL]);
    emffluxloop[dimen][FJDEL]=MAX(fluxloop[odimen1][FJDEL],fluxloop[odimen2][FJDEL]);
    emffluxloop[dimen][FKDEL]=MAX(fluxloop[odimen1][FKDEL],fluxloop[odimen2][FKDEL]);
    if(dimen==1) emffluxloop[dimen][FFACE]=CORN1;
    else if(dimen==1) emffluxloop[dimen][FFACE]=CORN2;
    else if(dimen==1) emffluxloop[dimen][FFACE]=CORN3;

    if(extrazones4emf){
      // then need defined at more positions
      emffluxloop[dimen][FIS]=MIN(fluxloop[odimen1][FIS]-avgoperdir[odimen1][1],fluxloop[odimen2][FIS]-avgoperdir[odimen2][1]);
      emffluxloop[dimen][FIE]=MAX(fluxloop[odimen1][FIE]+avgoperdir[odimen1][1],fluxloop[odimen2][FIE]+avgoperdir[odimen2][1]);
      emffluxloop[dimen][FJS]=MIN(fluxloop[odimen1][FJS]-avgoperdir[odimen1][2],fluxloop[odimen2][FJS]-avgoperdir[odimen2][2]);
      emffluxloop[dimen][FJE]=MAX(fluxloop[odimen1][FJE]+avgoperdir[odimen1][2],fluxloop[odimen2][FJE]+avgoperdir[odimen2][2]);
      emffluxloop[dimen][FKS]=MIN(fluxloop[odimen1][FKS]-avgoperdir[odimen1][3],fluxloop[odimen2][FKS]-avgoperdir[odimen2][3]);
      emffluxloop[dimen][FKE]=MAX(fluxloop[odimen1][FKE]+avgoperdir[odimen1][3],fluxloop[odimen2][FKE]+avgoperdir[odimen2][3]);
    }
    else{
      emffluxloop[dimen][FIS]=MIN(fluxloop[odimen1][FIS],fluxloop[odimen2][FIS]);
      emffluxloop[dimen][FIE]=MAX(fluxloop[odimen1][FIE],fluxloop[odimen2][FIE]);
      emffluxloop[dimen][FJS]=MIN(fluxloop[odimen1][FJS],fluxloop[odimen2][FJS]);
      emffluxloop[dimen][FJE]=MAX(fluxloop[odimen1][FJE],fluxloop[odimen2][FJE]);
      emffluxloop[dimen][FKS]=MIN(fluxloop[odimen1][FKS],fluxloop[odimen2][FKS]);
      emffluxloop[dimen][FKE]=MAX(fluxloop[odimen1][FKE],fluxloop[odimen2][FKE]);
    }
  }




  ////////////////
  //
  // Define range over which "average" conserved quantities are *evolved*/updated from some flux.  See advance.c.
  // Defines cell centers where conserved quantity exists
  //
  // Don't really need to evolve ghost+active region if FLUXRECON method unless FLUXRECON&&(FLUXB==FLUXCTSTAG), but not a problem to be a bit excessive
  //
  // if doing FLUXRECON && evolving point field, then no need to evolve ghost+active region -- latest method
  //
  ////////////////
  if(useghostplusactive==0 || (DOENOFLUX == ENOFLUXRECON && extrazones4emf==0 && dofluxreconevolvepointfield==1) ){
    Uconsevolveloop[FFACE]=CENT;

    dimen=1;
    Uconsevolveloop[FIS]=0;
    Uconsevolveloop[FIE]=N1-1;

    dimen=2;
    Uconsevolveloop[FJS]=0;
    Uconsevolveloop[FJE]=N2-1;

    dimen=3;
    Uconsevolveloop[FKS]=0;
    Uconsevolveloop[FKE]=N3-1;
  }
  else{
    // expanded loop using expanded range of fluxes so update Uf and ucum in layer of ghost zones so don't have to bound flux or Uf/Ui/ucum
    // only needed for FLUXRECON with STAG field method and only for fields, but do all for simplicity
    
    // inversion for this method just involves CZLOOP

    // loop over averaged U to get Uf
    //    Uconsevolveloop[FIDEL]=0;
    //    Uconsevolveloop[FJDEL]=0;
    //    Uconsevolveloop[FKDEL]=0;
    Uconsevolveloop[FFACE]=CENT;

    dimen=1;
    Uconsevolveloop[FIS]=-avgo[dimen]*N1NOT1;
    Uconsevolveloop[FIE]=N1-1+avgo[dimen]*N1NOT1;

    dimen=2;
    Uconsevolveloop[FJS]=-avgo[dimen]*N2NOT1;
    Uconsevolveloop[FJE]=N2-1+avgo[dimen]*N2NOT1;

    dimen=3;
    Uconsevolveloop[FKS]=-avgo[dimen]*N3NOT1;
    Uconsevolveloop[FKE]=N3-1+avgo[dimen]*N3NOT1;
  }



  ////////////////
  //
  // Define ghost+active region over which face (interpolated from center or natively existing in FLUXCTSTAG case) values of primitives are needed as necessary to define the final dissipative flux.
  // Does not need to be modified for merged method's use of more primitives used ultimately to get this same flux position through temporary left-right fluxes on a larger grid
  //
  ////////////////
  if(useghostplusactive){

    // scheme used to convert Uavg -> Upoint requires extra zones
    //    avgo=(interporder[avgscheme]-1)/2;
    // i.e. if avgscheme=WENO5, then interporder[WENO5]=5 and the Uconsloop goes from -2 .. N+1 inclusive  and fluxes go from -2 to N+2 inclusive
    // same for WENO4
    // this is -avgo .. N-1+avgo for Uconsloop and -avgo .. N+avgo for fluxes

    // loop over averaged U to get Uf
    //    Uconsloop[FIDEL]=0;
    //    Uconsloop[FJDEL]=0;
    //    Uconsloop[FKDEL]=0;
    Uconsloop[FFACE]=CENT;

    dimen=1;
    Uconsloop[FIS]=-avgo[dimen]*N1NOT1;
    Uconsloop[FIE]=N1-1+avgo[dimen]*N1NOT1;

    dimen=2;
    Uconsloop[FJS]=-avgo[dimen]*N2NOT1;
    Uconsloop[FJE]=N2-1+avgo[dimen]*N2NOT1;

    dimen=3;
    Uconsloop[FKS]=-avgo[dimen]*N3NOT1;
    Uconsloop[FKE]=N3-1+avgo[dimen]*N3NOT1;

    // inversion for this method just involves CZLOOP
  }
  else{
    Uconsloop[FFACE]=CENT;

    dimen=1;
    Uconsloop[FIS]=0;
    Uconsloop[FIE]=N1-1;

    dimen=2;
    Uconsloop[FJS]=0;
    Uconsloop[FJE]=N2-1;

    dimen=3;
    Uconsloop[FKS]=0;
    Uconsloop[FKE]=N3-1;
  }


  
  if(extrazones4emf){
    emfUconsloop[FFACE]=CENT;

    dimen=1;
    emfUconsloop[FIS]=Uconsloop[FIS]-avgo[dimen]*N1NOT1;
    emfUconsloop[FIE]=Uconsloop[FIE]+avgo[dimen]*N1NOT1;

    dimen=2;
    emfUconsloop[FJS]=Uconsloop[FJS]-avgo[dimen]*N2NOT1;
    emfUconsloop[FJE]=Uconsloop[FJE]+avgo[dimen]*N2NOT1;

    dimen=3;
    emfUconsloop[FKS]=Uconsloop[FKS]-avgo[dimen]*N3NOT1;
    emfUconsloop[FKE]=Uconsloop[FKE]+avgo[dimen]*N3NOT1;
  }
  else{
    emfUconsloop[FFACE]=CENT;

    dimen=1;
    emfUconsloop[FIS]=Uconsloop[FIS];
    emfUconsloop[FIE]=Uconsloop[FIE];

    dimen=2;
    emfUconsloop[FJS]=Uconsloop[FJS];
    emfUconsloop[FJE]=Uconsloop[FJE];

    dimen=3;
    emfUconsloop[FKS]=Uconsloop[FKS];
    emfUconsloop[FKE]=Uconsloop[FKE];
  }






  DIMENLOOP(dimen){
    for(jj=0;jj<NUMFLUXLOOPNUMBERS;jj++) trifprintf("fluxloop[dimen=%d][%d] = %d\n",dimen,jj,fluxloop[dimen][jj]);
  }
  DIMENLOOP(dimen){
    for(jj=0;jj<NUMFLUXLOOPNUMBERS;jj++) trifprintf("emffluxloop[dimen=%d][%d] = %d\n",dimen,jj,emffluxloop[dimen][jj]);
  }
  for(jj=0;jj<NUMFLUXLOOPNUMBERS;jj++) trifprintf("Uconsevolveloop[%d] = %d\n",jj,Uconsevolveloop[jj]);
  for(jj=0;jj<NUMFLUXLOOPNUMBERS;jj++) trifprintf("Uconsloop[%d] = %d\n",jj,Uconsloop[jj]);
  for(jj=0;jj<NUMFLUXLOOPNUMBERS;jj++) trifprintf("emfUconsloop[%d] = %d\n",jj,emfUconsloop[jj]);
  








  return(0);

}




int get_loop(int pointorlinetype, int interporflux, int dir, struct of_loop *loop)
{

  set_interpalltypes_loop_ranges(pointorlinetype, interporflux, dir, &(loop->intdir), &(loop->is), &(loop->ie), &(loop->js), &(loop->je), &(loop->ks), &(loop->ke), &(loop->di), &(loop->dj), &(loop->dk), &(loop->bs), &(loop->ps), &(loop->pe), &(loop->be));
    
  return(0);

}



// master interp range function for both point and line methods
// This particular loop gives back 3D grid range, not line-by-line as in original line type method (so don't use directly in interpline.c!)
int set_interpalltypes_loop_ranges(int pointorlinetype, int interporflux, int dir, int *intdir, int *is, int *ie, int *js, int *je, int *ks, int *ke, int *di, int *dj, int *dk, int *bs, int *ps, int *pe, int *be)
{
  int withshifts;

  if(pointorlinetype==INTERPPOINTTYPE){
    set_interppoint_loop_ranges(interporflux, dir, is, ie, js, je, ks, ke, di, dj, dk);
    // interpolation is always along dir-direction for point methods
    *intdir=dir;
    *ps=*is;
    *pe=*ie;
    // below is largest possible range that includes boundary cells
    *bs = *is - N1BND*(dir==1) - N2BND*(dir==2) - N3BND*(dir==3);
    *be = *ie + N1BND*(dir==1) + N2BND*(dir==2) + N3BND*(dir==3);
  }
  else if(pointorlinetype==INTERPLINETYPE){
    withshifts=0; // force to be without shifts so results can be put into loop that has shifts embedded
    set_interp_loop_gen(withshifts, interporflux, dir, intdir, is, ie, js, je, ks, ke, di, dj, dk, bs, ps, pe, be);
    // transcribe from loop over starting positions to full 3D loop
    if(*intdir==1){
      *is=*ps;
      *ie=*pe;
    }
    else if(*intdir==2){
      *js=*ps;
      *je=*pe;
    }
    else if(*intdir==3){
      *ks=*ps;
      *ke=*pe;
    }
  }
  else{
    dualfprintf(fail_file,"No such pointorlinetype=%d\n",pointorlinetype);
    myexit(ERRORCODEBELOWCLEANFINISH+287687456);
  }


  return(0);

}








// Uavg is usually unew and Upoint is usually ulast at t=0
// fieldfrompotential[1,2,3 correspond to B1,B2,B3]
int pi2Uavg(int *fieldfrompotential, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*Upoint)[NSTORE2][NSTORE3][NPR], FTYPE (*Uavg)[NSTORE2][NSTORE3][NPR])
{
  extern int initial_averageu_fv(int *fieldfrompotential, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*Upoint)[NSTORE2][NSTORE3][NPR], FTYPE (*Uavg)[NSTORE2][NSTORE3][NPR]);
  extern int initial_averageu_fluxrecon(int *fieldfrompotential, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*Upoint)[NSTORE2][NSTORE3][NPR], FTYPE (*Uavg)[NSTORE2][NSTORE3][NPR]);


#pragma omp parallel
  {
    struct of_geom geom,geomf;
    struct of_geom *ptrgeom,*ptrgeomf;
    struct of_state q;
    int i,j,k;
    int pl,pliter;
    FTYPE Utemp[NPR];
    
    OPENMP3DLOOPVARSDEFINE;

    //////  COMPFULLLOOP{
    OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);


      // set geometry
      ptrgeom=&geom; get_geometry(i, j, k, CENT, ptrgeom);
    

      // find U(p)
      MYFUN(get_state(MAC(prim,i,j,k), ptrgeom, &q),"initbasec:pi2Uavg()", "get_state()", 1);
      MYFUN(primtoU(UEVOLVE,MAC(prim,i,j,k), &q, ptrgeom, Utemp),"initbase.c:pi2Uavg()", "primtoU()", 1);

      PLOOPNOB1(pl) MACP0A1(Upoint,i,j,k,pl)=Utemp[pl];
      PLOOPNOB2(pl) MACP0A1(Upoint,i,j,k,pl)=Utemp[pl];

      if(FLUXB==FLUXCTSTAG){
        PLOOPBONLY(pl) if(fieldfrompotential[pl-B1+1]==0){
          ptrgeomf=&geomf; get_geometry(i, j, k, FACE1+(pl-B1), ptrgeomf);
          MACP0A1(Upoint,i,j,k,pl)=MACP0A1(pstag,i,j,k,pl)*(ptrgeomf->gdet);
        }
      }
      else{
        PLOOPBONLY(pl) if(fieldfrompotential[pl-B1+1]==0) MACP0A1(Upoint,i,j,k,pl)=Utemp[pl];
      }
    
      //    dualfprintf(fail_file,"Upoint[%d][%d][%d][UU]=%21.15g prim[UU]=%21.15g\n",i,j,k,MACP0A1(Upoint,i,j,k,UU)/(ptrgeom->gdet),MACP0A1(prim,i,j,k,UU));
    }
  }// end parallel region


  //////////////////////////////////
  //
  // now deal with higher-order interpolations
  //
  //////////////////////////////////
  if(DOENOFLUX == ENOFINITEVOLUME){
    initial_averageu_fv(fieldfrompotential, prim, Upoint, Uavg);
  }
  else if(DOENOFLUX == ENOFLUXRECON){
    initial_averageu_fluxrecon(fieldfrompotential, prim, Upoint, Uavg);
  }
  else{
    // then just copy over
    /////////    COMPFULLLOOP{


#pragma omp parallel
    {
      int i,j,k;
      int pl,pliter;
    
      OPENMP3DLOOPVARSDEFINE;

      //////  COMPFULLLOOP{
      OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
      OPENMP3DLOOPBLOCK{
        OPENMP3DLOOPBLOCK2IJK(i,j,k);


        PLOOPNOB1(pl) MACP0A1(Uavg,i,j,k,pl)=MACP0A1(Upoint,i,j,k,pl);
        PLOOPNOB2(pl) MACP0A1(Uavg,i,j,k,pl)=MACP0A1(Upoint,i,j,k,pl);
        PLOOPBONLY(pl) if(fieldfrompotential[pl-B1+1]==0) MACP0A1(Uavg,i,j,k,pl)=MACP0A1(Upoint,i,j,k,pl);
      }// end 3D LOOP
    }// end parallel region
  }// end else
  



  return(0);
}


void makedirs(void)
{

#if( USINGMPIAVOIDMKDIR ) 
  //AT: for certain systems, neither way works to create dirs, 
  //    so not create them at all
  //stderrfprintf("USEMPIAVOIDMKDIR==1: User must create dumps and images\n");
#else
  if ((mpicombine && (myid == 0)) || (mpicombine == 0)) {
    
    if(USEMPI && (!MPIAVOIDFORK) || USEMPI==0){
      system("mkdir dumps");
      system("mkdir images");
    }
    else{
#ifndef WIN32
      // assumes unix commands exist in <sys/stat.h>
      // see also "info libc"
      // create directory with rxw permissions for user only
      mkdir("dumps",0700);
      mkdir("images",0700);
#else
      stderrfprintf("WIN32: User must create dumps and images\n");
#endif
    }
  }

#if(USEMPI)
  // all cpus wait for directory to be created
  MPI_Barrier(MPI_COMM_GRMHD);
#endif
#endif  //USINGMPIAVOIDMKDIR

}


#include<sys/stat.h>





// acts on globals, assumes static internals that get recalled upon reentering
int addremovefromnpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR])
{
  int addremovefromanynpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int *anynprstart, int *anynprend, int *anynprlist, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR]);

  // applies only to NPR type list
  addremovefromanynpr(doadd, whichpltoavg, ifnotavgthencopy, &nprstart, &nprend, nprlist, nprlocalstart, nprlocalend, nprlocallist, in, out);


  return(0);

}

// acts on globals, assumes static internals that get recalled upon reenterin
int addremovefromanynpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int *anynprstart, int *anynprend, int *anynprlist, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR])
{
  int pl,pliter;
  int pl2,pl3;
  int i,j,k;
  int num;



  if(doadd==REMOVEFROMNPR){
    ////////////////////////////////////////////
    //
    // save choice for interpolations
    *nprlocalstart=*anynprstart;
    *nprlocalend=*anynprend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=anynprlist[pl];

    if(anynprlist==nprlist) num=NPR;
    else if(anynprlist==npr2interplist) num=NPR2INTERP;
    else if(anynprlist==nprboundlist) num=NPRBOUND;
    else{
      dualfprintf(fail_file,"No such type of list in addremovefromanynpr\n");
      myexit(ERRORCODEBELOWCLEANFINISH+7615156);
    }


    // now remove any other pl's not wanting to average for whatever reason
    for(pl3=0;pl3<num;pl3++){ // as above, but loop over all undesired quantities
      for(pl= *anynprstart;pl<= *anynprend;pl++){
        if(whichpltoavg[pl3]==0 && anynprlist[pl]==pl3){
          // need to copy over unchanged quantity
          if(ifnotavgthencopy[pl3] && in!=NULL && out!=NULL) copy_3d_onepl_fullloop(pl3,in,out); //COMPFULLLOOP MACP0A1(out,i,j,k,pl3)=MACP0A1(in,i,j,k,pl3);
          for(pl2=pl+1;pl2<= *anynprend;pl2++) anynprlist[pl2-1]=anynprlist[pl2]; // moving upper to lower index
          *anynprend-=1; // removed dir-field
          break;
        }
      }
    }

  }
  else if(doadd==RESTORENPR){


    ////////////////////////////////////////////
    //
    // restore choice for interpolations
    *anynprstart= *nprlocalstart;
    *anynprend= *nprlocalend;
    PMAXNPRLOOP(pl) anynprlist[pl]=nprlocallist[pl];
  }


  //  PALLLOOP(pl){
  //    dualfprintf(fail_file,"dir=%d interptype=%d nprstart=%d nprend=%d nprlist[%d]=%d\n",dir,interptype,*anynprstart,*anynprend,pl,anynprlist[pl]);
  //  }
  

  return(0);

}


// used to transform from one coordinate system to PRIMECOORDS
// when acting on pstag, only relevant for magnetic field part, and in that case if didn't use vector potential to define pstag then assume not too important to get high accuracy, so average field to other positions in simple way
int transform_primitive_vB(int whichvel, int whichcoord, int i,int j, int k, FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR])
{

  // deal with pstag using p before p is changed
  MYFUN(transform_primitive_pstag(whichvel, whichcoord, i,j, k, p, pstag),"initbase.c:transform_primitive_vB()","transform_primitive_pstag()",0);
  

  // For p, transform from whichcoord to MCOORD
  // This changes p directly, so must come AFTER pstag change that uses original p
  if (bl2met2metp2v(whichvel,whichcoord,MAC(p,i,j,k), i,j,k) >= 1) FAILSTATEMENT("initbase.c:transform_primitive_vB()", "bl2ks2ksp2v()", 1);


  return(0);
}




int assert_func_empty( int is_bad_val, char *s, ... )
{
  return(is_bad_val);
}

int assert_func( int is_bad_val, char *s, ... )
{
  va_list arglist;
  FILE *fileptr = fail_file;

  va_start (arglist, s);

  if( 0 != is_bad_val ) {
    if(fileptr==NULL){
      stderrfprintf("tried to print to null file pointer: %s\n",s);
      fflush(stderr);
    }
    else{
      dualfprintf( fail_file, "Assertion failed: " );
      vfprintf (fileptr, s, arglist);
      fflush(fileptr);
    }
    if(myid==0){
      vfprintf (stderr, s, arglist);
      fflush(stderr);
    }
    va_end (arglist);

    myexit( 1 );
  }

  return is_bad_val;
}


