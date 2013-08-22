      program testradiation

c     Reads in Jon's error file fails.dat and tests our ideas for
c     applying the radiation source term

c     variables with 'p' at the end refer to the previous time step,
c     those with 'i' at the end refer to the current time step
c     pre-radiation, and those with 'f' at the end are for the current
c     time step post-radiation.

c     variables with 'c' at the end are conserved quantities
c     corresponding to the problem currently being solved

c     variables with 'final' at the end correspond to the final solution

      implicit double precision (a-h,o-z)
      real*8 itertot
      dimension isc(4),gn(4,4),gv(4,4),hp(4,4),hf(4,4)
      dimension vgasp(3),vgasf(3),B1(5),B2(5),
     &     BBp(4),BBf(4),BBc(4),vradp(3),vradf(3),s(5),
     &     ugasconf(4),ugascovf(4),ugasconp(4),ugascovp(4),
     &     uradconf(4),uradcovf(4),uradconp(4),uradcovp(4),
     &     ugasconi(4),ugascovi(4),uradconi(4),uradcovi(4),
     &     bconp(4),bcovp(4),bconf(4),bcovf(4),bconi(4),bcovi(4),
     &     Tmunup(4,4),Tmunuf(4,4),Rmunup(4,4),Rmunuf(4,4),
     &     Tmunui(4,4),Rmunui(4,4),Ttcovi(4),Rtcovi(4),
     &     Ttcovc(4),Rtcovc(4),Ttotc(4),prim(4),error(4),
     &     ugasconfinal(4),ugascovfinal(4),uradconfinal(4),
     &     uradcovfinal(4),bconfinal(4),bcovfinal(4),
     &     Ttcovfinal(4),Rtcovfinal(4),
     &     Tmunufinal(4,4),Rmunufinal(4,4)
      common/metric/gn,gv
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0c,sc,Ttcovc,Rtcovc,
     &     BBc,dt
      external funcMHD1,funcMHD2,funcrad1,funcrad2
      character filehead*30,fileext*4,infile*60
      character conhead*3,confile*60
      character solhead*3,solfile*60
      character errhead*3,errfile*60
      double precision mymax,myabs,mydiv

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

c     Initialize constants and read in data from Jon's datafile

c     eps is fractional shift in primitives for numerical derivatives
c     epsbib is fractional accuracy desired in bisection
c     dvmin: if abs(uconmu)<1d-4 then shift used is dvmin*eps
c     tol: all fractional errors must be below tol for convergence
c     uminfac: minimum u_g allowed is uminfrac*rho
c     dlogmax: minimum log() in entropy is log() conserved - dlogmin
c     itermax is maximum no. of iterations with u_g, entropy below minimum

      eps=1.d-6
      epsbis=1.d-3
      dvmin=1.d-4
      tol=1.d-10
      uminfac=1.d-10
      dlogmax=log(2.d0)
      itermax=3
      gammaradceiling=50.d0

c     idatatype=1 means Jon's old data format with 134 numbers
c     idatatype=2 means Jon's new data format with 181 numbers
c     idatatype=3 means Jon's new data format with 209 numbers

c      write (*,*) ' which type data file? old(1) new(2) '
c      read (*,*) idatatype
      idatatype=3

c     iMHD=0 means don't do initial MHD inversion before radiation
c     iMHD=1 means do initial MHD inversion before radiation

      iMHD=0
      itertot=0
      iproblem=0


c     To compile/run with debug info, do:
c         cp ../test.f . ; gfortran -g -O2 test.f -o test.e ; ./test.e > test.e.out

c     To compile/run with debug info and gdb easy reading, do:
c         cp ../test.f . ; gfortran -g -O0 test.f -o test.e ; ./test.e > test.e.out
c     If crashes, then do:
c         gdb ./test.e core



c     CHOOSE FILENAME HEAD TO READ
      filehead='fails'
c      filehead='failsearly'
c      
c     CAN CHOOSE EXTENSION
      fileext='.txt'
      infile=trim(filehead)//fileext
c
c     CAN CHOOSE HEADER FOR OUTPUTS
      conhead='con'
      confile=conhead//'_'//trim(filehead)//fileext
      solhead='sol'
      solfile=solhead//'_'//trim(filehead)//fileext
      errhead='err'
      errfile=errhead//'_'//trim(filehead)//fileext
c
      open (11,file=infile)
      open (12,file=confile)
      open (13,file=solfile)
      open (14,file=errfile)
      write (14,"(9X,1A)",advance="no") 'NUM HITER    HARMERR'
      write (14,"(10X,1A)",advance="no")
     &     'HARMEOMTYPE HARMITERMODE'
      write (14,"(2X,1A)",advance="no") 'HARMSTAT'
      write (14,"(1X,1A)",advance="no") 'RAMSTAT'
      write (14,"(4X,1A)",advance="no") 'RAMENGITER   RAMENERR'
      write (14,"(19X,1A)") 'RAMENTITER RAMENTERR'

      do i=1,1000000
c      do i=1,1

c     If ientropy=0, we will try the energy equation. If it is 1, we
c     proceed directly to the entropy equation

         write (14,"(1I10)",advance="no") i

         write (13,*)
         write (13,"(25X,22X,A,22X,A)",advance="no")
     &        '|rho','|u_g'
         write (13,"(22X,A,22X,A,22X,A,22X,A)",advance="no")
     &        '|u^t','u^1','u^2','u^3|'
         write (13,"(22X,A)",advance="no")
     &        '|Erf'
         write (13,"(22X,A,22X,A,22X,A,22X,A)")
     &        '|u^t','u^1','u^2','u^3|'
         write (13,*) ' JON PROBLEM NUMBER: ',i

         ientropy=0
         
         if (idatatype.eq.1) then

         call readtype1(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,ifinish,errorabs)
         Gam=4.d0/3.d0

         else if (idatatype.eq.2) then

         call readtype2(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,Gam,ifinish,errorabs)

         else if (idatatype.eq.3) then

         call readtype3(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,Gam,ifinish,errorabs)

         endif

c     Gam=adiabatic index Gamma, Gam1=Gamma-1
c     en=polytropic index = 1/Gamma, en1 = n+1

         Gam1=Gam-1.d0
         en=1.d0/Gam1
         en1=en+1.d0

c         write (*,*)
c         write (*,*) (isc(j),j=1,4),dt,
c     &        ((gn(j,k),k=1,4),j=1,4),
c     &        ((gv(j,k),k=1,4),j=1,4),
c     &        rhof,rhop,rhou0i,rhou0f,rhou0p,
c     &        uf,up,T00i,T00f,T00p,
c     &        vgasf(1),vgasp(1),T01i,T01f,T01p,
c     &        vgasf(2),vgasp(2),T02i,T02f,T02p,
c     &        vgasf(3),vgasp(3),T03i,T03f,T03p,
c     &        BBf(2),BBp(2),scr,scr,scr,
c     &        BBf(3),BBp(3),scr,scr,scr,
c     &        BBf(4),BBp(4),scr,scr,scr,
c     &        Ef,Ep,R00i,R00f,R00p,
c     &        vradf(1),vradp(1),R01i,R01f,R01p,
c     &        vradf(2),vradp(2),R02i,R02f,R02p,
c     &        vradf(3),vradp(3),R03i,R03f,R03p,
c     &        (s(j),j=1,2),si,sf,sp,
c     &        (uradconf(j),uradcovf(j),j=1,4),
c     &        (ugasconf(j),ugascovf(j),j=1,4),
c     &        (uradconp(j),uradcovp(j),j=1,4),
c     &        (ugasconp(j),ugascovp(j),j=1,4)
         
         if (ifinish.eq.1) go to 10
         iproblem=iproblem+1

c     Calculate b^mu and b_mu for 'p' and 'f'

         call calcbconbcov(BBp,ugasconp,ugascovp,bconp,bcovp,bsqp)
         call calcbconbcov(BBf,ugasconf,ugascovf,bconf,bcovf,bsqf)

ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

         write (*,*)
         write (*,*)
         write (*,*) ' JON PROBLEM NUMBER: ',i
         write (12,*)
         write (12,*) ' JON PROBLEM NUMBER: ',i

c     Set up initial guess primitives ('p' quantities from prevous time
c     step). Make sure up is reasonable. If not, reset up to
c     umin=uminfac*rhop. 

c      CHOOSE to use previous primitve or harm solution as starting point.
c     0 = use original prims
c     1 = use harm prims and still do normal Ramesh stages
c     2 = use harm prims and go straight to 4D iterations to avoid mis-steps
c         guesstype=0
         guesstype=2
c         guesstype=1

         if(guesstype.eq.0.or.errorabs.gt.1E-6) then
            write(*,*) 'Using original primitives as guess: guesstype='
     &           ,guesstype
            prim(1)=mymax(up,uminfac*rhop)
            prim(2)=ugasconp(2)
            prim(3)=ugasconp(3)
            prim(4)=ugasconp(4)
            guessstype=0
         else
            write(*,*) 'Using harm primitives as guess: guesstype='
     &           ,guesstype
            prim(1)=mymax(uf,uminfac*rhof)
            prim(2)=ugasconf(2)
            prim(3)=ugasconf(3)
            prim(4)=ugasconf(4)
         endif

         write (*,*) ' prim: ',(prim(j),j=1,4)

c     Set up conserved quantities 'c' of the current time step ('i'
c     state). The values go into common block /conserved/


         rhou0c=rhou0i
         sc=si

         Ttcovc(1)=T00i
         Ttcovc(2)=T01i
         Ttcovc(3)=T02i
         Ttcovc(4)=T03i

         Rtcovc(1)=R00i
         Rtcovc(2)=R01i
         Rtcovc(3)=R02i
         Rtcovc(4)=R03i

         BBc(1)=0.d0
         BBc(2)=BBf(2)
         BBc(3)=BBf(3)
         BBc(4)=BBf(4)

      if (iMHD.eq.1) then

c     Carry out initial MHD inversion of the 'i' state

         write (*,*) 
         write (*,*) ' MHD INVERSION OF PRE-RADIATION SOLUTION '

         call MHDinvert(prim,iflag,jflag,ientropy,itertot,guesstype)

c     MHD inversion done. Update all the relevant quantities
c     corresponding to the 'i' solution

c     Calculate u, u^0 and rho

         ui=prim(1)
         do j=2,4
            ugasconi(j)=prim(j)
         enddo
         call solveu0(ugasconi)
         call contocov(ugasconi,ugascovi)
         rhoi=rhou0i/ugasconi(1)

c         write (*,*)
c         write (*,*) ' final solution: rho, u, u^mu: ',i,
c     &        rhoi,ui,(ugasconi(j),j=1,4)

c         write (*,*)
c         write (*,*) ' original T^t_mu: ',(Ttcovc(j),j=1,4)
c         write (*,*) ' original R^t_mu: ',(Rtcovc(j),j=1,4)

c     Calculate b^mu and b_mu, and update T^t_mu and R^t_mu so as to be
c     consistent with primitives

         do j=1,4
            Ttotc(j)=Ttcovc(j)+Rtcovc(j)
         enddo

         call calcbconbcov(BBc,ugasconi,ugascovi,bconi,bcovi,bsqi)

         call calcTmunu(rhoi,ui,bsqi,Gam,ugasconi,ugascovi,
     &        bconi,bcovi,Tmunui)

         do j=1,4
            Ttcovi(j)=Tmunui(1,j)
            Ttcovc(j)=Ttcovi(j)
            Rtcovi(j)=Ttotc(j)-Ttcovi(j)
            Rtcovc(j)=Rtcovi(j)
         enddo

