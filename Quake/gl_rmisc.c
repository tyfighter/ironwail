/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_misc.c

#include "quakedef.h"

//johnfitz -- new cvars
extern cvar_t r_clearcolor;
extern cvar_t r_flatlightstyles;
extern cvar_t r_lerplightstyles;
extern cvar_t gl_fullbrights;
extern cvar_t gl_farclip;
extern cvar_t gl_overbright;
extern cvar_t gl_overbright_models;
extern cvar_t r_waterwarp;
extern cvar_t r_oldskyleaf;
extern cvar_t r_drawworld;
extern cvar_t r_showtris;
extern cvar_t r_showbboxes;
extern cvar_t r_lerpmodels;
extern cvar_t r_lerpmove;
extern cvar_t r_nolerp_list;
extern cvar_t r_noshadow_list;
//johnfitz
extern cvar_t gl_zfix; // QuakeSpasm z-fighting fix

#if defined(USE_SIMD)
extern cvar_t r_simd;
#endif
qboolean use_simd;

extern gltexture_t *playertextures[MAX_SCOREBOARD]; //johnfitz


/*
====================
GL_Overbright_f -- johnfitz
====================
*/
static void GL_Overbright_f (cvar_t *var)
{
}

/*
====================
GL_Fullbrights_f -- johnfitz
====================
*/
static void GL_Fullbrights_f (cvar_t *var)
{
	TexMgr_ReloadNobrightImages ();
}

/*
====================
R_SetClearColor_f -- johnfitz
====================
*/
static void R_SetClearColor_f (cvar_t *var)
{
	byte	*rgb;
	int		s;

	s = (int)r_clearcolor.value & 0xFF;
	rgb = (byte*)(d_8to24table + s);
	glClearColor (rgb[0]/255.0,rgb[1]/255.0,rgb[2]/255.0,0);
}

/*
===============
R_Model_ExtraFlags_List_f -- johnfitz -- called when r_nolerp_list or r_noshadow_list cvar changes
===============
*/
static void R_Model_ExtraFlags_List_f (cvar_t *var)
{
	int i;
	for (i=0; i < MAX_MODELS; i++)
		Mod_SetExtraFlags (cl.model_precache[i]);
}

#if defined(USE_SIMD)
/*
====================
R_SIMD_f
====================
*/
static void R_SIMD_f (cvar_t *var)
{
#if defined(USE_SSE2)
	use_simd = SDL_HasSSE() && SDL_HasSSE2() && (var->value != 0.0f);
#else
	#error not implemented
#endif
}
#endif

/*
====================
R_SetWateralpha_f -- ericw
====================
*/
static void R_SetWateralpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWWATER) && var->value < 1)
		Con_Warning("Map does not appear to be water-vised\n");
	map_wateralpha = var->value;
	map_fallbackalpha = var->value;
}

/*
====================
R_SetLavaalpha_f -- ericw
====================
*/
static void R_SetLavaalpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWLAVA) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be lava-vised\n");
	map_lavaalpha = var->value;
}

/*
====================
R_SetTelealpha_f -- ericw
====================
*/
static void R_SetTelealpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWTELE) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be tele-vised\n");
	map_telealpha = var->value;
}

/*
====================
R_SetSlimealpha_f -- ericw
====================
*/
static void R_SetSlimealpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWSLIME) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be slime-vised\n");
	map_slimealpha = var->value;
}

/*
====================
GL_WaterAlphaForTextureType
====================
*/
float GL_WaterAlphaForTextureType (textype_t type)
{
	if (type == TEXTYPE_LAVA)
		return map_lavaalpha > 0 ? map_lavaalpha : map_fallbackalpha;
	else if (type == TEXTYPE_TELE)
		return map_telealpha > 0 ? map_telealpha : map_fallbackalpha;
	else if (type == TEXTYPE_SLIME)
		return map_slimealpha > 0 ? map_slimealpha : map_fallbackalpha;
	else
		return map_wateralpha;
}


/*
===============
R_Init
===============
*/
void R_Init (void)
{
	extern cvar_t gl_finish;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_lightmap);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_shadows);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_SetCallback (&r_wateralpha, R_SetWateralpha_f);
	Cvar_RegisterVariable (&r_dynamic);
	Cvar_RegisterVariable (&r_novis);
#if defined(USE_SIMD)
	Cvar_RegisterVariable (&r_simd);
	Cvar_SetCallback (&r_simd, R_SIMD_f);
	R_SIMD_f(&r_simd);
