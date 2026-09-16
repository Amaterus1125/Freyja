const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf",aiProcess_Triangulate);
const aiMesh* mesh = scene ->mMeshes[0];
std::vector<vec3> positions;
std::vector<unsigned int> indices;
for(unsigned i=0;i!=mesh->mNumVertices;i++){
    const aiVector3D v =mesh->mVertices[i];
    positions.push_back(vec3(v.x,v.z,v.y));}
for(unsigned i=0;i!=mesh->mNumFaces;i++){
    for(unsigned j=0;j!=3;j++){
        indices.push_back(mesh->mFaces[i].mIndices[j]);}
}
aiReleaseImport(scene);
std::vector,unsigned int> remap(indicies.size());
const size_t vertexCount=meshopt_generateVertexRemap(remap.data(),indices.data(),indicies.size(),positions.data(),indices.size(),sizeof(vec3));
std::vector<unsigned int>
remappedIndices(indices.size());
std::vector<vec3> remappedVertices(vertexCount);
meshopt_remapIndexBuffer(remappedIndices.data(),indices.data(),indices.size(),remap.data());
meshopt_remapVertexBuffer(remappedVertices.data(),positions.data(),positions.size(),sizeof(vec3),remap.data());
meshopt_optimizeVertexCache(remappedIndices.data(),remappedIndices.data(),indices.size(),vertexCount);
meshopt_optimizeOverdraw(remappedIndices.data(),remappedIndices.data(),indices.size(),glm::value_ptr(remappedVertices[0]),vertexCount,sizeof(vec3),1.05f);
meshopt_optimizeVertexFetch(remappedVertices.data(),remappedIndices.data(),indices.size(),remappedVertices.data(),vertexCount,sizeof(vec3));
const float threshold =0.2f;
const size_t target_index_count = size_t(remappedIndices.size() * threshold);
const float target_error = 1e-2f;
std::vector<unsigned int> indicesLod(remappedIndices.size());
indicesLod.resize(meshopt_simplify(&indicesLod[0],remappedIndices.data(),remappedIndices.size(),&remappedVertices[0].x,vertexCount,sizeof(vec3),target_index_count,target_error));
indices = remappedIndices;
postions = remappedVertices;
const size_t sizeIndices = sizeof(unsigned int)*indices.size();
const size_t sizeIndicesLod = sizeof(unsigned int)*indicesLod.size();
const size_t sizeVertices = sizeof(vec3)*positions.size();
glNamedBufferStorage(meshData,0,sizeIndices+sizeIndicesLod+sizeVertices,nullptr,GL_DYNAMIC_STORAGE);
glNamedBufferSubData(meshData,0,sizeIndices,indicies.data());
glNamedBufferSubData(meshData,sizeIndices,sizeIndicesLod,,indicesLod.data());
glNamedBufferSubData(meshData,sizeIndices+sizeIndicesLod,sizeVertices,positions.data());
glVertexArrayElementBuffer(VAO,meshData);
glVertexArrayVertexBuffer(VAO,0,meshData,sizeIndices+sizeIndicesLod,sizeof(vec3));
glEnableVertexArrayAtrib(VAO,0);
glVertexArrayAtribFormat(VAO,0,3,GL_FLOAT,GL_FALSE,0);
glVertexArrayAttribBinding(VAO,0,0);
glDrawElements(GL_TRIANGLES,indices.size(),GL_UNSIGNED_INT,nullptr);
glDrawElements(GL_TRIANGLES,indicesLod.size(),GL_UNSIGNED_INT,(void*)sizeIndices);
