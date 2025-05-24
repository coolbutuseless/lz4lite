

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Serialize to file, raw or connection
#' 
#' @param dst filename (character), raw()/NULL for raw vector 
#' @param src data source for unserialization. file, or raw_vector()
#' @param acc accerlation. Default 1. Valid range [1, 65535].  Higher values
#'        means faster compression, but larger compressed size.
#' @param dict dictionary. raw vector. NULL for no dictionary.
#'        create \code{zstd --train dirSamples/* -o dictName --maxdict=64KB}
#' @param x R object
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


if (FALSE) {
  
  tmp <- tempfile()
  set.seed(1)
  # vec <- mtcars
  vec <- penguins[sample(nrow(penguins), 10000, T), ]
  lz4_serialize(vec, tmp)
  
  serialize(vec, NULL) |> length()
  file.size(tmp)
  
  # serialize(vec, NULL) 
  # readBin(tmp, raw(), file.size(tmp))
  
  res <- lz4_unserialize(tmp)
  identical(res, vec)
  
  
  tmp <- tempfile()
  bench::mark(
    lz4_serialize(vec),
    lz4_serialize(vec, tmp),
    check = FALSE
  )
  
}


