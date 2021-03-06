#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "Resource\Resource.h"
#include "Texture.h"
#include "Shader.h"
#include "Container\Dict.h"
#include "Resource\XMLParser.h"


#define MULTI_TEXTUES 8



class Material : public Resource
{
	OBJECT(Material);
	BASEOBJECT(Material);
	LOADEROBJECT(FileLoader);
	DECLAR_ALLOCATER(Material);
private:
	// textures, diffuse, normal, specular
	//Texture * Textures[MULTI_TEXTUES];
	// shaders
	Shader* ShaderProgram;
	// texture id cache
	//int TexrureID[MULTI_TEXTUES];
	// parameters. 
	Dict Parameters;
	// xml doc
	xml_document<>* xml_doc;
	// raw xml data
	void* RawXML;
public:
	Material(Context* context);
	~Material();
	virtual int OnSerialize(Deserializer& deserializer);
	// on create complete
	virtual int OnCreateComplete(Variant& Parameter);
	// on raw data parse complete(worker thread)
	virtual int OnLoadComplete(Variant& Data) {/* DeSerial.Release();*/ return 0; };
	virtual int OnSubResource(int Message, Resource* Sub, Variant& Parameter);
	virtual int Compile(BatchCompiler* Compiler, int Stage, int Lod);
	Dict& GetParameter() { return Parameters; }
	Shader* GetShader() { return ShaderProgram; }
	virtual int OnDestroy(Variant& Data);
};

#endif