#endif
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_pos);

	Cvar_RegisterVariable (&gl_finish);
	Cvar_RegisterVariable (&gl_clear);
	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&gl_playermip);
	Cvar_RegisterVariable (&gl_nocolors);

	//johnfitz -- new cvars
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_SetCallback (&r_clearcolor, R_SetClearColor_f);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_flatlightstyles);
	Cvar_RegisterVariable (&r_lerplightstyles);
	Cvar_RegisterVariable (&r_oldskyleaf);
	Cvar_RegisterVariable (&r_drawworld);
	Cvar_RegisterVariable (&r_showtris);
	Cvar_RegisterVariable (&r_showbboxes);
	Cvar_RegisterVariable (&gl_farclip);
	Cvar_RegisterVariable (&gl_fullbrights);
	Cvar_RegisterVariable (&gl_overbright);
	Cvar_SetCallback (&gl_fullbrights, GL_Fullbrights_f);
	Cvar_SetCallback (&gl_overbright, GL_Overbright_f);
	Cvar_RegisterVariable (&gl_overbright_models);
	Cvar_RegisterVariable (&r_lerpmodels);
	Cvar_RegisterVariable (&r_lerpmove);
	Cvar_RegisterVariable (&r_nolerp_list);
	Cvar_SetCallback (&r_nolerp_list, R_Model_ExtraFlags_List_f);
	Cvar_RegisterVariable (&r_noshadow_list);
	Cvar_SetCallback (&r_noshadow_list, R_Model_ExtraFlags_List_f);
	//johnfitz

	Cvar_RegisterVariable (&gl_zfix); // QuakeSpasm z-fighting fix
	Cvar_RegisterVariable (&r_lavaalpha);
	Cvar_RegisterVariable (&r_telealpha);
	Cvar_RegisterVariable (&r_slimealpha);
	Cvar_RegisterVariable (&r_scale);
	Cvar_SetCallback (&r_lavaalpha, R_SetLavaalpha_f);
	Cvar_SetCallback (&r_telealpha, R_SetTelealpha_f);
	Cvar_SetCallback (&r_slimealpha, R_SetSlimealpha_f);

	R_InitParticles ();
	R_SetClearColor_f (&r_clearcolor); //johnfitz

	Sky_Init (); //johnfitz
	Fog_Init (); //johnfitz
}

/*
===============
R_TranslatePlayerSkin -- johnfitz -- rewritten.  also, only handles new colors, not new skins
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int			top, bottom;

	top = (cl.scores[playernum].colors & 0xf0)>>4;
	bottom = cl.scores[playernum].colors &15;

	//FIXME: if gl_nocolors is on, then turned off, the textures may be out of sync with the scoreboard colors.
	if (!gl_nocolors.value)
		if (playertextures[playernum])
			TexMgr_ReloadImage (playertextures[playernum], top, bottom);
}

/*
===============
R_TranslateNewPlayerSkin -- johnfitz -- split off of TranslatePlayerSkin -- this is called when
the skin or model actually changes, instead of just new colors
added bug fix from bengt jardup
===============
*/
void R_TranslateNewPlayerSkin (int playernum)
{
	char		name[64];
	byte		*pixels;
	aliashdr_t	*paliashdr;
	entity_t	*e;
	int		skinnum;

//get correct texture pixels
	e = &cl_entities[1+playernum];

	if (!e->model || e->model->type != mod_alias)
		return;

	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);

	skinnum = e->skinnum;

	//TODO: move these tests to the place where skinnum gets received from the server
	if (skinnum < 0 || skinnum >= paliashdr->numskins)
	{
		Con_DPrintf("(%d): Invalid player skin #%d\n", playernum, skinnum);
		skinnum = 0;
	}

	pixels = (byte *)paliashdr + paliashdr->texels[skinnum]; // This is not a persistent place!

//upload new image
	q_snprintf(name, sizeof(name), "player_%i", playernum);
	playertextures[playernum] = TexMgr_LoadImage (e->model, name, paliashdr->skinwidth, paliashdr->skinheight,
		SRC_INDEXED, pixels, paliashdr->gltextures[skinnum][0]->source_file, paliashdr->gltextures[skinnum][0]->source_offset, TEXPREF_PAD | TEXPREF_OVERWRITE);

//now recolor it
	R_TranslatePlayerSkin (playernum);
}

/*
===============
R_NewGame -- johnfitz -- handle a game switch
===============
*/
void R_NewGame (void)
{
	int i;

	//clear playertexture pointers (the textures themselves were freed by texmgr_newgame)
	for (i=0; i<MAX_SCOREBOARD; i++)
		playertextures[i] = NULL;
}

