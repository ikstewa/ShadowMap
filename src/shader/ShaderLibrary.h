//-----------------------------------------------------------------------------
// ShaderLibrary.h
//
// Author:Ian Stewart
// 08/2008
//
//-----------------------------------------------------------------------------

#ifndef __SHADER_LIBRARY_H__
#define __SHADER_LIBRARY_H__

#include "Shader.h"

//#define SHADER_LIB "./game/data/shaders/"
#define SHADER_LIB ""


enum ShaderType
{
    // placeholder, do not use
    SHADER_TEST = 0,
    // Toon shader
    SHADER_TOON_COLOR,
    SHADER_TOON_TEXTURE,

    // Fixed pipeline functionality
    SHADER_FIXED
};

class ShaderLibrary
{
public:
    static ShaderLibrary* getInstance();

    bool attachShader(ShaderType t);
    void RegisterTextures(int cnt);

protected:
    ShaderLibrary();
    ShaderLibrary(const ShaderLibrary&);
    ShaderLibrary& operator=(const ShaderLibrary&);

    void init();
    Shader* ShaderLibrary::loadShader(ShaderType t);

    Shader* shader[SHADER_FIXED+1];

private:
    static ShaderLibrary* inst;

    ShaderType lastAttatch;

    bool enableShaders;
};



#endif