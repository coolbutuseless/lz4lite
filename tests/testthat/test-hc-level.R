

test_that("HC compression/decompression of raw vectors works", {
  set.seed(1)
  for (level in 1:12) {
    for (N in c(1e2, 1e3, 1e4, 1e5)) {
      input_bytes <- as.raw(sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE))

      compressed_bytes <- lz4_compress(input_bytes, use_hc = TRUE, hc_level = level)
      # cat(N, " ", acc, " ", length(compressed_bytes)/length(input_bytes), "\n")
      decompressed_bytes <- lz4_decompress(compressed_bytes)

      expect_identical(input_bytes, decompressed_bytes)
    }
  }
})

