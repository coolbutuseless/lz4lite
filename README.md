
<!-- README.md is generated from README.Rmd. Please edit that file -->

# lz4lite

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![CRAN](https://www.r-pkg.org/badges/version/lz4lite)](https://cran.r-project.org/package=lz4lite)
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

## Installation

<!-- This package can be installed from CRAN -->

<!-- ``` r -->

<!-- install.packages('lz4lite') -->

<!-- ``` -->

You can install the latest development version from
[GitHub](https://github.com/coolbutuseless/lz4lite) with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/lz4lite')
```

Pre-built source/binary versions can also be installed from
[R-universe](https://r-universe.dev)

``` r
install.packages('lz4lite', repos = c('https://coolbutuseless.r-universe.dev', 'https://cloud.r-project.org'))
```

## Basic usage of lz4lite

``` r
buf <- lz4_serialize(penguins)
length(buf) # Number of bytes
```

    #> [1] 4877

``` r
# compression ratio
length(buf)/length(serialize(penguins, NULL))
```

    #> [1] 0.3392696

``` r
head(lz4_unserialize(buf))
```

    #>   species    island bill_len bill_dep flipper_len body_mass    sex year
    #> 1  Adelie Torgersen     39.1     18.7         181      3750   male 2007
    #> 2  Adelie Torgersen     39.5     17.4         186      3800 female 2007
    #> 3  Adelie Torgersen     40.3     18.0         195      3250 female 2007
    #> 4  Adelie Torgersen       NA       NA          NA        NA   <NA> 2007
    #> 5  Adelie Torgersen     36.7     19.3         193      3450 female 2007
    #> 6  Adelie Torgersen     39.3     20.6         190      3650   male 2007

## Benchmark

``` r
dat <- penguins[sample(nrow(penguins), 10000, T), ]
tmp <- tempfile()


res <- bench::mark(
  lz4lite::lz4_serialize(dat, tmp),
  saveRDS(dat, tmp, compress = FALSE),
  saveRDS(dat, tmp, compress = 'gzip'),
  saveRDS(dat, tmp, compress = 'bzip2'),
  saveRDS(dat, tmp, compress = 'xz'),
  check = FALSE
)

res[, 1:5] |>
  knitr::kable(caption = "Time to serialize data and write to file")
```

| expression                            |      min |   median |    itr/sec | mem_alloc |
|:--------------------------------------|---------:|---------:|-----------:|----------:|
| lz4lite::lz4_serialize(dat, tmp)      |   1.03ms |   1.11ms | 852.906215 |    8.63KB |
| saveRDS(dat, tmp, compress = FALSE)   |   1.34ms |   1.56ms | 594.615649 |    8.63KB |
| saveRDS(dat, tmp, compress = “gzip”)  |  21.79ms |  22.01ms |  45.296677 |    8.63KB |
| saveRDS(dat, tmp, compress = “bzip2”) |  21.74ms |  22.28ms |  44.668393 |   11.58KB |
| saveRDS(dat, tmp, compress = “xz”)    | 135.38ms | 137.05ms |   7.311095 |   11.58KB |

Time to serialize data and write to file
