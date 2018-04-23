// To prevent collisions with other apps using miniz
#define mz_adler32 udComp_adler32
#define mz_crc32 udComp_crc32
#define mz_free udComp_free
#define mz_version  udComp_version
#define mz_deflateEnd udComp_deflateEnd
#define mz_deflateBound udComp_deflateBound
#define mz_compressBound udComp_compressBound
#define mz_inflateInit2 udComp_inflateInit2
#define mz_inflateInit udComp_inflateInit
#define mz_inflateEnd udComp_inflateEnd
#define mz_error udComp_error
#define tinfl_decompress udCompTInf_decompress
#define tinfl_decompress_mem_to_heap udCompTInf_decompress_mem_to_heap
#define tinfl_decompress_mem_to_mem udCompTInf_decompress_mem_to_mem
#define tinfl_decompress_mem_to_callback udCompTInf_decompress_mem_to_callback
#define tdefl_compress udCompTDefl_compress
#define tdefl_compress_buffer udCompTDefl_compress_buffer
#define tdefl_init udCompTDefl_init
#define tdefl_get_prev_return_status udCompTDefl_get_prev_return_status
#define tdefl_get_adler32 udCompTDefl_get_adler32
#define tdefl_compress_mem_to_output udCompTDefl_compress_mem_to_output
#define tdefl_compress_mem_to_heap udCompTDefl_compress_mem_to_heap
#define tdefl_compress_mem_to_mem udCompTDefl_compress_mem_to_mem
#define tdefl_create_comp_flags_from_zip_params udCompTDefl_create_comp_flags_from_zip_params
#define tdefl_write_image_to_png_file_in_memory_ex udCompTDefl_write_image_to_png_file_in_memory_ex
#define tdefl_write_image_to_png_file_in_memory udCompTDefl_write_image_to_png_file_in_memory
#define mz_deflateInit2 udComp_deflateInit2
#define mz_deflateReset udComp_deflateReset
#define mz_deflate udComp_deflate
#define mz_inflate udComp_inflate
#define mz_uncompress udComp_uncompress
#define mz_deflateInit udComp_deflateInit
#define mz_compress2 udComp_compress2
#define mz_compress udComp_compress

#define mz_zip_writer_init_from_reader udComp_zip_writer_init_from_reader
#define mz_zip_reader_end udComp_mz_zip_reader_end
#define mz_zip_reader_init_mem udComp_mz_zip_reader_init_mem
#define mz_zip_reader_locate_file udComp_mz_zip_reader_locate_file
#define mz_zip_reader_file_stat udComp_mz_zip_reader_file_stat

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_MALLOC
#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable:4334)
#endif
#include "miniz/miniz.c"
#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#include "udPlatform.h"
#include "udCompression.h"

