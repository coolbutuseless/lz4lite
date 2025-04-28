
<!-- README.md is generated from README.Rmd. Please edit that file -->

# lz4lite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![CRAN](https://www.r-pkg.org/badges/version/yyjsonr)](https://cran.r-project.org/package=yyjsonr)
[![R-CMD-check](https://github.com/coolbutuseless/lz4lite/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/lz4lite/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`lz4lite` provides access to the extremely fast compression in
[lz4](https://github.com/lz4/lz4) for performing in-memory compression
of any R ojbect.

`lz4` code provided with this package is v1.10.0.

### What’s in the box

- **For arbitrary R objects**
  - `lz4_serialize()`/`lz4_unserialize()` serialize and compress any R
    object.
- **For raw vectors**
  - `lz4_compress()`/`lz4_decompress()`

### Installation

You can install from [GitHub](https://github.com/coolbutuseless/lz4lite)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/lz4lite)
```

Pre-built source/binary versions can also be installed from
[R-universe](https://r-universe.dev)

``` r
install.packages('lz4lite', repos = c('https://coolbutuseless.r-universe.dev', 'https://cloud.r-project.org'))
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

## Technical bits

### Framing of the compressed data

- `lz4lite` does **not** use the standard LZ4 frame to store data.
- The compressed representation is the compressed data prefixed with a
  custom 8 byte header consisting of
  - 3 bytes = ‘LZ4’
  - If this was produced with `lz4_serialize()` the next byte is ‘S’,
    otherwise ‘C’.
  - 4-byte length value i.e. the number of bytes in the original
    uncompressed data.
- This data representation
  - is not compatible with the standard LZ4 frame format.
  - is likely to evolve (so currently do not plan on compressing
    something in one version of `lz4lite` and decompressing in another
    version.)
