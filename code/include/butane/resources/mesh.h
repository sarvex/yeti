// This file is part of Butane. See README.md and LICENSE.md for details.
// Copyright (c) 2012 Michael Williams <devbug@bitbyte.ca>

#ifndef _BUTANE_MESH_RESOURCE_H_
#define _BUTANE_MESH_RESOURCE_H_

#include <butane/resource.h>
#include <butane/resources/texture.h>
#include <butane/resources/shader.h>
#include <butane/graphics/vertex_declaration.h>
#include <butane/graphics/vertex_buffer.h>
#include <butane/graphics/index_buffer.h>

namespace butane {
  class BUTANE_EXPORT MeshResource final
    : public butane::Resource
  {
    __foundation_trait(MeshResource, non_copyable);

    public:
      struct Material {
        typedef Hash<uint32_t, murmur_hash> Name;
        Name name;
        Resource::Handle<ShaderResource> shader;
        uint32_t num_of_textures;
        Resource::Handle<TextureResource> textures[8];
      };

    private:
      struct MemoryResidentData {
        VertexDeclaration vertex_declaration;
        uint32_t num_of_materials;
        uint32_t num_of_vertices;
        uint32_t num_of_indicies;

        struct Material {
          MeshResource::Material::Name name;
          Resource::Id shader;
          Resource::Id textures[8];
        };

        // Material materials[num_of_materials];
        // uint8_t  vertices[num_of_vertices * vertex_declaration.size()];
        // uint32_t indicies[num_of_indicies];
      };

    public:
      static const Resource::Type type;

    protected:
      MeshResource(
        const Resource::Id id );

      ~MeshResource();

    private:
      static MeshResource* load(
        const Resource::Id id,
        const Resource::Stream& stream );

      static void unload(
        MeshResource* mesh );

      static bool compile(
        const Resource::Compiler::Input& input,
        const Resource::Compiler::Output& output );

    public:
      FOUNDATION_INLINE VertexDeclaration vertex_declaration() const
      { return _vertex_declaration; }

      FOUNDATION_INLINE const Array<Material>& materials() const
      { return _materials; }

      FOUNDATION_INLINE VertexBuffer* vertices() const
      { return _vertices; }

      FOUNDATION_INLINE IndexBuffer* indicies() const
      { return _indicies; }

      FOUNDATION_INLINE uint32_t num_of_primitives() const
      { return (_num_of_indicies / 3); }

    private:
      VertexDeclaration _vertex_declaration;
      Array<Material> _materials;
      VertexBuffer* _vertices;
      IndexBuffer* _indicies;
      uint32_t _num_of_indicies;
  };
} // butane

#endif // _BUTANE_MESH_RESOURCE_H_