struct MiniZPrealloc
{
  size_t size;
  void *pMemory;
  bool inuse;
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, November 2017
static void *MiniZAlloc(void *pOpaque, size_t items, size_t size)
{
  MiniZPrealloc *pPrealloc = (MiniZPrealloc*)pOpaque;
  if (pPrealloc && !pPrealloc->inuse && pPrealloc->size == (items * size))
  {
    pPrealloc->inuse = true;
    return pPrealloc->pMemory;
  }
  return udAlloc(items * size);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, November 2017
static void MiniZFree(void *pOpaque, void *pPtr)
{
  MiniZPrealloc *pPrealloc = (MiniZPrealloc*)pOpaque;
  if (pPrealloc && pPrealloc->pMemory == pPtr)
  {
    pPrealloc->inuse = false;
    return;
  }
  udFree(pPtr);
}

// ****************************************************************************
// Author: Dave Pevreal, November 2017
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type)
{
  udResult result;
  void *pTemp = nullptr;
  mz_ulong destSize;
  mz_stream stream;
  int mzErr;
  tdefl_compressor compressorMemory;
  MiniZPrealloc prealloc = { sizeof(compressorMemory), &compressorMemory, false };

  memset(&stream, 0, sizeof(stream));
  UD_ERROR_IF(!ppDest || !pDestSize || !pSource, udR_InvalidParameter_);

  // Handle the special case of no compression, using udMemDup
  if (type == udCT_None)
  {
    *ppDest = udMemDup(pSource, sourceSize, 0, udAF_None);
    *pDestSize = sourceSize;
    return udR_Success;
  }

  UD_ERROR_IF(type != udCT_MiniZ, udR_InvalidParameter_);
  // Initially allocate a temp buffer that's conservatively larger in case deflate fails to make a gain
  destSize = mz_deflateBound(nullptr, (mz_ulong)sourceSize);
  pTemp = udAlloc(destSize);
  UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

  // We use the more advanced miniz interface to avoid miniz allocating via malloc
  stream.next_in = (const uint8_t*)pSource;
  stream.avail_in = (mz_uint32)sourceSize;
  stream.next_out = (uint8_t*)pTemp;
  stream.avail_out = (mz_uint32)destSize;
  stream.zalloc = MiniZAlloc;
  stream.zfree = MiniZFree;
  stream.opaque = &prealloc; // We know there's only one alloc, so save some processor power

  mzErr = mz_deflateInit2(&stream, MZ_BEST_COMPRESSION, MZ_DEFLATED, MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY);
  UD_ERROR_IF(mzErr != MZ_OK, udR_CompressionError);

  mzErr = mz_deflate(&stream, MZ_FINISH);
  UD_ERROR_IF(mzErr != MZ_STREAM_END, udR_OutputExhausted);

  destSize = stream.total_out;

  // Size the allocation as required
  *pDestSize = destSize;
  *ppDest = udRealloc(pTemp, destSize);
  UD_ERROR_NULL(*ppDest, udR_MemoryAllocationFailure);
  pTemp = nullptr; // Prevent freeing on successful realloc

  result = udR_Success;

epilogue:
  mz_deflateEnd(&stream); // Ok to call on a zero'd but not initialised stream
  udFree(pTemp);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2017
udResult udCompression_Inflate(void *pDest, size_t destSize, const void *pSource, size_t sourceSize, size_t *pInflatedSize, udCompressionType type)
{
  udResult result;
  tinfl_status mzErr;
  tinfl_decompressor decompressor;
  size_t inflatedSize = destSize; // in-out parameter, in is destSize, out is inflatedSize

  UD_ERROR_IF(!pDest || !pSource || type != udCT_MiniZ, udR_InvalidParameter_);

  tinfl_init(&decompressor);

  mzErr = udCompTInf_decompress(&decompressor, (const mz_uint8*)pSource, &sourceSize, (mz_uint8*)pDest, (mz_uint8*)pDest, &inflatedSize,
            TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_COMPUTE_ADLER32 | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

  UD_ERROR_IF(mzErr == TINFL_STATUS_NEEDS_MORE_INPUT, udR_InputExhausted);
  UD_ERROR_IF(mzErr == TINFL_STATUS_HAS_MORE_OUTPUT, udR_OutputExhausted);
  UD_ERROR_IF(mzErr != TINFL_STATUS_DONE, udR_CompressionError);

  if (pInflatedSize)
    *pInflatedSize = inflatedSize;
  UD_ERROR_IF(!pInflatedSize && inflatedSize != destSize, udR_InputExhausted);

  result = udR_Success;

epilogue:
  return result;
}




#define WINDOW_BITS (-MZ_DEFAULT_WINDOW_BITS)

static void *MiniZCompressor_Alloc(void *pOpaque, size_t items, size_t size);
static void MiniZCompressor_Free(void *pOpaque, void *address);

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
struct udMiniZCompressor
{
  tdefl_compressor compressor;
  mz_stream *pStream;
};

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZCompressor_Create(udMiniZCompressor **ppCompressor)
{
  if (ppCompressor == nullptr)
  {
    return udR_InvalidParameter_;
  }
  udMiniZCompressor *pCompressor = udAllocType(udMiniZCompressor, 1, udAF_None);

  if (!pCompressor)
  {
    return udR_MemoryAllocationFailure;
  }

  pCompressor->pStream = nullptr;

  *ppCompressor = pCompressor;
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
udResult udMiniZCompressor_InitStream(udMiniZCompressor *pCompressor, void *pDestBuffer, size_t size)
{
  if (!size || size > 0xFFFFFFFFU || pDestBuffer == nullptr)
  {
    return udR_InvalidParameter_;
  }

  int status;
  pCompressor->pStream = udAllocType(mz_stream, 1, udAF_Zero);

  pCompressor->pStream->next_out = (uint8_t*)pDestBuffer;
  pCompressor->pStream->avail_out = (mz_uint32)size;
  pCompressor->pStream->zalloc = MiniZCompressor_Alloc;
  pCompressor->pStream->zfree = MiniZCompressor_Free;
  pCompressor->pStream->opaque = (void*)pCompressor;

  status = udComp_deflateInit2(pCompressor->pStream, MZ_UBER_COMPRESSION, MZ_DEFLATED, WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY);
  if (status != MZ_OK)
  {
    return udR_Failure_;
  }

  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
udResult udMiniZCompressor_DeflateStream(udMiniZCompressor *pCompressor, void *pStream, size_t size, size_t *pCompressedSize)
{
  int status;
  if (!size || size > 0xFFFFFFFFU || pStream == nullptr)
  {
    return udR_InvalidParameter_;
  }

  pCompressor->pStream->next_in = (const uint8_t*)pStream;
  pCompressor->pStream->avail_in = (mz_uint32)size;

  int statusCheck;
  int flush;
  if (pCompressedSize)
  {
    statusCheck = MZ_STREAM_END;
    flush = MZ_FINISH;
  }
  else
  {
    statusCheck = MZ_NO_FLUSH;
    flush = MZ_OK;
  }

  status = udComp_deflate(pCompressor->pStream, flush);
  if (status != statusCheck)
  {
    udComp_deflateEnd(pCompressor->pStream);
    return udR_Failure_;
  }

  if (pCompressedSize)
  {
    *pCompressedSize = pCompressor->pStream->total_out;
    status = udComp_deflateEnd(pCompressor->pStream);
    if (status != MZ_OK)
    {
      return udR_Failure_;
    }
  }
  return udR_Success;
}

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZCompressor_Destroy(udMiniZCompressor **ppCompressor)
{
  if (ppCompressor == nullptr)
  {
    return udR_InvalidParameter_;
  }

  if (*ppCompressor)
  {
    udFree((*ppCompressor)->pStream);
    udFree(*ppCompressor);
  }
  return udR_Success;
}

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZCompressor_Deflate(udMiniZCompressor *pCompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pCompressedSize)
{
  if (pCompressor == nullptr || pDest == nullptr || pSource == nullptr)
  {
    return udR_InvalidParameter_;
  }

  int status;
  mz_stream stream;
  memset(&stream, 0, sizeof(stream));

  if ((sourceLength | destLength) > 0xFFFFFFFFU)
  {
    return udR_InvalidParameter_;
  }

  stream.next_in = (const uint8_t*)pSource;
  stream.avail_in = (mz_uint32)sourceLength;
  stream.next_out = (uint8_t*)pDest;
  stream.avail_out = (mz_uint32)destLength;
  stream.zalloc = MiniZCompressor_Alloc;
  stream.zfree = MiniZCompressor_Free;
  stream.opaque = (void*)pCompressor;

  status = udComp_deflateInit2(&stream, MZ_UBER_COMPRESSION, MZ_DEFLATED, WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY);
  if (status != MZ_OK)
  {
    return udR_Failure_;
  }

  status = udComp_deflate(&stream, MZ_FINISH);
  if (status != MZ_STREAM_END)
  {
    udComp_deflateEnd(&stream);
    return udR_OutputExhausted;
  }

  *pCompressedSize = stream.total_out;
  status = udComp_deflateEnd(&stream);
  if (status != MZ_OK)
  {
    return udR_Failure_;
  }
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, January 2017
size_t udMiniZCompressor_DeflateBounds(udMiniZCompressor *pCompressor, size_t sourceLength)
{
  return (size_t)udComp_deflateBound(pCompressor->pStream, (mz_ulong)sourceLength);
}

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
static void *MiniZCompressor_Alloc(void *pOpaque, size_t IF_UDASSERT(items), size_t IF_UDASSERT(size))
{
  UDASSERT(items * size == (sizeof(udMiniZCompressor) - sizeof(mz_stream*)), "Error allocation for the incorrect size");
  return pOpaque;
}

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
static void MiniZCompressor_Free(void *, void *) {}

// ----------------------------------------------------------------------------
// Author: David Ely, September 2014
struct udMiniZDecompressor
{
  tinfl_decompressor decompressor;
};

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZDecompressor_Create(udMiniZDecompressor **ppDecompressor)
{
  if (ppDecompressor == nullptr)
  {
    return udR_InvalidParameter_;
  }

  udMiniZDecompressor *pDecompressor = udAllocType(udMiniZDecompressor, 1, udAF_None);

  if (!pDecompressor)
  {
    return udR_MemoryAllocationFailure;
  }

  *ppDecompressor = pDecompressor;
  return udR_Success;
}

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZDecompressor_Destroy(udMiniZDecompressor **ppDecompressor)
{
  if (ppDecompressor == nullptr)
  {
    return udR_InvalidParameter_;
  }

  udFree(*ppDecompressor);
  return udR_Success;
}

// ****************************************************************************
// Author: David Ely, September 2014
udResult udMiniZDecompressor_Inflate(udMiniZDecompressor *pDecompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pInflatedSize)
{
  if (pDecompressor == nullptr || pDest == nullptr || pSource == nullptr)
  {
    return udR_InvalidParameter_;
  }

  tinfl_init(&pDecompressor->decompressor);

  mz_uint8* pCurrentDest = (mz_uint8*)pDest;
  tinfl_status status = udCompTInf_decompress(&pDecompressor->decompressor, (const mz_uint8*)pSource, &sourceLength,
                                              (mz_uint8*)pDest, pCurrentDest, &destLength,
                                              TINFL_FLAG_COMPUTE_ADLER32 | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

  if (status != TINFL_STATUS_DONE)
  {
    if (status == TINFL_STATUS_NEEDS_MORE_INPUT)
      return udR_InputExhausted;
    if (status == TINFL_STATUS_HAS_MORE_OUTPUT)
      return udR_OutputExhausted;
    return udR_CompressionError;
  }

  if (pInflatedSize)
  {
    *pInflatedSize = destLength;
  }

  return udR_Success;
}

// ****************************************************************************
// Author: David Ely, September 2014
size_t udMiniZDecompressor_GetStructureSize()
{
  return sizeof(udMiniZDecompressor);
}


#include "udFileHandler.h"

static mz_zip_archive *s_pZip;
struct udFile_MiniZFile : public udFile
{
  int64_t length;
  uint8_t data[1];
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of SeekReadHandler to access a file in the registered zip
static udResult udFileHandler_MiniZSeekRead(udFile *a_pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  udFile_MiniZFile *pFile = static_cast<udFile_MiniZFile *>(a_pFile);
  if (seekOffset >= pFile->length)
    return udR_ReadFailure;
  size_t actualRead = (seekOffset + (int64_t)bufferLength) > pFile->length ? (size_t)(pFile->length - seekOffset) : bufferLength;
  memcpy(pBuffer, pFile->data + seekOffset, actualRead);

  if (pActualRead)
    *pActualRead = actualRead;

  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of CloseHandler to access a file in the registered zip
static udResult udFileHandler_MiniZClose(udFile **ppFile)
{
  if (ppFile == nullptr)
    return udR_InvalidParameter_;
  udFile_MiniZFile *pFile = static_cast<udFile_MiniZFile *>(*ppFile);
  udFree(pFile);
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of OpenHandler to access a file in the registered zip
static udResult udFileHandler_MiniZOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  if (!s_pZip || (flags & udFOF_Write))
    return udR_OpenFailure;
  int index = mz_zip_reader_locate_file(s_pZip, pFilename, nullptr, 0);
  if (index < 0)
    return udR_OpenFailure;
  mz_zip_archive_file_stat stat;
  if (!mz_zip_reader_file_stat(s_pZip, index, &stat))
    return udR_OpenFailure;
  udFile_MiniZFile *pFile = (udFile_MiniZFile *)udAllocFlags(sizeof(udFile_MiniZFile) + (size_t)stat.m_uncomp_size, udAF_Zero);
  if (!pFile)
    return udR_MemoryAllocationFailure;

  if (!mz_zip_reader_extract_to_mem(s_pZip, index, pFile->data, (size_t)stat.m_uncomp_size, 0))
  {
    udFree(pFile);
    return udR_ReadFailure;
  }

  pFile->length = (size_t)stat.m_uncomp_size;
  pFile->fpRead = udFileHandler_MiniZSeekRead;
  pFile->fpClose = udFileHandler_MiniZClose;

  *ppFile = pFile;
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
static void *udMiniZ_Alloc(void * /*opaque*/, size_t items, size_t size) { return udAlloc(items * size); }
static void *udMiniZ_Realloc(void * /*opaque*/, void *address, size_t items, size_t size) { return udRealloc(address, items * size); }
static void udMiniZ_Free(void * /*opaque*/, void *address) { udFree(address); }

// ****************************************************************************
// Author: Dave Pevreal, October 2014
udResult udMiniZ_RegisterMemoryFileHandler(void *pMem, size_t size)
{
  udResult result;

  if (s_pZip)
    return udR_OutstandingReferences;
  s_pZip = udAllocType(mz_zip_archive, 1, udAF_Zero);
  if (!s_pZip)
  {
    result = udR_MemoryAllocationFailure;
    goto epilogue;
  }
  // Assign our internal allocator
  s_pZip->m_pAlloc = udMiniZ_Alloc;
  s_pZip->m_pRealloc = udMiniZ_Realloc;
  s_pZip->m_pFree = udMiniZ_Free;

  if (!mz_zip_reader_init_mem(s_pZip, pMem, size, 0))
  {
    result = udR_Failure_;
    goto epilogue;
  }

  result = udFile_RegisterHandler(udFileHandler_MiniZOpen, "");

epilogue:
  if (result != udR_Success)
    udFree(s_pZip);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, October 2014
udResult udMiniZ_DegisterFileHandler()
{
  if (!s_pZip || udFile_DeregisterHandler(udFileHandler_MiniZOpen))
    return udR_ObjectNotFound;
  mz_zip_reader_end(s_pZip);
  udFree(s_pZip);
  return udR_Success;
}

