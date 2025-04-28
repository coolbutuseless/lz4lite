

#include <R.h>
#include <Rinternals.h>

extern SEXP lz4_compress_(SEXP src_);
extern SEXP lz4_decompress_(SEXP src_);

extern SEXP lz4_serialize_(SEXP robj);
extern SEXP lz4_unserialize_(SEXP src_);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// .C      R_CMethodDef
// .Call   R_CallMethodDef
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static const R_CallMethodDef CEntries[] = {
  {"lz4_compress_"   , (DL_FUNC) &lz4_compress_   , 1},
  {"lz4_decompress_" , (DL_FUNC) &lz4_decompress_ , 1},
  {"lz4_serialize_"  , (DL_FUNC) &lz4_serialize_  , 1},
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
