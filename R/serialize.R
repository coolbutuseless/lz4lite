

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
  .Call(lz4_serialize_, robj)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_unserialize <- function(raw_vec) {
  .Call(lz4_unserialize_, raw_vec)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize to file, raw or connection
#' 
#' @param dst filename (character), raw()/NULL for raw vector 
#' @param src data source for unserialization. file, or raw_vector()
#' @param x R object
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_serialize_stream <- function(x, dst) {
  .Call(lz4_serialize_stream_, x, dst)
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' @rdname lz4_serialize_stream
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_unserialize_stream <- function(src) {
  .Call(lz4_unserialize_stream_, src)
}


if (FALSE) {
  
  tmp <- tempfile()
  set.seed(1)
  vec <- mtcars
  lz4_serialize_stream(vec, tmp)
  
  serialize(vec, NULL)
  readBin(tmp, raw(), file.size(tmp))
  
  res <- lz4_unserialize_stream(tmp)
  identical(res, vec)
}


