% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/serialize.R
\name{lz4_serialize}
\alias{lz4_serialize}
\alias{lz4_unserialize}
\title{Serialize an R object to a file or raw vector}
\usage{
lz4_serialize(x, dst = NULL, acc = 1L, dict = NULL)

lz4_unserialize(src, dict = NULL)
}
\arguments{
\item{x}{An R object}

\item{dst}{When \code{std} is a character, it will be treated as a 
filename. When \code{NULL} it indicates that the object should be 
serialized to a raw vector}

\item{acc}{LZ4 acceleration factor (for compression). 
Default 1. Valid range [1, 65535].  Higher values
mean faster compression, but larger compressed size.}

\item{dict}{Dictionary to aid in compression. raw vector. NULL for no dictionary.
create \code{zstd --train dirSamples/* -o dictName --maxdict=64KB}}

\item{src}{data source for unserialization. May be a file name, or raw vector}
}
\value{
If \code{dst} is a file, then no value is returned. Otherwise returns
        a raw vector.
}
\description{
Serialize an R object to a file or raw vector
}
\examples{
raw_vec <- lz4_serialize(mtcars)
head(raw_vec)
lz4_unserialize(raw_vec)
}
