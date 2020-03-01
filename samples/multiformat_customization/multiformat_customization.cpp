#include <iostream>
#include "bitserializer/bit_serializer.h"
#include "bitserializer_rapidjson/rapidjson_archive.h"
#include "bitserializer_pugixml/pugixml_archive.h"

using namespace BitSerializer::Xml::PugiXml;
using namespace BitSerializer::Json::RapidJson;
using namespace BitSerializer;

class CPoint
{
public:
	CPoint(int x, int y)
		: x(x), y(y)
	{ }

	template <class TArchive>
	void Serialize(TArchive& archive)
	{
		// Serialize as attributes when archive type is XML
		if constexpr (TArchive::archive_type == ArchiveType::Xml)
		{
			archive << MakeAutoAttributeValue("x", x);
			archive << MakeAutoAttributeValue("y", y);
		}
		else
		{
			archive << MakeAutoKeyValue("x", x);
			archive << MakeAutoKeyValue("y", y);
		}
	}

	int x, y;
};

int main()
{
	auto testObj = CPoint(100, 200);

	const auto jsonResult = BitSerializer::SaveObject<JsonArchive>(testObj);
	std::cout << "JSON: " << jsonResult << std::endl;

	// Used explicitly defined root node name "Point" (to avoid auto-generated name "root")
	const auto xmlResult = BitSerializer::SaveObject<XmlArchive>(MakeAutoKeyValue("Point", testObj));
	std::cout << "XML: " << xmlResult << std::endl;
	return 0;
}