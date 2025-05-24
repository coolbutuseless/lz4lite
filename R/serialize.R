

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize to file, raw or connection
#' 
#' @param dst filename (character), raw()/NULL for raw vector 
#' @param src data source for unserialization. file, or raw_vector()
#' @param acc accerlation. Default 1. Valid range [1, 65535].  Higher values
#'        means faster compression, but larger compressed size.
#' @param x R object
#' 
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
lz4_serialize_stream <- function(x, dst = NULL, acc = 1L) {
  res <- .Call(lz4_serialize_stream_, x, dst, acc)
  if (is.null(dst) || is.raw(dst)) {
    res
  } else {
    invisible()
  }
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
  # vec <- mtcars
  vec <- penguins[sample(nrow(penguins), 10000, T), ]
  lz4_serialize_stream(vec, tmp)
  
  serialize(vec, NULL) |> length()
  file.size(tmp)
  
  # serialize(vec, NULL) 
  # readBin(tmp, raw(), file.size(tmp))
  
  res <- lz4_unserialize_stream(tmp)
  identical(res, vec)
  
  
  tmp <- tempfile()
  bench::mark(
    lz4_serialize(vec),
    lz4_serialize_stream(vec, tmp),
    check = FALSE
  )
  
}


