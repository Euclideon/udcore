#ifndef UDVALUE_H
#define UDVALUE_H

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udChunkedArray.h"
#include "udMath.h"

/*
 * Expression syntax:
 *
 * Example JSON:
 * {
 *   "Settings": {
 *     "ProjectsPath": "C:/Temp/",
 *     "ImportAtFullScale": true,
 *     "TerrainIndex": 2,
 *     "Inside": { "Count": 5 },
 *     "TestArray": [ 0, 1, 2 ]
 *   }
 * }
 *
 * Equivalent XML:
 * <Settings ProjectsPath="C:/Temp/" ImportAtFullScale="true" TerrainIndex="2">
 *   <Inside Count="5"/>
 *   <TestArray>0</TestArray>
 *   <TestArray>1</TestArray>
 *   <TestArray>2</TestArray>
 * </Settings>
 *
 * Syntax to create:
 *
 * udValue v;
 * v.Set("Settings.ProjectsPath = '%s'", "C:/Temp/");
 * v.Set("Settings.ImportAtFullScale = %s", "true");
 * v.Set("Settings.TerrainIndex = %d", 2);
 * v.Set("Settings.Inside.Count = %d", 5);
 * v.Set("Settings.TestArray[] = 0"); // Append to array
 * v.Set("Settings.TestArray[] = 1"); // Append to array
 * v.Set("Settings.TestArray[2] = 2"); // Out-of-bounds assignment allowed when forming an append operation
 *
 * Syntax for retrieving:
 * v.Parse(json_or_xml_text_string);
 * printf("Projects path is %s\n", v.Get("Settings.ProjectsPath").AsString());
 * printf("Importing at %s scale is %s\n", v.Get("Settings.ImportAtFullScale").AsBool() ? "full" : "smaller");
 * printf("Terrain index is %d\n", v.Get("Settings.TerrainIndex").AsInt());
 * printf("Inside count is %d\n", v.Get("Settings.Inside.Count").AsInt());
 *
 * printf("There are %d items in the array\n", v.Get("Settings.TestArray").ArrayLength());
 *
 * Syntax to explore:
 * for (size_t i = 0; i < v.Get("Settings").MemberCount(); ++i)
 * {
 *   const char *pValueStr = nullptr;
 *   v.Get("Settings.%d", i).ToString(&pValueStr);
 *   udDebugPrintf("%s = %s\n", v.Get("Settings").GetMemberName(i), pValueStr ? pValueStr : "<object or array>");
 *   udFree(pValueStr);
 * }
 *
 * Export:
 * const char *pExportString = nullptr;
 * v.Export(&pExportString, udVEO_JSON | udVEO_StripWhiteSpace); // or udVEO_XML
 * .. write string or whatever ..
 * udFree(pExportString);
 * v.Destroy(); // To destroy object before waiting to go out of scope
 */

enum udValueExportOption { udVEO_JSON = 0, udVEO_XML = 1, udVEO_StripWhiteSpace = 2 };
static inline udValueExportOption operator|(udValueExportOption a, udValueExportOption b) { return (udValueExportOption)(int(a) | int(b)); }

class udValue;
struct udValueKVPair;
typedef udChunkedArray<udValue, 32> udValueArray;
typedef udChunkedArray<udValueKVPair, 32> udValueObject;

class udValue
{
public:
  enum Type
  {
    T_Void = 0,  // Guaranteed to be zero, thus a non-zero type indicates value exists
    T_Bool,
    T_Int64,
    T_Double,
    T_String,
    T_Array,     // A udChunkedArray of values
    T_Object,    // An list of key/value pairs (equiv of JSON object, or XML element attributes)
    T_Count
  };

  static const udValue s_void;
  static const size_t s_udValueTypeSize[T_Count];
  inline udValue();
  inline udValue(int64_t v);
  inline udValue(double v);
  inline void Clear();
  void Destroy();     // Free any memory associated, expects object to be constructed
  inline ~udValue();

  // Set the value
  inline void SetVoid();
  inline void Set(bool v);
  inline void Set(int64_t v);
  inline void Set(double v);

