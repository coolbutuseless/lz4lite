


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress a vector of raw, integer, real or logical values.
#'
#' This arguments \code{acceleration} and \code{hc_level} reflect the parameters
#' of the LZ4 functions.  It can get confusing, because
#'
#' \itemize{
#' \item Use \code{acceleration} when \code{use_hc = FALSE}, with higher numbers
#'       giving \emph{higher} speed, but \emph{lower} compression.
#' \item Use \code{hc_level} when \code{use_hc = TRUE}, with higher numbers giving
#'       \emph{lower} speed, but \emph{higher} compression.
#' }
#'
#' @param src source vector containing data to be compressed.
#'        Data must be raw, integer, real or logical only.
#' @param acceleration Acceleration, Default 1.  Higher values run faster but with
#'        lower compression. This value is ignored if \code{use_hc = TRUE}
#' @param hc_level Compression level for High Compression mode. Default 1.
#'        Valid range [1, 12]. Higher values run slower but with
#'        more compression.  This value is used only if \code{use_hc = TRUE}.
#'        This parameter corresponds to the \code{compressionLevel} parameter
#'        in the C API.
#' @param use_hc Use High Compression mode. Default: FALSE.  High Compression
#'        mode can be very slow to compress, but retains the very high
#'        decompression speeds.
#'
#' @return raw vector of compressed data
#'
#' @useDynLib lz4lite lz4_compress_
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_compress <- function(src, acceleration = 1, hc_level = 1, use_hc = FALSE) {
  .Call(lz4_compress_, src, acceleration, hc_level, use_hc)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompess a raw vector into the original data.
#'
#' The \code{src} vector must be a raw vector created by \code{lz4_compress} as
#' it relies on the bespoke framing created there.  This function will not
#' decompress an object compressed with another LZ4 implemetation e.g. \code{fst}
#'
#' @param src raw vector of compressed data
#'
#' @return uncompressed vector
#'
#' @useDynLib lz4lite lz4_decompress_
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_decompress <- function(src) {
  .Call(lz4_decompress_, src)
}
