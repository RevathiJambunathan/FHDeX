
#include "hydro_functions.H"

#include "common_functions.H"

#include "gmres_functions.H"

#include "multispec_functions.H"


#include <AMReX_ParallelDescriptor.H>
#include <AMReX_MultiFabUtil.H>


// argv contains the name of the inputs file entered at the command line
void advance_timestep_inertial(  std::array< MultiFab, AMREX_SPACEDIM >& umac, 
                                 MultiFab& pres, 
                                 MultiFab& rho, MultiFab& rhotot,
                                 const std::array< MultiFab, AMREX_SPACEDIM >& mfluxdiv_predict,
                                 const std::array< MultiFab, AMREX_SPACEDIM >& mfluxdiv_correct,
                                 std::array< MultiFab, AMREX_SPACEDIM >& alpha_fc,
                                 const MultiFab& beta, const MultiFab& gamma,
                                 const std::array< MultiFab, NUM_EDGE >& beta_ed, 
                                 const Geometry geom, const Real& dt)
{
  
  BL_PROFILE_VAR("advance()",advance);

  const Real* dx = geom.CellSize();
  const Real dtinv = 1.0/dt;
  Real theta_alpha = dtinv;
  Real norm_pre_rhs;

  const BoxArray& ba = beta.boxArray();
  const DistributionMapping& dmap = beta.DistributionMap();

   // rhs_p GMRES solve
   MultiFab gmres_rhs_p(ba, dmap, 1, 0);
   gmres_rhs_p.setVal(0.);

  // rhs_u GMRES solve
  std::array< MultiFab, AMREX_SPACEDIM > gmres_rhs_u;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    gmres_rhs_u[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    gmres_rhs_u[d].setVal(0.0);
  }
       
  std::array< MultiFab, AMREX_SPACEDIM > Lumac;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    Lumac[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    Lumac[d].setVal(0.0);
  }
  
  // advective terms
  std::array< MultiFab, AMREX_SPACEDIM > advFluxdiv;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    advFluxdiv[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    advFluxdiv[d].setVal(0.0);
  }

  std::array< MultiFab, AMREX_SPACEDIM > advFluxdivPred;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    advFluxdivPred[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    advFluxdivPred[d].setVal(0.0);
  }

  // staggered momentum
  std::array< MultiFab, AMREX_SPACEDIM > uMom;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    uMom[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    uMom[d].setVal(0.0);
  }

  // face-centered rho total
  std::array< MultiFab, AMREX_SPACEDIM > rhotot_face;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    rhotot_face[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    rhotot_face[d].setVal(0.0);
  }
  
  MultiFab advFluxdivS(ba,dmap,1,1);

  MultiFab rhoPred(ba,dmap,nspecies,1);
  rhoPred.setVal(0.0);
  MultiFab adv_mass_fluxdiv(ba,dmap,nspecies,1);

  MultiFab diff_mass_fluxdiv(ba,dmap,nspecies,1);
  
  std::array< MultiFab, AMREX_SPACEDIM > diff_mass_flux;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    diff_mass_flux[d].define(convert(ba,nodal_flag_dir[d]), dmap, nspecies, 1);
    diff_mass_flux[d].setVal(0.0);
  }

  // MultiFab stoch_mass_fluxdiv(ba,dmap,nspecies,1);
  // std::array< MultiFab, AMREX_SPACEDIM > stoch_mass_flux;
  // for (int d=0; d<AMREX_SPACEDIM; ++d) {
  //   stoch_mass_flux[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
  //   stoch_mass_flux[d].setVal(0.0);
  // }

  ///////////////////////////////////////////
  // Scaled alpha, beta, gamma:
  ///////////////////////////////////////////

  // alpha_fc_0 arrays
  std::array< MultiFab, AMREX_SPACEDIM > alpha_fc_0;
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    alpha_fc_0[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
    alpha_fc_0[d].setVal(0.0);
  }

  // Scaled by 1/2:
  // beta_wtd cell centered
  MultiFab beta_wtd(ba, dmap, 1, 1);
  MultiFab::Copy(beta_wtd, beta, 0, 0, 1, 1);
  beta_wtd.mult(0.5, 1);

  // beta_wtd on nodes in 2d
  // beta_wtd on edges in 3d
  std::array< MultiFab, NUM_EDGE > beta_ed_wtd;
#if (AMREX_SPACEDIM == 2)
  beta_ed_wtd[0].define(convert(ba,nodal_flag), dmap, 1, 1);
  MultiFab::Copy(beta_ed_wtd[0], beta_ed[0], 0, 0, 1, 1);
  beta_ed_wtd[0].mult(0.5, 1);
