

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize an R object to a file or raw vector
#' 
#' @param x An R object
#' @param dst When \code{std} is a character, it will be treated as a 
#'        filename. When \code{NULL} it indicates that the object should be 
#'        serialized to a raw vector 
#' @param src data source for unserialization. May be a file name, or raw vector
#' @param acc LZ4 acceleration factor (for compression). 
#'        Default 1. Valid range [1, 65535].  Higher values
#'        mean faster compression, but larger compressed size.
#' @param dict Dictionary to aid in compression. raw vector. NULL for no dictionary.
#'        create \code{zstd --train dirSamples/* -o dictName --maxdict=64KB}
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_serialize <- function(x, dst = NULL, acc = 1L, dict = NULL) {
  res <- .Call(lz4_serialize_, x, dst, acc, dict)
  if (is.null(dst) || is.raw(dst)) {
    res
  } else {
    invisible()
  }
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_unserialize <- function(src, dict = NULL) {
  .Call(lz4_unserialize_, src, dict)
}

