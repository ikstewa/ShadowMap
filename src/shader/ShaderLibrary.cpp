//-----------------------------------------------------------------------------
// ShaderLibrary.cpp
//
// Author:Ian Stewart
// 08/2008
//
//-----------------------------------------------------------------------------


#include "ShaderLibrary.h"

#include <stdio.h>
#include <string>
#include <GL/glut.h>

using namespace std;

ShaderLibrary* ShaderLibrary::inst = 0;

//-----------------------------------------------------------------------------
// Public methods
ShaderLibrary* ShaderLibrary::getInstance()
{
    if (inst == 0)
        inst = new ShaderLibrary();

    return inst;
}

bool ShaderLibrary::attachShader(ShaderType t)
{
    return shader[t]->attach();
}


//-----------------------------------------------------------------------------
// Protected methods
ShaderLibrary::ShaderLibrary()
{
    lastAttatch = SHADER_TEST;
    memset(shader, 0, SHADER_FIXED*sizeof(Shader*));

    GLenum err = glewInit();

    if (err != GLEW_OK || !glewIsSupported("GL_VERSION_2_0"))
    {
        printf("OpenGL 2.0 not supported. No shaders!\n");
        printf("%s\n", glewGetErrorString(err));
        printf("%s\n", (char*)glGetString( GL_VERSION ) );

        enableShaders = false;
        return;
    }

    enableShaders = true;
    printf("OpenGL 2.0 supported.\n");
    init();
}

void ShaderLibrary::init()
{
    lastAttatch = SHADER_TEST;
    for ( int i = SHADER_TEST; i <= SHADER_FIXED; i++ )
    {
        shader[i] = loadShader((ShaderType)(i));
    }
}

Shader* ShaderLibrary::loadShader(ShaderType t)
{
    if (!enableShaders)
        return 0;

    bool success = true;
    char *name;
    switch(t)
    {
        // blank shader, do not use
        case SHADER_TEST:
            name = "SHADER_TEST";
            shader[t] = 0;
            break;
        // toon shader
        case SHADER_TOON_COLOR:
            name = "SHADER_TOON_V2";
            shader[t] = new Shader();
            success &= shader[t]->addVert(((string)SHADER_LIB).append("toon_v2.vert").c_str());
            success &= shader[t]->addFrag(((string)SHADER_LIB).append("toon_v2_color.frag").c_str());
            success &= shader[t]->build();
            break;
        case SHADER_TOON_TEXTURE:
            name = "SHADER_TOON_TEXTURE";
            shader[t] = new Shader();
            success &= shader[t]->addVert(((string)SHADER_LIB).append("toon_v2.vert").c_str());
            success &= shader[t]->addFrag(((string)SHADER_LIB).append("toon_v2_tex.frag").c_str());
            success &= shader[t]->build();
            break;
        // fixed pipeline functionality
        case SHADER_FIXED:
            name = "SHADER_FIXED";
            break;
    }

    if (success)
        return shader[t];
    else
    {
        printf("ShaderLibrary ERROR: Could not load shader %s!\n", name);
        return 0;
    }
}