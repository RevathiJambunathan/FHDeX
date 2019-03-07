#include "hydro_functions.H"
#include "hydro_functions_F.H"

// void setBC(amrex::MultiFab & u_mac, amrex::MultiFab & v_mac, amrex::MultiFab & w_mac) {
//
//     for (amrex::MFIter mfi(u_mac); mfi.isValid(); ++mfi) {
//         const amrex::Box & bx = mfi.validbox();
//
//         set_bc(ARLIM_3D(bx.loVect()), ARLIM_3D(bx.hiVect()),
//                u_mac[mfi].dataPtr(),
//                v_mac[mfi].dataPtr(),
//                w_mac[mfi].dataPtr());
//
//     }
// }



void MultiFABPhysBC(amrex::MultiFab & pressure, const amrex::Geometry & geom) {
    MultiFABPhysBC(pressure, amrex::IntVect{AMREX_D_DECL(1, 1, 1)}, geom);
}



void MultiFABPhysBC(amrex::MultiFab & pressure, int seq_fill_ghost, const amrex::Geometry & geom) {

    amrex::IntVect fill_ghost{AMREX_D_DECL(1, 1, 1)};
    // for(int i=0; i<=seq_fill_ghost; i++)
    //     fill_ghost[i] = 1;

    MultiFABPhysBC(pressure, fill_ghost, geom);

}



void MultiFABPhysBC(amrex::MultiFab & pressure, const amrex::IntVect & dim_fill_ghost,
                    const amrex::Geometry & geom) {

    amrex::Box dom(geom.Domain());

    for (amrex::MFIter mfi(pressure); mfi.isValid(); ++mfi) {

        const amrex::Box & bx = mfi.validbox();
        fab_physbc(BL_TO_FORTRAN_BOX(bx),
                   BL_TO_FORTRAN_BOX(dom),
                   BL_TO_FORTRAN_FAB(pressure[mfi]), pressure.nGrow(),
                   dim_fill_ghost.getVect());
    }
}



void MultiFABPhysBCDomainVel(amrex::MultiFab & vel, const amrex::Geometry & geom) {
    MultiFABPhysBCDomainVel(vel, amrex::IntVect{AMREX_D_DECL(1,1,1)}, geom);
}



void MultiFABPhysBCDomainVel(amrex::MultiFab & vel, int seq_fill_ghost,
                             const amrex::Geometry & geom) {

    amrex::IntVect fill_ghost{AMREX_D_DECL(1, 1, 1)};
    // for(int i=0; i<=seq_fill_ghost; i++)
    //     fill_ghost[i] = 1;

    MultiFABPhysBCDomainVel(vel, fill_ghost, geom);
}



void MultiFABPhysBCDomainVel(amrex::MultiFab & vel, const amrex::IntVect & dim_fill_ghost,
                             const amrex::Geometry & geom) {

    amrex::Box dom(geom.Domain());

    for (amrex::MFIter mfi(vel); mfi.isValid(); ++mfi) {

        const amrex::Box & bx = mfi.validbox();
        fab_physbc_domainvel(BL_TO_FORTRAN_BOX(bx),
                             BL_TO_FORTRAN_BOX(dom),
                             BL_TO_FORTRAN_FAB(vel[mfi]), vel.nGrow(),
                             dim_fill_ghost.getVect());
    }
}



void MultiFABPhysBCMacVel(amrex::MultiFab & vel, const amrex::Geometry & geom) {
    MultiFABPhysBCMacVel(vel, amrex::IntVect{AMREX_D_DECL(1,1,1)}, geom);
}



void MultiFABPhysBCMacVel(amrex::MultiFab & vel, int seq_fill_ghost,
                          const amrex::Geometry & geom) {

    amrex::IntVect fill_ghost{AMREX_D_DECL(1, 1, 1)};
    // for(int i=0; i<=seq_fill_ghost; i++)
    //     fill_ghost[i] = 1;

    MultiFABPhysBCMacVel(vel, fill_ghost, geom);
}



void MultiFABPhysBCMacVel(amrex::MultiFab & vel, const amrex::IntVect & dim_fill_ghost,
                          const amrex::Geometry & geom) {

    amrex::Box dom(geom.Domain());

    for (amrex::MFIter mfi(vel); mfi.isValid(); ++mfi) {

        const amrex::Box & bx = mfi.validbox();
        fab_physbc_macvel(BL_TO_FORTRAN_BOX(bx),
                          BL_TO_FORTRAN_BOX(dom),
                          BL_TO_FORTRAN_FAB(vel[mfi]), vel.nGrow(),
                          dim_fill_ghost.getVect());
    }
}