/*
=============
R_ParseWorldspawn

called at map load
=============
*/
static void R_ParseWorldspawn (void)
{
	char key[128], value[4096];
	const char *data;

	map_fallbackalpha = r_wateralpha.value;
	map_wateralpha = (cl.worldmodel->contentstransparent&SURF_DRAWWATER)?r_wateralpha.value:1;
	map_lavaalpha = (cl.worldmodel->contentstransparent&SURF_DRAWLAVA)?r_lavaalpha.value:1;
	map_telealpha = (cl.worldmodel->contentstransparent&SURF_DRAWTELE)?r_telealpha.value:1;
	map_slimealpha = (cl.worldmodel->contentstransparent&SURF_DRAWSLIME)?r_slimealpha.value:1;

	data = COM_Parse(cl.worldmodel->entities);
	if (!data)
		return; // error
	if (com_token[0] != '{')
		return; // error
	while (1)
	{
		data = COM_Parse(data);
		if (!data)
			return; // error
		if (com_token[0] == '}')
			break; // end of worldspawn
		if (com_token[0] == '_')
			q_strlcpy(key, com_token + 1, sizeof(key));
		else
			q_strlcpy(key, com_token, sizeof(key));
		while (key[0] && key[strlen(key)-1] == ' ') // remove trailing spaces
			key[strlen(key)-1] = 0;
		data = COM_Parse(data);
		if (!data)
			return; // error
		q_strlcpy(value, com_token, sizeof(value));

		if (!strcmp("wateralpha", key))
			map_wateralpha = atof(value);

		if (!strcmp("lavaalpha", key))
			map_lavaalpha = atof(value);

		if (!strcmp("telealpha", key))
			map_telealpha = atof(value);

		if (!strcmp("slimealpha", key))
			map_slimealpha = atof(value);
	}
}


/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	R_ClearParticles ();

	GL_BuildLightmaps ();
	GL_BuildBModelVertexBuffer ();
	GL_BuildBModelMarkBuffers ();
	//ericw -- no longer load alias models into a VBO here, it's done in Mod_LoadAliasModel

	r_framecount = 0; //johnfitz -- paranoid?
	r_visframecount = 0; //johnfitz -- paranoid?

	Sky_NewMap (); //johnfitz -- skybox in worldspawn
	Fog_NewMap (); //johnfitz -- global fog in worldspawn
	R_ParseWorldspawn (); //ericw -- wateralpha, lavaalpha, telealpha, slimealpha in worldspawn
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int		i;
	double	start, stop, time;

	if (cls.state != ca_connected)
	{
		Con_Printf("Not connected to a server\n");
		return;
	}

	start = Sys_DoubleTime ();
	for (i = 0; i < 128; i++)
	{
		GL_BeginRendering(&glx, &gly, &glwidth, &glheight);
		r_refdef.viewangles[1] = i*(360.0/128.0);
		R_RenderView ();
		GL_EndRendering ();
	}

	glFinish ();
	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%lf seconds (%.1lf fps)\n", time, 128/time);
}

void D_FlushCaches (void)
{
}

static GLuint current_array_buffer;
static GLuint current_element_array_buffer;
static GLuint current_shader_storage_buffer;
static GLuint current_draw_indirect_buffer;

/*
====================
GL_BindBuffer

glBindBuffer wrapper
====================
*/
void GL_BindBuffer (GLenum target, GLuint buffer)
{
	GLuint *cache;

	switch (target)
	{
		case GL_ARRAY_BUFFER:
			cache = &current_array_buffer;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			cache = &current_element_array_buffer;
			break;
		case GL_SHADER_STORAGE_BUFFER:
			cache = &current_shader_storage_buffer;
			break;
		case GL_DRAW_INDIRECT_BUFFER:
			cache = &current_draw_indirect_buffer;
			break;
		default:
			Host_Error("GL_BindBuffer: unsupported target %d", (int)target);
			return;
	}

	if (*cache != buffer)
	{
		*cache = buffer;
		GL_BindBufferFunc (target, *cache);
	}
}

typedef struct {
	GLuint		buffer;
	GLintptr	offset;
	GLsizeiptr	size;
} bufferrange_t;

#define CACHED_BUFFER_RANGES 8

static bufferrange_t ssbo_ranges[CACHED_BUFFER_RANGES];

