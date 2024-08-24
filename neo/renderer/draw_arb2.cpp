/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/VertexCache.h"

#include "renderer/tr_local.h"

// DG: if this is defined, the soft particle shaders will be compiled into the executable
//  otherwise soft_particle.vfp will be opened as a file just like the other shaders
//  (useful when tweaking that shader - when loaded from disk, you can use `reloadARBprograms`
//   instead of recompiling the executable)
#ifndef D3_INTEGRATE_SOFTPART_SHADERS
  #define D3_INTEGRATE_SOFTPART_SHADERS 1
#endif

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
static void GL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );
}

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_ARB2_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, din->localLightOrigin.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_VIEW_ORIGIN, din->localViewOrigin.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_S, din->lightProjection[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_T, din->lightProjection[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_Q, din->lightProjection[2].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_FALLOFF_S, din->lightProjection[3].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_T, din->specularMatrix[1].ToFloatPtr() );

	// testing fragment based normal mapping
	if ( r_testARBProgram.GetBool() ) {
		qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, din->localLightOrigin.ToFloatPtr() );
		qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, din->localViewOrigin.ToFloatPtr() );
	}

	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, zero );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	case SVC_MODULATE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, one );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, zero );
		break;
	case SVC_INVERSE_MODULATE:
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, negOne );
		qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	}

	// set the constant colors
	qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, din->diffuseColor.ToFloatPtr() );
	qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, din->specularColor.ToFloatPtr() );

	// DG: brightness and gamma in shader as program.env[4]
	if ( r_gammaInShader.GetBool() ) {
		// program.env[4].xyz are all r_brightness, program.env[4].w is 1.0/r_gamma
		float parm[4];
		parm[0] = parm[1] = parm[2] = r_brightness.GetFloat();
		parm[3] = 1.0/r_gamma.GetFloat(); // 1.0/gamma so the shader doesn't have to do this calculation
		qglProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, PP_GAMMA_BRIGHTNESS, parm );
	}

	// set the textures

	// texture 1 will be the per-surface bump map
	GL_SelectTextureNoClient( 1 );
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	GL_SelectTextureNoClient( 2 );
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	GL_SelectTextureNoClient( 3 );
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	GL_SelectTextureNoClient( 4 );
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	GL_SelectTextureNoClient( 5 );
	din->specularImage->Bind();

	// draw it
	RB_DrawElementsWithCounters( din->surf->geo );
}


/*
=============
RB_ARB2_CreateDrawInteractions

=============
*/
void RB_ARB2_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex program
	if ( r_testARBProgram.GetBool() ) {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_TEST );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST );
	} else {
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION );
	}

	qglEnable(GL_VERTEX_PROGRAM_ARB);
	qglEnable(GL_FRAGMENT_PROGRAM_ARB);

	// enable the vertex arrays
	qglEnableVertexAttribArrayARB( 8 );
	qglEnableVertexAttribArrayARB( 9 );
	qglEnableVertexAttribArrayARB( 10 );
	qglEnableVertexAttribArrayARB( 11 );
	qglEnableClientState( GL_COLOR_ARRAY );

	// texture 0 is the normalization cube map for the vector towards the light
	GL_SelectTextureNoClient( 0 );
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	GL_SelectTextureNoClient( 6 );
	if ( r_testARBProgram.GetBool() ) {
		globalImages->specular2DTableImage->Bind();	// variable specularity in alpha channel
	} else {
		globalImages->specularTableImage->Bind();
	}


	for ( ; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->geo->ambientCache );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( idDrawVert ), ac->color );
		qglVertexAttribPointerARB( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		qglVertexAttribPointerARB( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		qglVertexAttribPointerARB( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		qglVertexAttribPointerARB( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		qglVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_ARB2_DrawInteraction );
	}

	qglDisableVertexAttribArrayARB( 8 );
	qglDisableVertexAttribArrayARB( 9 );
	qglDisableVertexAttribArrayARB( 10 );
	qglDisableVertexAttribArrayARB( 11 );
	qglDisableClientState( GL_COLOR_ARRAY );

	// disable features
	GL_SelectTextureNoClient( 6 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 5 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 4 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 3 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 2 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 1 );
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );

	qglDisable(GL_VERTEX_PROGRAM_ARB);
	qglDisable(GL_FRAGMENT_PROGRAM_ARB);
}