  // Set to a more complex type requiring memory allocation
  udResult SetString(const char *pStr);
  udResult SetArray(); // A dynamic array of udValues, whose types can change per element
  udResult SetObject();

  // Some convenience helpers to create an array of doubles
  udResult Set(const udDouble3 &v);
  udResult Set(const udDouble4 &v);
  udResult Set(const udQuaternion<double> &v);
  udResult Set(const udDouble4x4 &v, bool shrink = false); // If shrink true, 3x3's and 3x4's are detected and stored minimally

  // Accessors
  inline Type GetType() const;
  inline bool IsVoid() const;
  inline bool IsNumeric() const;
  inline bool IsIntegral() const;
  inline bool IsString() const;
  inline bool IsArray() const;
  inline bool IsObject() const;
  inline bool HasMemory() const;
  inline size_t ArrayLength() const;  // Get the length of the array (always 1 for an object)
  inline size_t MemberCount() const;  // Get the number of members for an object (zero for all other types)
  inline const char *GetMemberName(size_t index) const;  // Get the name of a member (null if out of range or not an object)
  inline const udValue *GetMember(size_t index) const;  // Get the member value (null if out of range or not an object)
  const udValue *FindMember(const char *pMemberName, size_t *pIndex = nullptr) const; // Get a member of an object
  bool IsEqualTo(const udValue &other) const;

  // Get the value as a specific type, unless object is udVT_Void in which case defaultValue is returned
  bool AsBool(bool defaultValue = false) const;
  int AsInt(int defaultValue = 0) const;
  int64_t AsInt64(int64_t defaultValue = 0) const;
  float AsFloat(float defaultValue = 0) const;
  double AsDouble(double defaultValue = 0) const;
  const char *AsString(const char *pDefaultValue = nullptr) const; // Returns "true"/"false" for bools, pDefaultValue for any other non-string

  // Some convenience accessors that expect the udValue to be a array of numbers, returning zero/identity on failure
  udDouble3 AsDouble3() const;
  udDouble4 AsDouble4() const;
  udQuaternion<double> AsQuaternion() const;
  udDouble4x4 AsDouble4x4() const;

  inline udValueArray *AsArray() const;
  inline udValueObject *AsObject() const;

  // Create (allocate) a string representing the value, caller is responsible for freeing
  // TODO: Add enum to allow caller to specify JSON or XML escaping
  inline udResult ToString(const char **ppStr, bool escapeBackslashes = false) const;

  // For values of type string, *ppStr is assigned the string before the value is Cleared.
  // The caller now has ownership of the memory and is responsible for calling udFree.
  inline udResult ExtractAndVoid(const char **ppStr);

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

  // Export to a JSON/XML string depending on content
  udResult Export(const char **ppText, udValueExportOption option = udVEO_JSON) const;

  // If pKey is non-null, create a HMAC of the white-space-stripped text (giving a private-key digital signature)
  // If pKey is null, create a SHA256 of the white-space-stripped text (giving a hash for a public key digital signature)
  // This function is a simple helper provided here only to encourage standardisation of how signatures are created/used
  // Remember to remove the existing "signature" attribute before calculating the signature
  // The result is always 32 bytes
  udResult CalculateHMAC(uint8_t hmac[32], size_t hmacLen, const uint8_t *pKey, size_t keyLen) const;

protected:
  typedef udChunkedArray<const char*, 32> LineList;

  udResult ParseJSON(const char *pJSON, int *pCharCount, int *pLineNumber);
  udResult ParseXML(const char *pJSON, int *pCharCount, int *pLineNumber);
  udResult ToString(const char **ppStr, int indent, const char *pPre, const char *pPost, const char *pQuote, int escape) const;
  udResult ExportJSON(const char *pKey, LineList *pLines, int indent, bool strip, bool comma) const;
  udResult ExportXML(const char *pKey, LineList *pLines, int indent, bool strip) const;

  union
  {
    const char *pStr;
    bool bVal;
    int64_t i64Val;
    double dVal;
    udValueArray *pArray;
    udValueObject *pObject;
  } u;
  Type type;
};


struct udValueKVPair
{
  const char *pKey;
  udValue value;
};

#include "udValue_Inl.h"

#endif // UDVALUE_H
