

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress a raw vector
#'
#' @param src raw vector containing data to be compressed.
#'
#' @return raw vector of compressed data
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_compress <- function(src) {
  .Call(lz4_compress_, src)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress a raw vector 
#'
#' The \code{src} vector must be a raw vector created by \code{lz4_compress}
#'
#' @param src raw vector of compressed data
#' @return uncompressed vector
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_decompress <- function(src) {
  .Call(lz4_decompress_, src)
}

