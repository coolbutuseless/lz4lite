


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress a vector of raw, integer, real or logical values.
#'
#'
#' @param src source vector containing data to be compressed.
#'        Data must be raw, integer, real or logical only.
#' @param acceleration Acceleration, Default 1.  Higher values run faster but with
#'        lower compression. This value is ignored if \code{use_hc = TRUE}
#'
#' @return raw vector of compressed data
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_compress <- function(src, acceleration = 1L) {
  .Call(lz4_compress_, src, as.integer(acceleration), FALSE, 1L)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_compress
#'
#' @param level compression level for LZ4HC compression. default 1
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4hc_compress <- function(src, level = 9L) {
  .Call(lz4_compress_, src, 0L, TRUE, as.integer(level))
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Uncompress a raw vector into the original data.
#'
#' The \code{src} vector must be a raw vector created by \code{lz4_compress} as
#' it relies on the bespoke framing created there.  This function will not
#' decompress an object compressed with another LZ4 implementation e.g. \code{fst}
#'
#' @param src raw vector of compressed data
#'
#' @return uncompressed vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_uncompress <- function(src) {
  .Call(lz4_uncompress_, src)
}
