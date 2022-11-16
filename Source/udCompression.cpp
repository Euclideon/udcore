#include "udCompression.h"
#include "libdeflate.h"
#include "udFileHandler.h"
#include "udMath.h"
#include "udStringUtil.h"
#include <atomic>

// ****************************************************************************
// Author: Dave Pevreal, August 2018
const char *udCompressionTypeAsString(udCompressionType type)
{
  switch (type)
  {
    case udCT_None:
      return "None";
    case udCT_RawDeflate:
      return "RawDeflate";
    case udCT_ZlibDeflate:
      return "ZlibDeflate";
    case udCT_GzipDeflate:
      return "GzipDeflate";
    default:
      return nullptr;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, November 2017
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type)
{
  udResult result;
  size_t destSize;
  void *pTemp = nullptr;
  struct libdeflate_compressor *ldComp = nullptr;

  UD_ERROR_IF(!ppDest || !pDestSize || !pSource, udR_InvalidParameter);
  if (!sourceSize)
  {
    // Special-case, when compressing zero bytes, result is zero bytes
    *ppDest = nullptr;
    *pDestSize = 0;
    UD_ERROR_SET(udR_Success);
  }
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
      UD_ERROR_SET(udR_InvalidParameter);
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

  UD_ERROR_IF(!pDest || !pSource, udR_InvalidParameter);
  if (!sourceSize)
  {
    // Special-case, when decompressing zero bytes, result is zero bytes
    if (pInflatedSize)
      *pInflatedSize = 0;
    UD_ERROR_SET(udR_Success);
  }
  switch (type)
  {
    case udCT_None:
      // Handle the special case of no compression
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
      if (lresult == LIBDEFLATE_INSUFFICIENT_SPACE)
        UD_ERROR_SET_NO_BREAK(udR_BufferTooSmall);
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
      if (lresult == LIBDEFLATE_INSUFFICIENT_SPACE)
        UD_ERROR_SET_NO_BREAK(udR_BufferTooSmall);
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
      if (lresult == LIBDEFLATE_INSUFFICIENT_SPACE)
        UD_ERROR_SET_NO_BREAK(udR_BufferTooSmall);
      UD_ERROR_IF(lresult != LIBDEFLATE_SUCCESS, udR_CompressionError);

      if (pInflatedSize)
        *pInflatedSize = inflatedSize;
      if (pTemp != pDest)
        memcpy(pDest, pTemp, inflatedSize);
      break;

    default:
      UD_ERROR_SET(udR_InvalidParameter);
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
#define mz_adler32                                 udComp_adler32
#define mz_crc32                                   udComp_crc32
#define mz_free                                    udComp_free
#define mz_version                                 udComp_version
#define mz_deflateEnd                              udComp_deflateEnd
#define mz_deflateBound                            udComp_deflateBound
#define mz_compressBound                           udComp_compressBound
#define mz_inflateInit2                            udComp_inflateInit2
#define mz_inflateInit                             udComp_inflateInit
#define mz_inflateEnd                              udComp_inflateEnd
#define mz_error                                   udComp_error
#define tinfl_decompress                           udCompTInf_decompress
#define tinfl_decompress_mem_to_heap               udCompTInf_decompress_mem_to_heap
#define tinfl_decompress_mem_to_mem                udCompTInf_decompress_mem_to_mem
#define tinfl_decompress_mem_to_callback           udCompTInf_decompress_mem_to_callback
#define tdefl_compress                             udCompTDefl_compress
#define tdefl_compress_buffer                      udCompTDefl_compress_buffer
#define tdefl_init                                 udCompTDefl_init
#define tdefl_get_prev_return_status               udCompTDefl_get_prev_return_status
#define tdefl_get_adler32                          udCompTDefl_get_adler32
#define tdefl_compress_mem_to_output               udCompTDefl_compress_mem_to_output
#define tdefl_compress_mem_to_heap                 udCompTDefl_compress_mem_to_heap
#define tdefl_compress_mem_to_mem                  udCompTDefl_compress_mem_to_mem
#define tdefl_create_comp_flags_from_zip_params    udCompTDefl_create_comp_flags_from_zip_params
#define tdefl_write_image_to_png_file_in_memory_ex udCompTDefl_write_image_to_png_file_in_memory_ex
#define tdefl_write_image_to_png_file_in_memory    udCompTDefl_write_image_to_png_file_in_memory
#define mz_deflateInit2                            udComp_deflateInit2
#define mz_deflateReset                            udComp_deflateReset
#define mz_deflate                                 udComp_deflate
#define mz_inflate                                 udComp_inflate
#define mz_uncompress                              udComp_uncompress
#define mz_deflateInit                             udComp_deflateInit
#define mz_compress2                               udComp_compress2
#define mz_compress                                udComp_compress

#define mz_zip_writer_init_from_reader udComp_zip_writer_init_from_reader
#define mz_zip_reader_end              udComp_mz_zip_reader_end
#define mz_zip_reader_init_mem         udComp_mz_zip_reader_init_mem
#define mz_zip_reader_locate_file      udComp_mz_zip_reader_locate_file
#define mz_zip_reader_file_stat        udComp_mz_zip_reader_file_stat

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
//#define MINIZ_NO_MALLOC Removed because the PNG creator requires malloc
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4334)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#  if __GNUC__ >= 6 && !defined(__clang_major__)
#    pragma GCC diagnostic ignored "-Wmisleading-indentation"
#  endif
#endif
#include "miniz/miniz.c"
#if defined(_MSC_VER)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif

struct udFile_Zip : public udFile
{
  mz_zip_archive mz;
  udFile *volatile pZipFile;
  uint8_t *pFileData;
  int index; // Index within the zip of the current file
  std::atomic<int32_t> lengthRead;
  std::atomic<bool> readComplete;
  std::atomic<bool> abortRead; // Set to true and wait for readComplete
  udRWLock *pRWLock;
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, November 2019
// Helper to wait for reads to abort
static void AbortRead(udFile_Zip *pZip)
{
  while (pZip->pFileData && !pZip->readComplete)
  {
    udDebugPrintf("Waiting for read of zip to abort\n");
    pZip->abortRead = true;
    udSleep(1);
  }
  if (pZip->pFileData)
  {
    udReadLockRWLock(pZip->pRWLock);
    udFree(pZip->pFileData);
    udReadUnlockRWLock(pZip->pRWLock);
  }
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of SeekReadHandler to access a file in the registered zip
static udResult udFileHandler_MiniZSeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udResult result;
  udFile_Zip *pZip = static_cast<udFile_Zip *>(pFile);
  size_t actualRead = 0;
  bool locked = false;

  UD_ERROR_NULL(pZip->pZipFile, udR_InvalidConfiguration);
  if (pZip->pFileData)
  {
    UD_ERROR_IF(seekOffset < 0 || seekOffset >= pZip->fileLength, udR_InvalidParameter);
    bufferLength = udMin(bufferLength, (size_t)pZip->fileLength - (size_t)seekOffset);

    // Passive wait for the read to complete
    while (!pZip->readComplete && pZip->lengthRead < int32_t(seekOffset + bufferLength))
    {
      if (pZip->abortRead)
        UD_ERROR_SET_NO_BREAK(udR_ReadFailure);
      udSleep(1);
    }
    UD_ERROR_IF(int64_t(pZip->lengthRead) <= seekOffset, udR_ReadFailure);

    actualRead = udMin(bufferLength, pZip->lengthRead - (size_t)seekOffset);
    udReadLockRWLock(pZip->pRWLock);
    locked = true;
    UD_ERROR_NULL(pZip->pFileData, udR_ReadFailure);
    memcpy(pBuffer, pZip->pFileData + seekOffset, actualRead);

    result = udR_Success;
  }
  else
  {
    result = udFile_Read(pZip->pZipFile, pBuffer, bufferLength, seekOffset, udFSW_SeekSet, &actualRead);
  }

epilogue:
  if (locked)
    udReadUnlockRWLock(pZip->pRWLock);

  if (pActualRead)
    *pActualRead = actualRead;
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of CloseHandler to access a file in the registered zip
static udResult udFileHandler_MiniZClose(udFile **ppFile)
{
  if (ppFile == nullptr)
    return udR_InvalidParameter;
  udFile_Zip *pZip = static_cast<udFile_Zip *>(*ppFile);
  if (pZip)
  {
    AbortRead(pZip);
    udFile *pZipFile = pZip->pZipFile;
    if (pZipFile && udInterlockedCompareExchangePointer((void **)&pZip->pZipFile, nullptr, pZipFile) == pZipFile)
      udFile_Close(&pZipFile);
    udDestroyRWLock(&pZip->pRWLock);
    mz_zip_reader_end(&pZip->mz);
    udFree(pZip);
  }
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
static void *udMiniZ_Alloc(void * /*pOpaque*/, size_t items, size_t size) { return udAlloc(items * size); }
static void *udMiniZ_Realloc(void * /*pOpaque*/, void *address, size_t items, size_t size) { return udRealloc(address, items * size); }
static void udMiniZ_Free(void * /*pOpaque*/, void *address) { udFree(address); }
static size_t udMiniZ_Read(void *pOpaque, mz_uint64 fileOffset, void *pBuf, size_t n)
{
  udFile_Read(((udFile_Zip *)pOpaque)->pZipFile, pBuf, n, fileOffset, udFSW_SeekSet, &n);
  return n;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2018
static size_t udMiniZ_ReadFileFromZipCallback(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n)
{
  udFile_Zip *pFile = (udFile_Zip *)pOpaque;
  if (pFile->abortRead)
    return 0;
  // Only handling the case of sequential feeding of data
  if (file_ofs != (mz_uint64)pFile->lengthRead)
    return 0;
  // Detect an overrun
  if ((file_ofs + n) > (mz_uint64)pFile->fileLength)
    return 0;
  udWriteLockRWLock(pFile->pRWLock);
  if (pFile->pFileData)
    memcpy(pFile->pFileData + file_ofs, pBuf, n);
  udWriteUnlockRWLock(pFile->pRWLock);
  pFile->lengthRead += (int32_t)n;
  return n;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, November 2019
// Special API to access individual subfiles of a zip without re-opening
udResult udFileHandler_MiniZSetSubFilename(udFile *pFile, const char *pSubFilename)
{
  udResult result;
  udFile_Zip *pZip = (udFile_Zip *)pFile;
  mz_zip_archive_file_stat stat;

  UD_ERROR_IF(pZip->fpRead != udFileHandler_MiniZSeekRead, udR_ObjectTypeMismatch);
  // First tidy up any existing sub file data, waiting for pending read if necessary
  AbortRead(pZip);
  pZip->fileLength = 0;
  UD_ERROR_NULL(pSubFilename, udR_Success); // Legal to "unset" the sub filename

  pZip->index = mz_zip_reader_locate_file(&pZip->mz, pSubFilename, nullptr, 0);
  if (pZip->index < 0 && udStrchr(pSubFilename, "/\\"))
  {
    // Sometimes the zip can be created on a different platform that uses different separators,
    // so before giving up attempt to locate using both kinds of separators
    // We assume the file in the zip uses separators consistently, but not necessarily in pSubFilename
    udStrcpy(stat.m_filename, pSubFilename); // Borrow stat.filename memory as it's the right size
    size_t sepIndex;
    // Try linux separators
    while (udStrchr(stat.m_filename, "\\", &sepIndex))
      stat.m_filename[sepIndex] = '/';
    pZip->index = mz_zip_reader_locate_file(&pZip->mz, stat.m_filename, nullptr, 0);
    if (pZip->index < 0)
    {
      // Try windows separators
      while (udStrchr(stat.m_filename, "/", &sepIndex))
        stat.m_filename[sepIndex] = '\\';
      pZip->index = mz_zip_reader_locate_file(&pZip->mz, stat.m_filename, nullptr, 0);
    }
  }
  UD_ERROR_IF(pZip->index < 0, udR_OpenFailure);
  UD_ERROR_IF(!mz_zip_reader_file_stat(&pZip->mz, pZip->index, &stat), udR_OpenFailure);
  pZip->fileLength = (int64_t)stat.m_uncomp_size;

  if (stat.m_method == 0)
  {
    // The file in the zip is just stored, so instead of going through the extraction
    // machinery, we can use the SeekBase machinery of udFile to auto-offset
    int64_t seekBase = (int64_t)stat.m_local_header_ofs;
    uint8_t localDirHeader[MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
    UD_ERROR_CHECK(udFile_Read(pZip->pZipFile, localDirHeader, sizeof(localDirHeader), seekBase, udFSW_SeekSet));
    uint32_t sig;
    uint16_t filenameLen;
    uint16_t extraLen;
    memcpy(&sig, localDirHeader + 0, sizeof(sig));
    memcpy(&filenameLen, localDirHeader + MZ_ZIP_LDH_FILENAME_LEN_OFS, sizeof(filenameLen));
    memcpy(&extraLen, localDirHeader + MZ_ZIP_LDH_EXTRA_LEN_OFS, sizeof(extraLen));
    UD_ERROR_IF(sig != MZ_ZIP_LOCAL_DIR_HEADER_SIG, udR_CorruptData);
    seekBase += MZ_ZIP_LOCAL_DIR_HEADER_SIZE + filenameLen + extraLen;

    pZip->filePos = pZip->seekBase = seekBase;
    pZip->readComplete = true;
  }
  else
  {
    // File is compressed, so allocate memory and begin the decompression on a thread
    pZip->pFileData = udAllocType(uint8_t, (size_t)stat.m_uncomp_size, udAF_None);
    UD_ERROR_NULL(pZip->pFileData, udR_MemoryAllocationFailure);
    pZip->filePos = 0;
    pZip->lengthRead = 0;
    pZip->readComplete = false;

    udThreadStart startFunc = [](void *pOpaque) -> unsigned int
    {
      udFile_Zip *pZip = (udFile_Zip *)pOpaque;
      mz_zip_reader_extract_to_callback(&pZip->mz, pZip->index, udMiniZ_ReadFileFromZipCallback, pOpaque, 0);
      pZip->readComplete = true; // If an error occured, lengthRead won't equal fileLength
      return 0;
    };
    udThread_Create(nullptr, startFunc, pZip);
  }
  result = udR_Success;

epilogue:
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, October 2014
// Implementation of OpenHandler to access a file in the registered zip
udResult udFileHandler_MiniZOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udResult result;
  udFile_Zip *pFile = nullptr;
  char *pSubFilename = nullptr;
  char *pZipName = nullptr;
  const char *pFolderDelim = nullptr;
  int64_t zipLen;

  UD_ERROR_IF(flags & udFOF_Write, udR_OpenFailure);

  pFile = udAllocType(udFile_Zip, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);
  pFile->pRWLock = udCreateRWLock();
  UD_ERROR_NULL(pFile->pRWLock, udR_MemoryAllocationFailure);

  pFile->fpSetSubFilename = udFileHandler_MiniZSetSubFilename;
  pFile->fpRead = udFileHandler_MiniZSeekRead;
  pFile->fpClose = udFileHandler_MiniZClose;
  pFile->readComplete = true;

  pFile->mz.m_pIO_opaque = pFile;
  pFile->mz.m_pAlloc = udMiniZ_Alloc;
  pFile->mz.m_pRealloc = udMiniZ_Realloc;
  pFile->mz.m_pFree = udMiniZ_Free;
  pFile->mz.m_pRead = udMiniZ_Read;

  // Need to extract just the zip filename
  pZipName = udStrdup(pFilename + 6); // Skip zip://
  // Find a colon, but importantly, AFTER a folder delimiter if one exists (to exclude drive letters / protocols such as raw://)
  pFolderDelim = udStrchr(pZipName, "/\\");
  pSubFilename = (char *)udStrrchr(pFolderDelim ? pFolderDelim : pZipName, ":");
  if (pSubFilename)
    *pSubFilename++ = 0; // Skip and null the colon

  // Now open the underlying zip file
  UD_ERROR_CHECK(udFile_Open((udFile **)&pFile->pZipFile, pZipName, udFOF_Read, &zipLen));

  // Initialise the zip reader
  UD_ERROR_IF(!mz_zip_reader_init(&pFile->mz, (mz_uint64)zipLen, 0), udR_OpenFailure);

  if (!pSubFilename)
  {
    // No sub-filename was specified, so read the TOC and return that as the file
    mz_zip_archive_file_stat stat;
    int fileCount = mz_zip_reader_get_num_files(&pFile->mz);
    size_t tocSize = 1; // final null terminator

    for (int i = 0; i < fileCount; ++i)
    {
      if (!mz_zip_reader_is_file_a_directory(&pFile->mz, i))
      {
        mz_zip_reader_file_stat(&pFile->mz, i, &stat);
        tocSize += udStrlen(stat.m_filename) + 1; // Add 1 for newline
      }
    }
    pFile->fileLength = (int64_t)tocSize;
    pFile->pFileData = udAllocType(uint8_t, tocSize, udAF_None);
    UD_ERROR_NULL(pFile->pFileData, udR_MemoryAllocationFailure);
    tocSize = 0;
    for (int i = 0; i < fileCount; ++i)
    {
      if (!mz_zip_reader_is_file_a_directory(&pFile->mz, i))
      {
        mz_zip_reader_file_stat(&pFile->mz, i, &stat);
        size_t len = udStrlen(stat.m_filename);
        memcpy(pFile->pFileData + tocSize, stat.m_filename, len);
        tocSize += len;
        pFile->pFileData[tocSize++] = '\n';
      }
    }
    pFile->pFileData[tocSize++] = '\0';
    pFile->lengthRead = (int32_t)pFile->fileLength;
  }
  else if (*pSubFilename) // If the sub filename is not an empty string, assign it
  {
    UD_ERROR_CHECK(pFile->fpSetSubFilename(pFile, pSubFilename));
  }

  result = udR_Success;
  *ppFile = pFile;
  pFile = nullptr;

epilogue:
  if (pFile)
  {
    mz_zip_reader_end(&pFile->mz);
    udFileHandler_MiniZClose((udFile **)&pFile);
  }
  udFree(pZipName);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2018
udResult udCompression_CreatePNG(void **ppPNG, size_t *pPNGLen, const uint8_t *pImage, int width, int height, int channels)
{
  udResult result;
  void *pPNG = nullptr;

  UD_ERROR_NULL(ppPNG, udR_InvalidParameter);
  UD_ERROR_NULL(pPNGLen, udR_InvalidParameter);
  UD_ERROR_NULL(pImage, udR_InvalidParameter);
  UD_ERROR_IF(width <= 0 || height <= 0, udR_InvalidParameter);
  UD_ERROR_IF(channels < 3 || channels > 4, udR_InvalidParameter);

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