c         write (*,*)
c         write (*,*) ' updated T^t_mu: ',(Ttcovc(j),j=1,4)
c         write (*,*) ' updated R^t_mu: ',(Rtcovc(j),j=1,4)

c     Initial MHD inversion done

      else

c     No MHD inversion done. Proceed directly to solve the radiation
c     problem.

         write (*,*) 
         write (*,*) ' SKIP MHD INVERSION OF PRE-RADIATION SOLUTION '

      endif

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

c     Now it is time to apply the implicit radiation source term and
c     solve for the post-radiation primitives

c     common block /conserved/ already contains the relevant conserved
c     quantities for the pre-radiation state. For the primitives, use
c     the 'i' state as the initial guess. Then solve the lab frame
c     energy equation using Newton-Raphson

         write (*,*)
         write (*,*) ' SOLVE WITH IMPLICIT RADIATION SOURCE TERM '

         call radsource(prim,iter,iflag,jflag,ientropy
     &        ,itertot,guesstype)
         ufinal=prim(1)

c     Radiation inversion done. Calculate relevant quantities
c     corresponding to the final solution

         ufinal=prim(1)

         do j=2,4
            ugasconfinal(j)=prim(j)
         enddo
         call solveu0(ugasconfinal)
         call contocov(ugasconfinal,ugascovfinal)
         rhou0final=rhou0c
         rhofinal=rhou0c/ugasconfinal(1)

c         write (*,*)
c         write (*,*) ' final solution: rho, u, u^mu: ',
c     &        rhofinal,ufinal,(ugasconfinal(j),j=1,4)

c     Update T^mu_nu so as to be consistent with the final
c     primitives. Adjust R^t_mu so that the total energy and momentum
c     density are unchaned, then calculate the full R^mu_nu

         do j=1,4
            Ttotc(j)=Ttcovc(j)+Rtcovc(j)
         enddo

         call calcbconbcov(BBc,ugasconfinal,ugascovfinal,
     &        bconfinal,bcovfinal,bsqfinal)

         call calcTmunu(rhofinal,ufinal,bsqfinal,Gam,ugasconfinal,
     &        ugascovfinal,bconfinal,bcovfinal,Tmunufinal)

         do j=1,4
            Ttcovfinal(j)=Tmunufinal(1,j)
            Rtcovfinal(j)=Ttotc(j)-Ttcovfinal(j)
         enddo

         call Rmunuinvert(Rtcovfinal,Efinal,uradconfinal,
     &        uradcovfinal,ugasconfinal,Rmunufinal)

         write (*,*)
         write (*,*)
         write (*,*) ' FINAL SOLUTION: '
         write (*,*)
         write (*,*) ' conserved rho u^0: ',rhou0final
         write (*,*) ' conserved T^t_mu: ',(Ttcovfinal(j),j=1,4)
         write (*,*) ' conserved R^t_mu: ',(Rtcovfinal(j),j=1,4)
         write (*,*) ' rho, u_g, E: ',rhofinal,ufinal,Efinal
         write (*,*) ' ugascon: ',(ugasconfinal(j),j=1,4)
         write (*,*) ' uradcon: ',(uradconfinal(j),j=1,4)
         alpha=1.d0/sqrt(-gn(1,1))
         gammagas=ugasconfinal(1)*alpha
         gammarad=uradconfinal(1)*alpha
         write (*,*) ' gammagas, gammarad: ',gammagas,gammarad

         write (13,*) 'RAMESH rho, u_g, u^mu Erf urad^mu: ',rhofinal,
     &        ufinal,
     &        (ugasconfinal(j),j=1,4),
     &        Efinal,
     &        (uradconfinal(j),j=1,4)

      enddo

 10   write (*,*)
      write (*,*)
      write (*,*) ' iproblem: ',iproblem
      write (*,*) ' itertot: ',itertot
      problem=iproblem
      write (*,*) ' iterations per problem: ',itertot/problem
      close (11)
      write (12,*)
      write (12,*) ' iproblem: ',iproblem
      write (12,*) ' itertot: ',itertot
      write (12,*) ' iterations per problem: ',itertot/problem
      write (*,*)
      close (12)
      close (13)
      close (14)

      stop
      end


      subroutine readtype1(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,ifinish,errorabs)

c     Read in data in Jon's old format (134 numbers)

      implicit double precision (a-h,o-z)
      dimension isc(4),gn(4,4),gv(4,4),vgasp(3),vgasf(3),
     &     BBp(4),BBf(4),vradp(3),vradf(3),s(5),
     &     ugasconf(4),ugascovf(4),ugasconp(4),ugascovp(4),
     &     uradconf(4),uradcovf(4),uradconp(4),uradcovp(4)

      ifinish=0

         read (11,*,end=10) (isc(j),j=1,4),dt,
     &        ((gn(j,k),k=1,4),j=1,4),
     &        ((gv(j,k),k=1,4),j=1,4),
     &        rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,
     &        vgasf(1),vgasp(1),T01i,T01f,T01p,
     &        vgasf(2),vgasp(2),T02i,T02f,T02p,
     &        vgasf(3),vgasp(3),T03i,T03f,T03p,
     &        BBf(2),BBp(2),scr,scr,scr,
     &        BBf(3),BBp(3),scr,scr,scr,
     &        BBf(4),BBp(4),scr,scr,scr,
     &        Ef,Ep,R00i,R00f,R00p,
     &        vradf(1),vradp(1),R01i,R01f,R01p,
     &        vradf(2),vradp(2),R02i,R02f,R02p,
     &        vradf(3),vradp(3),R03i,R03f,R03p,
     &        (s(j),j=1,2),si,sf,sp,
     &        (uradconf(j),uradcovf(j),j=1,4),
     &        (ugasconf(j),ugascovf(j),j=1,4),
     &        (uradconp(j),uradcovp(j),j=1,4),
     &        (ugasconp(j),ugascovp(j),j=1,4)

c         write (*,*) (isc(j),j=1,4),dt,
c     &        ((gn(j,k),k=1,4),j=1,4),
c     &        ((gv(j,k),k=1,4),j=1,4),
c     &        rhof,rhop,rhou0i,rhou0f,rhou0p,
c     &        uf,up,T00i,T00f,T00p,
c     &        vgasf(1),vgasp(1),T01i,T01f,T01p,
c     &        vgasf(2),vgasp(2),T02i,T02f,T02p,
c     &        vgasf(3),vgasp(3),T03i,T03f,T03p,
c     &        BBf(2),BBp(2),scr,scr,scr,
c     &        BBf(3),BBp(3),scr,scr,scr,
c     &        BBf(4),BBp(4),scr,scr,scr,
c     &        Ef,Ep,R00i,R00f,R00p,
c     &        vradf(1),vradp(1),R01i,R01f,R01p,
c     &        vradf(2),vradp(2),R02i,R02f,R02p,
c     &        vradf(3),vradp(3),R03i,R03f,R03p,
c     &        (s(j),j=1,2),si,sf,sp,
c     &        (uradconf(j),uradcovf(j),j=1,4),
c     &        (ugasconf(j),ugascovf(j),j=1,4),
c     &        (uradconp(j),uradcovp(j),j=1,4),
c     &        (ugasconp(j),ugascovp(j),j=1,4)
c         write (*,*) ' rho: ',rhof,rhop,rhou0i,rhou0f,rhou0p

         errorabs=1.0
         return

 10   ifinish=1

      return
      end



      subroutine readtype2(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,Gam,ifinish,errorabs)

c     Read in data in Jon's new format (181 numbers)

      implicit double precision (a-h,o-z)
      dimension isc(4),gn(4,4),gv(4,4),vgasp(3),vgasf(3),
     &     BBp(4),BBf(4),vradp(3),vradf(3),s(5),
     &     ugasconf(4),ugascovf(4),ugasconp(4),ugascovp(4),
     &     uradconf(4),uradcovf(4),uradconp(4),uradcovp(4),
     &     ugasconb(4),ugascovb(4),uradconb(4),uradcovb(4)

      ifinish=0

         read (11,*,end=10) (isc(j),j=1,4),
     &        errorabs,iters,dt,nstep,steppart,Gam,
     &        ((gn(j,k),k=1,4),j=1,4),
     &        ((gv(j,k),k=1,4),j=1,4),
     &        rhof,scr,rhob,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,scr,ub,up,T00i,T00f,T00p,
     &        vgasf(1),scr,scr,vgasp(1),T01i,T01f,T01p,
     &        vgasf(2),scr,scr,vgasp(2),T02i,T02f,T02p,
     &        vgasf(3),scr,scr,vgasp(3),T03i,T03f,T03p,
     &        BBf(2),scr,scr,BBp(2),scr,scr,scr,
     &        BBf(3),scr,scr,BBp(3),scr,scr,scr,
     &        BBf(4),scr,scr,BBp(4),scr,scr,scr,
     &        Ef,scr,Eb,Ep,R00i,R00f,R00p,
     &        vradf(1),scr,scr,vradp(1),R01i,R01f,R01p,
     &        vradf(2),scr,scr,vradp(2),R02i,R02f,R02p,
     &        vradf(3),scr,scr,vradp(3),R03i,R03f,R03p,
     &        s(1),scr,scr,s(2),si,sf,sp,
     &        (uradconf(j),uradcovf(j),j=1,4),
     &        (ugasconf(j),ugascovf(j),j=1,4),
     &        (uradconb(j),uradcovb(j),j=1,4),
     &        (ugasconb(j),ugascovb(j),j=1,4),
     &        (uradconp(j),uradcovp(j),j=1,4),
     &        (ugasconp(j),ugascovp(j),j=1,4)

         write (14,"(2X,1I5,2X,1E21.15,2X,1I8)",advance="no")
     & iters,errorabs,0

         if(errorabs.lt.1E-6) then
            write (13,"(1X,A)",advance="no") ' GOOD   '
            write (14,"(1X,A)",advance="no") ' GOOD   '
         else
            write (13,"(1X,A)",advance="no") '  BAD   '
            write (14,"(1X,A)",advance="no") '  BAD   '
         endif
         
         write (13,*) 'HARMJM rho, u_g, u^mu Erf urad^mu: '
     &        ,rhof,uf,
     &        (ugasconf(j),j=1,4),
     &        Ef,
     &        (uradconf(j),j=1,4)

c         write (*,*) ' gn: ',((gn(i,j),j=1,4),i=1,4)
c         write (*,*) ' gv: ',((gv(i,j),j=1,4),i=1,4)
c         write (*,*) ' rhou0i, rhou0f, rhou0p: ',rhou0i,rhou0f,rhou0p
c         write (*,*) ' uf, up, T001, T00f, T00p: ',uf,up,T00i,T00f,T00p
c         write (*,*) ' ugasconp: ',(ugasconp(j),j=1,4)
c         write (*,*) ' ugasconf: ',(ugasconf(j),j=1,4)
c         write (*,*) ' BBp: ',(BBp(j),j=2,4)
c         write (*,*) ' BBf: ',(BBf(j),j=2,4)
         write (*,*) ' Ef, Ep, R00i, R00f, R00p: ',Ef,Ep,R00i,R00f,R00p
c         write (*,*) ' uradconp: ',(uradconp(j),j=1,4)
c         write (*,*) ' uradconf: ',(uradconf(j),j=1,4)
         write (*,*) ' si, sf, sp: ',si,sf,sp

