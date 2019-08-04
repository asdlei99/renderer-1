#include "BlendShape.h"
#include "Resource\ResourceCache.h"
#include "Resource\XMLParser.h"


USING_ALLOCATER(BlendShape);

BlendShape::BlendShape(Context * context) : GPUResource(context), ShapeCount_(0), BufferSize_(0), ShapeStride_(0)
{
    ResourceType = R_BLEDNSHAPE;
}


BlendShape::~BlendShape()
{
}

int BlendShape::OnSerialize(Deserializer& deserializer)
{
    XMLParser Parser;
    auto RawXML = deserializer.Raw();
    auto xml_doc = Parser.Parse(RawXML);
    // parse technique and pass
    xml_node<> * node;
    node = xml_doc->first_node("blendshapes");
    // get the first blendshape
    node = node->first_node();
    int index = 0;
    while(node) {
        // get blendshape names
        String name = node->first_attribute("name")->value();
        String URL = node->first_attribute("url")->value();
        // add to dependency
        Dependencies[URL].as<int>() = index++;
        DepCount++;
        node = node->next_sibling();
    }
    return 0;
}

int BlendShape::OnSubResource(int Message, Resource * Sub, Variant & Param)
{
    Mesh * mesh = (Mesh*)Sub;
    auto index = Param.as<int>();
    // fill in mesh
    BlendShapes_[index] = mesh;
    // all sub-resources loaded
    if (!--DepCount) {
        // set update blendshape desc
        ShapeStride_ = mesh->VBSize;
        BufferSize_ = ShapeStride_ * ShapeCount_;
        VertexStride_ = mesh->VTSize;
        // alloc data for blendshape buffer
        char * buffer = new char[BufferSize_];
        for (auto i = 0; i < ShapeCount_; i++) {
            mesh = BlendShapes_[i];
            memcpy(buffer+(i * ShapeStride_), mesh->VBuffer, mesh->VBSize);
        }
        // create buffer
        R_BUFFER_DESC desc = {
            BufferSize_,
            DEFAULT,
            (R_BIND_FLAG)(BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS),
            (R_CPU_ACCESS)0,
            (R_MISC)0,
            VertexStride_,
            buffer
        };
        // create buffer in gpu, for blending shader resource
        id = renderinterface->CreateBuffer(&desc);
        // notify 
        NotifyOwner(RM_LOAD, OwnerParameter_);
    }
    return 0;
}

int BlendShape::OnCreateComplete(Variant & Parameter)
{
    // async load subresource, blendshapes
    ResourceCache * cache = context->GetSubsystem<ResourceCache>();
    for (auto iter = Dependencies.Begin(); iter != Dependencies.End(); iter++) {
        KeyValue& kv = *iter;
        // key is resource url
        printf("async loading %s \n", kv.key.ToStr());
        // char * tmp = (char*)kv.key;
        cache->AsyncLoadResource(kv.key, this, kv.Value);
        // add shap count
        ShapeCount_++;
    }
    // reserve vector size
    BlendShapes_.Resize(ShapeCount_);
    // defer owner parameter
    OwnerParameter_ = Parameter;
    return 0;
}

int BlendShape::OnLoadComplete(Variant& Data) {
    return 0;
}