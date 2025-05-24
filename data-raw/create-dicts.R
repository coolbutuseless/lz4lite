

words <- readLines("/usr/share/dict/words")
words <- sample(words)
length(words)

idx <- ceiling(seq_along(words)/1000)
ww <- split(words, idx)

for (i in seq_along(ww)) {
  filename <- sprintf("working/dict/words-%04i.txt", i)
  writeLines(ww[[i]], filename, sep = "")
}


system("zstd --train working/dict/* -o working/dict1 --maxdict=64KB")

dict <- readBin("working/dict1", raw(), file.size("working/dict1"))
length(dict)
sw <- sample(words, 100) |> paste(collapse = "")
nchar(sw)
lz4_serialize(sw) |> length()
lz4_serialize(sw, dict = dict) |> length()
lz4_serialize(sw, dict = dict) |> lz4_unserialize(dict = dict)


bench::mark(
  lz4_serialize(sw),
  lz4_serialize(sw, dict = dict),
  check = FALSE
)


zstdlite::zstd_serialize(sw) |> length()


rep('name', 10) |> 
  paste(collapse = "") |> 
  lz4_serialize() |> 
  length()