c         write (*,*) (isc(j),j=1,4),dt,
c     &        ((gn(j,k),k=1,4),j=1,4),
c     &        ((gv(j,k),k=1,4),j=1,4),
c     &        rhof,rhop,rhou0i,rhou0f,rhou0p,
c     &        uf,up,T00i,T00f,T00p,
c     &        vgasf(1),vgasp(1),T01i,T01f,T01p,
c     &        vgasf(2),vgasp(2),T02i,T02f,T02p,
c     &        vgasf(3),vgasp(3),T03i,T03f,T03p,
c     &        BBf(2),BBp(2),scr,scr,scr,
c     &        BBf(3),BBp(3),scr,scr,scr,
c     &        BBf(4),BBp(4),scr,scr,scr,
c     &        Ef,Ep,R00i,R00f,R00p,
c     &        vradf(1),vradp(1),R01i,R01f,R01p,
c     &        vradf(2),vradp(2),R02i,R02f,R02p,
c     &        vradf(3),vradp(3),R03i,R03f,R03p,
c     &        (s(j),j=1,2),si,sf,sp,
c     &        (uradconf(j),uradcovf(j),j=1,4),
c     &        (ugasconf(j),ugascovf(j),j=1,4),
c     &        (uradconp(j),uradcovp(j),j=1,4),
c     &        (ugasconp(j),ugascovp(j),j=1,4)

         return

 10   ifinish=1

      return
      end


      subroutine readtype3(dt,gn,gv,rhof,rhop,rhou0i,rhou0f,rhou0p,
     &        uf,up,T00i,T00f,T00p,vgasf,vgasp,T01i,T01f,T01p,
     &        T02i,T02f,T02p,T03i,T03f,T03p,BBf,BBp,
     &        Ef,Ep,R00i,R00f,R00p,vradf,vradp,R01i,R01f,R01p,
     &        R02i,R02f,R02p,R03i,R03f,R03p,s,si,sf,sp,
     &        uradconf,uradcovf,ugasconf,ugascovf,
     &        uradconp,uradcovp,ugasconp,ugascovp,Gam,ifinish,errorabs)

c     Read in data in Jon's new format (209 numbers)

      implicit double precision (a-h,o-z)
      integer eomtype,itermode
      dimension isc(4),gn(4,4),gv(4,4),vgasp(3),vgasf(3),
     &     BBp(4),BBf(4),vradp(3),vradf(3),s(5),
     &     ugasconf(4),ugascovf(4),ugasconp(4),ugascovp(4),
     &     uradconf(4),uradcovf(4),uradconp(4),uradcovp(4),
     &     ugasconb(4),ugascovb(4),uradconb(4),uradcovb(4)

      ifinish=0

c         read (11,*,end=10) (isc(j),j=1,4),
         read (11,*,end=10) failtype,myid,failnum,gotfirstnofail,
     &        eomtype,itermode,
     &        errorabs,iters,dt,nstep,steppart,Gam,
     &        ((gn(j,k),k=1,4),j=1,4),
     &        ((gv(j,k),k=1,4),j=1,4),
     &        rhof,scr,rhob,rhop,src,src,rhou0i,rhou0f,rhou0p,
     &        uf,scr,ub,up,src,src,T00i,T00f,T00p,
     &        vgasf(1),scr,scr,vgasp(1),src,src,T01i,T01f,T01p,
     &        vgasf(2),scr,scr,vgasp(2),src,src,T02i,T02f,T02p,
     &        vgasf(3),scr,scr,vgasp(3),src,src,T03i,T03f,T03p,
     &        BBf(2),scr,scr,BBp(2),src,src,scr,scr,scr,
     &        BBf(3),scr,scr,BBp(3),src,src,scr,scr,scr,
     &        BBf(4),scr,scr,BBp(4),src,src,scr,scr,scr,
     &        Ef,scr,Eb,Ep,src,src,R00i,R00f,R00p,
     &        vradf(1),scr,scr,vradp(1),src,src,R01i,R01f,R01p,
     &        vradf(2),scr,scr,vradp(2),src,src,R02i,R02f,R02p,
     &        vradf(3),scr,scr,vradp(3),src,src,R03i,R03f,R03p,
     &        s(1),scr,scr,s(2),src,src,si,sf,sp,
     &        (uradconf(j),uradcovf(j),j=1,4),
     &        (ugasconf(j),ugascovf(j),j=1,4),
     &        (uradconb(j),uradcovb(j),j=1,4),
     &        (ugasconb(j),ugascovb(j),j=1,4),
     &        (uradconp(j),uradcovp(j),j=1,4),
     &        (ugasconp(j),ugascovp(j),j=1,4)

         write (*,"(A,1X,1F21.15)") 'TEST',ugascovp(4)

         write (14,"(1I5,2X,1E21.15,2X,1I8,8X,1I1,8X)",advance="no")
     &        iters,errorabs,eomtype,itermode

         if(errorabs.lt.1E-6) then
            write (13,"(1X,A)",advance="no") ' GOOD   '
            write (14,"(1X,A)",advance="no") ' GOOD   '
         else
            write (13,"(1X,A)",advance="no") '  BAD   '
            write (14,"(1X,A)",advance="no") '  BAD   '
         endif
         
         write (13,*) 'HARMJM rho, u_g, u^mu Erf urad^mu: '
     &        ,rhof,uf,
     &        (ugasconf(j),j=1,4),
     &        Ef,
     &        (uradconf(j),j=1,4)

c         write (*,*) ' gn: ',((gn(i,j),j=1,4),i=1,4)
c         write (*,*) ' gv: ',((gv(i,j),j=1,4),i=1,4)
c         write (*,*) ' rhou0i, rhou0f, rhou0p: ',rhou0i,rhou0f,rhou0p
c         write (*,*) ' uf, up, T001, T00f, T00p: ',uf,up,T00i,T00f,T00p
c         write (*,*) ' ugasconp: ',(ugasconp(j),j=1,4)
c         write (*,*) ' ugasconf: ',(ugasconf(j),j=1,4)
c         write (*,*) ' BBp: ',(BBp(j),j=2,4)
c         write (*,*) ' BBf: ',(BBf(j),j=2,4)
         write (*,*) ' Ef, Ep, R00i, R00f, R00p: ',Ef,Ep,R00i,R00f,R00p
c         write (*,*) ' uradconp: ',(uradconp(j),j=1,4)
c         write (*,*) ' uradconf: ',(uradconf(j),j=1,4)
         write (*,*) ' si, sf, sp: ',si,sf,sp

c         write (*,*) (isc(j),j=1,4),dt,
c     &        ((gn(j,k),k=1,4),j=1,4),
c     &        ((gv(j,k),k=1,4),j=1,4),
c     &        rhof,rhop,rhou0i,rhou0f,rhou0p,
c     &        uf,up,T00i,T00f,T00p,
c     &        vgasf(1),vgasp(1),T01i,T01f,T01p,
c     &        vgasf(2),vgasp(2),T02i,T02f,T02p,
c     &        vgasf(3),vgasp(3),T03i,T03f,T03p,
c     &        BBf(2),BBp(2),scr,scr,scr,
c     &        BBf(3),BBp(3),scr,scr,scr,
c     &        BBf(4),BBp(4),scr,scr,scr,
c     &        Ef,Ep,R00i,R00f,R00p,
c     &        vradf(1),vradp(1),R01i,R01f,R01p,
c     &        vradf(2),vradp(2),R02i,R02f,R02p,
c     &        vradf(3),vradp(3),R03i,R03f,R03p,
c     &        (s(j),j=1,2),si,sf,sp,
c     &        (uradconf(j),uradcovf(j),j=1,4),
c     &        (ugasconf(j),ugascovf(j),j=1,4),
c     &        (uradconp(j),uradcovp(j),j=1,4),
c     &        (ugasconp(j),ugascovp(j),j=1,4)

         return

 10   ifinish=1

      return
      end


      

      double precision function mymax(x,y)
      double precision x,y

      mymax=max(x,y)
      if(x.ne.x) then
         mymax=x
      endif
      if(y.ne.y) then
         mymax=y
      endif

      if(isNan(x)) then
         mymax=x
      endif
      if(isNan(y)) then
         mymax=y
      endif

      return
      end


      double precision function myabs(x)
      double precision x

      myabs=abs(x)
      if(isNan(x)) then
         myabs=x
      endif

      return
      end


      double precision function mydiv(x,y)
      double precision x,y

      mydiv=x/y
      if(x.ne.x) then
         mydiv=x
      endif
      if(y.ne.y) then
         mydiv=y
      endif

      if(isNan(x)) then
         mydiv=x
      endif
      if(isNan(y)) then
         mydiv=y
      endif

      return
      end


      subroutine solveu0 (con)

c     Calculates u^0 given the other three components of the 4-velocity,
c     u^1, u^2, u^3

      implicit double precision (a-h,o-z)
      dimension con(4), gn(4,4),gv(4,4)
      common/metric/gn,gv

      au0=gv(1,1)
      bu0=0.d0
      cu0=1.d0
      do j=2,4
         bu0=bu0+2.d0*gv(1,j)*con(j)
      do k=2,4
         cu0=cu0+gv(j,k)*con(j)*con(k)
      enddo
      enddo
         
      con(1)=(-bu0-sqrt(bu0*bu0-4.d0*au0*cu0))/(2.d0*au0)

      return
      end



      subroutine contocov(vecin,vecout)

c     Converts a contravariant 4-vector to a covariant 4-vector

      implicit double precision (a-h,o-z)
      dimension vecin(4),vecout(4),gn(4,4),gv(4,4)
      common/metric/gn,gv

      do i=1,4
         vecout(i)=0.d0
      do j=1,4
         vecout(i)=vecout(i)+gv(i,j)*vecin(j)
      enddo
      enddo

      return
      end



      subroutine covtocon(vecin,vecout)

c     Converts a covariant 4-vector to a contravariant 4-vector

      implicit double precision (a-h,o-z)
      dimension vecin(4),vecout(4),gn(4,4),gv(4,4)
      common/metric/gn,gv

      do i=1,4
         vecout(i)=0.d0
      do j=1,4
         vecout(i)=vecout(i)+gn(i,j)*vecin(j)
      enddo
      enddo

      return
      end



      function condotcov(con,cov)

c     Calculates the dot product con^mu cov_mu

      implicit double precision (a-h,o-z)
      dimension con(4),cov(4),gn(4,4),gv(4,4)
      common/metric/gn,gv

      condotcov=0.d0
      do i=1,4
         condotcov=condotcov+con(i)*cov(i)
      enddo

      return
      end



      subroutine calcbconbcov(BB,ucon,ucov,bcon,bcov,bsq)

c     Given B(1), B(2), B(3), and gas 4-velocity, calculates the
c     four-vectors b^mu and b_mu, and bsq = b^mub_mu

      implicit double precision (a-h,o-z)
      dimension BB(4),ucon(4),ucov(4),bcon(4),bcov(4),gn(4,4),gv(4,4)
      common/metric/gn,gv

      BB(1)=0.d0
      udotB=condotcov(BB,ucov)
      do i=1,4
         bcon(i)=(BB(i)+udotB*ucon(i))/ucon(1)
      enddo

      call contocov(bcon,bcov)

      bsq=condotcov(bcon,bcov)

      return
      end



      subroutine calcTmunu(rho,u,bsq,Gam,ucon,ucov,
     &     bcon,bcov,Tmunu)

c     Given rho, u, 4-velocity, magnetic 4-vector, calculates the gas
c     stress energy tensor T^mu_nu

      implicit double precision (a-h,o-z)
      dimension ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4),
     &     gn(4,4),gv(4,4),delta(4,4)
      common/metric/gn,gv

      call makedelta(delta)

      p=(Gam-1.d0)*u
      bsq2=0.5d0*bsq

      do i=1,4
      do j=1,4

         Tmunu(i,j)=(rho+u+p+bsq)*ucon(i)*ucov(j)
     &        +(p+bsq2)*delta(i,j)
     &        -bcon(i)*bcov(j)

      enddo
      enddo

