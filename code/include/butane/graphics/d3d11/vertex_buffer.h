// This file is part of Butane. See README.md and LICENSE.md for details.
// Copyright (c) 2012 Michael Williams <devbug@bitbyte.ca>

#ifndef _BUTANE_D3D11_VERTEX_BUFFER_H_
#define _BUTANE_D3D11_VERTEX_BUFFER_H_

#include <butane/graphics/d3d11/d3d11.h>
#include <butane/graphics/vertex_buffer.h>

namespace butane {
  class BUTANE_EXPORT D3D11VertexBuffer final
    : public VertexBuffer
  {
    __foundation_trait(D3D11VertexBuffer, non_copyable);
  };
} // butane

#endif // _BUTANE_D3D11_VERTEX_BUFFER_H_