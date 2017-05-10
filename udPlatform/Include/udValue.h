#ifndef UDVALUE_H
#define UDVALUE_H

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"
#include "udMath.h"

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
  inline udValue(int64_t v);
  inline udValue(double v);
  inline void Clear();
  void Destroy();     // Free any memory associated, expects object to be constructed
  inline ~udValue();

  // Set the value
  inline void SetVoid();
  inline void Set(int64_t v);
  inline void Set(double v);

  // Set to a more complex type requiring memory allocation
  udResult SetString(const char *pStr);
  udResult SetList(); // A dynamic list of udValues, whose types can change per element
  udResult SetObject();
  udResult SetElement();

  // Some convenience helpers to create an list of doubles
  udResult Set(const udDouble3 &v);
  udResult Set(const udDouble4 &v);
  udResult Set(const udQuaternion<double> &v);
  udResult Set(const udDouble4x4 &v, bool shrink = false); // If shrink true, 3x3's and 3x4's are detected and stored minimally

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
  bool IsEqualTo(const udValue &other) const;

  // Get the value as a specific type, unless object is udVT_Void in which case defaultValue is returned
  bool AsBool(bool defaultValue = false) const;
  int AsInt(int defaultValue = 0) const;
  int64_t AsInt64(int64_t defaultValue = 0) const;
  double AsDouble(double defaultValue = 0) const;
  const char *AsString(char *pDefaultValue = nullptr) const;

  // Some convenience accessors that expect the udValue to be a list of numbers, returning zero/identity on failure
  udDouble3 AsDouble3() const;
  udDouble4 AsDouble4() const;
  udQuaternion<double> AsQuaternion() const;
  udDouble4x4 AsDouble4x4() const;

  inline udValueList *AsList() const;
  inline udValueObject *AsObject() const;
  inline udValueElement *AsElement() const;

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
    bool bVal;
    int64_t i64Val;
    double dVal;
    udValueList *pList;
    udValueObject *pObject;
    udValueElement *pElement;
  } u;
  udValueType type;
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