c     Replace T^0_0 -> T^0_0 + rho*u^0

      Tmunu(1,1)=Tmunu(1,1)+rho*ucon(1)

      return
      end



      subroutine calcRmunu(E,ucon,ucov,Rmunu)

c     Given the radiation frame energy density E and 4-velocity,
c     calculates the radiation stress-energy tensor R^mu_nu

      implicit double precision (a-h,o-z)
      dimension ucon(4),ucov(4),Rmunu(4,4),
     &     gn(4,4),gv(4,4),delta(4,4)
      common/metric/gn,gv

      call makedelta(delta)

      four3=4.d0/3.d0
      one3=1.d0/3.d0

      do i=1,4
      do j=1,4

         Rmunu(i,j)=four3*E*ucon(i)*ucov(j)
     &        +one3*E*delta(i,j)

      enddo
      enddo

      return
      end



      subroutine makedelta(delta)

c     Calculate Kronecker delta

      implicit double precision (a-h,o-z)
      dimension delta (4,4)

      do i=1,4
      do j=1,4

         if (i.eq.j) then
            delta(i,j)=1.d0
         else
            delta(i,j)=0.d0
         endif

      enddo
      enddo

      return
      end



      subroutine Rmunuinvert(Rtcov,E,ucon,ucov,ugascon,Rmunu)

