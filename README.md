
<!-- README.md is generated from README.Rmd. Please edit that file -->

# lz4lite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
<!-- badges: end -->

`lz4lite` provides access to the extremely fast compression in
[lz4](https://github.com/lz4/lz4) for performing in-memory compression.

The scope of this package is limited - it aims to provide functions for
direct hashing of vectors which contain raw, integer, real or logical
values. If you wanted to compress arbitrary R objects, you must first
convert into a raw vector representation using `base::serialize()`.

For a more general solution to fast serialization of R objects, see the
[fst](https://github.com/fstpackage/fst) package, which offers
multi-threading and support for most standard R object types.

Currently lz4 code provided with this package is v1.9.3.

### Design Choices

`lz4lite` will compress the *data payload* within a numeric-ish vector,
and not the R object itself.

### Limitations

  - As it is the *data payload* of the vector that is being compressed,
    this does not include any notion of the container for that data i.e
    dimensions or other attributes are not compressed with the data.
  - Values must be of type: raw, integer, real or logical.
  - Decompressed values are always returned as a vector i.e. all
    dimensional information is lost during compression.

### What’s in the box

  - `lz4compress()`
      - compress the data within a vector of raw, integer, real or
        logical values
      - set `use_hc = TRUE` to use the High Compression variant of LZ4.
        This variant can be slow to compress, but with higher
        compression ratios, and it retains the fast decompression speed
        i.e. multiple gigabytes per second\!
  - `lz4decompress()` - decompress a compressed representation that was
    created with `lz4compress()`

### Installation

You can install from [GitHub](https://github.com/coolbutuseless/lz4lite)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/lz4lite)
```

## Compressing 1 million Integers

`lz4lite` supports the direct compression of raw, integer, real and
logical vectors.

On this test data, compression speed is \~600 MB/s, and decompression
speed is \~3GB/s

``` r
library(lz4lite)

N             <- 1e6
input_ints    <- (sample(seq(1:5), N, prob = (1:5)^2, replace = TRUE))
compressed_lo <- lz4_compress(input_ints)
compressed_hi <- lz4_compress(input_ints, use_hc = TRUE, hc_level = 12)
```

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
library(lz4lite)
library(fst)

res <- bench::mark(
  lz4_compress(input_ints, acc   =   1),
  lz4_compress(input_ints, acc   =  10),
  lz4_compress(input_ints, acc   =  20),
  lz4_compress(input_ints, acc   =  50),
  lz4_compress(input_ints, acc   = 100),
  lz4_compress(input_ints, use_hc = TRUE, hc_level =   1),
  lz4_compress(input_ints, use_hc = TRUE, hc_level =   2),
  lz4_compress(input_ints, use_hc = TRUE, hc_level =   4),
  lz4_compress(input_ints, use_hc = TRUE, hc_level =   8),
  lz4_compress(input_ints, use_hc = TRUE, hc_level =  12),
  check = FALSE
)
```

</details>

| expression                                                 |  median | itr/sec |  MB/s | compression\_ratio |
| :--------------------------------------------------------- | ------: | ------: | ----: | -----------------: |
| lz4\_compress(input\_ints, acc = 1)                        |  6.38ms |     157 | 598.3 |              0.306 |
| lz4\_compress(input\_ints, acc = 10)                       |  6.32ms |     158 | 604.0 |              0.306 |
| lz4\_compress(input\_ints, acc = 20)                       |  6.34ms |     159 | 601.2 |              0.306 |
| lz4\_compress(input\_ints, acc = 50)                       |  6.29ms |     158 | 606.7 |              0.306 |
| lz4\_compress(input\_ints, acc = 100)                      |  6.25ms |     159 | 610.5 |              0.306 |
| lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 1)  | 35.38ms |      28 | 107.8 |              0.294 |
| lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 2)  | 35.09ms |      28 | 108.7 |              0.294 |
| lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 4)  | 68.16ms |      15 |  56.0 |              0.233 |
| lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 8)  |   454ms |       2 |   8.4 |              0.167 |
| lz4\_compress(input\_ints, use\_hc = TRUE, hc\_level = 12) |  11.52s |       0 |   0.3 |              0.122 |

### Decompressing 1 million integers

Decompression speed varies slightly depending upon the compressed size.

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  lz4_decompress(compressed_lo),
  lz4_decompress(compressed_hi)
)
```

</details>

| expression                      | median | itr/sec |   MB/s |
| :------------------------------ | -----: | ------: | -----: |
| lz4\_decompress(compressed\_lo) | 1.65ms |     570 | 2311.6 |
| lz4\_decompress(compressed\_hi) | 1.23ms |     756 | 3109.5 |

## Technical bits

### How it works.

##### Compression

1.  Given a pointer to a standard numeric vector from R, an *SEXP*
2.  Ignoring any attributes or dimensions, compress the data payload
    within the object.
3.  Prefix the compressed data with an 8 byte header giving size and
    SEXP type
4.  Return a raw vector to the user containing the compressed bytes.

##### Decompression

1.  Strip off the header information
2.  Feed the raw bytes in to the C LZ4 decompression function
3.  Use the header to decide what sort of R object this is
4.  Uncompress the data into an R object of the correct type.
5.  Return the R object to the user

**Note:** matrices and arrays may also be passed to `lz4_compress()`,
but since no attributes are retained (e.g. dims), the uncompressed
object returned by `lz4_decompress()` can only be a simple vector.

### Framing of the compressed data

  - `lz4lite` does **not** use the standard LZ4 frame to store data.
  - The compressed representation is the compressed data prefixed with a
    custom 8 byte header consisting of
      - ‘LZ4’
      - 1-byte for SEXP type i.e. INTSXP, RAWSXP, REALSXP or LGLSXP
      - 4-bytes representing an integer i.e. the number of bytes in the
        original uncompressed data.
  - This data representation
      - is not compatible with the standard LZ4 frame format.
      - is likely to evolve (so currently do not plan on compressing
        something in one version of `lz4lite` and decompressing in
        another version.)

## Related Software

  - [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd) - both by Yann Collet
  - [fst](https://github.com/fstpackage/fst) for serialisation of
    data.frames using lz4 and zstd

## Acknowledgements

  - Yann Collett for releasing, maintaining and advancing
    [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd)
  - R Core for developing and maintaining such a wonderful language.
  - CRAN maintainers, for patiently shepherding packages onto CRAN and
    maintaining the repository
