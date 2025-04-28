

test_that("basic compression/uncompression of raw vectors works", {
  set.seed(1)
  for (N in c(1e2, 1e3, 1e4, 1e5)) {
    input_bytes <- as.raw(sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE))

    compressed_bytes <- lz4_compress(input_bytes)
    expect_true(length(compressed_bytes) < length(input_bytes))  # for non-pathological inputs
    uncompressed_bytes <- lz4_decompress(compressed_bytes)

    expect_identical(input_bytes, uncompressed_bytes)
  }
})