c     Given the row R^t_mu of the radiation tensor, solves for the
c     radiation frame energy density E and 4-velocity and calculates the
c     full tensor R^mu_nu

      implicit double precision (a-h,o-z)
      dimension Rtcov(4),Rtcon(4),ucon(4),ucov(4),Rmunu(4,4),
     &     ugascon(4),gn(4,4),gv(4,4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/metric/gn,gv

c     Convert R^0_mu to R^0^mu

      call covtocon(Rtcov,Rtcon)
c      write (*,*) ' Rtcov: ',(Rtcov(i),i=1,4)
c      write (*,*) ' Rtcon: ',(Rtcon(i),i=1,4)

c     Set up and solve quadratic equation for E

      aquad=gn(1,1)
      bquad=-2.d0*Rtcon(1)
      cquad=0.d0
      do i=1,4
      do j=1,4
         cquad=cquad-gv(i,j)*Rtcon(i)*Rtcon(j)
      enddo
      enddo
      cquad=3.d0*cquad
      disc=bquad*bquad-4.d0*aquad*cquad

c     Make sure signs are okay for a physical solution. If not, go to 10
c     for alternative calculation

      if (aquad*cquad.ge.0.d0.or.disc.lt.0.d0) go to 10

c     Use negative sign of discriminant and solve for E and ucon

c      write (*,*) ' negative sign of discriminant '
      E=2.d0*cquad/(-bquad+sqrt(disc))
c      write (*,*) ' a, b, c, disc, E: ',aquad,bquad,cquad,disc,E

c     Make sure E is positive. If not, go to 10

      if (E.lt.0.d0) go to 10

      ucon(1)=0.5d0*sqrt(3.d0*Rtcon(1)/E-gn(1,1))
      do i=2,4
         ucon(i)=(3.d0*Rtcon(i)-E*gn(1,i))/(4.d0*E*ucon(1))
      enddo

c     Make sure the Lorentz factor is below the ceiling. If not go to 10

      if (ucon(1).le.gammaradceiling*sqrt(-gn(1,1))) then
         go to 20
      else
         go to 10
      endif

c     This segment is for problem cases. We set gamma_radiation equal to
c     its ceiling value and solve for E and u^i without using R^00.

 10   ucon(1)=gammaradceiling*sqrt(-gn(1,1))
c      write (*,*) ' gamma_rad hit ceiling: g^tt, urad^t = ',
c     &     gn(1,1),ucon(1)
c      write (*,*) ' Rtcon: ',(Rtcon(j),j=1,4)

      aquad=0.d0
      bquad=0.d0
      cquad=1.d0+gv(1,1)*ucon(1)*ucon(1)
      do i=2,4
         bquad=bquad+1.5d0*gv(1,i)*Rtcon(i)
         cquad=cquad-0.5d0*gv(1,i)*gn(1,i)
      do j=2,4
         aquad=aquad+9.d0*gv(i,j)*Rtcon(i)*Rtcon(j)/
     &        (16.d0*ucon(1)*ucon(1))
         bquad=bquad-3.d0*gv(i,j)*(Rtcon(i)*gn(1,j)+
     &        Rtcon(j)*gn(1,i))/(16.d0*ucon(1)*ucon(1))
         cquad=cquad+gv(i,j)*gn(1,i)*gn(1,j)/
     &        (16.d0*ucon(1)*ucon(1))
      enddo
      enddo

      disc=bquad*bquad-4.d0*aquad*cquad

c     Check if disc > 0. If not, we have trouble again, and we need to
c     use yet another scheme!

      if (disc.lt.0.d0) go to 30

      E=(-bquad-sqrt(disc))/(2.d0*cquad)
c      write (*,*) ' a, b, c, disc, E: ',aquad,bquad,cquad,disc,E

      do i=2,4
         ucon(i)=(3.d0*Rtcon(i)-E*gn(1,i))/(4.d0*E*ucon(1))
      enddo
c      write (*,*) ' ucon: ',(ucon(i),i=1,4)

      sum=0.d0
      do i=1,4
      do j=1,4
         sum=sum+gv(i,j)*ucon(i)*ucon(j)
      enddo
      enddo
c      write (*,*) ' check norm: ',sum
      go to 20

c     Third try! What should we do here?

 30   continue

c     Last-ditch effort. Set radiation velocity equal to gas velocity

c      write (*,*) ' Error: No solution for radiation '
      do i=1,4
         ucon(i)=ugascon(i)
      enddo

      E=3.d0*Rtcon(1)/(4.d0*ucon(1)**2-gn(1,1))

c     Calculate urad_mu

 20   call contocov(ucon,ucov)

c     Calculate the full tensor R^mu_nu

      call calcRmunu(E,ucon,ucov,Rmunu)

      return
      end



      subroutine MHDinvert(prim,iflag,jflag,ientropy,itertot,guesstype)

c     Given initial primitives prim(4), solves for primitives that
c     satisfy the initial pre-radiation conserved quantities: rhou0c, s,
c     Ttcovc(4), Rtcovc(4)

      implicit double precision (a-h,o-z)
      real*8 itertot
      dimension prim(4),primsave(4),Ttcovc(4),Rtcovc(4),BBc(4),
     &     ucon(4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0c,sc,Ttcovc,Rtcovc,
     &     BBc,dt
      external funcMHD1,funcMHD2,uMHD1,uMHD2

      write (*,*)
      write (*,*) ' initial primitives: ',(prim(j),j=1,4)

      if(guesstype.le.1) then
c     First do one round of Newton-Raphson just on the velocities

      call Newton3(prim,iter,iflag,jflag,funcMHD1)
      itertot=itertot+iter
      write (12,*) ' MHD inversion, velocities only ',
     &        iter      
      write (*,*)

c     Next work only on u_g using the energy equation

      call usolveMHD(prim,iter,iflag,jflag,funcMHD1,uMHD1)
      write (*,*) ' usolveMHD done, u: ',prim(1)
      write (*,*)
      write (12,*) ' MHD inversion, u_g only (energy) '

      do i=1,4
         primsave(i)=prim(i)
      enddo

      endif

c     Carry out the full Newton-Raphson MHD inversion via the
c     energy equation

      err4=1.0d5
      call Newton4(prim,iter,iflag,jflag,funcMHD1,err4)
      write (12,*) ' MHD inversion, energy equation ',
     &     iter
      itertot=itertot+iter

c     Check if the iterations converged. If not, re-solve using the
c     entropy equation.

      if (iflag.eq.0) then
         write (*,*)
         write (*,*) ' Energy equation converged: iter ',
     &        iter
         write (*,*) ' primitives: ',(prim(j),j=1,4)
      else
         write (*,*)
         write (*,*) ' ERROR: no convergence with energy equation: ',
     &        iter
         write (*,*) ' switching to entropy equation '
      endif

      if (iflag.eq.0.and.prim(1).lt.0.d0) then

c     Check if the internal energy is okay. If not, re-solve using the
c     entropy equation.

         write (*,*)
         write (*,*) ' ERROR: u is negative!! '
         write (*,*) ' switching to entropy equation '
         iflag=2

      else

c     Check if entropy looks okay. If not, re-solve using the entropy
c     equation.

         pressure=Gam1*prim(1)

         do j=2,4
            ucon(j)=prim(j)
         enddo
         call solveu0(ucon)
         rhoi=rhou0c/ucon(1)

         entr=log(pressure**en/
     &        rhoi**en1)
         if ((entr-sc/rhou0c).lt.(-dlogmax)) then
            write (*,*)
            write (*,*) ' ERROR: s is too low!! '
            write (*,*) ' (entropy,sc)/(rho u^0): ',entr,sc/rhou0c
            write (*,*) ' switching to entropy equation '
            iflag=3
         endif

      endif

      if (iflag.ge.1) then

c     Restore saved primitives to post-3+1 solution before proceeding to
c     solving the entropy equation
         
c     Reset jflag
         jflag=0

            

         do i=1,4
            prim(i)=primsave(i)
         enddo
         ientropy=1

c     Next work only on u_g using the entropy equation

c      call usolveMHD(prim,iter,iflag,jflag,funcMHD2,uMHD2)
c      write (*,*)
c      write (*,*) ' usolveMHD done, u: ',prim(1)
c      write (*,*)
c      write (12,*) ' Radiation inversion, u_g only (entropy) '

c     Carry out full Newton-Raphson inversion with the entropy equation

      err4=1.0d6
      call Newton4(prim,iter,iflag,jflag,funcMHD2,err4)
      itertot=itertot+iter

      if (iflag.eq.9) then
         write (*,*) ' ERROR: u is negative! '
         write (*,*) ' reverting to previous solution '
         do j=1,4
            prim(j)=primsave(j)
         enddo
      else
         write (12,*) ' MHD inversion, entropy equation ',
     &        iter
         write (*,*)
      endif

      if (iflag.eq.0) then
         write (*,*) ' entropy equation converged: iter ',
     &        iter
         write (*,*) ' primitives: ',(prim(j),j=1,4)
      else
         write (*,*) ' ERROR: no convergence with entropy equation ',
     &        iter
         write (*,*) ' reverting to saved solution '
         do j=1,4
            prim(j)=primsave(j)
         enddo
         iflag=9
      endif

      endif

      return
      end



      subroutine radsource(prim,iter,iflag,jflag,ientropy,
     &     itertot,guesstype)

c     Given initial primitives prim(4), solves for primitives that
c     satisfy the post-radiation equations

      implicit double precision (a-h,o-z)
      real*8 itertot
      dimension prim(4),primsave(4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      external funcrad1,funcrad2,uerr1,uerr2
      dimension ugascon(4)

      itereng=0
      erreng=1.d3
      iterent=0
      errent=1.d3

      write (*,*)
      write (*,*) ' initial primitives: ',(prim(j),j=1,4)
      write (*,*)

      if(guesstype.le.1) then

c     First do one round of Newton-Raphson just on the velocities

      call Newton3(prim,iter,iflag,jflag,funcrad1)
      itertot=itertot+iter
      write (12,*) ' Radiation inversion, velocities only ',
     &        iter      
      write (*,*)

      do i=1,4
         primsave(i)=prim(i)
      enddo

      if (ientropy.eq.0) then

c     ientropy=0, so we will first try the energy equation. Initially
c     work only on u_g using the energy equation. If ientropy=1, we skip
c     all this and go directly to working with the entropy equation.

      call usolverad(prim,iter,iflag,jflag,funcrad1,uerr1)
      write (*,*) ' usolverad done, u: ',prim(1)
      write (*,*)
      write (12,*) ' Radiation inversion, u_g only (energy) '
      if (prim(1).gt.0.d0) primsave(1)=prim(1)

      endif

c     Carry out full Newton-Raphson on all four primitives using
c     Newton-Raphson

      err4=1.0d8
      call Newton4(prim,iter,iflag,jflag,funcrad1,err4)
      itertot=itertot+iter
      itereng=iter
      erreng=err4
      write (*,*)
      write (12,*) ' Radiation inversion, energy equation ',
     &        iter

      if (iflag.eq.0) then
         write (*,*) ' Energy equation converged: iter ',
     &        iter
         write (*,*) ' primitives: ',(prim(j),j=1,4)
         do j=2,4
            ugascon(j)=prim(j)
         enddo
         call solveu0(ugascon)
         if (erreng.lt.1E-6
     &        .and.prim(1).gt.0.0
     &        .and.jflag.eq.0
     &        .and.prim(1).eq.prim(1)
     &        .and.prim(2).eq.prim(2)
     &        .and.prim(3).eq.prim(3)
     &        .and.prim(4).eq.prim(4)
     &        .and.ugascon(1).eq.ugascon(1)
     &        ) then
            write (14,"(1A)",advance="no") '  GOOD   '
            write (13,"(1A)",advance="no") '  GOOD   '
         else if (erreng.lt.1E-6
     &        .and.prim(1).eq.prim(1)
     &        .and.prim(2).eq.prim(2)
     &        .and.prim(3).eq.prim(3)
     &        .and.prim(4).eq.prim(4)
     &        .and.ugascon(1).eq.ugascon(1)
     &        ) then
            write (14,"(1A)",advance="no") '   BADNEG'
            write (13,"(1A)",advance="no") '   BADNEG'
         else
            write (14,"(1A)",advance="no") '   BAD   '
            write (13,"(1A)",advance="no") '   BAD   '
         endif
         write (14,*) itereng,erreng,iterent,errent
         return
      endif

c     If energy equation does not converge, calculate using the entropy
c     equation.

      write (*,*) ' ERROR: no convergence with energy equation: ',
     &     iter
      write (*,*) ' Proceed to entropy equation '
      write (*,*)
      write (12,*) ' ERROR: no convergence with energy equation: '

c     Restore saved primitives and do Newton-Raphson with the entropy
c     equation

      do i=1,4
         prim(i)=primsave(i)
      enddo

      endif

c     Next work only on u_g using the entropy equation. Since we are
c     caurrently using the entropy equation even for the energy equation
c     step, we do not need to repeat the 1D search. If and when we
c     modify uerr1 to do the proper energy equation, we need this second
c     round of 1D search using uerr2.

c      call usolverad(prim,iter,iflag,jflag,funcrad2,uerr2)
c      write (*,*)
c      write (*,*) ' usolverad done, u: ',prim(1)
c      write (*,*)
c      write (12,*) ' Radiation inversion, u_g only (entropy) '

      err4=1.0d8
      call Newton4(prim,iter,iflag,jflag,funcrad2,err4)
      itertot=itertot+iter
      iterent=iter
      errent=err4
      write (*,*)
      write (12,*) ' Radiation inversion, entropy equation ',
     &        iter
      do j=2,4
         ugascon(j)=prim(j)
      enddo
      call solveu0(ugascon)
      if ((erreng.lt.1E-6.or.errent.lt.1E-6)
     &     .and.prim(1).gt.0.0
     &     .and.jflag.eq.0
     &     .and.prim(1).eq.prim(1)
     &     .and.prim(2).eq.prim(2)
     &     .and.prim(3).eq.prim(3)
     &     .and.prim(4).eq.prim(4)
     &     .and.ugascon(1).eq.ugascon(1)
     &     ) then
         write (14,"(1A)",advance="no") '  GOOD   '
         write (13,"(1A)",advance="no") '  GOOD   '
      else if ((erreng.lt.1E-6.or.errent.lt.1E-6)
     &     .and.prim(1).eq.prim(1)
     &     .and.prim(2).eq.prim(2)
     &     .and.prim(3).eq.prim(3)
     &     .and.prim(4).eq.prim(4)
     &     .and.ugascon(1).eq.ugascon(1)
     &     ) then
         write (14,"(1A)",advance="no") '   BADNEG'
         write (13,"(1A)",advance="no") '   BADNEG'
      else
         write (14,"(1A)",advance="no") '   BAD   '
         write (13,"(1A)",advance="no") '   BAD   '
      endif
      write (14,*) itereng,erreng,iterent,errent

      if (iflag.eq.0) then
         write (*,*) ' Entropy equation converged: iter ',
     &        iter
         write (*,*) ' primitives: ',(prim(j),j=1,4)
         return
      endif

      write (*,*) ' ERROR: no convergence with entropy equation: ',
     &     iter
      write (12,*) ' ERROR: no convergence with entropy equation: '

c     We should never reach this point. Unclear what to do in this case!
c     We could keep the solution or restore the saved primitives.

c      do i=1,4
c         prim(i)=primsave(i)
c      enddo

      return
      end



      subroutine Newton4 (prim0,iter,iflag,jflag,func,err4)

c     Calculates iteratively by the Newton-Raphson technique the
c     solution to a set of four non-linear equations. The initial
c     primitives are in the array prim0, and the final solution is
c     returned in the same array. func is the function that computes the
c     four error terms for a given set of primitives.

      implicit double precision (a-h,o-z)
      dimension prim0(4),error0(4),errornorm(4),prim(4),error(4),
     &     AJac(4,4),indx(4),Ttcov(4),Rtcov(4),BB(4),
     &     primold(4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      external func
      double precision mymax,myabs,mydiv

c     niter is the maximum number of Newton-Raphson iterations
c     iflag=0 means that a good solution was found

      niter=20
      iflag=0
      jflag=0

c     Do Newton-Raphson until err is smaller than tolerance tol

      iter=0
c      write (*,*)
      do i=1,niter

c     Make sure u == prim0(1) is in safe territory

      prim0(1)=mymax(prim0(1),uminfac*rhou0)

      call func(prim0,error0,errornorm,err4,err3,iflag,jflag)
      write (*,*) ' Newton-Raphson-4 '
      write (*,*) ' i, primitives, error, errornorm, err4: ',i,
     &     (prim0(j),j=1,4),(error0(j),j=1,4),
     &     (errornorm(j),j=1,4),err4

c     iflag=9 means a serious error in the value of u. This can be
c     tolerated for a few steps (up to iter=itermax). After that, return
c     with iflag=9.

      if (iflag.eq.9.and.iter.ge.itermax) then
         write (*,*) 'Hit iflag=',iflag,'iter=',iter
         return
      endif

c     If all the four equations give fractional errors less than tol,
c     return

      if (err4.lt.tol) then
         write (*,*) 'Hit err4=',err4
         return
      endif

      if (i.gt.1) then
         sum=0.d0
         do j=1,4
            sum=sum+myabs(prim0(j)-primold(j))
         enddo
         if (sum.eq.0.d0) then
            write (12,*) ' sum = 0! '
            return
         endif
      endif

      do j=1,4
         primold(j)=prim0(j)
      enddo

c     Calculate the Jacobian numerically by shifting the primitives one
c     by one by a fraction eps of their current values and calculating
c     the errors.

      do j=1,4
         
      do k=1,4
         prim(k)=prim0(k)
      enddo

         if (j.eq.1.or.myabs(prim0(j)).gt.dvmin) then
            dprim=prim0(j)*eps
         else
            dprim=dvmin*eps
         endif
         prim(j)=prim0(j)+dprim

         call func(prim,error,errornorm,err4,err3,iflag,jflag)
c         write (*,*) ' error: ',j,(error(k),k=1,4)
      do k=1,4
         AJac(k,j)=(error(k)-error0(k))/dprim
      enddo

      enddo
c      write (*,*) ' Jacobian: ',((AJac(k,j),j=1,4),k=1,4)

c     Invert the Jacobian using subroutines from Numerical Recipes and
c     compute the shifts to the primitives

      call ludcmp(AJac,4,4,indx,d,retval)
      if(retval.ne.0) then
         err4=128.0
         return
      endif
      call lubksb(AJac,4,4,indx,error0)

c      do j=1,4
c         write (*,*) ' j, prim0(j), dprim(j): ',j,prim0(j),
c     &        error0(j)
c      enddo

c     Apply the Newton-Raphson shifts

      do j=1,4
         prim0(j)=prim0(j)-error0(j)
      enddo

      iter=iter+1

      enddo

      iflag=1

      return
      end



      subroutine Newton3 (prim0,iter,iflag,jflag,func)

c     Calculates iteratively by the Newton-Raphson technique the
c     solution to a set of three non-linear equations. The initial
c     primitives are in the 4-array prim0, of which the first element is
c     not varied and only the other three are solved for. The final
c     solution is returned in the same 4-array with the first element
c     unchanged. func is the function that computes the four error terms
c     for a given set of primitives. Only the last three errors are
c     used.

      implicit double precision (a-h,o-z)
      dimension prim0(4),prim30(3),error30(3),prim3(3),error3(3),
     &     AJac(3,3),indx(3),Ttcov(4),Rtcov(4),BB(4),errornorm(3)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      external func
      double precision mymax,myabs,mydiv

c     niter is the maximum number of Newton-Raphson
c     iterations. Currently we do only one iteration of Newton3.
c     iflag=0 measn that a good solution was found

      niter=1
c      niter=5
      iflag=0
      jflag=0

c     Make sure u == primsave(1) is in safe territory

      primsave=mymax(prim0(1),uminfac*rhou0)
      do i=1,3
         prim30(i)=prim0(i+1)
      enddo

c     Do Newton-Raphson until err is less than tolerance tol

      iter=0
c      write (*,*)
      do i=1,100

      call func3(primsave,prim30,error30,errornorm
     &        ,err4,err3,iflag,jflag,func)
      write (*,*) ' Newton-Raphson-3 '
      write (*,*) ' iter, primitives, error, errornorm, err3: ',iter,
     &     (prim30(j),j=1,3),(error30(j),j=1,3),
     &     (errornorm(j),j=1,3),err3

      if (iter.ge.niter) then
         prim0(1)=primsave
         do j=1,3
            prim0(j+1)=prim30(j)
         enddo
         return
      endif

c     If all the three equations give fractional errors less than tol,
c     return

      if (err3.lt.tol) then
         prim0(1)=primsave
         do j=1,3
            prim0(j+1)=prim30(j)
         enddo
         return
      endif

c     Calculate the Jacobian numerically by shifting the primitives one
c     by one by a fraction eps of their current values and calculating
c     the errors.

      do j=1,3
         
      do k=1,3
         prim3(k)=prim30(k)
      enddo

         if (myabs(prim30(j)).gt.dvmin) then
            dprim=prim30(j)*eps
         else
            dprim=dvmin*eps
         endif
         prim3(j)=prim30(j)+dprim

         call func3(primsave,prim3,error3,errornorm,
     &        err4,err3,iflag,jflag,func)
c         write (*,*) ' error: ',j,(error3(k),k=1,3),err3
      do k=1,3
         AJac(k,j)=(error3(k)-error30(k))/dprim
      enddo

      enddo
c      write (*,*) ' Jacobian: ',((AJac(k,j),j=1,3),k=1,3)

c     Invert the Jacobian using subroutines from Numerical Recipes and
c     compute the shifts to the primitives

      call ludcmp(AJac,3,3,indx,d,retval)
      if(retval.ne.0) then
         err3=1.0d4
         return
      endif
      call lubksb(AJac,3,3,indx,error30)

c      do j=1,3
c         write (*,*) ' j, prim30(j), dprim(j): ',j,prim0(j),
c     &        error30(j)
c      enddo

c     Apply the Newton-Raphson shifts

      do j=1,3
         prim30(j)=prim30(j)-error30(j)
      enddo

      iter=iter+1

      enddo

      iflag=1
c      write (*,*) ' too many iterations! '

      prim0(1)=primsave
      do i=1,3
         prim0(i+1)=prim30(i)
      enddo

      return
      end



      subroutine usolveMHD(prim,iter,iflag,jflag,funcMHD,uMHD)

c     Solves a 1D equation for u, using either the energy or entropy
c     equation without radiation source term

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),errornorm(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4),
     &     urcon(4),urcov(4),Rmunu(4,4),Gcon(4),Gcov(4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcMHDd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov
      external funcMHD,uMHD
      double precision mymax,myabs,mydiv

c     Call funcMHD and obtain basic parameters needed for solving the 1D
c     energy equation: rho, Ehat

      u0=prim(1)
      call funcMHD(prim,error,errornorm,err4,err3,iflag,jflag)
      rho0=rho
      Ehat0=Ehat
c      write (*,*) ' usolveMHD: ',(prim(i),i=1,4),rho0,Ehat0

c     Calculate initial error

      call uMHD(u0,u0,rho0,Ehat0,ffkap,arad,dtau,err0)
      write (*,*) ' Bracketing the solution '
      write (*,*) ' u, err: ',u0,err0

c     Decide whether u is too low or too high and search accordingly to
c     bracket the solution for u

      if (err0.lt.0.d0) then

         ur=u0
         er=err0

         do i=1,50
           ul=ur
           el=er
           ur=2.d0*ur
           call uMHD(ur,u0,rho0,Ehat0,ffkap,arad,dtau,er)
           write (*,*) ' u, err: ',ur,er
           if (er.ge.0.d0) go to 10
         enddo
         write (*,*) ' No bracket! '
         return

      else

         ul=u0
         el=err0

         do i=1,50
           ur=ul
           er=el
           ul=0.5d0*ul
           call uMHD(ul,u0,rho0,Ehat0,ffkap,arad,dtau,el)
           write (*,*) ' u, err: ',ul,el
           if (er.lt.0.d0) go to 10
         enddo
         write (*,*) ' No bracket! '
         return

      endif

c     Bracketing is done. Now solve for u by bisection

 10   continue
      write (*,*)
      write (*,*) ' Solving by bisection '

      do i=1,50
         umid=0.5d0*(ul+ur)
         call uMHD(umid,u0,rho0,Ehat0,ffkap,arad,dtau,emid)
         write (*,*) ' u, err: ',umid,emid
         if (emid.ge.0.d0) then
            ur=umid
            er=emid
         else
            ul=umid
            el=emid
         endif
         if (myabs(ur-ul).lt.epsbis*ul) then
            prim(1)=0.5d0*(ul+ur)
            return
         endif

      enddo

      prim(1)=0.5d0*(ul+ur)
      write (*,*) ' bisection did not converge! '

      return
      end



      subroutine uMHD1(u,u0,rho,Ehat,ffkap,arad,dtau,err)

c     Error function for 1D search in u for the energy equation without
c     radiation source term. Currently, this has the entropy equation.

      implicit double precision (a-h,o-z)
      dimension Ttcov(4),Rtcov(4),BB(4)
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt

c     Compute entropy and compute deviation from conserved entropy
      
      Tgas=Gam1*u/rho
c      B4pi=arad*Tgas**4
c      ff=ffkap*rho*rho/Tgas**(3.5d0)
c      Gdt=ff*(Ehat-B4pi)*dt
      entropy=rhou0*log((Gam1*u)**en/rho**en1)
      err=entropy-s
c      err=entropy-s-Gdtau
c      write (*,*) ' rhou0, rho, u, entropy, s, err: ',
c     &     rhou0,rho,u,entropy,s,err

      return
      end



      subroutine uMHD2(u,u0,rho,Ehat,ffkap,arad,dtau,err)

c     Error function for 1D search in u for the entropy equation without
c     radiation source term.

      implicit double precision (a-h,o-z)
      dimension Ttcov(4),Rtcov(4),BB(4)
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      
c     Compute entropy and compute deviation from conserved entropy
      
      Tgas=Gam1*u/rho
c      B4pi=arad*Tgas**4
c      ff=ffkap*rho*rho/Tgas**(3.5d0)
c      Gdt=ff*(Ehat-B4pi)*dt
      entropy=rhou0*log((Gam1*u)**en/rho**en1)
      err=entropy-s
c      err=entropy-s-Gdtau

      return
      end



      subroutine usolverad(prim,iter,iflag,jflag,funcrad,uerr)

c     Solves a 1D equation for u, using either the energy or entropy
c     equation including the radiation source term

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4),
     &     urcon(4),urcov(4),Rmunu(4,4),Gcon(4),Gcov(4),
     &     errornorm(4)
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcradd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov,u0,iuerr
      external funcrad,uerr,uuerr

c     Call funcrad and obtain basic parameters needed for solving the 1D
c     energy equation: rho, Ehat

      u0=prim(1)
      call funcrad(prim,error,errornorm,err4,err3,iflag,jflag)
      rho0=rho
      Ehat0=Ehat

c     Calculate initial error

      write (*,*) ' Bracketing solution for u_g '
      iuerr=0
      call uuerr(u0,err0,derr)
c      call uerr(u0,u0,rho0,Ehat0,ffkap,arad,dtau,err0)
c      write (*,*) ' u, err: ',u0,err0

c     Decide whether u is too low or too high and search accordingly to
c     bracket the solution for u

      if (err0.lt.0.d0) then

         ur=u0
         er=err0

         do i=1,50
           ul=ur
           el=er
           ur=3.d0*ur
           call uuerr(ur,er,derr)
c           write (*,*) ' u, err: ',ur,er
           if (er.ge.0.d0) go to 10
         enddo
         write (*,*) ' No bracket! '
         return

      else

         ul=u0
         el=err0

         do i=1,50
           ur=ul
           er=el
           ul=0.3d0*ul
           call uuerr(ul,el,derr)
c           write (*,*) ' u, err: ',ul,el
           if (el.lt.0.d0) go to 10
         enddo
         write (*,*) ' No bracket! '
         return

      endif

c     Bracketing is done. Now solve for u using rtsafe

 10   continue

      iuerr=1
      write (*,*)
      write (*,*) ' Solving for u_g using rtsafe '

      uacc=eps*min(ul,ur)
      ugas=rtsafe(uuerr,ul,ur,el,er,uacc)
      prim(1)=ugas

c      write (*,*)
c      write (*,*) ' Solving for u_g by bisection '

c      do i=1,50
c         umid=0.5d0*(ul+ur)
c         call uerr(umid,u0,rho0,Ehat0,ffkap,arad,dtau,emid)
c         write (*,*) ' u, err: ',umid,emid
c         if (emid.ge.0.d0) then
c            ur=umid
c            er=emid
c         else
c            ul=umid
c            el=emid
c         endif
c         if (myabs(ur-ul).lt.epsbis*ul) then
c            prim(1)=0.5d0*(ul+ur)
c            return
c         endif

c      enddo

c      prim(1)=0.5d0*(ul+ur)
c      write (*,*) ' bisection did not converge! '

      return
      end



      subroutine uerr1(u,u0,rho,Ehat0,ffkap,arad,dtau,err)

c     Error function for 1D search in u for the energy equation
c     including the radiation source term. Currently, this has the
c     entropy equation.

      implicit double precision (a-h,o-z)
      dimension Ttcov(4),Rtcov(4),BB(4)
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt

c     Compute entropy and compute deviation from conserved entropy
      
      Tgas=Gam1*u/rho
      B4pi=arad*Tgas**4
      ff=ffkap*rho*rho/Tgas**(3.5d0)

      Ehat=u0+Ehat0-u
      Gdtau=ff*(Ehat-B4pi)*dtau

      entropy=rhou0*log((Gam1*u)**en/rho**en1)
      err=entropy-s-Gdtau

      return
      end



      subroutine uerr2(u,u0,rho,Ehat0,ffkap,arad,dtau,err)

c     Error function for 1D search in u for the entropy equation
c     including the radiation source term

      implicit double precision (a-h,o-z)
      dimension Ttcov(4),Rtcov(4),BB(4)
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      
c     Compute entropy and compute deviation from conserved entropy

      Tgas=Gam1*u/rho
      B4pi=arad*Tgas**4
      ff=ffkap*rho*rho/Tgas**(3.5d0)
      
      Ehat=u0+Ehat0-u
      Gdtau=ff*(Ehat-B4pi)*dtau

      entropy=rhou0*log((Gam1*u)**en/rho**en1)
      err=entropy-s-Gdtau

      return
      end



      subroutine uuerr(u,err,derr)

      implicit double precision (a-h,o-z)
      dimension Ttcov(4),Rtcov(4),BB(4),ucon(4),ucov(4),bcon(4),
     &     bcov(4),Tmunu(4,4),urcon(4),urcov(4),Rmunu(4,4),Gcon(4),
     &     Gcov(4)
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcradd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat0,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov,u0,iuerr

c     The fluid frame error is:
c        rho*ln(p^n/rho^(n+1)) - s - (ffkap*rho^2/T^3.5)*dtau*(Ehat-B4pi)
c     We compute coefficients corresponding to these terms and then
c     compute the error and its derivative wrt u

      c1=Gam1/rho
      Tgas=c1*u

      c2=arad*c1**4
      B4pi=c2*u**4

      c3=ffkap*rho*rho/c1**(3.5d0)
      ff=c3/u**(3.5d0)
c      write (*,*) ' Tgas, B4pi, ff: ',Tgas,B4pi,ff

      c4=u0+Ehat0
      Ehat=c4-u

      Gdtau=(c3*dtau)*u**(-3.5d0)*(c4-u-c2*u**4)

      entropy=rho*log((Gam1*u)**en/rho**en1)

c      write (*,*) ' Ehat, Gdtau, entropy: ',Ehat,Gdtau,entropy

      err=entropy-(s/ucon(1))-Gdtau
c      write (*,*) ' s, ucon(1), err: ',s,ucon(1),err

      if (iuerr.eq.1) then

         derr=(rho*en/u)+3.5d0*c3*dtau*c4*u**(-4.5d0)-
     &        2.5d0*c3*dtau*u**(-3.5d0)+
     &        0.5d0*c3*dtau*c2*u**(-0.5d0)
         write (*,10) u,err,derr
 10      format (' u, err, derr: ',3e20.12)

      else

         write (*,*) ' u, err: ',u,err

      endif

      return
      end



      subroutine funcMHD1(prim,error,errornorm,err4,err3,iflag,jflag)

c     This subroutine calculates errors for the MHD inversion problem
c     without radiation source term using the energy equation

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),errornorm(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4)
      common/metric/gn,gv
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcMHDd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov
      double precision mymax,myabs,mydiv

c     Save u, compute rho, ucon, ucov, bcon, bcov, Tmunu

      u=prim(1)

      do i=2,4
         ucon(i)=prim(i)
      enddo

      call solveu0(ucon)
      call contocov(ucon,ucov)

      rho=rhou0/ucon(1) 

      if(rho.lt.0.0.or.isnan(rho)) then
         jflag=1
      else
         jflag=0
      endif

      call calcbconbcov(BB,ucon,ucov,bcon,bcov,bsq)

      call calcTmunu(rho,u,bsq,Gam,ucon,ucov,bcon,bcov,Tmunu)

c     Compute normalized err3 from the three momentum equations and the
c     normalized err4 from all four equations

      err3=0.d0
      do i=2,4
         error(i)=Tmunu(1,i)-Ttcov(i)
         errornorm(i)=myabs(mydiv(error(i),(myabs(error(i))+
     &        myabs(Tmunu(1,i))+myabs(Ttcov(i)))))
         err3=mymax(err3,errornorm(i))
      enddo

c     Use lab frame energy equation

      error(1)=Tmunu(1,1)-Ttcov(1)
      errornorm(1)=myabs(mydiv(error(1),(myabs(error(1))+
     &     myabs(Tmunu(1,1))+myabs(Ttcov(1)))))
      err4=mymax(err3,errornorm(1))

c      write (*,*)
c      write (*,*) ' target Ttcov: ',(Ttcov(i),i=1,4)
c      write (*,*) ' current Ttcov: ',(Tmunu(1,i),i=1,4)
      write (*,*) 'B error: ',(error(i),i=1,4)
      write (*,*) 'B err3, err4: ',err3,err4

c     If u has an unreasonable value, set iflag=9 and reset u

      if (u.lt.0.d0) then
         iflag=9
         write (*,*) 'BSet iflag=',iflag
      else
         entropy=log((Gam1*u)**en/rho**en1)
         if (entropy.lt.((s/rhou0)-dlogmax)) iflag=9
      endif
      if (iflag.eq.9) prim(1)=mymax(prim(1),uminfac*rho)

c      write (*,*) ' error: ',(error(i),i=1,4),err3

      return 
      end



      subroutine funcMHD2(prim,error,errornorm,err4,err3,iflag,jflag)

c     This subroutine calculates errors for the MHD inversion problem
c     without radiation source term using the entropy equation

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),errornorm(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4)
      common/metric/gn,gv
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      double precision mymax,myabs,mydiv

c     Save u, compute rho, ucon, ucov, bcon, bcov, Tmunu

      u=prim(1)

      do i=2,4
         ucon(i)=prim(i)
      enddo

      call solveu0(ucon)
      call contocov(ucon,ucov)

      rho=rhou0/ucon(1) 

      if(rho.lt.0.0.or.isnan(rho)) then
         jflag=1
      else
         jflag=0
      endif

      call calcbconbcov(BB,ucon,ucov,bcon,bcov,bsq)

      call calcTmunu(rho,u,bsq,Gam,ucon,ucov,bcon,bcov,Tmunu)

c     Compute normalized err3 from the three momentum equations and the
c     normalized err4 from all four equations

      err3=0.d0
      do i=2,4
         error(i)=Tmunu(1,i)-Ttcov(i)
         errornorm(i)=myabs(mydiv(error(i),(myabs(error(i))+
     &        myabs(Tmunu(1,i))+myabs(Ttcov(i)))))
         err3=mymax(err3,errornorm(i))
      enddo

c     Use entropy equation

      entropy=ucon(1)*rho*log((Gam1*u)**en/rho**en1)
      error(1)=entropy-s
      errornorm(1)=myabs(mydiv(error(1),(myabs(error(1))+
     &     myabs(entropy)+myabs(s))))
      err4=mymax(err3,errornorm(1))

c      write (*,*)
c      write (*,*) ' target entropy: ',s
c      write (*,*) ' current entropy: ',entropy

c     If u has an unreasonable value, set iflag=9 and reset u

      if (prim(1).lt.0.d0) then
         iflag=9
         write (*,*) 'CSet iflag=',iflag
         prim(1)=mymax(prim(1),uminfac*rho)
      endif

c      write (*,*) ' primitives: ',(prim(i),i=1,4)
c      write (*,*) ' target Ttcov: ',(Ttcov(i),i=2,4)
c      write (*,*) ' current Ttcov: ',(Tmunu(1,i),i=2,4)
      write (*,*) 'A error: ',(error(i),i=1,4)
      write (*,*) 'A err3, err4: ',err3,err4

      return
      end



      subroutine func3(primsave,prim3,error3,errornorm3,
     &     err4,err3,iflag,jflag,func)

c     This function is called by Newton3. It takes a 3-array with
c     primitives prim3(3), transfers the value primsave to prim4(1) and
c     the three velocity components to prim4(2-4) and computes the
c     4-array of errors error4(4). Then transfers appropriate elements
c     to error3(3) and returns this along with overall error err3.

      implicit double precision (a-h,o-z)
      dimension prim3(3),error3(3),prim4(4),error4(4),
     &     errornorm4(4),errornorm3(3)
      external func

c     Transfer primsave and prim3(3) to prim4(4)

      prim4(1)=primsave
      do i=1,3
         prim4(i+1)=prim3(i)
      enddo

c     Call appropriate function to calculate error4(4)

      call func(prim4,error4,errornorm4,err4,err3,iflag,jflag)

c     Transfer the momentum equation errors to error3(3) and return

      do i=1,3
         error3(i)=error4(i+1)
         errornorm3(i)=errornorm4(i+1)
      enddo

      return
      end



      subroutine funcrad1(prim,error,errornorm,err4,err3,iflag,jflag)

c     This subroutine calculates errors for the radiation inversion
c     problem including the radiation source term using the energy
c     equation

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),errornorm(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4),
     &     Tt(4),Rt(4),urcon(4),urcov(4),Rmunu(4,4),
     &     gn(4,4),gv(4,4),Gcon(4),Gcov(4)
      common/metric/gn,gv
      common/accuracy/eps,epsbis,dvmin,tol,uminfac,dlogmax,
     &     gammaradceiling,itermax
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcradd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov,u0,iuerr
      double precision mymax,myabs,mydiv

      ffkap=3.46764d-17
      eskap=5.90799d5
      arad=1.18316d17