#elif (AMREX_SPACEDIM == 3)
  beta_ed_wtd[0].define(convert(ba,nodal_flag_xy), dmap, 1, 1);
  beta_ed_wtd[1].define(convert(ba,nodal_flag_xz), dmap, 1, 1);
  beta_ed_wtd[2].define(convert(ba,nodal_flag_yz), dmap, 1, 1);
  for(int d=0; d<AMREX_SPACEDIM; d++) {
    MultiFab::Copy(beta_ed_wtd[d], beta_ed[d], 0, 0, 1, 1);
    beta_ed_wtd[d].mult(0.5, 1);
  }
#endif

  // cell-centered gamma_wtd
  MultiFab gamma_wtd(ba, dmap, 1, 1);
  MultiFab::Copy(gamma_wtd, gamma, 0, 0, 1, 1);
  gamma_wtd.mult(-0.5, 1);

  // Scaled by -1/2:
  // beta_negwtd cell centered
  MultiFab beta_negwtd(ba, dmap, 1, 1);
  MultiFab::Copy(beta_negwtd, beta, 0, 0, 1, 1);
  beta_negwtd.mult(-0.5, 1);

  // beta_negwtd on nodes in 2d
  // beta_negwtd on edges in 3d
  std::array< MultiFab, NUM_EDGE > beta_ed_negwtd;
#if (AMREX_SPACEDIM == 2)
  beta_ed_negwtd[0].define(convert(ba,nodal_flag), dmap, 1, 1);
  MultiFab::Copy(beta_ed_negwtd[0], beta_ed[0], 0, 0, 1, 1);
  beta_ed_negwtd[0].mult(-0.5, 1);
#elif (AMREX_SPACEDIM == 3)
  beta_ed_negwtd[0].define(convert(ba,nodal_flag_xy), dmap, 1, 1);
  beta_ed_negwtd[1].define(convert(ba,nodal_flag_xz), dmap, 1, 1);
  beta_ed_negwtd[2].define(convert(ba,nodal_flag_yz), dmap, 1, 1);
  for(int d=0; d<AMREX_SPACEDIM; d++) {
    MultiFab::Copy(beta_ed_negwtd[d], beta_ed[d], 0, 0, 1, 1);
    beta_ed_negwtd[d].mult(-0.5, 1);
  }