/*
==================
RB_ARB2_DrawInteractions
==================
*/
void RB_ARB2_DrawInteractions( void ) {
	viewLight_t		*vLight;

	GL_SelectTexture( 0 );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions ) {
			continue;
		}

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			qglStencilFunc( GL_ALWAYS, 128, 255 );
		}

		if ( r_useShadowVertexProgram.GetBool() ) {
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->globalShadows );
			RB_ARB2_CreateDrawInteractions( vLight->localInteractions );
			qglEnable( GL_VERTEX_PROGRAM_ARB );
			qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW );
			RB_StencilShadowPass( vLight->localShadows );
			RB_ARB2_CreateDrawInteractions( vLight->globalInteractions );
			qglDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on
		} else {
			RB_StencilShadowPass( vLight->globalShadows );
			RB_ARB2_CreateDrawInteractions( vLight->localInteractions );
			RB_StencilShadowPass( vLight->localShadows );
			RB_ARB2_CreateDrawInteractions( vLight->globalInteractions );
		}

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}

		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_ARB2_CreateDrawInteractions( vLight->translucentInteractions );

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
}

//===================================================================================


typedef struct {
	GLenum			target;
	GLuint			ident;
	char			name[64];
} progDef_t;

static	const int	MAX_GLPROGS = 200;