c     Save u and solve for lab frame conserved quantities corresponding
c     to the given primitives: rho, ucon, ucov

      u=prim(1)

      do i=2,4
         ucon(i)=prim(i)
      enddo

c     Calculate u^0 and rho

      call solveu0(ucon)
      call contocov(ucon,ucov)

      rho=rhou0/ucon(1) 
c      write (*,*)
c      write (*,*) ' rho, u: ',rho,u

c     Calculate b^mu, b_mu, bsq

      call calcbconbcov(BB,ucon,ucov,bcon,bcov,bsq)

c     Calculate the full gas stress-energy tensor T^mu_nu

      call calcTmunu(rho,u,bsq,Gam,ucon,ucov,bcon,bcov,Tmunu)

c     Evaluate the first row of the radiation stress-energy tensor:
c     R^0_mu

      do i=1,4
         Tt(i)=Tmunu(1,i)
         Rt(i)=Ttcov(i)+Rtcov(i)-Tt(i)
      enddo
c      write (*,*) ' Tt: ',(Tt(i),i=1,4)
c      write (*,*) ' Rt: ',(Rt(i),i=1,4)

c     Calculate the full R^mu_nu tensor

      call Rmunuinvert(Rt,E,urcon,urcov,ucon,Rmunu)
c      write (*,*) ' E, urcon, urcov: ',E,(urcon(i),urcov(i),i=1,4)

