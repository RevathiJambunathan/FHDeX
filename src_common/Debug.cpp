#include "common_functions.H"
#include "common_functions_F.H"

void PrintMF (const MultiFab& MF)
{
    const BoxArray& ba = MF.boxArray();
    const DistributionMapping& dm = MF.DistributionMap();
    const int myProc = ParallelDescriptor::MyProc();

    for (int i=0; i<ba.size(); ++i) {
        if (dm[i] == myProc) {

            // we want all processors to write, not just the IOProcessor
            std::cout << "Grid #" << i << std::endl;
            std::cout << "Processor #" << myProc << std::endl;

            const Box& validBox = ba[i];

            // call fortran subroutine
            // use macros in AMReX_ArrayLim.H to pass in each FAB's data, 
            // lo/hi coordinates (including ghost cells), and/or the # of components
            // We will also pass "validBox", which specifies the "valid" region.
            print_mf(ARLIM_3D(validBox.loVect()), ARLIM_3D(validBox.hiVect()),
                     BL_TO_FORTRAN_FAB(MF[i]));

        }
        // add this barrier so only one grid gets printed out at a time
        ParallelDescriptor::Barrier();
    }
}