#endif

  // cell-centered gamma
  MultiFab gamma_negwtd(ba, dmap, 1, 1);
  MultiFab::Copy(gamma_negwtd, gamma, 0, 0, 1, 1);
  gamma_negwtd.mult(-0.5, 1);
  ///////////////////////////////////////////

  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    umac[d].FillBoundary(geom.periodicity());
  }

  //////////////////////////

  //////////////////////////////////////////////////
  // ADVANCE species concentration (Stage 1)
  //////////////////////////////////////////////////

  rho.FillBoundary(geom.periodicity());

  // FIXME: stage_time = 0.0
  ComputeMassFluxdiv(rho,rhotot,
  		     diff_mass_fluxdiv,diff_mass_flux,
  		     dt,0.0,geom);
  // ComputeMassFluxdiv(rho,rhotot,diff_mass_fluxdiv,
  // 		     stoch_mass_fluxdiv,diff_mass_flux,
  // 		     stoch_mass_flux,dt,0.0,geom);
  
  for(int i=0; i<nspecies; i++) {
    MkAdvSFluxdiv(umac,rho,adv_mass_fluxdiv,dx,geom,i,0);
  }

  MultiFab::Copy(rhoPred, rho, 0, 0, nspecies, 1);

  diff_mass_fluxdiv.mult(-dt);
  MultiFab::Add(rhoPred,diff_mass_fluxdiv,0,0,nspecies,0);

  adv_mass_fluxdiv.mult(dt);
  MultiFab::Add(rhoPred,adv_mass_fluxdiv, 0,0,nspecies,0);

  //////////////////////////////////////////////////
    
  //////////////////////////////////////////////////
  // ADVANCE velocity field (Stage 1)
  //////////////////////////////////////////////////

  // PREDICTOR STEP (heun's method: part 1)
  // compute advective term
  AverageCCToFace(rhotot, 0, rhotot_face, 0, 1);
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    MultiFab::Copy(uMom[d], umac[d], 0, 0, 1, 0);
    MultiFab::Multiply(uMom[d], rhotot_face[d], 0, 0, 1, 0);
    uMom[d].FillBoundary(geom.periodicity());
  }

  MkAdvMFluxdiv(umac,uMom,advFluxdiv,dx,0);

  // crank-nicolson terms
  StagApplyOp(geom,beta_negwtd,gamma_negwtd,beta_ed_negwtd,umac,Lumac,alpha_fc_0,dx,theta_alpha);

  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    MultiFab::Copy(gmres_rhs_u[d], umac[d], 0, 0, 1, 0);
	       
    gmres_rhs_u[d].mult(dtinv, 1);

    MultiFab::Add(gmres_rhs_u[d], mfluxdiv_predict[d],  0, 0, 1, 0);
    MultiFab::Add(gmres_rhs_u[d], Lumac[d],             0, 0, 1, 0);
    MultiFab::Add(gmres_rhs_u[d], advFluxdiv[d],        0, 0, 1, 0);

    gmres_rhs_u[d].FillBoundary(geom.periodicity());
  }

  // initial guess for new solution
  for (int d=0; d<AMREX_SPACEDIM; ++d) {

  }
  pres.setVal(0.);  // initial guess

  // call GMRES to compute predictor
  /*
  GMRES(gmres_rhs_u,gmres_rhs_p,umacNew,pres,
	alpha_fc,beta_wtd,beta_ed_wtd,gamma_wtd,
	theta_alpha,geom,norm_pre_rhs);
  */

  //////////////////////////////////////////////////

  //////////////////////////////////////////////////
  // ADVANCE species concentration (Stage 2)
  //////////////////////////////////////////////////

  rhoPred.FillBoundary(geom.periodicity());

  // FIXME: stage_time = 0.0
  ComputeMassFluxdiv(rhoPred,rhotot,
  		     diff_mass_fluxdiv,diff_mass_flux,
  		     dt,0.0,geom);
  // ComputeMassFluxdiv(rhoPred,rhotot,diff_mass_fluxdiv,
  // 		     stoch_mass_fluxdiv,diff_mass_flux,
  // 		     stoch_mass_flux,dt,0.0,geom);

  for(int i=0; i<nspecies; i++) {
    MkAdvSFluxdiv(umac,rhoPred,adv_mass_fluxdiv,dx,geom,i,0);
  }

  diff_mass_fluxdiv.mult(-dt);
  MultiFab::Add(rho,diff_mass_fluxdiv,0,0,nspecies,0);

  adv_mass_fluxdiv.mult(dt);
  MultiFab::Add(rho,adv_mass_fluxdiv, 0,0,nspecies,0);

  MultiFab::Add(rho,rhoPred,0,0,nspecies,0);

  rho.mult(0.5);

  // // Hack: Write out mfabs
  // std::string plotfilename;
  // plotfilename = "rho_test";
  // VisMF::Write(rhotot,plotfilename);
  // exit(0);
  
  //////////////////////////////////////////////////

  //////////////////////////////////////////////////
  // ADVANCE velocity field (Stage 2)
  //////////////////////////////////////////////////

  // Compute predictor advective term
  AverageCCToFace(rhotot, 0, rhotot_face, 0, 1);
  for (int d=0; d<AMREX_SPACEDIM; ++d) {
    umac[d].FillBoundary(geom.periodicity());
    MultiFab::Copy(uMom[d], umac[d], 0, 0, 1, 0);
    MultiFab::Multiply(uMom[d], rhotot_face[d], 0, 0, 1, 0);
    uMom[d].FillBoundary(geom.periodicity());
  }

  MkAdvMFluxdiv(umac,uMom,advFluxdivPred,dx,0);

  // ADVANCE STEP (crank-nicolson + heun's method)

  // Compute gmres_rhs

  // trapezoidal advective terms
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    advFluxdiv[d].mult(0.5, 1);
    advFluxdivPred[d].mult(0.5, 1);
  }

  // crank-nicolson terms
  StagApplyOp(geom,beta_negwtd,gamma_negwtd,beta_ed_negwtd,umac,Lumac,alpha_fc_0,dx,theta_alpha);

  for (int d=0; d<AMREX_SPACEDIM; d++) {
    MultiFab::Copy(gmres_rhs_u[d], umac[d], 0, 0, 1, 0);
  
    gmres_rhs_u[d].mult(dtinv, 1);
    
    MultiFab::Add(gmres_rhs_u[d], mfluxdiv_correct[d],  0, 0, 1, 0);
    MultiFab::Add(gmres_rhs_u[d], Lumac[d],             0, 0, 1, 0);
    MultiFab::Add(gmres_rhs_u[d], advFluxdiv[d],        0, 0, 1, 0);
    MultiFab::Add(gmres_rhs_u[d], advFluxdivPred[d],    0, 0, 1, 0);

    gmres_rhs_u[d].FillBoundary(geom.periodicity());

    // initial guess for new solution
    MultiFab::Copy(umac[d], umac[d], 0, 0, 1, 0);
  }
	       
  pres.setVal(0.);  // initial guess

  // call GMRES here
  GMRES(gmres_rhs_u,gmres_rhs_p,umac,pres,
	alpha_fc,beta_wtd,beta_ed_wtd,gamma_wtd,
	theta_alpha,geom,norm_pre_rhs);

  //////////////////////////////////////////////////

}