c     Calculate radiation energy density in the gas frame \hat{E}, and
c     the gas and radiation temperatures

      Ehat=0.d0
      do i=1,4
      do j=1,4
         Ehat=Ehat+Rmunu(i,j)*ucov(i)*ucon(j)
      enddo
      enddo
      write (*,*) ' \hat{E}: ',Ehat

      Trad=(Ehat/arad)**(0.25d0)
      Tgas=Gam1*u/rho
      write (*,*) ' Tgas, Trad: ',Tgas,Trad

      if(Trad.lt.0.0.or.isnan(Trad)) then
         jflag=1
      else
         jflag=0
      endif

c     Calculate quantities needed to compute the radiation source term

      ff=ffkap*rho*rho/Tgas**(3.5d0)
      es=eskap*rho
      B4pi=arad*Tgas**4
c      write (*,*) ' ff, es, B4pi: ',ff,es,B4pi

c     The following side calculation is to estimate quantities that are
c     relevant for deciding whether the radiation source term can be
c     handled explicitly. Currently, all calculations are done
c     implicitly.

      alpha=1.d0/sqrt(-gn(1,1))
      gamma=ucon(1)*alpha

c     Decide how to set dtau: either dt/gamma or dt/ucon(1)

c      dtau=dt/gamma
      dtau=dt/ucon(1)

      chi1=ff*dtau*(1.d0+4.d0*B4pi/u)
      chi2=(ff+es)*dtau*(1.d0+Ehat/(rho+Gam*u))
c      write (*,*) ' chi1, chi2: ',chi1,chi2

c     Compute the radiation source term G^mu and G_mu directly in the
c     lab frame (formula taken from Jon's draft of the paper, with some
c     corrections)

      do i=1,4
         Gcon(i)=-(ff*B4pi+es*Ehat)*ucon(i)
      do j=1,4
         Gcon(i)=Gcon(i)-(ff+es)*Rmunu(i,j)*ucon(j)
      enddo
      enddo

      call contocov(Gcon,Gcov)
c      write (*,*)
c      write (*,*) ' lab Gcon: ',(Gcon(i),i=1,4)
c      write (*,*) ' lab Gcov: ',(Gcov(i),i=1,4)

c     Compute normalized err3 from the three momentum equations and the
c     normalized err4 from all four equations

      err3=0.d0
      do i=2,4
         error(i)=Tt(i)-Ttcov(i)-Gcov(i)*dt
         errornorm(i)=myabs(mydiv(error(i),(myabs(Gcov(i)*dt)+
     &        myabs(Tt(i))+myabs(Ttcov(i)))))
         err3=mymax(err3,errornorm(i))
      enddo

c     Calculate error(1) from lab frame energy equation

         error(1)=Tt(1)-Ttcov(1)-Gcov(1)*dt
         errornorm(1)=myabs(mydiv(error(1),(myabs(Gcov(1)*dt)+
     &        myabs(Tt(1))+myabs(Ttcov(1)))))
         err4=mymax(err3,errornorm(1))

c      write (*,*)
c      write (*,*) ' target Ttcov: ',(Ttcov(i),i=1,4)
c      write (*,*) ' current Ttcov: ',(Tt(i),i=1,4)
      write (*,*) 'C error: ',(error(i),i=1,4)
      write (*,*) 'C error2: ',(myabs(error(i)),i=1,4)
      write (*,*) 'C errornorm: ',(errornorm(i),i=1,4)
      write (*,*) 'C errornorm2: ',
     & (myabs(Gcov(1)*dt)+myabs(Tt(1))+myabs(Ttcov(1)))
      write (*,*) 'C err3, err4: ',err3,err4

c     If u has an unreasonable value, set iflag=9 and reset u

      if (u.lt.0.d0) then
         iflag=9
         write (*,*) 'DSet iflag=',iflag
      else
         entropy=log((Gam1*u)**en/rho**en1)
         if (entropy.lt.((s/rhou0)-dlogmax)) iflag=9
      endif
      if (iflag.eq.9) prim(1)=mymax(prim(1),uminfac*rho)

      u0=prim(1)

      return
      end



      subroutine funcrad2(prim,error,errornorm,err4,err3,iflag,jflag)

