

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize arbitrary objects to a compressed stream of bytes using LZ4
#'
#' @param robj Any R object understood by \code{base::serialize()}
#' @param raw_vec Raw vector containing a compressed serialized representation of
#'        an R object
#'
#' @return serialized representation compressed into a raw byte vector
#'
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_serialize <- function(robj) {
  .Call('lz4_serialize_', robj)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_unserialize <- function(raw_vec) {
  .Call('lz4_unserialize_', raw_vec)
}

