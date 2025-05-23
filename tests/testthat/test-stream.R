

test_that("double buffered stream works", {
  
  tmp <- tempfile()
  lz4_serialize_stream(mtcars, tmp)
  
  res <- lz4_unserialize_stream(tmp)  
  expect_identical(res, mtcars)
  
})