c     This subroutine calculates errors for the radiation inversion
c     problem including the radiation source term using the entropy
c     equation

      implicit double precision (a-h,o-z)
      dimension prim(4),error(4),errornorm(4),Ttcov(4),Rtcov(4),BB(4),
     &     ucon(4),ucov(4),bcon(4),bcov(4),Tmunu(4,4),
     &     Tt(4),Rt(4),urcon(4),urcov(4),Rmunu(4,4),
     &     gn(4,4),gv(4,4),Gcon(4),Gcov(4)
      common/metric/gn,gv
      common/conserved/Gam,Gam1,en,en1,rhou0,s,Ttcov,Rtcov,BB,dt
      common/funcradd/ffkap,eskap,arad,ucon,ucov,rho,bcon,bcov,bsq,
     &     Tmunu,E,Ehat,urcon,urcov,Rmunu,Tgas,Trad,B4pi,gamma,dtau,
     &     Gcon,Gcov,u0,iuerr
      double precision mymax,myabs,mydiv

      ffkap=3.46764d-17
      eskap=5.90799d5
      arad=1.18316d17

c     Save u and solve for lab frame conserved quantities corresponding
c     to the given primitives: rho, ucon, ucov

      u=prim(1)
      if (prim(1).lt.0.d0) then
         iflag=9
         write (*,*) 'FSet iflag=',iflag
      endif

      do i=2,4
         ucon(i)=prim(i)
      enddo

c     Calculate u^0 and rho

      call solveu0(ucon)
      call contocov(ucon,ucov)

      rho=rhou0/ucon(1) 
c      write (*,*)
c      write (*,*) ' rho, u: ',rho,u

c     Calculate b^mu, b_mu, bsq

      call calcbconbcov(BB,ucon,ucov,bcon,bcov,bsq)

c     Calculate the full gas stress-energy tensor T^mu_nu

      call calcTmunu(rho,u,bsq,Gam,ucon,ucov,bcon,bcov,Tmunu)

c     Evaluate the first row of the radiation stress-energy tensor:
c     R^0_mu

      do i=1,4
         Tt(i)=Tmunu(1,i)
         Rt(i)=Ttcov(i)+Rtcov(i)-Tt(i)
      enddo

c     Calculate the full R^mu_nu tensor

      call Rmunuinvert(Rt,E,urcon,urcov,ucon,Rmunu)

c     Calculate radiation energy density in the gas frame \hat{E}, and
c     the gas and radiation temperatures

      Ehat=0.d0
      do i=1,4
      do j=1,4
         Ehat=Ehat+Rmunu(i,j)*ucov(i)*ucon(j)
      enddo
      enddo
c      write (*,*) ' \hat{E}: ',Ehat

      Trad=(Ehat/arad)**(0.25d0)
      Tgas=Gam1*u/rho
c      write (*,*) ' Tgas, Trad: ',Tgas,Trad

c     Calculate quantities needed to compute the radiation source term

      ff=ffkap*rho*rho/Tgas**(3.5d0)
      es=eskap*rho
      B4pi=arad*Tgas**4
c      write (*,*) ' ff, es, B4pi: ',ff,es,B4pi

      if(Trad.lt.0.0.or.isnan(Trad)) then
         jflag=1
      else
         jflag=0
      endif

c     The following side calculation is to estimate quantities that are
c     relevant for deciding whether the radiation source term can be
c     explicitly. Currently, all calculations are done implicitly.

      alpha=1.d0/sqrt(-gn(1,1))
      gamma=ucon(1)*alpha

c     Decide how to set dtau: either dt/gamma or dt/ucon(1)

c      dtau=dt/gamma
      dtau=dt/ucon(1)

      Ghatdtau=ff*(Ehat-B4pi)*dtau

      chi1=ff*dtau*(1.d0+4.d0*B4pi/u)
      chi2=(ff+es)*dtau*(1.d0+Ehat/(rho+Gam*u))
c      write (*,*) ' chi1, chi2: ',chi1,chi2

c     Compute the radiation source term G^mu and G_mu directly in the
c     lab frame (formula taken from Jon's draft of the paper, with some
c     corrections)

      do i=1,4
         Gcon(i)=-(ff*B4pi+es*Ehat)*ucon(i)
      do j=1,4
         Gcon(i)=Gcon(i)-(ff+es)*Rmunu(i,j)*ucon(j)
      enddo
      enddo

      call contocov(Gcon,Gcov)
c      write (*,*)
c      write (*,*) ' lab Gcon: ',(Gcon(i),i=1,4)
c      write (*,*) ' lab Gcov: ',(Gcov(i),i=1,4)

c     Compute normalized err3 from the three momentum equations and the
c     normalized err4 from all four equations

      err3=0.d0
      do i=2,4
         error(i)=Tt(i)-Ttcov(i)-Gcov(i)*dt
         errornorm(i)=myabs(mydiv(error(i),(myabs(Gcov(i)*dt)+
     &        myabs(Tt(i))+myabs(Ttcov(i)))))
         err3=mymax(err3,errornorm(i))
      enddo

c     Calculate error(1) corresponding to the lab frame entropy equation

      entropy=ucon(1)*rho*log((Gam1*prim(1))**en/rho**en1)
      Gt=Gcov(1)*dt
      error(1)=entropy-s-Gt
      errornorm(1)=myabs(mydiv(error(1),(myabs(entropy)+
     &     myabs(s)+myabs(Gt))))

c      entropy=rho*log((Gam1*prim(1))**en/rho**en1)
c      Gt=Ghatdtau
c      error(1)=entropy-(s/ucon(1))-Gt
c      errornorm(1)=myabs(mydiv(error(1),(myabs(entropy)+
c     &     myabs(s/ucon(1))+myabs(Gt))))

c     Alternatively, use the fluid frame entropy equation

c      entropy=ucon(1)*rho*log((Gam1*prim(1))**en/rho**en1)
c      Gdtau=ff*(Ehat-B4pi)*dtau
c      Gt=Gdtau
c      error(1)=entropy-s-Gdtau
c      write (*,*) ' entropy, s, Gdtau: ',entropy,s,Gdtau

      err4=mymax(err3,errornorm(1))
c      write (*,*) ' error(1), err3, err4 ',error(1),err3,err4

c      write (*,*)
c      write (*,*) ' target Ttcov: ',(Ttcov(i),i=1,4)
c      write (*,*) ' current Ttcov: ',(Tt(i),i=1,4)
      write (*,*) 'E error: ',(error(i),i=1,4)
      write (*,*) 'E err3, err4: ',err3,err4

      u0=prim(1)

      return
      end



      SUBROUTINE lubksb(a,n,np,indx,b)

c     Matrix inversion subroutine 1 from Numerical Recipes

      implicit double precision (a-h,o-z)
      INTEGER n,np,indx(n)
      REAL*8 a(np,np),b(n)
      INTEGER i,ii,j,ll
      REAL*8 sum
      ii=0
      do 12 i=1,n
        ll=indx(i)
        sum=b(ll)
        b(ll)=b(i)
        if (ii.ne.0)then
          do 11 j=ii,i-1
            sum=sum-a(i,j)*b(j)
11        continue
        else if (sum.ne.0.d0) then
          ii=i
        endif
        b(i)=sum
12    continue
      do 14 i=n,1,-1
        sum=b(i)
        do 13 j=i+1,n
          sum=sum-a(i,j)*b(j)
13      continue
        b(i)=sum/a(i,i)
14    continue
      return
      END



      SUBROUTINE ludcmp(a,n,np,indx,d,retval)

c     Matrix inversion subroutine 2 from Numerical Recipes

      implicit double precision (a-h,o-z)
      INTEGER n,np,indx(n),NMAX
      REAL*8 d,a(np,np),TINY
      PARAMETER (NMAX=500,TINY=1.0d-80)
      INTEGER i,imax,j,k
      REAL*8 aamax,dum,sum,vv(NMAX)
      double precision mymax,myabs,mydiv
      d=1.d0
      do 12 i=1,n
        aamax=0.d0
        do 11 j=1,n
          if (myabs(a(i,j)).gt.aamax) aamax=myabs(a(i,j))
11      continue
        if (aamax.eq.0.d0) then
           write (*,*) 'singular matrix in ludcmp'
           retval=1
           return
        endif
        vv(i)=1.d0/aamax
12    continue
      do 19 j=1,n
        do 14 i=1,j-1
          sum=a(i,j)
          do 13 k=1,i-1
            sum=sum-a(i,k)*a(k,j)
13        continue
          a(i,j)=sum
14      continue
        aamax=0.d0
        do 16 i=j,n
          sum=a(i,j)
          do 15 k=1,j-1
            sum=sum-a(i,k)*a(k,j)
15        continue
          a(i,j)=sum
          dum=vv(i)*myabs(sum)
          if (dum.ge.aamax) then
            imax=i
            aamax=dum
          endif
16      continue
        if (imax.gt.n) then
           write (*,*) 'Bad imax in ludcmp: ',imax
           retval=2
           return
        endif
        if (j.ne.imax)then
          do 17 k=1,n
            dum=a(imax,k)
            a(imax,k)=a(j,k)
            a(j,k)=dum
17        continue
          d=-d
          vv(imax)=vv(j)
        endif
        indx(j)=imax
        if(a(j,j).eq.0.d0)a(j,j)=TINY
        if(j.ne.n)then
          dum=1.d0/a(j,j)
          do 18 i=j+1,n
            a(i,j)=a(i,j)*dum
18        continue
        endif
19    continue
      retval=0
      return
      END



      FUNCTION rtsafe(funcd,x1,x2,fl,fr,xacc)
      implicit double precision (a-h,o-z)
      INTEGER MAXIT
      REAL*8 rtsafe,x1,x2,xacc
      EXTERNAL funcd
      PARAMETER (MAXIT=100)
      INTEGER j
      REAL*8 df,dx,dxold,f,fh,fl,temp,xh,xl
      double precision mymax,myabs,mydiv
c      call funcd(x1,fl,df)
c      call funcd(x2,fh,df)
c      if((fl.gt.0.d0.and.fh.gt.0.d0).or.(fl.lt.0.d0.and.fh.lt.0.d0))
c     &     write (*,*) 'root is not bracketed in rtsafe! '
c      if(fl.eq.0.d0)then
c        rtsafe=x1
c        return
c      else if(fh.eq.0.d0)then
c        rtsafe=x2
c        return
      if(fl.lt.0.d0)then
        xl=x1
        xh=x2
      else
        xh=x1
        xl=x2
      endif
      rtsafe=.5d0*(x1+x2)
      call funcd(rtsafe,f,df)
      if (f.lt.0.d0) then
         xl=rtsafe
      else
         xh=rtsafe
      endif
      dxold=myabs(xh-xl)
      dx=dxold
      do 11 j=1,MAXIT
        if(((rtsafe-xh)*df-f)*((rtsafe-xl)*df-f).ge.0.d0.or. 
     &        myabs(2.d0*f).gt.myabs(dxold*df) ) then
          dxold=dx
          dx=0.5d0*(xh-xl)
          rtsafe=xl+dx
          if(xl.eq.rtsafe)return
        else
          dxold=dx
          dx=f/df
          temp=rtsafe
          rtsafe=rtsafe-dx
          if(temp.eq.rtsafe)return
        endif
        if(myabs(dx).lt.xacc) return
        call funcd(rtsafe,f,df)
        if(f.lt.0.d0) then
          xl=rtsafe
        else
          xh=rtsafe
        endif
11    continue
      write (*,*) ' rtsafe has exceeded maximum iterations '
      return
      END
