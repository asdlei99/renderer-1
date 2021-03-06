#ifndef __H3D_H__
#define __H3D_H__

#define H3DMAGIC   "HU3D"


namespace ModelSystem{ namespace h3d {

#pragma pack(1)



typedef struct h3d_mesh
{
	DWORD VertexSize;
	DWORD IndexSize;
	DWORD VertexNum;
	DWORD OffsetVertex;
	DWORD IndexNum;
	DWORD OffsetIndex;
	char  Texture[256];
}h3d_mesh;

//280

typedef struct h3d_bone
{
	int    Frames;
	int    BoneNum;
	DWORD  OffsetBone;
}h3d_bone;

//12

typedef struct h3d_header
{
	DWORD  Magic;
	DWORD  Version;
	int    MeshNum;
}h3d_header;

//12

#pragma pack()

}} //end namespace

#endif