#ifndef UDVALUE_H
#define UDVALUE_H

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"

enum udValueType
{
  udVT_Void = 0,      // Guaranteed to be zero, thus a non-zero type indicates value exists
  udVT_Bool,
  udVT_Uint8,
  udVT_Int32,
  udVT_Int64,
  udVT_Float,
  udVT_Double,
  udVT_String,
  udVT_List,    // A udChunkedArray of values
  udVT_Object,  // JSON object
  udVT_Element, // XML Element (which inherits from Object to provide attributes)

  udVT_Count
};

class udValue;
typedef udChunkedArray<udValue, 32> udValueList;
struct udValueObject;
struct udValueElement;

class udValue
{
public:
  static const udValue s_void;
  static const size_t s_udValueTypeSize[udVT_Count];
  inline udValue();
  inline udValue(uint8_t v);
  inline udValue(int32_t v);
  inline udValue(int64_t v);
  inline udValue(float v);
  inline udValue(double v);
  inline void Clear();
  void Destroy();     // Free any memory associated, expects object to be constructed
  inline ~udValue();

  // Set the value
  inline void SetVoid();
  inline void Set(uint8_t v);
  inline void Set(int32_t v);
  inline void Set(int64_t v);
  inline void Set(float v);
  inline void Set(double v);

  udResult SetString(const char *pStr);
  udResult SetList(); // A dynamic list of udValues, whose types can change per element
  udResult SetArray(udValueType arrayType, size_t length); // Only plain types are allowed, no strings/lists/kvstores
  udResult SetObject();
  udResult SetElement();

  // Accessors
  inline bool IsVoid() const;
  inline bool IsNumeric() const;
  inline bool IsIntegral() const;
  inline bool IsString() const;
  inline bool IsList() const;
  inline bool IsObject() const;
  inline bool IsElement() const;
  inline bool HasMemory() const;
  inline size_t ListLength() const;
  inline size_t ArrayLength() const;
  bool IsEqualTo(const udValue &other) const;


  // Array access
  inline bool *GetArrayBool(size_t *pLength = nullptr);
  inline const bool *GetArrayBool(size_t *pLength = nullptr) const;
  inline uint8_t *GetArrayUint8(size_t *pLength = nullptr);
  inline const uint8_t *GetArrayUint8(size_t *pLength = nullptr) const;
  inline int32_t *GetArrayInt32(size_t *pLength = nullptr);
  inline const int32_t *GetArrayInt32(size_t *pLength = nullptr) const;
  inline int64_t *GetArrayInt64(size_t *pLength = nullptr);
  inline const int64_t *GetArrayInt64(size_t *pLength = nullptr) const;
  inline float *GetArrayFloat(size_t *pLength = nullptr);
  inline const float *GetArrayFloat(size_t *pLength = nullptr) const;
  inline double *GetArrayDouble(size_t *pLength = nullptr);
  inline const double *GetArrayDouble(size_t *pLength = nullptr) const;
  inline char **GetArrayString(size_t *pLength = nullptr);
  inline char *const *GetArrayString(size_t *pLength = nullptr) const;

  // Get the value as a specific type, unless object is udVT_Void in which case defaultValue is returned
  bool AsBool(bool defaultValue = false) const;
  uint8_t AsUint8(uint8_t defaultValue = 0) const;
  int32_t AsInt32(int32_t defaultValue = 0) const;
  int64_t AsInt64(int64_t defaultValue = 0) const;
  float AsFloat(float defaultValue = 0) const;
  double AsDouble(double defaultValue = 0) const;
  char *AsString(char *pDefaultValue = nullptr) const;
  inline udValueList *AsList() const;
  inline udValueObject *AsObject() const;
  inline udValueElement *AsElement() const;

  // Convert a list of elements to an array of a specified type, can fail if an element can't be converted
  udResult ConvertListToArray(udValueType toType);

  // Get a pointer to a key's value, this pointer is valid as long as the key remains, ppValue may be null if just testing existence
  // Allowed operators are . and [] to dereference (eg "instances[%d].%s", (int)instanceIndex, (char*)pInstanceKeyName)
  udResult Get(udValue **ppValue, const char *pKeyExpression, ...);
  const udValue &Get(const char *pKeyExpression, ...) const;

  // Set a new key/value pair to the store, overwriting a existing key, using = operator in the expression
  // Allowed operators are . and [] to dereference and for version without pValue, = to assign value (eg "obj.value = %d", 5)
  // Set with null for pValue or nothing following the = will remove the key
  udResult Set(udValue *pValue, const char *pKeyExpression, ...);
  udResult Set(const char *pKeyExpression, ...);

  // Parse a string an assign the type/value, supporting string, integer and float/double, JSON or XML
  udResult Parse(const char *pString, int *pCharCount = nullptr, int *pLineNumber = nullptr);

  enum ExportOption { EO_None, EO_StripWhiteSpace = 1 };
  // Export to a JSON/XML string depending on content
  udResult Export(const char **ppText, ExportOption option = EO_None) const;

  // Create a HMAC of the white-space-stripped text (giving a private-key digital signature)
  // This function is a simple helper provided here only to encourage standardisation of how signatures are created/used
  // Remember to remove the existing "signature" attribute before calculating the signature
  udResult CalculateHMAC(uint8_t hmac[32], size_t hmacLen, const uint8_t *pKey, size_t keyLen) const;

protected:
  typedef udChunkedArray<const char*, 32> LineList;
  enum ExportValueOptions { EVO_None = 0, EVO_StripWhiteSpace=1, EVO_XML = 2 };

  udResult ParseJSON(const char *pJSON, int *pCharCount, int *pLineNumber);
  udResult ParseXML(const char *pJSON, int *pCharCount, int *pLineNumber);
  udResult ExportValue(const char *pKey, LineList *pLines, int indent, ExportValueOptions options, bool comma) const;

  union
  {
    char *pStr;
    int32_t i32Val;
    bool bVal;
    uint8_t u8Val;
    int64_t i64Val;
    float fVal;
    double dVal;
    udValueList *pList;
    udValueObject *pObject;
    udValueElement *pElement;
    void *pArray;
  } u;
  udValueType type;
  int32_t arrayLength;

};

struct udValueObject
{
  struct KVPair
  {
    const char *pKey;
    udValue value;
  };
  udChunkedArray<KVPair, 32> attributes;
};

struct udValueElement : public udValueObject
{
  const char *pName;
  udValueList children;
};

#include "udValue_Inl.h"
#endif // UDVALUE_H