/*
====================
GL_BindBufferRange

glBindBufferRange wrapper
====================
*/
void GL_BindBufferRange (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	if (target == GL_SHADER_STORAGE_BUFFER && index < CACHED_BUFFER_RANGES)
	{
		bufferrange_t *range = &ssbo_ranges[index];
		if (range->buffer == buffer && range->offset == offset && range->size == size)
			return;
		range->buffer = buffer;
		range->offset = offset;
		range->size   = size;
	}

	current_shader_storage_buffer = buffer;
	GL_BindBufferRangeFunc (target, index, buffer, offset, size);
}

/*
====================
GL_BindBuffersRange

glBindBuffersRange wrapper with fallback
if ARB_multi_bind is not present
====================
*/
void GL_BindBuffersRange (GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes)
{
	GLsizei i;
	if (gl_multi_bind_able)
	{
		if (target == GL_SHADER_STORAGE_BUFFER)
		{
			for (i = 0; i < count && first + i < countof (ssbo_ranges); i++)
			{
				bufferrange_t *range = &ssbo_ranges[first + i];
				range->buffer = buffers[first + i];
				range->offset = offsets[first + i];
				range->size   = sizes[first + i];
			}
		}
		GL_BindBuffersRangeFunc (target, first, count, buffers, offsets, sizes);
	}
	else
	{
		for (i = 0; i < count; i++)
			GL_BindBufferRange (target, first + i, buffers[i], offsets[i], sizes[i]);
	}
}

/*
====================
GL_DeleteBuffer
====================
*/
void GL_DeleteBuffer (GLuint buffer)
{
	int i;

	if (buffer == current_array_buffer)
		current_array_buffer = 0;
	if (buffer == current_element_array_buffer)
		current_element_array_buffer = 0;
	if (buffer == current_draw_indirect_buffer)
		current_draw_indirect_buffer = 0;
	if (buffer == current_shader_storage_buffer)
		current_shader_storage_buffer = 0;

	for (i = 0; i < countof(ssbo_ranges); i++)
		if (ssbo_ranges[i].buffer == buffer)
			ssbo_ranges[i].buffer = 0;

	GL_DeleteBuffersFunc (1, &buffer);
}

/*
====================
GL_ClearBufferBindings

This must be called if you do anything that could make the cached bindings
invalid (e.g. manually binding, destroying the context).
====================
*/
void GL_ClearBufferBindings ()
{
	int i;

	current_array_buffer = 0;
	current_element_array_buffer = 0;
	current_draw_indirect_buffer = 0;
	current_shader_storage_buffer = 0;

	for (i = 0; i < countof(ssbo_ranges); i++)
		ssbo_ranges[i].buffer = 0;

	GL_BindBufferFunc (GL_ARRAY_BUFFER, 0);
	GL_BindBufferFunc (GL_ELEMENT_ARRAY_BUFFER, 0);
	GL_BindBufferFunc (GL_DRAW_INDIRECT_BUFFER, 0);
	GL_BindBufferFunc (GL_SHADER_STORAGE_BUFFER, 0);
}

/*
============================================================================
							DYNAMIC BUFFERS
============================================================================
*/

#define DYNABUF_FRAMES 3

typedef struct dynabuf_t {
	GLsync			fence;
	GLuint			handle;
	GLubyte			*ptr;
	GLuint			*garbage;
} dynabuf_t;

static dynabuf_t	dynabufs[DYNABUF_FRAMES];
static int			dynabuf_idx = 0;
static size_t		dynabuf_offset = 0;
static size_t		dynabuf_size = 8 * 1024 * 1024;

/*
====================
GL_AddGarbageBuffer
====================
*/
void GL_AddGarbageBuffer (GLuint handle)
{
	VEC_PUSH (dynabufs[dynabuf_idx].garbage, handle);
}

/*
====================
GL_AllocDynamicBuffers
====================
*/
static void GL_AllocDynamicBuffers (void)
{
	int i;
	for (i = 0; i < countof(dynabufs); i++)
	{
		char name[64];
		GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		dynabuf_t *buf = &dynabufs[i];

		if (buf->handle)
		{
			if (buf->ptr)
			{
				GL_BindBuffer (GL_ARRAY_BUFFER, buf->handle);
				GL_UnmapBufferFunc (GL_ARRAY_BUFFER);
			}
			GL_AddGarbageBuffer (buf->handle);
		}

		GL_GenBuffersFunc (1, &buf->handle);
		GL_BindBuffer (GL_ARRAY_BUFFER, buf->handle);
		q_snprintf (name, sizeof(name), "dynamic buffer %d", i);
		GL_ObjectLabelFunc (GL_BUFFER, buf->handle, -1, name);
		if (gl_buffer_storage_able)
		{
			GL_BufferStorageFunc (GL_ARRAY_BUFFER, dynabuf_size, NULL, flags);
			buf->ptr = GL_MapBufferRangeFunc (GL_ARRAY_BUFFER, 0, dynabuf_size, flags);
			if (!buf->ptr)
				Sys_Error ("GL_AllocDynamicBuffers: MapBufferRange failed on %zu bytes", dynabuf_size);
		}
		else
		{
			GL_BufferDataFunc (GL_ARRAY_BUFFER, dynabuf_size, NULL, GL_STREAM_DRAW);
		}
	}

	dynabuf_offset = 0;
}

