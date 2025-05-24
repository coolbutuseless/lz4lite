
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lz4.h"


#define BUF_SIZE 512 * 1024
#define FMODE_READ  0
#define FMODE_WRITE 1

typedef struct {
  FILE *file;
  int fmode;
  uint8_t buf[2][BUF_SIZE];
  int idx;
  uint32_t pos;
  uint32_t data_length;
  LZ4_stream_t       *stream_out;
  LZ4_streamDecode_t *stream_in;
  
  uint8_t *comp;
  int comp_capacity;
} dbuf_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//     ###                  #                    #    
//    #   #                 #                    #    
//    #       ###   # ##   ####    ###   #   #  ####  
//    #      #   #  ##  #   #     #   #   # #    #    
//    #      #   #  #   #   #     #####    #     #    
//    #   #  #   #  #   #   #  #  #       # #    #  # 
//     ###    ###   #   #    ##    ###   #   #    ##  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void db_destroy(dbuf_t *db) {
  if (db->file != NULL) {
    if (db->fmode == FMODE_WRITE) {
#ifdef USE_LZ4
      int comp_len = LZ4_compress_fast_continue(
        db->stream_in,     // Stream
        db->buf[db->idx],  // Source Raw Buffer
               db->comp,          // Dest Compressed buffer
               db->pos,           // Source size
               db->comp_capacity, // dstCapacity
               1                  // acceleration
      );
      if (comp_len < 0) Rf_error("Error compression lz4");
      fwrite(&db->pos, 1, sizeof(uint32_t), db->file); // Write raw length
      fwrite(&comp_len, 1, sizeof(int32_t), db->file); // write compressed length
      fwrite(db->comp, 1, comp_len, db->file);  // Write compressed buffer
#else
      fwrite(&db->pos, 1, sizeof(uint32_t), db->file);
      fwrite(db->buf[db->idx], 1, db->pos, db->file);
#endif
    }
    fclose(db->file);
  } else {
    Rf_error("Unknown output in 'db_flush()");
  }
  
  if (db->stream_out != NULL) {
    LZ4_freeStream(db->stream_out);
  }
  
  if (db->stream_in != NULL) {
    LZ4_freeStreamDecode(db->stream_in);
  }
  
  
  free(db);
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
  // Remember: We need to keep some historical bytes around so that
  // the lz4 compression has a data reference
  if (db->pos + length > BUF_SIZE) {
    
#ifdef USE_LZ4
    
    int comp_len = LZ4_compress_fast_continue(
      db->stream_in,     // Stream
      db->buf[db->idx],  // Source Raw Buffer
      db->comp,          // Dest Compressed buffer
      db->pos,           // Source size
      db->comp_capacity, // dstCapacity
      1                  // acceleration
    );
    if (comp_len < 0) Rf_error("Error compression lz4");
    fwrite(&db->pos, 1, sizeof(uint32_t), db->file); // Write raw length
    fwrite(&comp_len, 1, sizeof(int32_t), db->file); // write compressed length
    fwrite(db->comp, 1, comp_len, db->file);  // Write compressed buffer
    
#else
    fwrite(&db->pos, 1, sizeof(uint32_t), db->file); // Write length
    fwrite(db->buf[db->idx], 1, db->pos, db->file);  // Write buffer
#endif
    db->idx = 1 - db->idx; // switch buffers
    db->pos = 0;           // reset buffer position
  } 
  
  // This should never happen.  R serialization infrastrucutre seems
  // to work in chunks of 8k items. This is ~64k for floating point doubles
  if (db->pos + length > BUF_SIZE) {
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
SEXP lz4_serialize_stream_(SEXP x_, SEXP dst_) {
  
  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  if (TYPEOF(dst_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(dst_, 0));
    db->file = fopen(filename, "wb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for output: '%s'", filename);
    }
    db->fmode = FMODE_WRITE;
  } else {
    Rf_error("Don't know how to deal with 'dst' of type: [%i] %s", 
             TYPEOF(dst_), Rf_type2char(TYPEOF(dst_)));
  }
  
  // Setup LZ4 encoding stream context
  db->stream_out = LZ4_createStream();
  db->comp_capacity = LZ4_COMPRESSBOUND(BUF_SIZE);
  db->comp = malloc(db->comp_capacity);
  if (db->comp == NULL) {
    Rf_error("lz4_serialize_stream() couldnt allocate compressed buffer");
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

  // Flush buffers to output, close
  db_destroy(db);
  return R_NilValue;
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
    
    
    db->idx = 1 - db->idx; // swtich buffers
    db->pos = 0;           // Reset position
    
    // Read buffer length, then read buffer data
#ifdef USE_LZ4
    int comp_len;
    fread(&db->data_length, 1, sizeof(uint32_t), db->file);
    fread(&comp_len, 1, sizeof(int32_t), db->file);
    fread(db->comp, 1, comp_len, db->file);
    int res = LZ4_decompress_safe_continue(
      db->stream_in, 
      db->comp, 
      db->buf[db->idx],
      comp_len,
      BUF_SIZE
    );
    
#else
    fread(&db->data_length, 1, sizeof(uint32_t), db->file);
    unsigned long nread = fread(db->buf[db->idx], 1, db->data_length, db->file);
    if (nread != db->data_length) {
      Rf_error("Read failed: %i/%i", (int)nread, db->data_length);
    }
#endif
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
SEXP lz4_unserialize_stream_(SEXP src_) {

  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  if (TYPEOF(src_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(src_, 0));
    db->file = fopen(filename, "rb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for input: '%s'", filename);
    }
    db->fmode = FMODE_READ;
  } else {
    Rf_error("Don't know how to deal with 'src' of type: [%i] %s", 
             TYPEOF(src_), Rf_type2char(TYPEOF(src_)));
  }
  
  // LZ4 stream handling
  db->stream_in = LZ4_createStreamDecode();
  db->comp_capacity = LZ4_COMPRESSBOUND(BUF_SIZE);
  db->comp = malloc(db->comp_capacity);
  if (db->comp == NULL) {
    Rf_error("lz4_unserialize_stream() couldnt allocate compressed buffer");
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

  db_destroy(db);
  UNPROTECT(1);
  return res_;
}

