

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Compress a raw vector
#'
#' @param src raw vector to be compressed.
#'
#' @return raw vector of compressed data
#' @examples
#' src <- as.raw(rep(1L, 10000))
#' length(src)
#' enc <- lz4_compress(src)
#' length(enc)
#' result <- lz4_decompress(enc)
#' length(result)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_compress <- function(src) {
  .Call(lz4_compress_, src)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Decompress a raw vector of compressed data 
#'
#' @param src raw vector of compressed data created with \code{\link{lz4_compress}()}
#' @return uncompressed vector
#' @examples
#' src <- as.raw(rep(1L, 10000))
#' length(src)
#' enc <- lz4_compress(src)
#' length(enc)
#' result <- lz4_decompress(enc)
#' length(result)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_decompress <- function(src) {
  .Call(lz4_decompress_, src)
}

