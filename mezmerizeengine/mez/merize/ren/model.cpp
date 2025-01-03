#include "model.h"
#include "../staticdefs.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

using namespace Assimp;

class RModel_Internal
{
public:
    Assimp::Importer m_importer;
};

RModel::RModel()
{
    m_ = new RModel_Internal();
}

RModel::~RModel()
{
    console_printf("freeing rmodel %i %i\n",m_valid, m_expected_deallocation);
    if (m_expected_deallocation) return;
    delete m_;
    m_ = 0;
}

void RModel::unload_internal()
{
    delete m_;
    m_ = 0;
}

RModel* RModel::LoadModelFromFile(const char* filename, bool essential)
{
    RModel* model =new RModel();
    //this whole system is terrible
    const char* desired_folder = "content/models";
    const char* fn;
    if (strstr(filename, ".")) //this has a filetype attached to it
    {
        fn = static_format("%s/%s", desired_folder, filename);
    }
    else //assume .obj
    {
        fn = static_format("%s/%s.obj", desired_folder, filename);
    }
    const aiScene* scene = model->m_->m_importer.ReadFile(fn,0);
    if (!scene)
    {
        //whoever is calling this func will be upset to hear this
        static_format_t t = static_format("Failed to find model %s", fn);
        if (!essential)
        {
            printf("%s\n",t);
        }
        else
        {
            engine->fatalmsg(t);
        }
        model->m_valid = 0;
        return model;
    }
    assert(scene);
    assert(scene->HasMeshes());


    model->m_meshes = new RModelMeshSingle[ scene->mNumMeshes ];
    model->m_mesh_count = scene->mNumMeshes;
    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        model->m_meshes[i].m_Verts = reintrp_noderef(scene->mMeshes[i]->mVertices,Vector*);
        model->m_meshes[i].m_Normals = reintrp_noderef(scene->mMeshes[i]->mNormals,Vector*);
        model->m_meshes[i].m_UVs = reintrp_noderef(scene->mMeshes[i]->mTextureCoords[0], Vector*);
        model->m_meshes[i].m_NumOfVerts = scene->mMeshes[i]->mNumVertices;
        model->m_meshes[i].m_DrawMode = RModelMeshSingle::DM_Triangles; //test
    }
    console_printf("Loaded model %s\n", fn);
    model->m_name = filename;
    return model;
}