// a single file can have both a vertex program and a fragment program
static progDef_t	progs[MAX_GLPROGS] = {
	{ GL_VERTEX_PROGRAM_ARB, VPROG_TEST, "test.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_TEST, "test.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION, "interaction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION, "interaction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_AMBIENT, "ambientLight.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_STENCIL_SHADOW, "shadow.vp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_GLASSWARP, "arbVP_glasswarp.txt" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_GLASSWARP, "arbFP_glasswarp.txt" },

	// SteveL #3878: Particle softening applied by the engine
	{ GL_VERTEX_PROGRAM_ARB, VPROG_SOFT_PARTICLE, "soft_particle.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_SOFT_PARTICLE, "soft_particle.vfp" },

	// additional programs can be dynamically specified in materials
};

#if D3_INTEGRATE_SOFTPART_SHADERS
// DG: the following two shaders are taken from TheDarkMod 2.04 (glprogs/soft_particle.vfp)
// (C) 2005-2016 Broken Glass Studios (The Dark Mod Team) and the individual authors
//     released under a revised BSD license and GPLv3
const char* softpartVShader = "!!ARBvp1.0  \n"
	"OPTION ARB_position_invariant;  \n"
	"# NOTE: unlike the TDM shader, the following lines use .texcoord and .color  \n"
	"#   instead of .attrib[8] and .attrib[3], to make it work with non-nvidia drivers \n"
	"#   Furthermore, I added support for a texture matrix \n"
	"PARAM defaultTexCoord = { 0, 0.5, 0, 1 }; \n"
	"MOV    result.texcoord, defaultTexCoord; \n"
	"# program.env[12] is PP_DIFFUSE_MATRIX_S, 13 is PP_DIFFUSE_MATRIX_T \n"
	"DP4    result.texcoord.x, vertex.texcoord, program.env[12]; \n"
	"DP4    result.texcoord.y, vertex.texcoord, program.env[13]; \n"
	"MOV    result.color, vertex.color; \n"
	"END \n";

const char* softpartFShader = "!!ARBfp1.0  \n"
	"# == Fragment Program == \n"
	"# taken from The Dark Mod 2.04, adjusted for dhewm3 \n"
	"# (C) 2005-2016 Broken Glass Studios (The Dark Mod Team) \n"
	"# \n"
	"# Input textures \n"
	"#   texture[0]   particle diffusemap \n"
	"#   texture[1]   _currentDepth \n"
	"# \n"
	"# Constants set by the engine: \n"
	"#   program.env[22] is reciprocal of _currentDepth size. Lets us convert a screen position to a texcoord in _currentDepth \n"
	"#      { 1.0f / depthtex.width, 1.0f / depthtex.height, float(depthtex.width)/int(depthtex.width), \n"
	"#          float(depthtex.height)/int(depthtex.height) } \n"
	"#   program.env[23] is the particle radius, given as { radius, 1/(fadeRange), 1/radius } \n"
	"#      fadeRange is the particle diameter for alpha blends (like smoke), but the particle radius for additive \n"
	"#      blends (light glares), because additive effects work differently. Fog is half as apparent when a wall   \n"
	"#      is in the middle of it. Light glares lose no visibility when they have something to reflect off.  \n"
	"#   program.env[24] is the color channel mask. Particles with additive blend need their RGB channels modified to blend them out. \n"
	"#                                              Particles with an alpha blend need their alpha channel modified. \n"
	"# \n"
	"# Hard-coded constants \n"
	"#    depth_consts allows us to recover the original depth in Doom units of anything in the depth \n"
	"#    buffer. Doom3's and thus TDM's projection matrix differs slightly from the classic projection matrix as \n"
	"#    it implements a \"nearly-infinite\" zFar. The matrix is hard-coded in the engine, so we use hard-coded \n"
	"#    constants here for efficiency. depth_consts is derived from the numbers in that matrix. \n"
	"# \n"
	"# next line: prevent dhewm3 from injecting gamma in shader code into this shader,  \n"
	"#            because that looks bad when rendered with additive blending (gets too bright) \n"
	"# nodhewm3gammahack \n"
	"\n"
	"PARAM   depth_consts = { 0.33333333, -0.33316667, 0.0, 0.0 }; \n"
	"PARAM   particle_radius  = program.env[23]; \n"
	"TEMP    tmp, scene_depth, particle_depth, near_fade, fade; \n"
	"\n"
	"# Map the fragment to a texcoord on our depth image, and sample to find scene_depth \n"
	"MUL   tmp.xy, fragment.position, program.env[22]; \n"
	"TEX   scene_depth, tmp, texture[1], 2D; \n"
	"MIN   scene_depth, scene_depth, 0.9994; # Required by TDM projection matrix. Equates to max recoverable  \n"
	"                                        # depth of 30k units, which is enough. 0.9995 is infinite depth. \n"
	"                                        # This is needed only if there is caulk sky on show (which writes \n"
	"                                        # no depth, so leaves 1 in the depth texture).  \n"
	"\n"
	"# Recover original depth in doom units  \n"
	"MAD   tmp, scene_depth, depth_consts.x, depth_consts.y; \n"
	"RCP   scene_depth, tmp.x; \n"
	"\n"
	"# Convert particle depth to doom units too \n"
	"MAD   tmp, fragment.position.z, depth_consts.x, depth_consts.y; \n"
	"RCP   particle_depth, tmp.x; \n"
	"\n"
	"# Scale the depth difference by the particle diameter to calc an alpha  \n"
	"# value based on how much of the 3d volume represented by the particle  \n"
	"# is in front of the solid scene  \n"
	"ADD      tmp, -scene_depth, particle_depth;     # NB depth is negative. 0 at the eye, -100 at 100 units into the screen. \n"
	"ADD      tmp, tmp, particle_radius.x;           # Add the radius so a depth difference of particle radius now equals 0 \n"
	"MUL_SAT  fade, tmp, particle_radius.y;          # divide by the particle radius or diameter and clamp \n"
	"\n"
	"# Also fade if the particle is too close to our eye position, so it doesn't 'pop' in and out of view \n"
	"# Start a linear fade at particle_radius distance from the particle. \n"
	"MUL_SAT  near_fade, particle_depth, -particle_radius.z;  \n"
	"\n"
	"# Calculate final fade and apply the channel mask \n"
	"MUL      fade, near_fade, fade; \n"
	"ADD_SAT  fade, fade, program.env[24];  # saturate the channels that don't want modifying \n"
	"\n"
	"# Set the color. Multiply by vertex/fragment color as that's how the particle system fades particles in and out \n"
	"TEMP  oColor; \n"
	"TEX   oColor, fragment.texcoord, texture[0], 2D; \n"
	"MUL   oColor, oColor, fade; \n"
	"MUL   result.color, oColor, fragment.color; \n"
	"\n"
	"END \n";

#endif // D3_INTEGRATE_SOFTPART_SHADERS

/*
=================
R_LoadARBProgram
=================
*/

static char* findLineThatStartsWith( char* text, const char* findMe ) {
	char* res = strstr( text, findMe );
	while ( res != NULL ) {
		// skip whitespace before match, if any
		char* cur = res;
		if ( cur > text ) {
			--cur;
		}
		while ( cur > text && ( *cur == ' ' || *cur == '\t' ) ) {
			--cur;
		}
		// now we should be at a newline (or at the beginning)
		if ( cur == text ) {
			return cur;
		}
		if ( *cur == '\n' || *cur == '\r' ) {
			return cur+1;
		}
		// otherwise maybe we're in commented out text or whatever, search on
		res = strstr( res+1, findMe );
	}
	return NULL;
}

static ID_INLINE bool isARBidentifierChar( int c ) {
	// according to chapter 3.11.2 in ARB_fragment_program.txt identifiers can only
	// contain these chars (first char mustn't be a number, but that doesn't matter here)
	// NOTE: isalnum() or isalpha() apparently doesn't work, as it also matches spaces (?!)
	return  c == '$' || c == '_'
	      || (c >= '0' && c <= '9')
	      || (c >= 'A' && c <= 'Z')
	      || (c >= 'a' && c <= 'z');
}

void R_LoadARBProgram( int progIndex ) {
	int		ofs;
	int		err;
	char	*buffer;
	char	*start = NULL, *end;

#if D3_INTEGRATE_SOFTPART_SHADERS
	if ( progs[progIndex].ident == VPROG_SOFT_PARTICLE || progs[progIndex].ident == FPROG_SOFT_PARTICLE ) {
		// these shaders are loaded directly from a string
		common->Printf( "<internal> %s", progs[progIndex].name );
		const char* srcstr = (progs[progIndex].ident == VPROG_SOFT_PARTICLE) ? softpartVShader : softpartFShader;

		// copy to stack memory
		buffer = (char *)_alloca( strlen( srcstr ) + 1 );
		strcpy( buffer, srcstr );
	}
	else
#endif // D3_INTEGRATE_SOFTPART_SHADERS
	{
		idStr	fullPath = "glprogs/";
		fullPath += progs[progIndex].name;
		char	*fileBuffer;
		common->Printf( "%s", fullPath.c_str() );

		// load the program even if we don't support it, so
		// fs_copyfiles can generate cross-platform data dumps
		fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, NULL );
		if ( !fileBuffer ) {
			common->Printf( ": File not found\n" );
			return;
		}

		// copy to stack memory and free
		buffer = (char *)_alloca( strlen( fileBuffer ) + 1 );
		strcpy( buffer, fileBuffer );
		fileSystem->FreeFile( fileBuffer );
	}

	if ( !glConfig.isInitialized ) {
		return;
	}

	//
	// submit the program string at start to GL
	//
	if ( progs[progIndex].ident == 0 ) {
		// allocate a new identifier for this program
		progs[progIndex].ident = PROG_USER + progIndex;
	}

	// vertex and fragment programs can both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end

	if ( progs[progIndex].target == GL_VERTEX_PROGRAM_ARB ) {
		if ( !glConfig.ARBVertexProgramAvailable ) {
			common->Printf( ": GL_VERTEX_PROGRAM_ARB not available\n" );
			return;
		}
		start = strstr( buffer, "!!ARBvp" );
	}
	if ( progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB ) {
		if ( !glConfig.ARBFragmentProgramAvailable ) {
			common->Printf( ": GL_FRAGMENT_PROGRAM_ARB not available\n" );
			return;
		}
		start = strstr( buffer, "!!ARBfp" );
	}
	if ( !start ) {
		common->Printf( ": !!ARB not found\n" );
		return;
	}
	end = strstr( start, "END" );

	if ( !end ) {
		common->Printf( ": END not found\n" );
		return;
	}
	end[3] = 0;

	// DG: hack gamma correction into shader
	if ( r_gammaInShader.GetBool() && progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB
	     && strstr( start, "nodhewm3gammahack" ) == NULL )
	{

		// note that strlen("dhewm3tmpres") == strlen("result.color")
		const char* tmpres = "TEMP dhewm3tmpres; # injected by dhewm3 for gamma correction\n";

		// Note: program.env[21].xyz = r_brightness; program.env[21].w = 1.0/r_gamma
		// outColor.rgb = pow(dhewm3tmpres.rgb*r_brightness, vec3(1.0/r_gamma))
		// outColor.a = dhewm3tmpres.a;
		const char* extraLinesDefault =
			"# gamma correction in shader, injected by dhewm3 \n"
			// MUL_SAT clamps the result to [0, 1] - it must not be negative because
			// POW might not work with a negative base (it looks wrong with intel's Linux driver)
			// and clamping values >1 to 1 is ok because when writing to result.color
			// it's clamped anyway and pow(base, exp) is always >= 1 for base >= 1
			"MUL_SAT dhewm3tmpres.xyz, program.env[21], dhewm3tmpres;\n" // first multiply with brightness
			"POW result.color.x, dhewm3tmpres.x, program.env[21].w;\n" // then do pow(dhewm3tmpres.xyz, vec3(1/gamma))
			"POW result.color.y, dhewm3tmpres.y, program.env[21].w;\n" // (apparently POW only supports scalars, not whole vectors)
			"POW result.color.z, dhewm3tmpres.z, program.env[21].w;\n"
			"MOV result.color.w, dhewm3tmpres.w;\n" // alpha remains unmodified
			"\nEND\n\n"; // we add this block right at the end, replacing the original "END" string


		/*
		float a = 2.51;
		float b = 0.03;
		float c = 2.43;
		float d = 0.59;
		float e = 0.14;
		return saturate((color * (a * color + b)) / (color * (c * x + d) + e));
		*/
		// Attempt at filmic tonemappingmapping with ARB
		const char* extraLinesTonemapped =
			/*
				Filmic shader taken from RBDoom3BFG
				Ported to ARB by worleydl
			*/
			"TEMP leftColor;\n"
			"TEMP rightColor;\n"

			"MUL_SAT dhewm3tmpres.xyz, program.env[4], dhewm3tmpres;\n" // first multiply with exposure

			// Left side ops
			"MAD leftColor.xyz, {2.51}.x, dhewm3tmpres, {0.03}.x;\n"
			"MUL leftColor.xyz, leftColor, dhewm3tmpres;\n"

			// Right side ops
			"MAD rightColor.xyz, {2.43}.x, dhewm3tmpres, {0.59}.x;\n"
			"MAD rightColor.xyz, rightColor, dhewm3tmpres, {0.14}.x;\n"

			// Final
			"RCP rightColor.x, rightColor.x;\n"
			"RCP rightColor.y, rightColor.y;\n"
			"RCP rightColor.z, rightColor.z;\n"
			"MUL_SAT dhewm3tmpres.xyz, leftColor, rightColor;\n"

			// Gamma correction
			"POW result.color.x, dhewm3tmpres.x, program.env[4].w;\n"
			"POW result.color.y, dhewm3tmpres.y, program.env[4].w;\n"
			"POW result.color.z, dhewm3tmpres.z, program.env[4].w;\n"
			"MOV result.color.w, dhewm3tmpres.w;\n" // alpha remains unmodified


			"\nEND\n\n"; // we add this block right at the end, replacing the original "END" string
		
		const char* extraLines;
		if (strstr(fullPath, "glass") || strstr(fullPath, "heatHaze")) {
			extraLines = extraLinesDefault;
		}
		else {
			extraLines = extraLinesTonemapped;
		}
		extraLines = extraLinesTonemapped;


		int fullLen = strlen( start ) + strlen( tmpres ) + strlen( extraLines );
		char* outStr = (char*)_alloca( fullLen + 1 );

		// add tmpres right after OPTION line (if any)
		char* insertPos = findLineThatStartsWith( start, "OPTION" );
		if ( insertPos == NULL ) {
			// no OPTION? then just put it after the first line (usually sth like "!!ARBfp1.0\n")
			insertPos = start;
		}
		// but we want the position *after* that line
		while( *insertPos != '\0' && *insertPos != '\n' && *insertPos != '\r' ) {
			++insertPos;
		}
		// skip  the newline character(s) as well
		while( *insertPos == '\n' || *insertPos == '\r' ) {
			++insertPos;
		}

		// copy text up to insertPos
		int curLen = insertPos-start;
		memcpy( outStr, start, curLen );
		// copy tmpres ("TEMP dhewm3tmpres; # ..")
		memcpy( outStr+curLen, tmpres, strlen( tmpres ) );
		curLen += strlen( tmpres );
		// copy remaining original shader up to (excluding) "END"
		int remLen = end - insertPos;
		memcpy( outStr+curLen, insertPos, remLen );
		curLen += remLen;

		outStr[curLen] = '\0'; // make sure it's NULL-terminated so normal string functions work

		// replace all existing occurrences of "result.color" with "dhewm3tmpres"
		for( char* resCol = strstr( outStr, "result.color" );
		     resCol != NULL; resCol = strstr( resCol+13, "result.color" ) ) {
			memcpy( resCol, "dhewm3tmpres", 12 ); // both strings have the same length.

			// if this was part of "OUTPUT bla = result.color;", replace
			// "OUTPUT bla" with "ALIAS  bla" (so it becomes "ALIAS  bla = dhewm3tmpres;")
			{
				char* s = resCol - 1;
				// first skip whitespace before "result.color"
				while( s > outStr && (*s == ' ' || *s == '\t') ) {
					--s;
				}
				// if there's no '=' before result.color, this line can't be affected
				if ( *s != '=' || s <= outStr + 8 ) {
					continue; // go on with next "result.color" in the for-loop
				}
				--s; // we were on '=', so go to the char before and it's time to skip whitespace again
				while( s > outStr && ( *s == ' ' || *s == '\t' ) ) {
					--s;
				}
				// now we should be at the end of "bla" (or however the variable/alias is called)
				if ( s <= outStr+7 || !isARBidentifierChar( *s ) ) {
					continue;
				}
				--s;
				// skip all the remaining chars that are legal in identifiers
				while( s > outStr && isARBidentifierChar( *s ) ) {
					--s;
				}
				// there should be at least one space/tab between "OUTPUT" and "bla"
				if ( s <= outStr + 6 || ( *s != ' ' && *s != '\t' ) ) {
					continue;
				}
				--s;
				// skip remaining whitespace (if any)
				while( s > outStr && ( *s == ' ' || *s == '\t' ) ) {
					--s;
				}
				// now we should be at "OUTPUT" (specifically at its last 'T'),
				// if this is indeed such a case
				if ( s <= outStr + 5 || *s != 'T' ) {
					continue;
				}
				s -= 5; // skip to start of "OUTPUT", if this is indeed "OUTPUT"
				if ( idStr::Cmpn( s, "OUTPUT", 6 ) == 0 ) {
					// it really is "OUTPUT" => replace "OUTPUT" with "ALIAS "
					memcpy(s, "ALIAS ", 6);
				}
			}
		}

		assert( curLen + strlen( extraLines ) <= fullLen );

		// now add extraLines that calculate and set a gamma-corrected result.color
		// strcat() should be safe because fullLen was calculated taking all parts into account
		strcat( outStr, extraLines );
		start = outStr;
	}

	qglBindProgramARB( progs[progIndex].target, progs[progIndex].ident );
	qglGetError();

	qglProgramStringARB( progs[progIndex].target, GL_PROGRAM_FORMAT_ASCII_ARB,
		strlen( start ), start );

	err = qglGetError();
	qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, (GLint *)&ofs );
	if ( err == GL_INVALID_OPERATION ) {
		const GLubyte *str = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n" );
		} else if ( ofs >= (int)strlen( start ) ) {
			common->Printf( "error at end of program\n" );
		} else {
			int printOfs = Max( ofs - 20, 0 ); // DG: print some more context
			common->Printf( "error at %i:\n%s", ofs, start + printOfs );
		}
		return;
	}
	if ( ofs != -1 ) {
		common->Printf( "\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n" );
		return;
	}

	common->Printf( "\n" );
}