/*
====================
GL_CreateDynamicBuffers
====================
*/
void GL_CreateDynamicBuffers (void)
{
	GL_AllocDynamicBuffers ();
}

/*
====================
GL_DeleteDynamicBuffers
====================
*/
void GL_DeleteDynamicBuffers (void)
{
	size_t i, j, num_garbage_bufs;

	glFinish ();

	for (i = 0; i < countof (dynabufs); i++)
	{
		dynabuf_t *buf = &dynabufs[i];

		if (buf->fence)
		{
			GL_DeleteSyncFunc (buf->fence);
			buf->fence = NULL;
		}

		for (j = 0, num_garbage_bufs = VEC_SIZE (buf->garbage); j < num_garbage_bufs; j++)
			GL_DeleteBuffer (buf->garbage[j]);
		VEC_CLEAR (buf->garbage);

		if (buf->ptr)
		{
			GL_BindBuffer (GL_ARRAY_BUFFER, buf->handle);
			GL_UnmapBufferFunc (GL_ARRAY_BUFFER);
			buf->ptr = NULL;
		}

		if (buf->handle)
		{
			GL_DeleteBuffer (buf->handle);
			buf->handle = 0;
		}
	}
}

/*
====================
GL_DynamicBuffersBeginFrame
====================
*/
void GL_DynamicBuffersBeginFrame (void)
{
	dynabuf_t *prev_buf = &dynabufs[(dynabuf_idx + DYNABUF_FRAMES - 1) % DYNABUF_FRAMES];
	dynabuf_t *buf = &dynabufs[dynabuf_idx];
	size_t i, num_garbage_bufs;

	if (prev_buf->fence)
		GL_WaitSyncFunc (prev_buf->fence, 0, GL_TIMEOUT_IGNORED);

	if (buf->fence)
	{
		GLuint64 timeout = 2ull * 1000 * 1000 * 1000; // 2 seconds
		GLenum result = GL_ClientWaitSyncFunc (buf->fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
		if (result == GL_WAIT_FAILED)
			Sys_Error ("GL_DynamicBuffersBeginFrame: wait failed (0x%04X)", glGetError ());
		if (result != GL_CONDITION_SATISFIED && result != GL_ALREADY_SIGNALED)
			Sys_Error ("GL_DynamicBuffersBeginFrame: sync failed (0x%04X)", result);
		GL_DeleteSyncFunc (buf->fence);
		buf->fence = NULL;
	}

	num_garbage_bufs = VEC_SIZE (buf->garbage);
	for (i = 0; i < num_garbage_bufs; i++)
		GL_DeleteBuffer (buf->garbage[i]);
	VEC_CLEAR (buf->garbage);
}

/*
====================
GL_DynamicBuffersEndFrame
====================
*/
void GL_DynamicBuffersEndFrame (void)
{
	dynabuf_t *buf = &dynabufs[dynabuf_idx];

	SDL_assert (!buf->fence);
	buf->fence = GL_FenceSyncFunc (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	if (++dynabuf_idx == countof(dynabufs))
		dynabuf_idx = 0;
	dynabuf_offset = 0;
}

/*
====================
GL_Upload
====================
*/
void GL_Upload (GLenum target, const void *data, size_t numbytes, GLuint *outbuf, GLbyte **outofs)
{
	dynabuf_t *buf;

	if (dynabuf_offset + numbytes > dynabuf_size)
	{
		dynabuf_size = dynabuf_offset + numbytes;
		dynabuf_size += dynabuf_size >> 1;
		GL_AllocDynamicBuffers ();
	}

	buf = &dynabufs[dynabuf_idx];
	if (buf->ptr)
		memcpy (buf->ptr + dynabuf_offset, data, numbytes);
	else
	{
		GL_BindBuffer (target, buf->handle);
		GL_BufferSubDataFunc (target, dynabuf_offset, numbytes, data);
	}

	*outbuf = buf->handle;
	*outofs = (GLbyte*) dynabuf_offset;

	dynabuf_offset += (numbytes + ssbo_align) & ~ssbo_align;
}
