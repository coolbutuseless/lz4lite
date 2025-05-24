
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "lz4.h"


#define BUF_SIZE 512 * 1024

// Source / Destination mode
#define MODE_RAW    1
#define MODE_FILE   2

// Direction modes
#define MODE_UNSERIALIZE   8
#define MODE_SERIALIZE    16                                           



typedef struct {
  int mode;
  
  // For file output
  FILE *file;
  
  // For raw vector output
  uint8_t *raw;
  int raw_capacity;
  int raw_pos;
  
  // Double bufferes
  uint8_t buf[2][BUF_SIZE];
  int idx;              // which buffer is active
  uint32_t pos;         // position within active buffer
  uint32_t data_length; // total data length in active buffer (for reading)
  
  // For LZ4
  LZ4_stream_t       *stream_out;  // compression
  LZ4_streamDecode_t *stream_in;   // decompression
  int acceleration;                // range [1, 65535]
  uint8_t *comp;                   // compressed buffer
  int comp_capacity;               // capacity of compressed buffer
  
  bool checked_magic;
} dbuf_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress the current buffer and output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_compressed_buf(dbuf_t *db) {
  int comp_len = LZ4_compress_fast_continue(
    db->stream_out,                  // Stream
    (const char *)db->buf[db->idx],  // Source Raw Buffer
                         (char *)db->comp,                // Dest Compressed buffer
                         db->pos,                         // Source size
                         db->comp_capacity,               // dstCapacity
                         db->acceleration
  );
  if (comp_len < 0) Rf_error("Error compression lz4");
  
  if (db->mode & MODE_FILE) {
    fwrite(&db->pos, 1, sizeof(uint32_t), db->file); // Write raw length
    fwrite(&comp_len, 1, sizeof(int32_t), db->file); // write compressed length
    fwrite(db->comp, 1, comp_len, db->file);  // Write compressed buffer
  } else if (db->mode & MODE_RAW) {
    
    if (db->raw_pos + 2 * sizeof(uint32_t) + comp_len >= db->raw_capacity) {
      db->raw_capacity *= 2;
      db->raw = realloc(db->raw, db->raw_capacity);
    }
    
    memcpy(db->raw + db->raw_pos, &db->pos ,        4); db->raw_pos += 4;
    memcpy(db->raw + db->raw_pos, &comp_len,        4); db->raw_pos += 4;
    memcpy(db->raw + db->raw_pos, db->comp , comp_len); db->raw_pos += comp_len;
    
  } else {
    Rf_error("write_compressed_buf(): unknown mode");
  }    
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//     ###                  #                    #    
//    #   #                 #                    #    
//    #       ###   # ##   ####    ###   #   #  ####  
//    #      #   #  ##  #   #     #   #   # #    #    
//    #      #   #  #   #   #     #####    #     #    
//    #   #  #   #  #   #   #  #  #       # #    #  # 
//     ###    ###   #   #    ##    ###   #   #    ##  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalise the context after serializing/unserializing
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP db_finalize(dbuf_t *db) {
  
  int nprotect = 0;
  SEXP res_ = R_NilValue;
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Flush any contents of the write buffers
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (db->mode & MODE_SERIALIZE) {
    write_compressed_buf(db);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Close the file
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (db->mode & MODE_FILE) {
    fclose(db->file);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // When serializzing to raw, create an R raw vector and then free the
  // allocated memory in C
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (db->mode & MODE_RAW && db->mode & MODE_SERIALIZE) {
    res_ = PROTECT(Rf_allocVector(RAWSXP, db->raw_pos)); nprotect++;
    memcpy(RAW(res_), db->raw, db->raw_pos);
    free(db->raw);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free the LZ4 encoding stream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (db->mode & MODE_SERIALIZE) {
    LZ4_freeStream(db->stream_out);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Free the LZ4 decoding stream
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (db->mode & MODE_UNSERIALIZE) {
    LZ4_freeStreamDecode(db->stream_in);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Both serialize and unserialize use the same comrpession buffer. Free it.
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  free(db->comp);
  free(db);
  
  UNPROTECT(nprotect);
  return res_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    #   #           #     #           
//    #   #                 #           
//    #   #  # ##    ##    ####    ###  
//    # # #  ##  #    #     #     #   # 
//    # # #  #        #     #     ##### 
//    ## ##  #        #     #  #  #     
//    #   #  #       ###     ##    ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_stream(R_outpstream_t stream, int c) {
  Rf_error("'write_byte_stream()' is never called");
}


void write_bytes_stream(R_outpstream_t stream, void *src, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  
  
  // If this 'write' would overflow the size of the current buffer, 
  // then write out the current buffer and switch to the other.
  // Remember: We need to keep these historical bytes around so that
  // the lz4 compression has a data reference of 64kB
  if (db->pos + length >= BUF_SIZE) {
    write_compressed_buf(db); // compress and write the current buffer
    db->idx = 1 - db->idx;    // switch buffers
    db->pos = 0;              // reset buffer position
  } 
  
  
  // This should never happen.  R serialization infrastrucutre seems
  // to work in chunks of 8k items. This is ~64k for floating point doubles
  // and should never exceed BUF_SIZE
  if (db->pos + length >= BUF_SIZE) {
    Rf_error("Out-of-range write %i/%i", length, BUF_SIZE);
  }
  
  
  // Append data to the current buffer
  memcpy(db->buf[db->idx] + db->pos, src, length);
  db->pos += length;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   ###                   #            ##      #                 
//  #   #                                #                        
//  #       ###   # ##    ##     ###     #     ##    #####   ###  
//   ###   #   #  ##  #    #        #    #      #       #   #   # 
//      #  #####  #        #     ####    #      #      #    ##### 
//  #   #  #      #        #    #   #    #      #     #     #     
//   ###    ###   #       ###    ####   ###    ###   #####   ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_serialize_(SEXP x_, SEXP dst_, SEXP acc_, SEXP dict_) {
  
  // Allocate the double-buffer context
  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  
  // Set the user option for 'acceleration'
  db->acceleration = Rf_asInteger(acc_);
  db->mode = MODE_SERIALIZE;
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set up the destination:
  //   - character   => output to file
  //   - NULL or raw => output to a raw vector
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(dst_) == STRSXP) {
    db->mode |= MODE_FILE;
    const char *filename = CHAR(STRING_ELT(dst_, 0));
    db->file = fopen(filename, "wb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for output: '%s'", filename);
    }
  } else if (Rf_isNull(dst_) || TYPEOF(dst_) == RAWSXP) {
    db->mode |= MODE_RAW;
    db->raw_pos      = 0;
    db->raw_capacity = BUF_SIZE;
    db->raw          = malloc(BUF_SIZE);
    if (db->raw == NULL) Rf_error("Couldn't initialize raw buffer");
  } else {
    Rf_error("Don't know how to deal with 'dst' of type: [%i] %s", 
             TYPEOF(dst_), Rf_type2char(TYPEOF(dst_)));
  }
  
  
  // Setup LZ4 encoding stream context
  db->stream_out    = LZ4_createStream();
  db->comp_capacity = LZ4_COMPRESSBOUND(BUF_SIZE);
  db->comp          = malloc(db->comp_capacity);
  if (db->comp == NULL) {
    Rf_error("lz4_serialize() couldnt allocate compressed buffer");
  }
  
  
  // Dictionary
  if (TYPEOF(dict_) == RAWSXP) {
    int res = LZ4_loadDict(db->stream_out, (const char *)RAW(dict_), (int)Rf_length(dict_));
    if (res <= 0) {
      Rf_error("Error loading dictionary");
    }
  } else if (!Rf_isNull(dict_)) {
    Rf_error("Dictionary must be raw() vector or NULL");
  }
  
  // Write magic
  char *magic = "LZ4S";
  if (db->mode & MODE_FILE) {
    fwrite(magic, 1, 4, db->file);
  } else {
    memcpy(db->raw, magic, 4);
    db->raw_pos += 4;
  }

  // Create & initialise the output stream structure
  struct R_outpstream_st output_stream;
  R_InitOutPStream(
    &output_stream,          // The stream object which wraps everything
    (R_pstream_data_t) db,   // The actual data
    R_pstream_binary_format, // Store as binary
    3,                       // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte_stream,       // Function to write single byte to buffer
    write_bytes_stream,      // Function for writing multiple bytes to buffer
    NULL,                    // Func for special handling of reference data.
    R_NilValue               // Data related to reference data handling
  );

  
  // Serialize the object into the output_stream
  R_Serialize(x_, &output_stream);

  // Flush buffers to output, close.
  // Return vector if serializing to raw.
  return db_finalize(db);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ####                     # 
//  #   #                    # 
//  #   #   ###    ###    ## # 
//  ####   #   #      #  #  ## 
//  # #    #####   ####  #   # 
//  #  #   #      #   #  #  ## 
//  #   #   ###    ####   ## # 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte_stream(R_inpstream_t stream) {
  Rf_error("'read_byte_stream()' is never called");
  return 0;
}


void read_bytes_stream(R_inpstream_t stream, void *dst, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  
  while (db->pos + length > db->data_length) {
    // Not enough bytes to satisfy the request. So:
    //  - copy across available bytes
    //  - load the next buffer
    
    // Copy across available bytes
    int nbytes = db->data_length - db->pos; // bytes left in current buffer
    memcpy(dst, db->buf[db->idx] + db->pos, nbytes);
    length -= nbytes;
    
    db->idx = 1 - db->idx; // switch buffers
    db->pos = 0;           // Reset position
    
    if (!db->checked_magic) {
      db->checked_magic = true;
      if (db->mode & MODE_RAW) {
        if (memcmp("LZ4S", db->raw, 4) != 0) {
          Rf_error("Raw vector is not a lz4 serialized stream");
        }
        db->raw_pos += 4;
      } else {
        char buf[10];
        unsigned long nread = fread(buf, 1, 4, db->file);
        if (nread != 4 || strncmp(buf, "LZ4S", 4) != 0) {
          Rf_error("File is not a lz4 serialized stream");
        }
      }
    }
    
    
    // Read 
    //   - buffer length, 
    //   - compressed length
    //   - compressed data
    int comp_len;
    
    if (db->mode & MODE_FILE) {
      unsigned long nread = fread(&db->data_length, 1, sizeof(uint32_t), db->file);
      if (nread != 4) Rf_error("Error reading 4 byte data length");
      nread = fread(&comp_len, 1, sizeof(int32_t), db->file);
      if (nread != 4) Rf_error("Error reading 4 byte compressed length");
      nread = fread(db->comp, 1, comp_len, db->file);
      if (nread != comp_len) Rf_error("Error reading compressed data of length %i", (int)nread);
    } else if (db->mode & MODE_RAW) {
      memcpy (&db->data_length, db->raw + db->raw_pos,        4); db->raw_pos += 4;
      memcpy (&comp_len       , db->raw + db->raw_pos,        4); db->raw_pos += 4;
      memcpy(db->comp         , db->raw + db->raw_pos, comp_len); db->raw_pos += comp_len;
      
    } else {
      Rf_error("Unserialize [000]");  
    }
    
    // Decompress
    int res = LZ4_decompress_safe_continue(
      db->stream_in,             // Stream
      (const char *)db->comp,    // Src compressed buffer
      (char *)db->buf[db->idx],  // Dst raw buffer
      comp_len,                  // Src size
      BUF_SIZE                   // Compressed size
    );
    if (res < 0) {
      Rf_error("Lz4 decompression error %i", res);
    }
  }
  
  
  // copy across bytes
  memcpy(dst, db->buf[db->idx] + db->pos, length);
  db->pos += length;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  #   #                                #            ##      #                 
//  #   #                                              #                        
//  #   #  # ##    ###    ###   # ##    ##     ###     #     ##    #####   ###  
//  #   #  ##  #  #      #   #  ##  #    #        #    #      #       #   #   # 
//  #   #  #   #   ###   #####  #        #     ####    #      #      #    ##### 
//  #   #  #   #      #  #      #        #    #   #    #      #     #     #     
//   ###   #   #  ####    ###   #       ###    ####   ###    ###   #####   ###  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_unserialize_(SEXP src_, SEXP dict_) {

  // Allocate double-buffer context
  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  db->mode = MODE_UNSERIALIZE;
  
  
  // Set input type to be raw vector or a filename
  if (TYPEOF(src_) == STRSXP) {
    db->mode |= MODE_FILE;
    const char *filename = CHAR(STRING_ELT(src_, 0));
    db->file = fopen(filename, "rb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for input: '%s'", filename);
    }
  } else if (TYPEOF(src_) == RAWSXP) {
    db->mode |= MODE_RAW;
    db->raw          = RAW(src_);
    db->raw_pos      = 0;
    db->raw_capacity = (int)Rf_length(src_);
  } else {
    Rf_error("Don't know how to deal with 'src' of type: [%i] %s", 
             TYPEOF(src_), Rf_type2char(TYPEOF(src_)));
  }
  
  
  // LZ4 stream handling
  db->stream_in     = LZ4_createStreamDecode();
  db->comp_capacity = LZ4_COMPRESSBOUND(BUF_SIZE);
  db->comp          = malloc(db->comp_capacity);
  if (db->comp == NULL) {
    Rf_error("lz4_unserialize() couldnt allocate compressed buffer");
  }
  
  
  // Dictionary
  if (TYPEOF(dict_) == RAWSXP) {
    int res = LZ4_setStreamDecode(db->stream_in, (const char *)RAW(dict_), (int)Rf_length(dict_));
    if (res <= 0) {
      Rf_error("Error loading dictionary");
    }
  } else if (!Rf_isNull(dict_)) {
    Rf_error("Dictionary must be raw() vector or NULL");
  }
  

  // INitialise the input stream structure
  struct R_inpstream_st input_stream;
  R_InitInPStream(
    &input_stream,           // Stream object wrapping data buffer
    (R_pstream_data_t) db,  // Actual data buffer
    R_pstream_any_format,    // Unpack all serialized types
    read_byte_stream,        // Function to read single byte from buffer
    read_bytes_stream,       // Function for reading multiple bytes from buffer
    NULL,                    // Func for special handling of reference data.
    NULL                     // Data related to reference data handling
  );

  
  // Unserialize the input_stream into an R object
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));

  db_finalize(db);
  UNPROTECT(1);
  return res_;
}

