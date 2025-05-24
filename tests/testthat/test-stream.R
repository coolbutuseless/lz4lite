

test_that("double buffered stream works", {
  
  dat <- mtcars
  tmp <- tempfile()
  lz4_serialize_stream(dat, tmp)
  
  res <- lz4_unserialize_stream(tmp)  
  expect_identical(res, dat)
  
  
  
  dat <- mtcars[sample(nrow(mtcars), 10000, T), ]
  tmp <- tempfile()
  lz4_serialize_stream(dat, tmp)
  
  res <- lz4_unserialize_stream(tmp)  
  expect_identical(res, dat)
  
  
  
})
