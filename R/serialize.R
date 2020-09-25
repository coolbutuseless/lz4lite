

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using LZ4
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param acceleration Default: 50
#' @param raw_vec Raw vector containing a compressed serialized representation of
#'        an R object
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_serialize <- function(robj, acceleration = 1L) {
  .Call('lz4_serialize_', robj, as.integer(acceleration), FALSE, 1L)
}

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize
#' @param level compression level. Default: 9. Valid range 3-12
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4hc_serialize <- function(robj, level = 9L) {
  .Call('lz4_serialize_', robj, 1L, TRUE, as.integer(level))
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_unserialize <- function(raw_vec) {
  .Call('lz4_unserialize_', raw_vec)
}



if (FALSE) {
  obj <- sample(1:3, 1000, replace = TRUE)
  obj <- do.call(rbind, replicate(100, mtcars, simplify = FALSE))
  length(serialize(obj, NULL, xdr = FALSE))
  lz4_serialize(obj, 0) -> zz
  length(zz)
  lz4_unserialize(zz)
}
