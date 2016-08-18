/*
	todo:
	・異なるサイズのレイヤーに適用した際の不具合修正
	・AEから変数名の指定
	
	・シェーダ選択できるようにしたい
	・Slider動的に増やしたい
*/

#include "GLSL.h"
#include "GL_base.h"
using namespace AESDK_OpenGL;

string CreateShaderPath( string inPluginPath, string inShaderFileName );

static GLuint S_GLator_InputFrameTextureIDSu; //input texture
static AESDK_OpenGL::AESDK_OpenGL_EffectCommonData S_GLator_EffectCommonData; //effect state variables
static bool initGL = false;

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	//PF_SPRINTF(out_data->return_msg, "About");

	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	//PF_SPRINTF(out_data->return_msg, "GlobalSetup");


	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags = 	PF_OutFlag_I_EXPAND_BUFFER				 |
							PF_OutFlag_I_HAVE_EXTERNAL_DEPENDENCIES;

	out_data->out_flags2 =  PF_OutFlag2_NONE;
	
	PF_Err err = PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	AESDK_OpenGL_Err error_desc;

	if ((error_desc = AESDK_OpenGL_Startup(S_GLator_EffectCommonData)) != AESDK_OpenGL_OK)
	{
		PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
		CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
	}

	return err;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	
	//PF_SPRINTF(out_data->return_msg, "ParamsSetup");


	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Param0[A]",
		0, 		// MIN
		100,	// MAX
		0,		// MIN
		100, 	// MAX
		100,	// Default
		SLIDER0_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Param1[R]",
		0, 		// MIN
		100,	// MAX
		0,		// MIN
		100, 	// MAX
		0,		// Default
		SLIDER1_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Param2[G]",
		0, 		// MIN
		100,	// MAX
		0,		// MIN
		100, 	// MAX
		0,		// Default
		SLIDER2_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Param3[B]",
		0, 		// MIN
		100,	// MAX
		0,		// MIN
		100, 	// MAX
		0,		// Default
		SLIDER3_DISK_ID);

	out_data->num_params = GLATOR_NUM_PARAMS;

	return err;
}


static PF_Err 
GlobalSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err			err			=	PF_Err_NONE;
	//PF_SPRINTF(out_data->return_msg, "GlobalSetdown");

	try
	{
		SetPluginContext(S_GLator_EffectCommonData);
		glDeleteTextures( 1, &S_GLator_InputFrameTextureIDSu);
		AESDK_OpenGL_Err error_desc;
		if( (error_desc = AESDK_OpenGL_ReleaseResources(S_GLator_EffectCommonData)) != AESDK_OpenGL_OK)
		{
			PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
			CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
		}

		SetHostContext(S_GLator_EffectCommonData);
		
		if (in_data->sequence_data) {
			PF_DISPOSE_HANDLE(in_data->sequence_data);
			out_data->sequence_data = NULL;
		}

		wglDeleteContext(S_GLator_EffectCommonData.mHRC);
		DestroyWindow(S_GLator_EffectCommonData.mHWnd);
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
		
		SetHostContext(S_GLator_EffectCommonData);
	}
	
	return err;
}


