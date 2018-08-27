#include "udPlatform.h"
#include "udCompression.h"
#include "udFileHandler.h"
#include "libdeflate.h"

// ****************************************************************************
// Author: Dave Pevreal, November 2017
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type)
{
  udResult result;
  size_t destSize;
  void *pTemp = nullptr;
  struct libdeflate_compressor *ldComp = nullptr;

  UD_ERROR_IF(!ppDest || !pDestSize || !pSource, udR_InvalidParameter_);
  switch (type)
  {
    case udCT_None:
      // Handle the special case of no compression, using udMemDup
      *ppDest = udMemDup(pSource, sourceSize, 0, udAF_None);
      if (pDestSize)
        *pDestSize = sourceSize;
      break;

    case udCT_RawDeflate:
      ldComp = libdeflate_alloc_compressor(6);
      UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

      destSize = libdeflate_deflate_compress_bound(ldComp, sourceSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);
      pTemp = udAlloc(destSize);
      UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

      destSize = libdeflate_deflate_compress(ldComp, pSource, sourceSize, pTemp, destSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);

      // Size the allocation as required
      *pDestSize = destSize;
      *ppDest = udRealloc(pTemp, destSize);
      UD_ERROR_NULL(*ppDest, udR_MemoryAllocationFailure);
      pTemp = nullptr; // Prevent freeing on successful realloc
      break;

    case udCT_ZlibDeflate:
      ldComp = libdeflate_alloc_compressor(6);
      UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

      destSize = libdeflate_zlib_compress_bound(ldComp, sourceSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);
      pTemp = udAlloc(destSize);
      UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

      destSize = libdeflate_zlib_compress(ldComp, pSource, sourceSize, pTemp, destSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);

      // Size the allocation as required
      *pDestSize = destSize;
      *ppDest = udRealloc(pTemp, destSize);
      UD_ERROR_NULL(*ppDest, udR_MemoryAllocationFailure);
      pTemp = nullptr; // Prevent freeing on successful realloc
      break;

    case udCT_GzipDeflate:
      ldComp = libdeflate_alloc_compressor(6);
      UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

      destSize = libdeflate_gzip_compress_bound(ldComp, sourceSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);
      pTemp = udAlloc(destSize);
      UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

      destSize = libdeflate_gzip_compress(ldComp, pSource, sourceSize, pTemp, destSize);
      UD_ERROR_IF(destSize == 0, udR_CompressionError);

      // Size the allocation as required
      *pDestSize = destSize;
      *ppDest = udRealloc(pTemp, destSize);
      UD_ERROR_NULL(*ppDest, udR_MemoryAllocationFailure);
      pTemp = nullptr; // Prevent freeing on successful realloc
      break;

    default:
      UD_ERROR_SET(udR_InvalidParameter_);
  }

  result = udR_Success;

epilogue:
  if (ldComp)
    libdeflate_free_compressor(ldComp);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2017
udResult udCompression_Inflate(void *pDest, size_t destSize, const void *pSource, size_t sourceSize, size_t *pInflatedSize, udCompressionType type)
{
  udResult result;
  size_t inflatedSize;
  void *pTemp = nullptr;
  struct libdeflate_decompressor *ldComp = nullptr;
  libdeflate_result lresult;

  UD_ERROR_IF(!pDest || !pSource, udR_InvalidParameter_);
  switch (type)
  {
  case udCT_None:
    // Handle the special case of no compression, using udMemDup
    memcpy(pDest, pSource, sourceSize);
    if (pInflatedSize)
      *pInflatedSize = sourceSize;
    break;

  case udCT_RawDeflate:
    ldComp = libdeflate_alloc_decompressor();
    UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

    pTemp = (pDest == pSource) ? udAlloc(destSize) : pDest;
    UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

    lresult = libdeflate_deflate_decompress(ldComp, pSource, sourceSize, pTemp, destSize, &inflatedSize);
    UD_ERROR_IF(lresult == LIBDEFLATE_INSUFFICIENT_SPACE, udR_BufferTooSmall);
    UD_ERROR_IF(lresult != LIBDEFLATE_SUCCESS, udR_CompressionError);

    if (pInflatedSize)
      *pInflatedSize = inflatedSize;
    if (pTemp != pDest)
      memcpy(pDest, pTemp, inflatedSize);
    break;

  case udCT_ZlibDeflate:
    ldComp = libdeflate_alloc_decompressor();
    UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

    pTemp = (pDest == pSource) ? udAlloc(destSize) : pDest;
    UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

    lresult = libdeflate_zlib_decompress(ldComp, pSource, sourceSize, pTemp, destSize, &inflatedSize);
    UD_ERROR_IF(lresult == LIBDEFLATE_INSUFFICIENT_SPACE, udR_BufferTooSmall);
    UD_ERROR_IF(lresult != LIBDEFLATE_SUCCESS, udR_CompressionError);

    if (pInflatedSize)
      *pInflatedSize = inflatedSize;
    if (pTemp != pDest)
      memcpy(pDest, pTemp, inflatedSize);
    break;

  case udCT_GzipDeflate:
    ldComp = libdeflate_alloc_decompressor();
    UD_ERROR_NULL(ldComp, udR_MemoryAllocationFailure);

    pTemp = (pDest == pSource) ? udAlloc(destSize) : pDest;
    UD_ERROR_NULL(pTemp, udR_MemoryAllocationFailure);

    lresult = libdeflate_gzip_decompress(ldComp, pSource, sourceSize, pTemp, destSize, &inflatedSize);
    UD_ERROR_IF(lresult == LIBDEFLATE_INSUFFICIENT_SPACE, udR_BufferTooSmall);
    UD_ERROR_IF(lresult != LIBDEFLATE_SUCCESS, udR_CompressionError);

    if (pInflatedSize)
      *pInflatedSize = inflatedSize;
    if (pTemp != pDest)
      memcpy(pDest, pTemp, inflatedSize);
    break;

  default:
    UD_ERROR_SET(udR_InvalidParameter_);
  }

  result = udR_Success;

epilogue:
  if (pTemp && pTemp != pDest)
    udFree(pTemp);
  if (ldComp)
    libdeflate_free_decompressor(ldComp);

  return result;
}

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
//#define MINIZ_NO_MALLOC Removed because the PNG creator requires malloc
#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable:4334)
#else
# pragma GCC diagnostic push
# if __GNUC__ >= 6 && !defined(__clang_major__)
#  pragma GCC diagnostic ignored "-Wmisleading-indentation"
# endif
#endif
#include "miniz/miniz.c"
#if defined(_MSC_VER)
# pragma warning(pop)
#else
# pragma GCC diagnostic pop
#endif

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
udResult udCompression_RegisterMemoryZipHandler(void *pMem, size_t size)
{
  udResult result;

  UD_ERROR_IF(s_pZip, udR_OutstandingReferences);

  s_pZip = udAllocType(mz_zip_archive, 1, udAF_Zero);
  UD_ERROR_NULL(s_pZip, udR_MemoryAllocationFailure);

  // Assign our internal allocator
  s_pZip->m_pAlloc = udMiniZ_Alloc;
  s_pZip->m_pRealloc = udMiniZ_Realloc;
  s_pZip->m_pFree = udMiniZ_Free;

  UD_ERROR_IF(!mz_zip_reader_init_mem(s_pZip, pMem, size, 0), udR_Failure_);

  result = udFile_RegisterHandler(udFileHandler_MiniZOpen, "");

epilogue:
  if (result != udR_Success)
    udFree(s_pZip);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, October 2014
udResult udCompression_DegisterMemoryZipHandler()
{
  if (!s_pZip || udFile_DeregisterHandler(udFileHandler_MiniZOpen))
    return udR_ObjectNotFound;
  mz_zip_reader_end(s_pZip);
  udFree(s_pZip);
  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2018
udResult udCompression_CreatePNG(void **ppPNG, size_t *pPNGLen, const uint8_t *pImage, int width, int height, int channels)
{
  udResult result;
  void *pPNG = nullptr;

  UD_ERROR_NULL(ppPNG, udR_InvalidParameter_);
  UD_ERROR_NULL(pPNGLen, udR_InvalidParameter_);
  UD_ERROR_NULL(pImage, udR_InvalidParameter_);
  UD_ERROR_IF(width <= 0 || height <= 0, udR_InvalidParameter_);
  UD_ERROR_IF(channels < 3 || channels > 4, udR_InvalidParameter_);

  pPNG = tdefl_write_image_to_png_file_in_memory((const void *)pImage, width, height, channels, pPNGLen);
  UD_ERROR_NULL(pPNG, udR_InvalidConfiguration); // Something went wrong, but we don't know what

  // Unfortunately the PNG writer doesn't support custom memory allocators, so to allow
  // the caller to free with regular udFree we must duplicate the allocation.
  *ppPNG = udMemDup(pPNG, *pPNGLen, 0, udAF_None);
  UD_ERROR_NULL(*ppPNG, udR_MemoryAllocationFailure);

  result = udR_Success;

epilogue:
  if (pPNG)
    MZ_FREE(pPNG);
  return result;
}
