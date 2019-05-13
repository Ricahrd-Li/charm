#include "charm-api.h"

CLINKAGE void FTN_NAME(MPI_MAIN,mpi_main)(void);

CLINKAGE CMI_EXPORT void AMPI_Main_fortran_export(void);
CLINKAGE CMI_EXPORT void AMPI_Main_fortran_export(void)
{
  FTN_NAME(MPI_MAIN,mpi_main)();
}