static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	PF_EffectWorld	openGL_world;
	A_long				widthL	=	output->width,
						heightL	=	output->height;
	
	GLfloat sliderVal0 = params[SLIDER0]->u.fd.value / 100.0f;
	GLfloat sliderVal1 = params[SLIDER1]->u.fd.value / 100.0f;
	GLfloat sliderVal2 = params[SLIDER2]->u.fd.value / 100.0f;
	GLfloat sliderVal3 = params[SLIDER3]->u.fd.value / 100.0f;

	//Init AESDK_OpenGL
	if (!initGL) {
		try
		{
			AEGP_SuiteHandler suites(in_data->pica_basicP);
			AESDK_OpenGL_Err error_desc;

			u_short width = (u_short)(output->rowbytes / sizeof(PF_Pixel));
			u_short height = (u_short)(output->height);

			SetPluginContext(S_GLator_EffectCommonData);
			if ((error_desc = AESDK_OpenGL_InitResources(S_GLator_EffectCommonData, width, height)) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}

			//create an empty texture for the input surface
			S_GLator_InputFrameTextureIDSu = -1;

			PF_Handle	dataH = suites.HandleSuite1()->host_new_handle(((S_GLator_EffectCommonData.mRenderBufferWidthSu * S_GLator_EffectCommonData.mRenderBufferHeightSu)* sizeof(GL_RGBA)));
			if (dataH)
			{
				unsigned int *dataP = reinterpret_cast<unsigned int*>(suites.HandleSuite1()->host_lock_handle(dataH));

				//create empty input frame texture
				glGenTextures(1, &S_GLator_InputFrameTextureIDSu);
				glBindTexture(GL_TEXTURE_2D, S_GLator_InputFrameTextureIDSu);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, 4, S_GLator_EffectCommonData.mRenderBufferWidthSu, S_GLator_EffectCommonData.mRenderBufferHeightSu, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataP);

				suites.HandleSuite1()->host_unlock_handle(dataH);
				suites.HandleSuite1()->host_dispose_handle(dataH);
			}
			else
			{
				CHECK(PF_Err_OUT_OF_MEMORY);
			}

			//initialize and compile the shader objects
			A_char pluginFolderPath[AEFX_MAX_PATH];

			// CC12 PF_PlatData_EXE_FILE_PATH > PF_PlatData_EXE_FILE_PATH_DEPRECATED
			PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_DEPRECATED, &pluginFolderPath);
			string pluginPath = string(pluginFolderPath);

			if ((error_desc = AESDK_OpenGL_InitShader(S_GLator_EffectCommonData,
				CreateShaderPath(pluginPath, string("vertex_shader.vert")),
				CreateShaderPath(pluginPath, string("fragment_shader.frag")))) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}
			SetHostContext(S_GLator_EffectCommonData);
			initGL = true;
		}
		catch (PF_Err& thrown_err)
		{
			err = thrown_err;

			SetHostContext(S_GLator_EffectCommonData);
		}

	}

	try
	{
		SetPluginContext(S_GLator_EffectCommonData);
		
		//setup openGL_world
		AEFX_CLR_STRUCT(openGL_world);	
		ERR(suites.WorldSuite1()->new_world(	in_data->effect_ref,
												widthL,
												heightL,
												PF_NewWorldFlag_CLEAR_PIXELS,
												&openGL_world));

		//update the texture - only a portion of the texture that is
		PF_EffectWorld	*inputP		=	&params[GLATOR_INPUT]->u.ld;
		glEnable( GL_TEXTURE_2D );
			
		PF_Handle bufferH	= NULL;
		bufferH = suites.HandleSuite1()->host_new_handle(((S_GLator_EffectCommonData.mRenderBufferWidthSu * S_GLator_EffectCommonData.mRenderBufferHeightSu)* sizeof(GL_RGBA)));
		if (bufferH)
		{
			unsigned int *bufferP = reinterpret_cast<unsigned int*>(suites.HandleSuite1()->host_lock_handle(bufferH));

			//copy inputframe to openGL_world
			for (int ix = 0; ix < inputP->height; ++ix)
			{
				PF_Pixel8 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA8(inputP, NULL, &pixelDataStart);
				::memcpy(bufferP + (ix * S_GLator_EffectCommonData.mRenderBufferWidthSu),
					pixelDataStart + (ix * (inputP->rowbytes) / sizeof(GL_RGBA)),
					inputP->width * sizeof(GL_RGBA));
			}

			//upload to texture memory
			glBindTexture(GL_TEXTURE_2D, S_GLator_InputFrameTextureIDSu);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, S_GLator_EffectCommonData.mRenderBufferWidthSu, S_GLator_EffectCommonData.mRenderBufferHeightSu, GL_RGBA, GL_UNSIGNED_BYTE, bufferP);


			AESDK_OpenGL_Err error_desc;
			if ((error_desc = AESDK_OpenGL_MakeReadyToRender(S_GLator_EffectCommonData)) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}

			//unbind all textures
			glBindTexture(GL_TEXTURE_2D, 0);

			//set the matrix modes
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0, (GLdouble)widthL / heightL, 0.1, 100.0);

			// Set up the frame-buffer object just like a window.
			glViewport(0, 0, widthL, heightL);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(0.0f, 0.0f, -1.207f);


			glBindTexture(GL_TEXTURE_2D, S_GLator_InputFrameTextureIDSu);
			AESDK_OpenGL_StartRenderToShader(S_GLator_EffectCommonData);

			// Identify the texture to use and bind it to texture unit 0
			if ((error_desc = AESDK_OpenGL_BindTextureToTarget(S_GLator_EffectCommonData, S_GLator_InputFrameTextureIDSu, string("videoTexture"))) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}

			//if ((error_desc = ValueToTarget1f(S_GLator_EffectCommonData, string("test"), sliderVal0)) != AESDK_OpenGL_OK)

			if ((error_desc = ValueToTarget4f(S_GLator_EffectCommonData, string("color"), sliderVal0, sliderVal1, sliderVal2, sliderVal3)) != AESDK_OpenGL_OK)
			{
				PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}

			//input frame
			fpshort aspectF = (static_cast<fpshort>(widthL)) / heightL;
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);	glVertex3f(-0.5f * aspectF, -0.5f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);	glVertex3f(0.5f * aspectF, -0.5f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);	glVertex3f(0.5f * aspectF, 0.5f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);	glVertex3f(-0.5f * aspectF, 0.5f, 0.0f);
			glEnd();

			AESDK_OpenGL_StopRenderToShader();

			glFlush();

			// Check for errors...
			string error_msg;
			if ((error_msg = CheckFramebufferStatus()) != string("OK"))
			{
				PF_SPRINTF(out_data->return_msg, error_msg.c_str());
				CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
			}
			glFlush();

			//download from texture memory onto the same surface
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glReadPixels(0, 0, S_GLator_EffectCommonData.mRenderBufferWidthSu, S_GLator_EffectCommonData.mRenderBufferHeightSu, GL_RGBA, GL_UNSIGNED_BYTE, bufferP);

			//copy to openGL_world
			for (int ix = 0; ix < openGL_world.height; ++ix)
			{
				PF_Pixel8 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA8(&openGL_world, NULL, &pixelDataStart);
				::memcpy(pixelDataStart + (ix * openGL_world.rowbytes / sizeof(GL_RGBA)),
					bufferP + (ix * S_GLator_EffectCommonData.mRenderBufferWidthSu),
					openGL_world.width * sizeof(GL_RGBA));
			}

			//clean the data after being copied
			suites.HandleSuite1()->host_unlock_handle(bufferH);
			suites.HandleSuite1()->host_dispose_handle(bufferH);
		}
		else
		{
			CHECK(PF_Err_OUT_OF_MEMORY);
		}
		
		if (PF_Quality_HI == in_data->quality) {
			ERR(suites.WorldTransformSuite1()->copy_hq(	in_data->effect_ref,
														&openGL_world,
														output,
														NULL,
														NULL));
		}
		else
		{
			ERR(suites.WorldTransformSuite1()->copy(	in_data->effect_ref,
														&openGL_world,
														output,
														NULL,
														NULL));

		}
		
		SetHostContext(S_GLator_EffectCommonData);
		
		ERR( suites.WorldSuite1()->dispose_world( in_data->effect_ref, &openGL_world));
		ERR(PF_ABORT(in_data));
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
		SetHostContext(S_GLator_EffectCommonData);
	}
	catch(...)
	{
		SetHostContext(S_GLator_EffectCommonData);
	}
	
	return err;
}

DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	//PF_SPRINTF(out_data->return_msg, "EntryPointFunc %i", cmd);

	try {
		switch (cmd) {
			case 5:
				if (initGL) {
					SetPluginContext(S_GLator_EffectCommonData);

					//OpenGL開放
					glDeleteTextures(1, &S_GLator_InputFrameTextureIDSu);
					AESDK_OpenGL_Err error_desc;
					if ((error_desc = AESDK_OpenGL_ReleaseResources(S_GLator_EffectCommonData)) != AESDK_OpenGL_OK)
					{
						PF_SPRINTF(out_data->return_msg, ReportError(error_desc).c_str());
						CHECK(PF_Err_INTERNAL_STRUCT_DAMAGED);
					}

					initGL = false;
				}
				break;

			case PF_Cmd_ABOUT:
				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:
				err = Render(		in_data,
								out_data,
								params,
								output);
				break;

			case PF_Cmd_GLOBAL_SETDOWN:
				err = GlobalSetdown(	in_data,
										out_data,
										params,
										output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

//helper function
string CreateShaderPath( string inPluginPath, string inShaderFileName )
{
	string::size_type pos;
	string shaderPath;
#ifdef AE_OS_WIN
	//delete the plugin name
	pos = inPluginPath.rfind("\\",inPluginPath.length());
	shaderPath = inPluginPath.substr( 0, pos);
	shaderPath = shaderPath + string("\\") + inShaderFileName;
#elif defined(AE_OS_MAC)
#if __LP64__
	const char *delim = "/";
#else
	const char *delim = ":";
#endif

	//delete the plugin name
	pos = inPluginPath.rfind(delim,inPluginPath.length());
	shaderPath = inPluginPath.substr( 0, pos);
	//delete the parent volume
	pos = shaderPath.find_first_of( string(delim), 0);
	shaderPath.erase( 0, pos);
	//next replace all the colons with slashes
	while( string::npos != (pos = inPluginPath.find( string(":"), 0)) )
	{
		shaderPath.replace(pos, 1, string("/"));
	}
	shaderPath = shaderPath + string(delim) + inShaderFileName;
#endif
	return shaderPath;
}