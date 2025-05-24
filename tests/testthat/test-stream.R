

test_that("double buffered stream to file works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # small write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars
  tmp <- tempfile()
  lz4_serialize(dat, tmp)
  
  res <- lz4_unserialize(tmp)  
  expect_identical(res, dat)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # large write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars[sample(nrow(mtcars), 10000, T), ]
  tmp <- tempfile()
  lz4_serialize(dat, tmp)
  
  res <- lz4_unserialize(tmp)  
  expect_identical(res, dat)
  
  
  
})




test_that("double buffered stream to raw works", {
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # small write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars
  enc <- lz4_serialize(dat, NULL)
  
  res <- lz4_unserialize(enc)  
  expect_identical(res, dat)
  
  
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # large write to file
  #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dat <- mtcars[sample(nrow(mtcars), 10000, T), ]
  enc <- lz4_serialize(dat, NULL)
  
  res <- lz4_unserialize(enc)  
  expect_identical(res, dat)
  
  
  
})
