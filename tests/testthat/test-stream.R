

test_that("double buffered stream to file works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # small write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars
  tmp <- tempfile()
  lz4_serialize_stream(dat, tmp)
  
  res <- lz4_unserialize_stream(tmp)  
  expect_identical(res, dat)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # large write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars[sample(nrow(mtcars), 10000, T), ]
  tmp <- tempfile()
  lz4_serialize_stream(dat, tmp)
  
  res <- lz4_unserialize_stream(tmp)  
  expect_identical(res, dat)
  
  
  
})




test_that("double buffered stream to raw works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # small write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars
  enc <- lz4_serialize_stream(dat, NULL)
  
  res <- lz4_unserialize_stream(enc)  
  expect_identical(res, dat)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # large write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars[sample(nrow(mtcars), 10000, T), ]
  enc <- lz4_serialize_stream(dat, NULL)
  
  res <- lz4_unserialize_stream(enc)  
  expect_identical(res, dat)
  
  
  
})
