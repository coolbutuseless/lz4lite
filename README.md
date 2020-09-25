
<!-- README.md is generated from README.Rmd. Please edit that file -->

# lz4lite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg) [![Lifecycle:
experimental](https://img.shields.io/badge/lifecycle-experimental-orange.svg)](https://www.tidyverse.org/lifecycle/#experimental)
<!-- badges: end -->

`lz4lite` provides access to the extremely fast compression in
[lz4](https://github.com/lz4/lz4) for performing in-memory compression.

As of v0.2.0, `lz4lite` can now serialize and compress any R object
understood by `base::serialize()`.

If the input is known to be an atomic, numeric vector, and you do not
care about any attributes or names on this vector, then
`lz4_compress()`/`lz4_uncompress()` can be used. These are bespoke
serialization routines for atomic numeric vectors that run faster since
they avoid R’s internals.

For a more general solution to fast serialization of R objects, see the
[fst](https://github.com/fstpackage/fst) or
[qs](https://cran.r-project.org/package=qs) packages.

Currently lz4 code provided with this package is v1.9.3.

### What’s in the box

  - **For arbitrary R objects**
      - `lz4_serialize`/`lz4_unserialize` serialize and compress any R
        object.
  - **For atomic vectors with numeric values**
      - `lz4_compress()`/`lz4_uncompress()`
          - compress the data within a vector of raw, integer, real,
            complex or logical values
          - faster than `lz4_serialize/unserialize` but throws away all
            attributes i.e. names, dims etc

### Installation

You can install from [GitHub](https://github.com/coolbutuseless/lz4lite)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/lz4lite)
```

## Basic usage of lz4lite

``` r
dat <- mtcars


buf <- lz4_serialize(dat)
length(buf) # Number of bytes
```

    #> [1] 1862

``` r
# compression ratio
length(buf)/length(serialize(dat, NULL))
```

    #> [1] 0.489099

``` r
head(lz4_unserialize(buf))
```

    #>                    mpg cyl disp  hp drat    wt  qsec vs am gear carb
    #> Mazda RX4         21.0   6  160 110 3.90 2.620 16.46  0  1    4    4
    #> Mazda RX4 Wag     21.0   6  160 110 3.90 2.875 17.02  0  1    4    4
    #> Datsun 710        22.8   4  108  93 3.85 2.320 18.61  1  1    4    1
    #> Hornet 4 Drive    21.4   6  258 110 3.08 3.215 19.44  1  0    3    1
    #> Hornet Sportabout 18.7   8  360 175 3.15 3.440 17.02  0  0    3    2
    #> Valiant           18.1   6  225 105 2.76 3.460 20.22  1  0    3    1

## Compressing 1 million Integers

``` r
library(lz4lite)

max_hc <- 12

set.seed(1)
N                <- 5e6
input_ints       <- sample(1:3, N, prob = (1:3)^3, replace = TRUE)
serialize_base   <- serialize(input_ints, NULL, xdr = FALSE)
serialize_lo     <- lz4_serialize(input_ints, acceleration = 1)
serialize_hi_3   <- lz4hc_serialize(input_ints, level =  3)
serialize_hi_9   <- lz4hc_serialize(input_ints, level =  9)
serialize_hi_12  <- lz4hc_serialize(input_ints, level = max_hc)
compress_lo      <- lz4_compress(input_ints, acceleration = 1)
compress_hi_3    <- lz4hc_compress(input_ints, level = 3)
compress_hi_9    <- lz4hc_compress(input_ints, level = 9)
compress_hi_12   <- lz4hc_compress(input_ints, level = max_hc)
```

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
library(lz4lite)

res <- bench::mark(
  serialize(input_ints, NULL, xdr = FALSE),
  lz4_serialize(input_ints, acceleration = 1),
  lz4hc_serialize(input_ints, level =  3),
  lz4hc_serialize(input_ints, level =  9),
  lz4hc_serialize(input_ints, level = max_hc),
  lz4_compress (input_ints, acceleration = 1),
  lz4hc_compress (input_ints, level =  3),
  lz4hc_compress (input_ints, level =  9),
  lz4hc_compress (input_ints, level = max_hc),
  check = FALSE
)
```

</details>

| expression                                     |   median | itr/sec |  MB/s | compression\_ratio |
| :--------------------------------------------- | -------: | ------: | ----: | -----------------: |
| serialize(input\_ints, NULL, xdr = FALSE)      |  20.19ms |      49 | 944.8 |              1.000 |
| lz4\_serialize(input\_ints, acceleration = 1)  |  36.12ms |      28 | 528.1 |              0.222 |
| lz4hc\_serialize(input\_ints, level = 3)       | 212.26ms |       5 |  89.9 |              0.155 |
| lz4hc\_serialize(input\_ints, level = 9)       |    3.25s |       0 |   5.9 |              0.088 |
| lz4hc\_serialize(input\_ints, level = max\_hc) |   36.02s |       0 |   0.5 |              0.063 |
| lz4\_compress(input\_ints, acceleration = 1)   |   25.8ms |      39 | 739.2 |              0.222 |
| lz4hc\_compress(input\_ints, level = 3)        |  206.8ms |       5 |  92.2 |              0.155 |
| lz4hc\_compress(input\_ints, level = 9)        |    3.27s |       0 |   5.8 |              0.088 |
| lz4hc\_compress(input\_ints, level = max\_hc)  |   36.25s |       0 |   0.5 |              0.063 |

### uncompressing 1 million integers

uncompression speed varies slightly depending upon the compressed size.

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  lz4_uncompress(compress_lo),
  lz4_uncompress(compress_hi_3),
  lz4_uncompress(compress_hi_9),
  lz4_uncompress(compress_hi_12)
)
```

</details>

| expression                        |  median | itr/sec |   MB/s |
| :-------------------------------- | ------: | ------: | -----: |
| lz4\_uncompress(compress\_lo)     | 12.09ms |      73 | 1577.5 |
| lz4\_uncompress(compress\_hi\_3)  | 10.36ms |      93 | 1841.1 |
| lz4\_uncompress(compress\_hi\_9)  |  6.16ms |     152 | 3094.6 |
| lz4\_uncompress(compress\_hi\_12) |  4.76ms |     189 | 4002.6 |

### uncompressing 1 million integers

uncompression speed varies slightly depending upon the compressed size.

<details>

<summary> Click here to show/hide benchmark code </summary>

``` r
res <- bench::mark(
  unserialize(serialize_base),
  lz4_unserialize(serialize_lo),
  lz4_unserialize(serialize_hi_3),
  lz4_unserialize(serialize_hi_9),
  lz4_unserialize(serialize_hi_12)
)
```

</details>

| expression                          |  median | itr/sec |   MB/s |
| :---------------------------------- | ------: | ------: | -----: |
| unserialize(serialize\_base)        |  5.86ms |     130 | 3255.3 |
| lz4\_unserialize(serialize\_lo)     | 27.62ms |      42 |  690.5 |
| lz4\_unserialize(serialize\_hi\_3)  | 30.19ms |      39 |  631.9 |
| lz4\_unserialize(serialize\_hi\_9)  | 26.14ms |      46 |  729.8 |
| lz4\_unserialize(serialize\_hi\_12) | 23.91ms |      48 |  797.8 |

## Technical bits

### Framing of the compressed data

  - `lz4lite` does **not** use the standard LZ4 frame to store data.
  - The compressed representation is the compressed data prefixed with a
    custom 8 byte header consisting of
      - 3 bytes = ‘LZ4’
      - If this was produced with `lz4_serialize()` the next byte is
        0x00, otherwise it is a byte representing the SEXP of the
        encoded object.
      - 4-byte length value i.e. the number of bytes in the original
        uncompressed data.
  - This data representation
      - is not compatible with the standard LZ4 frame format.
      - is likely to evolve (so currently do not plan on compressing
        something in one version of `lz4lite` and uncompressing in
        another version.)

## Related Software

  - [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd) - both by Yann Collet
  - [fst](https://github.com/fstpackage/fst) for serialisation of
    data.frames using lz4 and zstd
  - [qs](https://cran.r-project.org/package=qs) for fast serialization
    of arbitrary R objects with lz4 and zstd

## Acknowledgements

  - Yann Collett for releasing, maintaining and advancing
    [lz4](https://github.com/lz4/lz4) and
    [zstd](https://github.com/facebook/zstd)
  - R Core for developing and maintaining such a wonderful language.
  - CRAN maintainers, for patiently shepherding packages onto CRAN and
    maintaining the repository
