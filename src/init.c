

#include <R.h>
#include <Rinternals.h>

extern SEXP lz4_compress_();
extern SEXP lz4_uncompress_();

extern SEXP lz4_serialize_();
extern SEXP lz4_unserialize_();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"lz4_compress_"   , (DL_FUNC) &lz4_compress_   , 4},
  {"lz4_uncompress_" , (DL_FUNC) &lz4_uncompress_ , 1},
  {"lz4_serialize_"  , (DL_FUNC) &lz4_serialize_  , 4},
  {"lz4_unserialize_", (DL_FUNC) &lz4_unserialize_, 1},
  {NULL, NULL, 0}
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Register the methods
//
// Change the '_simplecall' suffix to match your package name
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void R_init_lz4lite(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}