/*
==================
R_FindARBProgram

Returns a GL identifier that can be bound to the given target, parsing
a text file if it hasn't already been loaded.
==================
*/
int R_FindARBProgram( GLenum target, const char *program ) {
	int		i;
	idStr	stripped = program;

	stripped.StripFileExtension();

	// see if it is already loaded
	for ( i = 0 ; progs[i].name[0] ; i++ ) {
		if ( progs[i].target != target ) {
			continue;
		}

		idStr	compare = progs[i].name;
		compare.StripFileExtension();

		if ( !idStr::Icmp( stripped.c_str(), compare.c_str() ) ) {
			return progs[i].ident;
		}
	}

	if ( i == MAX_GLPROGS ) {
		common->Error( "R_FindARBProgram: MAX_GLPROGS" );
	}

	// add it to the list and load it
	progs[i].ident = (program_t)0;	// will be gen'd by R_LoadARBProgram
	progs[i].target = target;
	idStr::Copynz( progs[i].name, program, sizeof( progs[i].name ) );

	R_LoadARBProgram( i );

	return progs[i].ident;
}

/*
==================
R_ReloadARBPrograms_f
==================
*/
void R_ReloadARBPrograms_f( const idCmdArgs &args ) {
	int		i;

	common->Printf( "----- R_ReloadARBPrograms -----\n" );
	for ( i = 0 ; progs[i].name[0] ; i++ ) {
		R_LoadARBProgram( i );
	}
}

/*
==================
R_ARB2_Init

==================
*/
void R_ARB2_Init( void ) {
	glConfig.allowARB2Path = false;

	common->Printf( "ARB2 renderer: " );

	if ( !glConfig.ARBVertexProgramAvailable || !glConfig.ARBFragmentProgramAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	}

	common->Printf( "Available.\n" );

	glConfig.allowARB2Path = true;
}
